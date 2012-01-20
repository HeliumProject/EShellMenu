#pragma once

#include "Project.h"
#include "TrayIcon.h"

#include <set>
#include <string>
#include <stdint.h>

namespace EShellMenu
{
	class Application : public wxApp
	{
	public:
		Application();
		virtual ~Application();

		void LoadState();

		void AddProject( const tstring& project );
		void RemoveProject( const tstring& project );

		void AddFavorite( const tstring& command );
		void RemoveFavorite( const tstring& command );
		bool IsFavorite( const tstring& command );

		bool IsUpdateAvailable() const
		{
			return m_NetworkVersion > m_CurrentVersion;
		}

		friend class EShellMenu::TrayIcon;

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

		tstring m_EShellPath;
		tstring m_InstallPath;
		uint64_t m_CurrentVersion;
		uint64_t m_NetworkVersion;
		bool m_UpdateNow;
		wxTimer m_UpdateTimer;
	};
}
