#include "ac3file.h"
#include "ac3file_dlg.h"
#include "guids.h"

// setup data

const AMOVIESETUP_MEDIATYPE sudPinTypes[] =
{
{&MEDIATYPE_Audio, &MEDIASUBTYPE_AVI_AC3   },
{&MEDIATYPE_Audio, &MEDIASUBTYPE_DOLBY_AC3 },
{&MEDIATYPE_Audio, &MEDIASUBTYPE_DTS       },
{&MEDIATYPE_Audio, &MEDIASUBTYPE_AVI_DTS   },
};

const AMOVIESETUP_PIN psudPins[] =                 
{
  { 
    L"Output",          // String pin name
    FALSE,              // Is it rendered
    TRUE,               // Is it an output
    FALSE,              // Allowed none
    FALSE,              // Allowed many
    &CLSID_NULL,        // Connects to filter
    L"Input",           // Connects to pin
    sizeof(sudPinTypes) / sizeof(sudPinTypes[0]), // Number of types
    sudPinTypes         // The pin details
  }
};


const AMOVIESETUP_FILTER sudFilter =
{
    &CLSID_AC3File,         // Filter CLSID
    L"AC3File",             // Filter name
    MERIT_NORMAL,           // Its merit      MERIT_PREFERRED
    sizeof(psudPins) / sizeof(psudPins[0]), // Number of pins
    psudPins                // Pin details
};


CFactoryTemplate g_Templates[] = {
    { L"AC3File property page"
    , &CLSID_AC3FileDlg
    , AC3FileDlg::Create },
    { L"AC3File"
    , &CLSID_AC3File
    , &VALibSource::CreateInstance
    , NULL
    , &sudFilter }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// Handle registration of this filter
//

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
  return DllEntryPoint(hinst, reason, reserved);
}
