#pragma once

#include "Settings.h"
#include "Shortcut.h"

namespace Launcher
{
	class Application;

	namespace LauncherEventIDs
	{
		enum LauncherEventID
		{
			START_ID = wxID_HIGHEST + 1,

			Exit,
			Help,
			Redraw,
			Refresh,
			Update,
		};
	}

	class TrayIcon : public wxTaskBarIcon
	{
	public:
		TrayIcon( Application* application );
		virtual ~TrayIcon();

		void Initialize();    
		void Cleanup();

		bool IsMenuShowing() const { return m_IsMenuShowing; }
		void BeginBusy();
		void EndBusy();

	protected:
		void OnTrayIconClick( wxTaskBarIconEvent& evt );
		void OnMenuExit( wxCommandEvent& evt );
		void OnMenuHelp( wxCommandEvent& evt );
		void OnMenuRedraw( wxCommandEvent& evt );
		void OnMenuRefresh( wxCommandEvent& evt );
		void OnMenuShortcut( wxCommandEvent& evt );

	private:
		void Refresh( bool reload );

		void CreateMenu();
		void DetectAndSetIcon( Shortcut& shortcut, wxMenuItem* shortcutMenuItem );
		void CreateProjectsMenu( wxMenu* parentMenu );

	private:
		Application* m_Application;
		V_Settings m_Settings;
		M_Shortcut m_Shortcuts;
		wxMenu* m_Menu;
		int m_BusyCount;
		bool m_IsMenuShowing;
	};
}
