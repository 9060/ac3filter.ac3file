#include "ac3file.h"
#include "guids.h"
#include "win32\winspk.h"

#include "parsers\ac3\ac3_header.h"
#include "parsers\dts\dts_header.h"
#include "parsers\mpa\mpa_header.h"
#include "parsers\spdif\spdif_header.h"

char *
wide2char(LPCWSTR _wide_str)
{
  size_t size = wcstombs(0, _wide_str, MAX_PATH) + 1;
  if (!size) return 0;

  char *char_str = new char[size];
  if (!char_str) return 0;

  WideCharToMultiByte(CP_ACP, 0, _wide_str, -1, char_str, (int)size, 0, 0);
  char_str[size-1] = 0; // make sure that string is properly ended
  return char_str;
}

inline FileParser::units_t get_units(int units)
{
  switch (units)
  {
    case AC3FILE_BYTES:  return FileParser::bytes;
    case AC3FILE_TIME:   return FileParser::time;
    case AC3FILE_FRAMES: return FileParser::frames;
  }
  return FileParser::bytes;
}


///////////////////////////////////////////////////////////////////////////////
// VALibSource
///////////////////////////////////////////////////////////////////////////////

VALibSource::VALibSource(char *_filter_name, LPUNKNOWN _lpunk, CLSID _clsid)
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

///////////////////////////////////////////////////////////
// IUnknown

STDMETHODIMP 
VALibSource::NonDelegatingQueryInterface(REFIID _riid, void **_ppv)
{
  CheckPointer(_ppv, E_POINTER);

  if (_riid == IID_IAC3File)
    return GetInterface((IAC3File *) this, _ppv);

  if (_riid == IID_IFileSourceFilter)
    return GetInterface((IFileSourceFilter *) this, _ppv);

  if (_riid == IID_IAMFilterMiscFlags)
    return GetInterface((IAMFilterMiscFlags *) this, _ppv);

  if (_riid == IID_ISpecifyPropertyPages)
    return GetInterface((ISpecifyPropertyPages *) this, _ppv);

  return CSource::NonDelegatingQueryInterface(_riid, _ppv);
}

///////////////////////////////////////////////////////////
// IAC3File

STDMETHODIMP 
VALibSource::get_info(char *_info, size_t _len)
{
  if (!stream) return E_FAIL;

  // windows controls require '\n' to be replaced with '\r\n'
  size_t len = stream->get_info(_info, _len);
  size_t cnt = 0;

  for (size_t i = 0; i < len; i++)
    if (_info[i+1] == '\n')
      cnt++;

  char *src = _info + len - 1;
  char *dst = src + cnt + 1;
  if (dst > _info + _len)
    dst = _info + len;
  *dst-- = 0;

  while (src != dst)
    if (src[0] != '\r' && src[1] == '\n')
    {
      *dst-- = '\r';
      *dst-- = *src--;
    }
    else
      *dst-- = *src--;

  return S_OK;
}

STDMETHODIMP 
VALibSource::get_pos(unsigned *_frames, unsigned *_bytes, unsigned *_ms)
{
  if (!stream) return E_FAIL;

  if (_frames) *_frames = (unsigned)stream->get_pos(FileParser::frames);
  if (_bytes)  *_bytes  = (unsigned)stream->get_pos(FileParser::bytes);
  if (_ms)     *_ms     = (unsigned)stream->get_pos(FileParser::time) * 1000;

  return S_OK;
}

STDMETHODIMP 
VALibSource::get_size(unsigned *_frames, unsigned *_bytes, unsigned *_ms)
{
  if (!stream) return E_FAIL;

  if (_frames) *_frames = (unsigned)stream->get_size(FileParser::frames);
  if (_bytes)  *_bytes  = (unsigned)stream->get_size(FileParser::bytes);
  if (_ms)     *_ms     = (unsigned)stream->get_size(FileParser::time) * 1000;

  return S_OK;
}

STDMETHODIMP 
VALibSource::get_stat(double *_size, double *_pos, int _units)
{
  if (!stream) return E_FAIL;

  if (_size) *_size = stream->get_size(get_units(_units));
  if (_pos)  *_pos  = stream->get_pos(get_units(_units));

  return S_OK;
}

///////////////////////////////////////////////////////////
// IFileSourceFilter

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

    size_t size = mbstowcs(0, filename, 0) + 1;
    if (!size) return E_OUTOFMEMORY;

    *_filename = (LPOLESTR)CoTaskMemAlloc(size * sizeof(wchar_t));
    if (!*_filename) return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, filename, -1, *_filename, (int)size);
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

///////////////////////////////////////////////////////////
// ISpecifyPropertyPages

STDMETHODIMP 
VALibSource::GetPages(CAUUID *pPages)
{
  pPages->cElems = 1;
  pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
  if (!pPages->pElems) return E_OUTOFMEMORY;
  (pPages->pElems)[0] = CLSID_AC3FileDlg;
  return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////
// VALibStream
///////////////////////////////////////////////////////////////////////////////

VALibStream::VALibStream(char *_filename, CSource *_parent, HRESULT *_phr)
: CSourceStream(_filename, _phr, _parent, L"Output")
, CSourceSeeking(_filename, (IPin*)this, _phr, &seek_lock)
{
  CAutoLock auto_lock(&seek_lock);

  if (_phr && FAILED(*_phr))
    return;

  const HeaderParser *parser_list[] = { &spdif_header, &ac3_header, &dts_header };
  multi_parser.set_parsers(parser_list, array_size(parser_list));

  if (!file.open(_filename, &multi_parser, 1000000))
  {
    *_phr = E_FAIL;
    return;
  }
    
  if (!file.stats())
  {
    *_phr = E_FAIL;
    return;
  }

  if (!file.load_frame())
  {
    *_phr = E_FAIL;
    return;
  }

  spk = file.get_spk();

  pos = 0;
  m_rtStart = pos;
  m_rtStop = REFERENCE_TIME(file.get_size(file.time) * 10000000);
  m_rtDuration = m_rtStop - m_rtStart;
}

VALibStream::~VALibStream()
{
  CAutoLock auto_lock(&seek_lock);
  file.close();
}


size_t
VALibStream::get_info(char *_buf, size_t _len) const
{
  size_t used = file.file_info(_buf, _len);
  used += file.stream_info(_buf + used, _len - used);
  return used;
}


///////////////////////////////////////////////////////////
// IUnknown

STDMETHODIMP 
VALibStream::NonDelegatingQueryInterface(REFIID _riid, void **_ppv)
{
  CheckPointer(_ppv, E_POINTER);

  return _riid == IID_IMediaSeeking?
    CSourceSeeking::NonDelegatingQueryInterface(_riid, _ppv):
    CSourceStream::NonDelegatingQueryInterface(_riid, _ppv);
}

///////////////////////////////////////////////////////////
// CBaseOutputPin

HRESULT 
VALibStream::GetMediaType(int i, CMediaType* pmt)
{
  CAutoLock auto_lock(&seek_lock);
  if (i < 0) return E_INVALIDARG;

  WAVEFORMATEX wfe;
  memset(&wfe, 0, sizeof(WAVEFORMATEX));

  switch (spk.format)
  {
    case FORMAT_AC3:
      wfe.wFormatTag = WAVE_FORMAT_AVI_AC3;
      switch(i)
      {
        case 0: pmt->SetSubtype(&MEDIASUBTYPE_DOLBY_AC3); break;
        case 1: pmt->SetSubtype(&MEDIASUBTYPE_AVI_AC3);   break;
        default: return VFW_S_NO_MORE_ITEMS;
      }
      break;

    case FORMAT_DTS:
      wfe.wFormatTag = WAVE_FORMAT_AVI_DTS;
      switch(i)
      {
        case 0: pmt->SetSubtype(&MEDIASUBTYPE_DTS);     break;
        case 1: pmt->SetSubtype(&MEDIASUBTYPE_AVI_DTS); break;
        default: return VFW_S_NO_MORE_ITEMS;
      }
      break;

    case FORMAT_SPDIF:
      wfe.wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
      switch(i)
      {
        case 0: pmt->SetSubtype(&MEDIASUBTYPE_DOLBY_AC3_SPDIF); break;
        case 1: pmt->SetSubtype(&MEDIASUBTYPE_PCM); break;
        default: return VFW_S_NO_MORE_ITEMS;
      }
      break;

    default:
      return VFW_S_NO_MORE_ITEMS;
  }

  wfe.nChannels = spk.nch();
  wfe.nSamplesPerSec = spk.sample_rate;
  wfe.wBitsPerSample = 0;
  wfe.nBlockAlign = 1;
  wfe.nAvgBytesPerSec = 0;
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

  switch (spk.format)
  {
    case FORMAT_AC3:
      if (*pmt->Subtype() == MEDIASUBTYPE_DOLBY_AC3 ||
          *pmt->Subtype() == MEDIASUBTYPE_AVI_AC3)
        return S_OK;

      if (*pmt->FormatType() == FORMAT_WaveFormatEx &&
          ((WAVEFORMATEX*)pmt->Format())->wFormatTag == WAVE_FORMAT_AVI_AC3)
          return S_OK;

      return E_INVALIDARG;

    case FORMAT_DTS:
      if (*pmt->Subtype() == MEDIASUBTYPE_DTS ||
          *pmt->Subtype() == MEDIASUBTYPE_AVI_DTS)
        return S_OK;

      if (*pmt->FormatType() == FORMAT_WaveFormatEx && 
          ((WAVEFORMATEX*)pmt->Format())->wFormatTag == WAVE_FORMAT_AVI_DTS)
          return S_OK;

    case FORMAT_SPDIF:
      if (*pmt->Subtype() == MEDIASUBTYPE_DOLBY_AC3_SPDIF)
        return S_OK;

      if (*pmt->FormatType() == FORMAT_WaveFormatEx && 
          ((WAVEFORMATEX*)pmt->Format())->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF)
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
  pProperties->cbBuffer = (long)multi_parser.max_frame_size();

  ALLOCATOR_PROPERTIES Actual;

  if FAILED(hr = pAlloc->SetProperties(pProperties, &Actual)) 
    return hr;

  if (Actual.cbBuffer < pProperties->cbBuffer) 
    return E_FAIL;

  ASSERT(Actual.cBuffers == pProperties->cBuffers);

  return NOERROR;
}

///////////////////////////////////////////////////////////
// CSourceStream

HRESULT 
VALibStream::OnThreadCreate()
{
  CAutoLock auto_lock(&seek_lock);

  pos = m_rtStart;
  file.seek(double(pos) / 10000000, file.time);

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

    if (!file.load_frame())
      return S_FALSE;

    if (file.eof())
      return S_FALSE;

    if (file.is_new_stream())
    {
      spk = file.get_spk();

    }

    // copy data
    BYTE *buf;
    if (FAILED(sample->GetPointer(&buf)) || !buf)
      return S_FALSE;

    size_t frame_size = file.get_frame_size();
    if FAILED(sample->SetActualDataLength((long)frame_size))
      return S_FALSE;

    memcpy(buf, file.get_frame(), frame_size);

    // timing
    REFERENCE_TIME t_start, t_end;
    t_start = REFERENCE_TIME((pos - m_rtStart) / m_dRateSeeking);
    pos += REFERENCE_TIME(double(file.header_info().nsamples) / spk.sample_rate * 10000000);
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


///////////////////////////////////////////////////////////
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
    file.seek(double(pos) / 10000000, file.time);
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
