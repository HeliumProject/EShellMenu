#pragma once

namespace Launcher
{
  #define SETUP_ROOT "\\\\locutus.insomniacgames.com\\R-Drive\\Install\\"
  static const char* s_SignalsFolder = SETUP_ROOT"Signals\\";

  static std::string s_DefaultLauncherInstallDir  = "\\\\locutus\\toolshed\\installs\\Tools Launcher\\";
  static std::string s_TestLauncherInstallDir     = "\\\\locutus\\toolshed\\installs\\Tools Launcher\\Test\\";
  static std::string s_DefaultLauncherInstallFile = "LauncherSetup.exe";

  #define SETTINGS_FILENAME         "ProjectSettings.xml"
  #define CODE_SETTINGS_FILENAME    "CodeSettings.xml"
  #define GAME_SETTINGS_FILENAME    "GameSettings.xml"
  #define RELEASE_SETTINGS_FILENAME "ReleaseSettings.xml"
  
  #define PERL_VERSION "5.8.10"
  #define PERL_EXE "perl.exe"

  #define ESHELL_VERSION "2.2.7"

  static const char* s_DefaultAssetBranch   = "devel";
  static const char* s_DefaultBuildConfig   = "release";
  static const char* s_DefaultCodeBranch    = "devel";
  static const char* s_DefaultGame          = "resistance";
  static const char* s_DefaultToolsRelease  = "default";

  static const int s_CheckUpdatesEvery = 1800;
}