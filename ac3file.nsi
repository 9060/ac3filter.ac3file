OutFile "${SETUP_FILE}"
CRCCheck on

InstallDir "$PROGRAMFILES\AC3File"
SetCompressor bzip2
DirText "This will install AC3File ver ${VERSION} to your computer:"

InstallColors {000000 C0C0C0}
InstProgressFlags "smooth"
ShowInstDetails "show"

UninstallText 'This will uninstall AC3File ver ${VERSION}. Hit "Uninstall" to continue.'
ShowUnInstDetails "show"


Section "Install"
  SetOutPath $INSTDIR

  ;; Copy Files
  File /r "${SOURCE_DIR}\*.*"

  ;; Make an uninstaller
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AC3File" "DisplayName" "AC3File (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AC3File" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteUninstaller "uninstall.exe"

  ;; Register filter
  UnRegDLL "$INSTDIR\ac3file.ax"
  RegDll   "$INSTDIR\ac3file.ax"

  ;; Register Source Filter
  WriteRegStr HKLM "Software\Classes\Media Type\Extensions\.ac3" "Source Filter" "{F7380D4C-DE45-4f03-9209-15EBA8552463}"
  WriteRegStr HKLM "Software\Classes\Media Type\Extensions\.dts" "Source Filter" "{F7380D4C-DE45-4f03-9209-15EBA8552463}"

SectionEnd

Section "Uninstall"

  ;; Unregister filter
  UnRegDLL "$INSTDIR\ac3file.ax"

  ;; Unregister Source Filter
  DeleteRegKey   HKLM "Software\Classes\Media Type\Extensions\.ac3"
  DeleteRegKey   HKLM "Software\Classes\Media Type\Extensions\.dts"

  ;; Delete uninstaller
  DeleteRegKey   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AC3File"

  ;; Delete files
  Delete "$INSTDIR\*.*"
  RMDir  "$INSTDIR"

SectionEnd
