#include "ac3file.h"
#include "guids.h"
#include "win32\winspk.h"

///////////////////////////////////////////////////////////////////////////////

VALibSource::VALibSource(TCHAR *_filter_name, LPUNKNOWN _lpunk, CLSID _clsid)
:CSource(_filter_name, _lpunk, _clsid)
{ 
}

CUnknown *WINAPI 
VALibSource::CreateInstance(LPUNKNOWN _lpunk, HRESULT* _phr)
{
  CUnknown *_punk = new VALibSource("AC3File", _lpunk, CLSID_AC3File);
  if (!_punk) 
    *_phr = E_OUTOFMEMORY;
	return _punk;
}

STDMETHODIMP 
VALibSource::NonDelegatingQueryInterface(REFIID _riid, void **_ppv)
{
  CheckPointer(_ppv, E_POINTER);

  if (_riid == IID_IFileSourceFilter)
    return GetInterface((IFileSourceFilter *) this, _ppv);

  if (_riid == IID_IAMFilterMiscFlags)
    return GetInterface((IAMFilterMiscFlags *) this, _ppv);

  return CSource::NonDelegatingQueryInterface(_riid, _ppv);
}

STDMETHODIMP 
VALibSource::Load(LPCOLESTR _filename, const AM_MEDIA_TYPE *_pmt)
{
  if(GetPinCount() > 0)
    return VFW_E_ALREADY_CONNECTED;

  HRESULT hr = S_OK;

  int size = wcstombs(0, _filename, 0) + 1;
  char *filename = new char[size];
  if (!filename)
    return E_OUTOFMEMORY;

  size = WideCharToMultiByte(CP_ACP, 0, _filename, -1, filename, size, 0, 0);
  //size = wcstombs(filename, _filename, size);

  VALibStream *stream = new VALibStream(filename, this, &hr);

  delete filename;

  if (!stream)
    return E_OUTOFMEMORY;
    
  if FAILED(hr)
  {
    delete stream;
    return hr;
  }

  return S_OK;
}

STDMETHODIMP 
VALibSource::GetCurFile(LPOLESTR *_filename, AM_MEDIA_TYPE *_pmt)
{
  return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////

VALibStream::VALibStream(TCHAR *_filename, CSource *_parent, HRESULT *_phr)
: CSourceStream(_filename, _phr, _parent, L"Output")
, CSourceSeeking(_filename, (IPin*)this, _phr, &seek_lock)
{
	CAutoLock auto_lock(&seek_lock);

  if (_phr && FAILED(*_phr))
    return;

  format = FORMAT_UNKNOWN;
  if (file.open(&ac3, _filename) && file.probe())
    format = FORMAT_AC3;
  else if (file.open(&dts, _filename) && file.probe())
    format = FORMAT_DTS;
  else
  {
    *_phr = E_FAIL;
    return;
  } 

  file.stats();

  pos = 0;
  m_rtStart = pos;
  m_rtStop = REFERENCE_TIME(file.get_size(file.ms) * 10000);
  m_rtDuration = m_rtStop - m_rtStart;
}

VALibStream::~VALibStream()
{
	CAutoLock auto_lock(&seek_lock);
  file.close();
}

// IUnknown

STDMETHODIMP 
VALibStream::NonDelegatingQueryInterface(REFIID _riid, void **_ppv)
{
  CheckPointer(_ppv, E_POINTER);

  return _riid == IID_IMediaSeeking?
    CSourceSeeking::NonDelegatingQueryInterface(_riid, _ppv):
    CSourceStream::NonDelegatingQueryInterface(_riid, _ppv);
}

// CBaseOutputPin


HRESULT 
VALibStream::GetMediaType(int i, CMediaType* pmt)
{
  CAutoLock auto_lock(&seek_lock);
  if (i < 0) return E_INVALIDARG;
  if (i > 1) return VFW_S_NO_MORE_ITEMS;

  WAVEFORMATEX wfe;
	memset(&wfe, 0, sizeof(WAVEFORMATEX));

  switch (format)
  {
    case FORMAT_AC3: wfe.wFormatTag = WAVE_FORMAT_AC3; break;
    case FORMAT_DTS: wfe.wFormatTag = WAVE_FORMAT_DTS; break;
    default: return E_FAIL;
  }

  wfe.nChannels = file.get_spk().nch();
  wfe.nSamplesPerSec = file.get_spk().sample_rate;
  wfe.wBitsPerSample = 0;
  wfe.nBlockAlign = 1;
  wfe.nAvgBytesPerSec = int(file.get_bitrate()) / 8;
  wfe.cbSize = 0;

  pmt->SetType(&MEDIATYPE_Audio);
  switch (i)
  {
    case 0:
      switch (format)
      {
        case FORMAT_AC3: pmt->SetSubtype(&MEDIASUBTYPE_DOLBY_AC3); break;
        case FORMAT_DTS: pmt->SetSubtype(&MEDIASUBTYPE_DTS); break;
      }
      break;

    case 1:
      switch (format)
      {
        case FORMAT_AC3: pmt->SetSubtype(&MEDIASUBTYPE_AVI_AC3); break;
        case FORMAT_DTS: pmt->SetSubtype(&MEDIASUBTYPE_AVI_DTS); break;
      }
      break;
  }

  pmt->SetFormatType(&FORMAT_WaveFormatEx);
  pmt->SetFormat((BYTE*)&wfe, sizeof(WAVEFORMATEX) + wfe.cbSize);
  pmt->SetTemporalCompression(FALSE);

	return S_OK;
}

HRESULT 
VALibStream::CheckMediaType(const CMediaType* pmt)
{
	if ((*pmt->Type()) != MEDIATYPE_Audio)
    return E_INVALIDARG;

  if ((*pmt->Subtype()) == MEDIASUBTYPE_DOLBY_AC3 ||
      (*pmt->Subtype()) == MEDIASUBTYPE_AVI_AC3 ||
      (*pmt->Subtype()) == MEDIASUBTYPE_DTS ||
      (*pmt->Subtype()) == MEDIASUBTYPE_AVI_DTS)
    return S_OK;

  if ((*pmt->FormatType()) == FORMAT_WaveFormatEx)
  {
    WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->Format();
    if (wfe->wFormatTag == WAVE_FORMAT_AC3 || 
        wfe->wFormatTag == WAVE_FORMAT_DTS)
      return S_OK;
  }

  return E_INVALIDARG;
}

HRESULT 
VALibStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
  ASSERT(pAlloc);
  ASSERT(pProperties);

  HRESULT hr = NOERROR;

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 4096;

  ALLOCATOR_PROPERTIES Actual;

  if FAILED(hr = pAlloc->SetProperties(pProperties, &Actual)) 
    return hr;

  if (Actual.cbBuffer < pProperties->cbBuffer) 
    return E_FAIL;

  ASSERT(Actual.cBuffers == pProperties->cBuffers);

  return NOERROR;
}

// CSourceStream

HRESULT 
VALibStream::OnThreadCreate()
{
  CAutoLock auto_lock(&seek_lock);

  pos = m_rtStart;
  file.seek(double(pos) / 10000, file.ms);

  return CSourceStream::OnThreadCreate();
}

HRESULT 
VALibStream::OnThreadStartPlay()
{
  discontinuity = true;
  return DeliverNewSegment(m_rtStart, m_rtStop, m_dRateSeeking);
}

HRESULT 
VALibStream::FillBuffer(IMediaSample* sample)
{
  {
    CAutoLock auto_lock(&seek_lock);

    if(file.eof())
      return S_FALSE;

    if (!file.load_frame())
      return S_FALSE;

    // copy data
    BYTE *buf;
    if (FAILED(sample->GetPointer(&buf)) || !buf)
      return S_FALSE;

    int frame_size = file.get_frame_size();
    if FAILED(sample->SetActualDataLength(frame_size))
      return S_FALSE;

    memcpy(buf, file.get_frame(), frame_size);

    // timing
    REFERENCE_TIME t_start, t_end;
    t_start = REFERENCE_TIME((pos - m_rtStart) / m_dRateSeeking);
    pos += REFERENCE_TIME(double(file.get_nsamples()) / file.get_spk().sample_rate * 10000000);
    t_end = REFERENCE_TIME((pos - m_rtStart) / m_dRateSeeking);

    sample->SetTime(&t_start, &t_end);
  }

  sample->SetSyncPoint(TRUE);
  if (discontinuity) 
  {
    sample->SetDiscontinuity(TRUE);
    discontinuity = false;
  }

  return S_OK;
}


// CSourceSeeking

void 
VALibStream::restart()
{
  if (ThreadExists())
  {
    DeliverBeginFlush();
    Stop();
    DeliverEndFlush();
    Run();
  }
}

HRESULT 
VALibStream::ChangeStart()
{
  {
    CAutoLock auto_lock(CSourceSeeking::m_pLock);

    pos = m_rtStart;
    file.seek(double(pos) / 10000, file.ms);
  }

  restart();
  return S_OK;
}

HRESULT 
VALibStream::ChangeStop()
{
  {
    CAutoLock auto_lock(CSourceSeeking::m_pLock);
    if(pos < m_rtStop)
      return S_OK;
  }

  restart();
  return S_OK;
}

HRESULT 
VALibStream::ChangeRate() 
{
  return S_OK;
}

STDMETHODIMP  
VALibStream::SetRate(double _rate)
{
	if (_rate <= 0)
		return E_INVALIDARG;

  {
    CAutoLock auto_lock(CSourceSeeking::m_pLock);
    m_dRateSeeking = _rate;
  }

  restart();
	return S_OK;
}


