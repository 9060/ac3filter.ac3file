#ifndef AC3FILE_DLG_H
#define AC3FILE_DLG_H

#include <streams.h>
#include "guids.h"

          
class AC3FileDlg : public CBasePropertyPage
{
public:
  static CUnknown * WINAPI Create(LPUNKNOWN lpunk, HRESULT *phr);


private:
  IAC3File *filter;

  bool     visible;
  bool     refresh;
  int      refresh_time;

  AC3FileDlg(TCHAR *pName, LPUNKNOWN lpunk, HRESULT *phr, int DialogId, int TitleId, int flags);

  INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  HRESULT OnConnect(IUnknown *pUnknown);
  HRESULT OnDisconnect();
  HRESULT OnActivate();
  HRESULT OnDeactivate();

  void command(int control, int message);
  void reload_state();

  void init_controls();
  void set_dynamic_controls();
  void set_controls();
};

#endif
