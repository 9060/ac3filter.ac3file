#ifndef AC3FILE_H
#define AC3FILE_H

#include <streams.h>
#include "parsers\ac3\ac3_parser.h"
#include "parsers\dts\dts_parser.h"
#include "parsers\file_parser.h"

class VALibSource;
class VALibStream;

class VALibSource : public CSource, IFileSourceFilter, IAMFilterMiscFlags
{
protected:
  VALibStream *stream;

public:
  VALibSource(TCHAR *_filter_name, LPUNKNOWN _lpunk, CLSID _clsid);
  virtual ~VALibSource();
  static CUnknown *WINAPI CreateInstance(LPUNKNOWN _lpunk, HRESULT* _phr);

  /////////////////////////////////////////////////////////
  // IUnknown

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID _riid, void **_ppv);

  /////////////////////////////////////////////////////////
  // IFileSourceFilter

  STDMETHODIMP Load(LPCOLESTR _filename, const AM_MEDIA_TYPE *_pmt);
  STDMETHODIMP GetCurFile(LPOLESTR *_filename, AM_MEDIA_TYPE *_pmt);

  /////////////////////////////////////////////////////////
  // IAMFilterMiscFlags

  STDMETHODIMP_(ULONG) GetMiscFlags()
  {
    return AM_FILTER_MISC_FLAGS_IS_SOURCE;
  }
};

class VALibStream: public CSourceStream, public CSourceSeeking
{
protected:
  CCritSec seek_lock;
  bool discontinuity;
  REFERENCE_TIME pos;

  int format;
  AC3Parser ac3;
  DTSParser dts;
  FileParser file;

public:
  VALibStream(TCHAR *_filename, CSource *_parent, HRESULT *_phr);
  virtual ~VALibStream();

  inline const char *get_filename()
  { return file.get_filename(); }

  /////////////////////////////////////////////////////////
  // IUnknown

  STDMETHODIMP NonDelegatingQueryInterface(REFIID _riid, void **_ppv);

  /////////////////////////////////////////////////////////
  // CBaseOutputPin

  HRESULT GetMediaType(int i, CMediaType* pmt);
  HRESULT CheckMediaType(const CMediaType* pmt);
  HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);

  /////////////////////////////////////////////////////////
  // CSourceStream

  HRESULT OnThreadCreate();
  HRESULT OnThreadStartPlay();
  HRESULT FillBuffer(IMediaSample* sample);

  /////////////////////////////////////////////////////////
  // CSourceSeeking

  void restart();
  HRESULT ChangeStart();
  HRESULT ChangeStop();
  HRESULT ChangeRate();
  STDMETHODIMP SetRate(double _rate);
};

#endif
