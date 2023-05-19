;----------------------------------------------------------------------------------------------------------------------
; NSIS installer Script
; @author Thomas Reidemeister <thomas@labforge.ca>
; @date 2023.03.23
;
;    Copyright 2023 Labforge Inc.
;
;    Licensed under the Apache License, Version 2.0 (the "License");
;    you may not use this file except in compliance with the License.
;    You may obtain a copy of the License at
;
;        http://www.apache.org/licenses/LICENSE-2.0
;
;    Unless required by applicable law or agreed to in writing, software
;    distributed under the License is distributed on an "AS IS" BASIS,
;    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;    See the License for the specific language governing permissions and
;    limitations under the License.

!define COMPANYNAME "${COMPANY}"
!define APPNAME "${NAME}"
!define REGISTRY_KEY "${SHORTNAME}Install"
!define ENTRYPOINT "${SHORTNAME}.exe"

!include "FileFunc.nsh"
!include "MUI2.nsh"

;----------------------------------------------------------------------------------------------------------------------
; Distribution settings
Name "${APPNAME}"
BrandingText "${COMPANYNAME} Installer"
; Set the output directory for the installer
OutFile "${OUT}"
; Set the default installation directory
InstallDir "$PROGRAMFILES64\${COMPANYNAME}\${APPNAME}"

;----------------------------------------------------------------------------------------------------------------------
!macro InstallVC14
  ; Check if distributable is already installed for 64 bit
  ReadRegStr $1 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
  StrCmp $1 1 installed
  ; Install VC runtime
  ExecWait $INSTDIR\vc_redist.x64.exe
  installed: ; Do nothing
  Delete $INSTDIR\vc_redist.x64.exe
!macroend

;----------------------------------------------------------------------------------------------------------------------
; Modern User Interface Customizations
!define MUI_ICON ${SRC}\..\utility\img\labforge.ico
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${SRC}\installer_banner.bmp"
!define MUI_ABORTWARNING
; But do use old-school fullscreen background as seen in the good old Win16 times
BGGradient 0000FF 000000 FFFFFF ; Blue to black gradient, white text

;--------------------------------
; Pages
;--------------------------------
!insertmacro MUI_PAGE_LICENSE "${SRC}\..\LICENSE"

; Uncomment if there are optional components as seen below
;!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

; Copy the necessary files to the installation directory
Section "-hidden section"
  SetOutPath $INSTDIR
  File /r "${SRC}\dist\main\*"

  ; Driver utility install
  File "${SRC}\build\driver.exe"
  File "C:\Program Files\Common Files\Pleora\eBUS SDK\PtUtilsLib64.dll"
  File "C:\Program Files\Common Files\Pleora\eBUS SDK\EbTransportLayerLib64.dll"
  File "C:\Program Files\Common Files\Pleora\eBUS SDK\EbInstallerLib64.dll"
  File "C:\Program Files\Common Files\Pleora\eBUS SDK\log4cxx\x64\Release\log4cxx.dll"
  ExecWait '"$INSTDIR\driver.exe" --install=eBUSUniversalProForEthernet'

  ; Explicitly include GenICam libraries, not directly referenced by pyinstaller
  File /r "C:\Program Files\Common Files\Pleora\eBUS SDK\GenICam\bin\Win64_x64\*"

  createDirectory "$SMPROGRAMS\${COMPANYNAME}"
  createShortCut "$SMPROGRAMS\${COMPANYNAME}\${APPNAME}.lnk" "$INSTDIR\${ENTRYPOINT}"

  ; Calibration Utility for Windows
  File /r "${SRC}\..\stereo_viewer\install\bin\*.exe"
  File /r "${SRC}\..\stereo_viewer\install\bin\*.dll"
  ; Qt windows plugin requirements
  File /r "${SRC}\..\stereo_viewer\install\bin\styles"
  File /r "${SRC}\..\stereo_viewer\install\bin\platforms"
  File /r "${SRC}\..\stereo_viewer\install\bin\imageformats"
  createShortCut "$SMPROGRAMS\${COMPANYNAME}\Stereo Viewer.lnk" "$INSTDIR\stereoviewer.exe"

  ; Add Uninstaller with hooks to "Programs and Features"
  writeUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${REGISTRY_KEY}" \
                   "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${REGISTRY_KEY}" \
                   "UninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${REGISTRY_KEY}" \
                   "DisplayIcon" "$INSTDIR\${ENTRYPOINT},0"



  ; Install MSVC 2015 redistributable
  File "${SRC}\vc_redist.x64.exe"
  !insertmacro InstallVC14
SectionEnd

; Create an uninstaller
Section "Uninstall"
  ; Remove shortcut
  Delete "$SMPROGRAMS\${COMPANYNAME}\${APPNAME}.lnk"
  ; Delete the entire installation dir
  RMDir /R /REBOOTOK "$INSTDIR"
  ; Remove registry entry
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${REGISTRY_KEY}"
SectionEnd