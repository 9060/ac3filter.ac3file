#ifndef VALIBSOURCEGUIDS_H
#define VALIBSOURCEGUIDS_H

///////////////////////////////////////////////////////////////////////////////
// CLSIDs
///////////////////////////////////////////////////////////////////////////////

// {F7380D4C-DE45-4f03-9209-15EBA8552463}
DEFINE_GUID(CLSID_AC3File,
0xf7380d4c, 0xde45, 0x4f03, 0x92, 0x9, 0x15, 0xeb, 0xa8, 0x55, 0x24, 0x63);

// {8AA8EDDE-659E-4003-87FE-B05F73A2D964}
DEFINE_GUID(CLSID_AC3FileDlg, 
0x8aa8edde, 0x659e, 0x4003, 0x87, 0xfe, 0xb0, 0x5f, 0x73, 0xa2, 0xd9, 0x64);

// {6EDE6010-A521-46f8-8CD2-6475C5CFC279}
DEFINE_GUID(IID_IAC3File, 
0x6ede6010, 0xa521, 0x46f8, 0x8c, 0xd2, 0x64, 0x75, 0xc5, 0xcf, 0xc2, 0x79);

///////////////////////////////////////////////////////////////////////////////
// Media types
///////////////////////////////////////////////////////////////////////////////

DEFINE_GUID(MEDIASUBTYPE_AVI_AC3, 
0x00002000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

DEFINE_GUID(MEDIASUBTYPE_AVI_DTS, 
0x00002001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

///////////////////////////////////////////////////////////////////////////////
// Interfaces
///////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_(IAC3File, IUnknown)
{
  // Stats
  STDMETHOD (get_info)  (char *info, int len) = 0;
  STDMETHOD (get_frames)(unsigned *frames, unsigned *errors) = 0;
  STDMETHOD (get_pos)   (unsigned *filepos, unsigned *pos_ms) = 0;
};


#endif