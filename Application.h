#pragma once

#include "Settings.h"
#include "TrayIcon.h"

#include <set>
#include <string>
#include <stdint.h>

namespace Launcher
{
	// The "Controller" between the Settings and the TrayIcon
	class Application : public wxApp
	{
	public:
		Application();
		virtual ~Application();

		void LoadState();

		void AddProject( const std::string& project );
		void AddFavorite( const std::string& command );
		void RemoveFavorite( const std::string& command );
		bool IsFavorite( const std::string& command );

		bool IsUpdateAvailable() const
		{
			return m_NetworkVersion > m_CurrentVersion;
		}

		friend class Launcher::TrayIcon;

	protected:
		virtual void OnInitCmdLine( wxCmdLineParser& parser );
		virtual bool OnCmdLineParsed( wxCmdLineParser& parser );
		virtual bool OnInit();
		virtual int OnRun();
		virtual int OnExit();

		void OnUpdateTimer( wxTimerEvent& evt );
		void OnMenuUpdate( wxCommandEvent& evt );

	private:
		HANDLE m_MutexHandle;
		std::string m_Title;
		std::string m_MutexName;

		TrayIcon* m_TrayIcon;
		std::set< std::string > m_Projects;
		std::set< std::string > m_Favorites;

		std::string m_PerlExePath;
		std::string m_PerlLibPath;
		std::string m_EShellPlPath;
		std::string m_LauncherInstallPath;
		uint64_t m_CurrentVersion;
		uint64_t m_NetworkVersion;
		bool m_UpdateLauncherNow;
		wxTimer m_UpdateTimer;
	};
}
