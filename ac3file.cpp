#include "ac3file.h"
#include "guids.h"
#include "win32\winspk.h"

char *
wide2char(LPCWSTR _wide_str)
{
  int size = wcstombs(0, _wide_str, 0) + 1;
  if (!size) return 0;

  char *char_str = new char[size];
  if (!char_str) return 0;

  WideCharToMultiByte(CP_ACP, 0, _wide_str, -1, char_str, size, 0, 0);
  char_str[size-1] = 0; // make sure that string is properly ended
  return char_str;
}

LPWSTR
char2wide(const char *_char_str)
{
  int size = mbstowcs(0, _char_str, 0) + 1;
  if (!size) return 0;

  wchar_t *wide_str = new wchar_t[size];
  if (!wide_str) return 0;

  MultiByteToWideChar(CP_ACP, 0, _char_str, -1, wide_str, size);
  wide_str[size-1] = 0; // make sure that string is properly ended
  return wide_str;
}

///////////////////////////////////////////////////////////////////////////////

VALibSource::VALibSource(TCHAR *_filter_name, LPUNKNOWN _lpunk, CLSID _clsid)
:CSource(_filter_name, _lpunk, _clsid)
{}

VALibSource::~VALibSource()
{}

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
  // The Load method causes a source filter to load a media file. 

  // This method initializates the interface. It is not designed to load 
  // multiple files, and any calls to this method after the first call 
  // will fail. 

  // Depending on the filter, the pszFileName parameter should specify the 
  // absolute path to a file on disk (File Source (Async) filter, WM ASF 
  // Reader filter) or the URL of a file to download (File Source (URL) 
  // filter).

  if (GetPinCount() > 0)
    return VFW_E_ALREADY_CONNECTED;

  char *filename = wide2char(_filename);
  if (!filename) return E_OUTOFMEMORY;

  HRESULT hr = S_OK;
  stream = new VALibStream(filename, this, &hr);
  delete filename;

  if (!stream)
    return E_OUTOFMEMORY;
    
  if FAILED(hr)
  {
    delete stream;
    stream = 0;
    return hr;
  }

  return S_OK;
}

STDMETHODIMP 
VALibSource::GetCurFile(LPOLESTR *_filename, AM_MEDIA_TYPE *_pmt)
{
  // The GetCurFile method retrieves the name and media type of the current 
  // file. 

  // If the filter has not opened a file, the method might succeed but 
  // return NULL in the ppszFileName parameter. Check the value when the 
  // method returns.

  // The method allocates the memory for the string returned in ppszFileName, 
  // and the memory for the format block in the media type (if any). The 
  // caller must free them by calling CoTaskMemFree.

  if (!stream)
    return E_FAIL;

  if (_filename)
  {
    const char *filename = stream->get_filename();

    int size = mbstowcs(0, filename, 0) + 1;
    if (!size) return E_OUTOFMEMORY;

    *_filename = (LPOLESTR)CoTaskMemAlloc(size * sizeof(wchar_t));
    if (!*_filename) return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, filename, -1, *_filename, size);
    (*_filename)[size-1] = 0; // make sure that string is properly ended
  }
  else
    return E_POINTER;

  if (_pmt)
    if FAILED(stream->ConnectionMediaType(_pmt))
    {
      HRESULT hr;
      IEnumMediaTypes *enum_mt;

      hr = stream->EnumMediaTypes(&enum_mt);
      if FAILED(hr) 
      {
        CoTaskMemFree(*_filename);
        return hr;
      }

      hr = enum_mt->Next(1, &_pmt, 0);
      enum_mt->Release();

      if FAILED(hr)
      {
        CoTaskMemFree(*_filename);
        return hr;
      }

      if (hr == S_FALSE)
        memset(_pmt, 0, sizeof(AM_MEDIA_TYPE));
    }

  return S_OK;
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

  WAVEFORMATEX wfe;
	memset(&wfe, 0, sizeof(WAVEFORMATEX));

  switch (format)
  {
    case FORMAT_AC3:
      wfe.wFormatTag = WAVE_FORMAT_AC3;
      switch(i)
      {
        case 0: pmt->SetSubtype(&MEDIASUBTYPE_DOLBY_AC3); break;
        case 1: pmt->SetSubtype(&MEDIASUBTYPE_AVI_AC3);   break;
        default: return VFW_S_NO_MORE_ITEMS;
      }
      break;

    case FORMAT_DTS:
      wfe.wFormatTag = WAVE_FORMAT_DTS;
      switch(i)
      {
        case 0: pmt->SetSubtype(&MEDIASUBTYPE_DTS);     break;
        case 1: pmt->SetSubtype(&MEDIASUBTYPE_AVI_DTS); break;
        default: return VFW_S_NO_MORE_ITEMS;
      }
      break;

    default:
      return VFW_S_NO_MORE_ITEMS;
  }

  wfe.nChannels = file.get_spk().nch();
  wfe.nSamplesPerSec = file.get_spk().sample_rate;
  wfe.wBitsPerSample = 0;
  wfe.nBlockAlign = 1;
  wfe.nAvgBytesPerSec = int(file.get_bitrate()) / 8;
  wfe.cbSize = 0;

  pmt->SetType(&MEDIATYPE_Audio);
  pmt->SetFormatType(&FORMAT_WaveFormatEx);
  pmt->SetFormat((BYTE*)&wfe, sizeof(WAVEFORMATEX) + wfe.cbSize);
  pmt->SetTemporalCompression(FALSE);

	return S_OK;
}

HRESULT 
VALibStream::CheckMediaType(const CMediaType* pmt)
{
	if (*pmt->Type() != MEDIATYPE_Audio)
    return E_INVALIDARG;

  switch (format)
  {
    case FORMAT_AC3:
      if (*pmt->Subtype() == MEDIASUBTYPE_DOLBY_AC3 ||
          *pmt->Subtype() == MEDIASUBTYPE_AVI_AC3)
        return S_OK;

      if (*pmt->FormatType() == FORMAT_WaveFormatEx &&
          ((WAVEFORMATEX*)pmt->Format())->wFormatTag == WAVE_FORMAT_AC3)
          return S_OK;

      return E_INVALIDARG;

    case FORMAT_DTS:
      if (*pmt->Subtype() == MEDIASUBTYPE_DTS ||
          *pmt->Subtype() == MEDIASUBTYPE_AVI_DTS)
        return S_OK;

      if (*pmt->FormatType() == FORMAT_WaveFormatEx && 
          ((WAVEFORMATEX*)pmt->Format())->wFormatTag == WAVE_FORMAT_DTS)
          return S_OK;

      return E_INVALIDARG;
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


