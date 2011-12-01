#include "stdafx.h"
#include "Application.h"

#include "Config.h"
#include "Helper.h"
#include "Preferences.h"
#include "Settings.h"
#include "Version.h"

#include <regex>

using namespace Launcher;

Application::Application()
	: m_MutexHandle( NULL )
	, m_Title( "EShell Launcher [v"LAUNCHER_VERSION_STRING"]" )
	, m_TrayIcon( NULL )
	, m_SettingsFileName( SETTINGS_FILENAME )
	, m_CurrentVersion( 0 )
	, m_NetworkVersion( 0 )
	, m_UpdateLauncherNow( false )
	, m_CheckForUpdatesTimer( this )
	, m_LauncherInstallPath( s_DefaultLauncherInstallDir + s_DefaultLauncherInstallFile )
{
	// Figure out the current version
	uint32_t versionHi = ( LAUNCHER_VERSION_MAJOR << 16 ) | LAUNCHER_VERSION_MINOR;
	uint32_t versionLo = ( LAUNCHER_VERSION_PATCH << 16 ) | 0;
	m_CurrentVersion = ( ( uint64_t )versionHi << 32 ) | versionLo;

#ifdef _DEBUG
	m_MutexName = "EShellLauncher_DEBUG";
#else
	m_MutexName = "EShellLauncher";
#endif

	Connect( wxEVT_TIMER, wxTimerEventHandler( Application::OnCheckForUpdatesTimer ), NULL, this );
	Connect( LauncherEventIDs::Update, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( Application::OnMenuUpdate ), NULL, this ); 
}

Application::~Application()
{
	Disconnect( wxEVT_TIMER, wxTimerEventHandler( Application::OnCheckForUpdatesTimer ), NULL, this );
	Disconnect( LauncherEventIDs::Update, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( Application::OnMenuUpdate ), NULL, this );
}

void Application::AddFavorite( const std::string& command )
{
	if ( m_Favorites.find( command ) != m_Favorites.end() )
	{
		m_Favorites.erase( command );
	}
	else
	{
		m_Favorites.insert( command );
	}

	Preferences::SaveFavorites( m_Favorites );
}

bool Application::FindFavorite( const std::string& command )
{
	return m_Favorites.find( command ) != m_Favorites.end();
}

std::string Application::GetAvailableVersionString() const
{
	return Launcher::GetFileVersionString( m_LauncherInstallPath );
}

void Application::OnInitCmdLine( wxCmdLineParser& parser )
{
	SetVendorName( "EShell Games" );
	parser.SetLogo( wxT( "Tools Launcher (c) 2009 - EShell Games\n" ) );

	parser.AddOption( "settingsFileName", "SettingsFileName", "Example: ProjectSettingsTest.xml, or Settings.xml" );
	parser.AddSwitch( "test", "Test", "Used for testing." );
	parser.AddOption( "verbose" );
}

bool Application::OnCmdLineParsed( wxCmdLineParser& parser )
{
	wxString settingsFileName;
	if ( parser.Found( "settingsFileName", &settingsFileName ) )
	{
		m_SettingsFileName = settingsFileName.c_str();
	}

	return __super::OnCmdLineParsed( parser );
}

bool Application::OnInit()
{    
	if ( !__super::OnInit() )
	{
		return false;
	}

	bool tryAgain = true;
	bool haveMutex = false;
	do 
	{
		//---------------------------------------
		// try to get the Mutex
		m_MutexHandle = ::CreateMutex( NULL, true, m_MutexName.c_str() );

		if ( m_MutexHandle == NULL || ::GetLastError() == ERROR_ALREADY_EXISTS )
		{
			// if we couldn't get it, ask if we want to try again
			int promptResult = wxMessageBox(
				wxT( "EShell Launcher is currently running.\n\nPlease close all instances of it now, then click OK to continue, or Cancel to exit." ),
				wxT( "EShell Launcher" ),
				wxOK |wxCANCEL | wxCENTER | wxICON_QUESTION );

			if ( promptResult == wxCANCEL )
			{
				return false;
			}

			CloseHandle(m_MutexHandle);
			m_MutexHandle = NULL;
			tryAgain = true;
		}
		else
		{
			// we got it
			haveMutex = true;
			tryAgain = false;
		}
	}
	while ( tryAgain );

	wxInitAllImageHandlers();

	wxImageHandler* curHandler = wxImage::FindHandler( wxBITMAP_TYPE_CUR );
	if ( curHandler )
	{
		// Force the cursor handler to the end of the list so that it doesn't try to open TGA files.
		wxImage::RemoveHandler( curHandler->GetName() );
		curHandler = NULL;
		wxImage::AddHandler( new wxCURHandler );
	}

	m_TrayIcon = new TrayIcon( this );

	m_CheckForUpdatesTimer.Start( s_CheckUpdatesEvery, wxTIMER_ONE_SHOT );

	return true;
}

int Application::OnRun()
{
	try
	{
		m_TrayIcon->Initialize();

		return __super::OnRun( );
	}
	catch ( std::exception& ex )
	{
		wxString error( "EShell Launcher failed to run.\n\n  Reason:\n  ");
		error += ex.what();
		error += "\n\nExiting...\n"; 

		wxMessageDialog dialog(
			NULL,
			error,
			wxT("EShell Launcher Error"), wxOK | wxICON_INFORMATION );
		dialog.ShowModal();
	}

	return 1;
}

int Application::OnExit()
{
	// clean up the TrayIcon
	m_TrayIcon->Cleanup();
	delete m_TrayIcon;
	m_TrayIcon = NULL;

	wxImage::CleanUpHandlers();

	CloseHandle(m_MutexHandle);
	m_MutexHandle = NULL;

	// Update the launcher if necessary.
	if ( m_UpdateLauncherNow && FileExists( m_LauncherInstallPath ) )
	{
		std::string command = "\"" + m_LauncherInstallPath + "\" /SILENT";
		Launcher::ExecuteCommand( command, "", false, false );
	}

	return __super::OnExit( );
}

void Application::OnCheckForUpdatesTimer( wxTimerEvent& evt )
{
	if( evt.GetId() == m_CheckForUpdatesTimer.GetId() )
	{
		if ( m_TrayIcon->IsMenuShowing() )
		{
			// the menu was open, start the timer but make it sooner than usual
			m_CheckForUpdatesTimer.Start( 10 * 1000, wxTIMER_ONE_SHOT );
		}
		else
		{
			m_TrayIcon->BeginBusy();

			uint64_t previousNetworkVersion = m_NetworkVersion;

			Launcher::GetFileVersion( m_LauncherInstallPath, m_NetworkVersion );

			// only refresh if the network version has changed
			if ( IsUpdateAvailable() && previousNetworkVersion != m_NetworkVersion )
			{
				std::string newVersion = Launcher::GetFileVersionString( m_LauncherInstallPath );
				wxString itemTitle = "New Update Available";
				if ( !newVersion.empty() )
				{
				  itemTitle += wxString( " [v" ) + wxString( newVersion.c_str() ) + wxString( "]" );
				}
				m_TrayIcon->ShowBalloon( wxT("EShell Launcher"), itemTitle );

				wxCommandEvent pending( wxEVT_COMMAND_MENU_SELECTED, LauncherEventIDs::Redraw );
				m_TrayIcon->AddPendingEvent( pending );
			}
			m_TrayIcon->EndBusy();

			m_CheckForUpdatesTimer.Start( s_CheckUpdatesEvery * 1000, wxTIMER_ONE_SHOT );
		}
	}
}

void Application::OnMenuUpdate( wxCommandEvent& evt )
{
	m_UpdateLauncherNow = false;
	if ( IsUpdateAvailable() )
	{
		const char* title = "Update Launcher?";
		const char* msg = "There is an update available for the Launcher.  Would you like to exit the Launcher and update now?";
		m_UpdateLauncherNow = wxYES == wxMessageBox( msg, title, wxYES_NO | wxICON_QUESTION );

		if ( m_UpdateLauncherNow )
		{
			// Shut down the launcher.  Doing this delayed is not actually
			// necessary, but it's still a good idea not to shutdown within
			// a callback.  Of course, all we are doing is shutting down from
			// another callback, but that's because the launcher doesn't have
			// a top level window frame.
			wxCommandEvent pending( wxEVT_COMMAND_MENU_SELECTED, LauncherEventIDs::Exit );
			m_TrayIcon->AddPendingEvent( pending );
		}
	}
}

bool Application::LoadSettings()
{
#if 0
	if ( !m_ProjectSettings.LoadFile( settingsFile ) )
	{
		return false;
	}
#endif

	m_Favorites.clear();
	Preferences::LoadFavorites( m_Favorites );

	return true;
}

#ifdef _DEBUG
long& g_BreakOnAlloc (_crtBreakAlloc);
#endif

#ifdef _DEBUG
IMPLEMENT_APP_CONSOLE( Application );
#else
IMPLEMENT_APP( Application );
#endif