#ifndef AC3FILE_H
#define AC3FILE_H

#include <streams.h>
#include "guids.h"
#include "parsers\ac3\ac3_parser.h"
#include "parsers\dts\dts_parser.h"
#include "parsers\file_parser.h"

class VALibSource;
class VALibStream;

class VALibSource : public CSource, IFileSourceFilter, IAMFilterMiscFlags, ISpecifyPropertyPages, IAC3File
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
  // IAC3File

  STDMETHODIMP get_info(char *info, int len);
  STDMETHODIMP get_frames(unsigned *frames, unsigned *errors);
  STDMETHODIMP get_pos(unsigned *filepos, unsigned *pos_ms);

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

  int format;
  AC3Parser ac3;
  DTSParser dts;
  FileParser file;

public:
  VALibStream(TCHAR *_filename, CSource *_parent, HRESULT *_phr);
  virtual ~VALibStream();

  inline const char *get_filename()          const { return file.get_filename(); }
  inline void get_info(char *_buf, int _len) const { file.get_info(_buf, _len); }
  inline unsigned get_frames()               const { return (unsigned)file.get_pos(file.frames); }
  inline unsigned get_errors()               const { return file.get_errors(); }
  inline unsigned get_filepos()              const { return (unsigned)file.get_pos(file.bytes); }
  inline unsigned get_pos_ms()               const { return (unsigned)file.get_pos(file.ms); }

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
