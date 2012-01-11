;
; Launcher Installer
;

; This script uses Inno Setup Preprocessor (ISPP) by Alex Yackimoff.
; To download and install ISPP, get the Inno Setup QuickStart Pack from http://www.jrsoftware.org/isdl.php#qsp
#define _AppName          	"EShell"
#define _AppMutex         	"EShellLauncher"
#define _AppPublisher     	"Helium Project"
#define _AppPublisherURL  	"http://www.heliumproject.org/"

#ifndef _AppVersionMajor
#define _AppVersionMajor	"0"
#endif

#ifndef _AppVersionMinor
#define _AppVersionMinor	"0"
#endif

#ifndef _AppVersionPatch
#define _AppVersionPatch  	"0"
#endif

#define _AppVersion			_AppVersionMajor + "." + _AppVersionMinor + "." + _AppVersionPatch

#ifndef _BuildConfig
#define _BuildConfig		"Release"
#endif

#define _VersionInfoComments    "EShell is a system tray applicaiton used to launch tools in the correct process environment."
#define _VersionInfoCopyright   "Copyright (C) " + _AppPublisher
#define _VersionInfoDescription "EShell"
#define _VersionInfoVersion     _AppVersion

[Setup]
AllowNoIcons=yes
AlwaysShowDirOnReadyPage=yes
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{1CE781B5-9145-4A7B-9DE9-80E307DEEB48}
AppName={#_AppName}
AppMutex={#_AppMutex}
AppVerName={cm:NameAndVersion,{#_AppName},{#_AppVersion}}
AppPublisher={#_AppPublisher}
AppPublisherURL={#_AppPublisherURL}
AppSupportURL={#_AppPublisherURL}
AppUpdatesURL={#_AppPublisherURL}
Compression=lzma
CreateUninstallRegKey=yes
DefaultDirName={pf64}\{#_AppName}
DisableDirPage=no
DefaultGroupName={#_AppName}
OutputDir=.\
OutputBaseFilename=EShellLauncherSetup
PrivilegesRequired=admin
SetupIconFile=icons\launcher.ico
SolidCompression=yes
Uninstallable=yes
UsePreviousAppDir=yes
UsePreviousGroup=yes
UsePreviousTasks=yes
VersionInfoCompany={#_AppPublisher}
VersionInfoCopyright={#_AppName} {#_AppVersion}
VersionInfoDescription={#_VersionInfoDescription}
VersionInfoProductName={#_AppName}
VersionInfoProductVersion={#_VersionInfoVersion}
VersionInfoTextVersion={#_VersionInfoVersion}
VersionInfoVersion={#_VersionInfoVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "windowsstartupicon"; Description: "Start EShell on Windows Startup (Recommended)"; GroupDescription: "Startup:"
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "build\{#_BuildConfig}\EShellLauncher.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\{#_BuildConfig}\EShellLauncher.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "submodule\EShell\eshell.pl"; DestDir: "{app}"; Flags: ignoreversion
Source: "submodule\StrawberryPerl\c\bin\*"; DestDir: "{app}\StrawberryPerl\c\bin"; Flags: recursesubdirs
Source: "submodule\StrawberryPerl\perl\*"; DestDir: "{app}\StrawberryPerl\perl"; Flags: recursesubdirs

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: string; ValueName: "ESHELL_LAUNCHER_INSTALL_DIR"; ValueData: "{#_InstallDir}"

[Icons]
Name: "{commonstartup}\{#_AppName}"; Filename: "{app}\EShellLauncher.exe"; Comment: {#_VersionInfoComments}; Tasks: windowsstartupicon
Name: "{group}\{#_AppName}"; Filename: "{app}\EShellLauncher.exe"; Comment: {#_VersionInfoComments}
Name: "{group}\{cm:UninstallProgram,{#_AppName}}"; Filename: "{uninstallexe}"; Comment: {#_VersionInfoComments}
Name: "{commondesktop}\{#_AppName}"; Filename: "{app}\EShellLauncher.exe"; Comment: {#_VersionInfoComments}; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#_AppName}"; Filename: "{app}\EShellLauncher.exe"; Comment: {#_VersionInfoComments}; Tasks: quicklaunchicon

[Run]
Filename: "{app}\EShellLauncher.exe"; Description: "Run {#_AppName}"; Flags: nowait postinstall
