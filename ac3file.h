#ifndef AC3FILE_H
#define AC3FILE_H

#include <streams.h>
#include "guids.h"

#include "parsers\file_parser.h"
#include "parsers\multi_header.h"

class VALibSource;
class VALibStream;

class VALibSource : public CSource, IFileSourceFilter, IAMFilterMiscFlags, ISpecifyPropertyPages, IAC3File
{
protected:
  VALibStream *stream;

public:
  VALibSource(char *_filter_name, LPUNKNOWN _lpunk, CLSID _clsid);
  virtual ~VALibSource();
  static CUnknown *WINAPI CreateInstance(LPUNKNOWN _lpunk, HRESULT* _phr);

  /////////////////////////////////////////////////////////
  // IUnknown

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID _riid, void **_ppv);

  /////////////////////////////////////////////////////////
  // IAC3File

  STDMETHODIMP get_info(char *info, size_t len);
  STDMETHODIMP get_pos(unsigned *frames, unsigned *bytes, unsigned *ms);
  STDMETHODIMP get_size(unsigned *frames, unsigned *bytes, unsigned *ms);

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

  /////////////////////////////////////////////////////////
  // ISpecifyPropertyPages

  STDMETHODIMP GetPages(CAUUID *pPages);
};

class VALibStream: public CSourceStream, public CSourceSeeking
{
protected:
  CCritSec seek_lock;
  bool discontinuity;
  REFERENCE_TIME pos;

  FileParser file;
  MultiHeader multi_parser;
  Speakers spk;

public:
  VALibStream(char *_filename, CSource *_parent, HRESULT *_phr);
  virtual ~VALibStream();

  size_t get_info(char *_buf, size_t _len) const;

  inline const char *get_filename()          const { return file.get_filename(); }

  inline unsigned get_pos_frames()           const { return (unsigned)file.get_pos(file.frames); }
  inline unsigned get_pos_bytes()            const { return (unsigned)file.get_pos(file.bytes); }
  inline unsigned get_pos_ms()               const { return (unsigned)file.get_pos(file.time) * 1000; }

  inline unsigned get_size_frames()          const { return (unsigned)file.get_size(file.frames); }
  inline unsigned get_size_bytes()           const { return (unsigned)file.get_size(file.bytes); }
  inline unsigned get_size_ms()              const { return (unsigned)file.get_size(file.time) * 1000; }

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
