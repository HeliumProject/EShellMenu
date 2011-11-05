#pragma once

namespace Launcher
{
	class Application;

	class DisplayShortcut;
	typedef std::vector< DisplayShortcut > V_DisplayShortcut;
	typedef std::map< std::string, V_DisplayShortcut > M_DisplayShortcuts;

	///////////////////////////////////////
	// TrayIcon LauncherEventIDs
	namespace LauncherEventIDs
	{
		enum LauncherEventID
		{
			START_ID = wxID_HIGHEST + 1,

			Exit,
			Help,
			Redraw,
			Refresh,
			UpdateLauncher,
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	/// Class TrayIcon
	///////////////////////////////////////////////////////////////////////////////
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
		void Refresh( bool reloadProjects = true );

		void CreateMenu();
		void DetectAndSetIcon( DisplayShortcut& displayShortcut, wxMenuItem* shortcutMenuItem );
		void CreateProjectsMenu( wxMenu* parentMenu );

	private:
		Application*  m_Application;

		M_DisplayShortcuts  m_DisplayShortcuts;

		wxMenu*       m_Menu;
		int           m_BusyCount;
		bool          m_IsMenuShowing;
	};
}
