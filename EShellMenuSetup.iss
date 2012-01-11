; This script uses Inno Setup Preprocessor (ISPP) by Alex Yackimoff.
; To download and install ISPP, get the Inno Setup QuickStart Pack from http://www.jrsoftware.org/isdl.php#qsp

#define _AppName          	"EShell Menu"
#define _AppMutex         	"EShellMenu"
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

#define _VersionInfoComments    "EShell Menu is a system tray applicaiton used to launch tools in the correct process environment."
#define _VersionInfoCopyright   "Copyright (C) " + _AppPublisher
#define _VersionInfoDescription "EShell Menu"
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
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
Compression=lzma
CreateUninstallRegKey=yes
DefaultDirName={pf64}\{#_AppName}
DisableDirPage=no
DefaultGroupName={#_AppName}
OutputDir=.\
OutputBaseFilename=EShellMenuSetup
PrivilegesRequired=admin
SetupIconFile=icons\launcher.ico
SolidCompression=yes
Uninstallable=yes
UninstallDisplayIcon={uninstallexe}
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
Name: "windowsstartupicon"; Description: "Start EShell Menu on Windows Startup (Recommended)"; GroupDescription: "Startup:"
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "install_dir.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\{#_BuildConfig}\EShellMenu.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\{#_BuildConfig}\EShellMenu.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "submodule\EShell\eshell.pl"; DestDir: "{app}"; Flags: ignoreversion
Source: "submodule\StrawberryPerl\c\bin\*"; DestDir: "{app}\StrawberryPerl\c\bin"; Flags: recursesubdirs
Source: "submodule\StrawberryPerl\perl\*"; DestDir: "{app}\StrawberryPerl\perl"; Flags: recursesubdirs

[Icons]
Name: "{commonstartup}\{#_AppName}"; Filename: "{app}\EShellMenu.exe"; Comment: {#_VersionInfoComments}; Tasks: windowsstartupicon
Name: "{group}\{#_AppName}"; Filename: "{app}\EShellMenu.exe"; Comment: {#_VersionInfoComments}
Name: "{group}\{cm:UninstallProgram,{#_AppName}}"; Filename: "{uninstallexe}"; Comment: {#_VersionInfoComments}
Name: "{commondesktop}\{#_AppName}"; Filename: "{app}\EShellMenu.exe"; Comment: {#_VersionInfoComments}; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#_AppName}"; Filename: "{app}\EShellMenu.exe"; Comment: {#_VersionInfoComments}; Tasks: quicklaunchicon

[Run]
Filename: "{app}\EShellMenu.exe"; Description: "Run {#_AppName}"; Flags: nowait postinstall

[Code]
/////////////////////////////////////////////////////////////////////
function GetUninstallString(): String;
var
  sUnInstPath: String;
  sUnInstallString: String;
begin
  sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\{#emit SetupSetting("AppId")}_is1');
  sUnInstallString := '';
  if not RegQueryStringValue(HKLM, sUnInstPath, 'UninstallString', sUnInstallString) then
    RegQueryStringValue(HKCU, sUnInstPath, 'UninstallString', sUnInstallString);
  Result := sUnInstallString;
end;


/////////////////////////////////////////////////////////////////////
function IsUpgrade(): Boolean;
begin
  Result := (GetUninstallString() <> '');
end;


/////////////////////////////////////////////////////////////////////
function UnInstallOldVersion(): Integer;
var
  sUnInstallString: String;
  iResultCode: Integer;
begin
// Return Values:
// 1 - uninstall string is empty
// 2 - error executing the UnInstallString
// 3 - successfully executed the UnInstallString

  // default return value
  Result := 0;

  // get the uninstall string of the old app
  sUnInstallString := GetUninstallString();
  if sUnInstallString <> '' then begin
    sUnInstallString := RemoveQuotes(sUnInstallString);
    if Exec(sUnInstallString, '/SILENT /NORESTART /SUPPRESSMSGBOXES','', SW_HIDE, ewWaitUntilTerminated, iResultCode) then
      Result := 3
    else
      Result := 2;
  end else
    Result := 1;
end;

/////////////////////////////////////////////////////////////////////
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep=ssInstall) then
  begin
    if (IsUpgrade()) then
    begin
      UnInstallOldVersion();
    end;
  end;
end;