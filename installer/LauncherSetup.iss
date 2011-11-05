;
; Insomniac Games Launcher Installer
;

; This script uses Inno Setup Preprocessor (ISPP) by Alex Yackimoff.
; To download and install ISPP, get the Inno Setup QuickStart Pack from http://www.jrsoftware.org/isdl.php#qsp
#define IG_AppName          "Insomniac Launcher"
#define IG_AppMutex         "IGToolsLauncher"

#define IG_AppPublisher     "Insomniac Games, Inc."
#define IG_AppPublisherURL  "http://www.insomniacgames.com/"


#ifndef  IG_AppVersionMayor
  #define IG_AppVersionMayor    "0"
#endif

#ifndef  IG_AppVersionMinor
  #define IG_AppVersionMinor    "0"
#endif

#ifndef  IG_AppVersionRelease
  #define IG_AppVersionRelease  "0"
#endif

#ifndef  IG_AppVersionBuild
  #define IG_AppVersionBuild    "0"
#endif

#ifndef  IG_BuildConfig
  #define IG_BuildConfig        "Release"
#endif

#define IG_AppVersion             IG_AppVersionMayor + "." + IG_AppVersionMinor + "." + IG_AppVersionRelease + "." + IG_AppVersionBuild

#define IG_VersionInfoComments    "The Insomniac Launcher is a system tray applicaiton used to launch Insomniac's tools in the correct project environment. This version of the Launcher is compatible with EShell (and related XML settings format ) [Version 2.2.5] and 32bit Perl [Version 5.8.10]"
#define IG_VersionInfoCopyright   "Copyright (C) 2009 " + IG_AppPublisher
#define IG_VersionInfoDescription "Insomniac EShell Tools Launcher"
#define IG_VersionInfoVersion     IG_AppVersion

[Setup]
AllowNoIcons=yes
AlwaysShowDirOnReadyPage=yes
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{1CE781B5-9145-4A7B-9DE9-80E307DEEB48}
AppName={#IG_AppName}
AppMutex={#IG_AppMutex}
AppVerName={cm:NameAndVersion,{#IG_AppName},{#IG_AppVersion}}
AppPublisher={#IG_AppPublisher}
AppPublisherURL={#IG_AppPublisherURL}
AppSupportURL={#IG_AppPublisherURL}
AppUpdatesURL={#IG_AppPublisherURL}
Compression=lzma
CreateUninstallRegKey=yes
DefaultDirName={pf}\{#IG_AppName}
DisableDirPage=no
DefaultGroupName={#IG_AppName}
OutputDir=.\
OutputBaseFilename=LauncherSetup
PrivilegesRequired=admin
SetupIconFile=..\icons\launcher.ico
SolidCompression=yes
Uninstallable=yes
UsePreviousAppDir=yes
UsePreviousGroup=yes
UsePreviousTasks=yes
VersionInfoCompany={#IG_AppPublisher}
VersionInfoCopyright={#IG_AppName} {#IG_AppVersion}
VersionInfoDescription={#IG_VersionInfoDescription}
VersionInfoProductName={#IG_AppName}
VersionInfoProductVersion={#IG_VersionInfoVersion}
VersionInfoTextVersion={#IG_VersionInfoVersion}
VersionInfoVersion={#IG_VersionInfoVersion}


[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "windowsstartupicon"; Description: "Start Launcher on Windows Startup (Recommended)"; GroupDescription: "Startup:"
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}";
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "..\release\changelist.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\built\{#IG_BuildConfig}\InsomniacLauncher.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\built\{#IG_BuildConfig}\libmysql.dll"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonstartup}\{#IG_AppName}"; Filename: "{app}\InsomniacLauncher.exe"; Comment: {#IG_VersionInfoComments}; Tasks: windowsstartupicon
Name: "{group}\{#IG_AppName}"; Filename: "{app}\InsomniacLauncher.exe"; Comment: {#IG_VersionInfoComments}
Name: "{group}\{cm:UninstallProgram,{#IG_AppName}}"; Filename: "{uninstallexe}"; Comment: {#IG_VersionInfoComments}
Name: "{commondesktop}\{#IG_AppName}"; Filename: "{app}\InsomniacLauncher.exe"; Comment: {#IG_VersionInfoComments}; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#IG_AppName}"; Filename: "{app}\InsomniacLauncher.exe"; Comment: {#IG_VersionInfoComments}; Tasks: quicklaunchicon

[Run]
Filename: "{app}\InsomniacLauncher.exe"; Description: "Run {#IG_AppName}"; Flags: nowait postinstall
Filename: "{app}\changelist.txt"; Description: "View the changelist.txt file?"; Flags: postinstall shellexec skipifsilent unchecked

[UninstallDelete]
Type: files; Name: "{userdocs}\.insomniac\Launcher\favorits.txt"
