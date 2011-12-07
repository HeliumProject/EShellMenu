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

		void AddProject( const tstring& project );
		void AddFavorite( const tstring& command );
		void RemoveFavorite( const tstring& command );
		bool IsFavorite( const tstring& command );

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
		tstring m_Title;
		tstring m_MutexName;

		TrayIcon* m_TrayIcon;
		std::set< tstring > m_Projects;
		std::set< tstring > m_Favorites;

		tstring m_PerlExePath;
		tstring m_PerlLibPath;
		tstring m_EShellPlPath;
		tstring m_LauncherInstallPath;
		uint64_t m_CurrentVersion;
		uint64_t m_NetworkVersion;
		bool m_UpdateLauncherNow;
		wxTimer m_UpdateTimer;
	};
}
