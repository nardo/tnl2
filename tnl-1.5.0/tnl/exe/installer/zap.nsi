; zap.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
; optional settings are left to their default settings. The instalelr simply 
; prompts the user asking them where to install, and drops of notepad.exe
; there. If your Windows directory is not C:\windows, change it below.
;

; This script assumes that there is a /Dversion=curVersion

; The name of the installer
Name "Zap!"
BrandingText " "

; The file to write
OutFile "Zap-${version}-Installer.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Zap
AutoCloseWindow true
; Icon ".\main.ico"

; The text to prompt the user to enter a directory
DirText "This will install Zap! version ${version} on your computer. Choose a directory"

; The stuff to install
Section "Zap! Game Files (required)"
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File /r "zap-${version}\*.*"
  Delete $INSTDIR\uninst-zap.exe
  WriteUninstaller $INSTDIR\uninst-zap.exe
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "Zap! has been installed.  Would you like to add shortcuts in the start menu?" \
             IDNO NoStartMenu
    SetOutPath $SMPROGRAMS\Zap
    WriteINIStr "$SMPROGRAMS\Zap\Zap Home Page.url" \
                "InternetShortcut" "URL" "http://www.zapthegame.com/"
    CreateShortCut "$SMPROGRAMS\Zap\Uninstall Zap!.lnk" \
                   "$INSTDIR\uninst-zap.exe"
    SetOutPath $INSTDIR
    CreateShortCut "$SMPROGRAMS\Zap\Zap!.lnk" \
                   "$INSTDIR\zap.exe"
  NoStartMenu:
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "Would you like to add a desktop icon for Zap?" IDNO NoDesktopIcon
    SetOutPath $INSTDIR
    CreateShortCut "$DESKTOP\Zap!.lnk" "$INSTDIR\Zap.exe"
  NoDesktopIcon:
  WriteRegStr HKLM SOFTWARE\Zap "" $INSTDIR
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Zap" \
                   "DisplayName" "Zap! (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Zap" \
                   "UninstallString" '"$INSTDIR\uninst-zap.exe"'
  MessageBox MB_YESNO|MB_ICONQUESTION \
              "Zap! installation has completed.  Would you like to view the README file now?" \
              IDNO NoReadme
        ExecShell open '$INSTDIR\README.txt'
        NoReadme:
SectionEnd ; end the section

Section Uninstall
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "Are you sure you wish to uninstall Zap!?" \
             IDNO Removed

  Delete $SMPROGRAMS\Zap\*.*
  Delete "$DESKTOP\Zap!.lnk"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Zap"
  DeleteRegKey HKLM SOFTWARE\Zap
  RMDir $SMPROGRAMS\Zap
  RMDir /r $INSTDIR
  IfFileExists $INSTDIR 0 Removed 
      MessageBox MB_OK|MB_ICONEXCLAMATION \
                 "Note: $INSTDIR could not be removed."
Removed:
SectionEnd
  


; eof
