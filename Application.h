#pragma once

#include "TrayIcon.h"

#include <set>
#include <string>
#include <stdint.h>

typedef std::set<std::string> S_string;

namespace Launcher
{
	// The "Controller" between the Settings and the TrayIcon
	class Application : public wxApp
	{
	public:
		Application();
		virtual ~Application();

		bool LoadSettings();

		void AddFavorite( const std::string& command );
		bool FindFavorite( const std::string& command );

		bool IsUpdateAvailable() const { return m_NetworkVersion > m_CurrentVersion; }
		std::string GetAvailableVersionString() const;

		friend class Launcher::TrayIcon;

	protected:
		virtual void OnInitCmdLine( wxCmdLineParser& parser );
		virtual bool OnCmdLineParsed( wxCmdLineParser& parser );
		virtual bool OnInit();
		virtual int OnRun();
		virtual int OnExit();

		void OnCheckForUpdatesTimer( wxTimerEvent& evt );
		void OnMenuUpdate( wxCommandEvent& evt );

	private:
		HANDLE m_MutexHandle;
		std::string m_SettingsFileName;
		std::string m_Title;
		std::string m_MutexName;

		TrayIcon* m_TrayIcon;

		S_string m_ProjectNames;
		S_string m_Favorites;

		std::string m_LauncherInstallPath;
		uint64_t m_CurrentVersion;
		uint64_t m_NetworkVersion;
		bool m_UpdateLauncherNow;
		wxTimer m_CheckForUpdatesTimer;
	};
}
