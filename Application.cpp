#include "stdafx.h"
#include "Application.h"

#include "resource.h"

#include "Config.h"
#include "Preferences.h"
#include "ProjectSettings.h"
#include "Helper.h"
#include "Version.h"

#include <regex>

using namespace Launcher;

///////////////////////////////////////////////////////////////////////////////
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
	, m_Test ( false )
{
	// Figure out the current version
	uint32_t versionHi = ( LAUNCHER_VERSION_COMPATIBLE << 16 ) | LAUNCHER_VERSION_FEATURE;
	uint32_t versionLo = ( LAUNCHER_VERSION_PATCH << 16 ) | 0;
	m_CurrentVersion = ( ( uint64_t )versionHi << 32 ) | versionLo;

#ifdef _DEBUG
	m_MutexName = "EShellLauncher_DEBUG";
#else
	m_MutexName = "EShellLauncher";
#endif

	Connect( wxEVT_TIMER, wxTimerEventHandler( Application::OnCheckForUpdatesTimer ), NULL, this );
	Connect( LauncherEventIDs::UpdateLauncher, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( Application::OnMenuUpdateLauncher ), NULL, this ); 
}

///////////////////////////////////////////////////////////////////////////////
Application::~Application()
{
	Disconnect( wxEVT_TIMER, wxTimerEventHandler( Application::OnCheckForUpdatesTimer ), NULL, this );
	Disconnect( LauncherEventIDs::UpdateLauncher, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( Application::OnMenuUpdateLauncher ), NULL, this );
}

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
bool Application::FindFavorite( const std::string& command )
{
	return m_Favorites.find( command ) != m_Favorites.end();
}

///////////////////////////////////////////////////////////////////////////////
std::string Application::GetAvailableVersionString() const
{
	return Launcher::GetFileVersionString( m_LauncherInstallPath );
}

///////////////////////////////////////////////////////////////////////////////
void Application::OnInitCmdLine( wxCmdLineParser& parser )
{
	SetVendorName( "EShell Games" );
	parser.SetLogo( wxT( "Tools Launcher (c) 2009 - EShell Games\n" ) );

	parser.AddOption( "settingsFileName", "SettingsFileName", "Example: ProjectSettingsTest.xml, or ProjectSettings.xml" );
	parser.AddSwitch( "test", "Test", "Used for testing." );
	parser.AddOption( "verbose" );
}

///////////////////////////////////////////////////////////////////////////////
bool Application::OnCmdLineParsed( wxCmdLineParser& parser )
{
	wxString settingsFileName;
	if ( parser.Found( "settingsFileName", &settingsFileName ) )
	{
		m_SettingsFileName = settingsFileName.c_str();
	}

	m_Test = parser.Found( "test" );
	if ( m_Test )
	{
		m_LauncherInstallPath = ( s_TestLauncherInstallDir + s_DefaultLauncherInstallFile );
		m_MutexName = "EShellToolsLauncher_TEST";
		m_Title = "TEST: " + m_Title;
	}

	return __super::OnCmdLineParsed( parser );
}

///////////////////////////////////////////////////////////////////////////////
// Called before OnRun(), this is a good place to do initialization -- if
// anything fails, return false from here to prevent the program from
// continuing. The command line is normally parsed here, call the base
// class OnInit() to do it.
//
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

	//---------------------------------------
	// TODO find PERL_EXE (perl.exe) in the user's path and Check the installed version of perl with PERL_VERSION
	//bool foundPerl = true;
	//if ( !foundPerl )
	//{
	//}

	//---------------------------------------
	// ImageHandlers:
	//    wxWidgets normally doesn't link in image handlers for all types of images,
	//    in order to be a bit more efficient.  Consequently, you have to initialize
	//    and add image handlers.  You can see how it is done for each type in the
	//    demo in MyApp.OnInit.  Or you can call wxInitAllImageHandlers() to do them
	//    all.  However, there is a limitation to doing them all.  TGA files may be
	//    handled by the wxCURHandler instead of the wxTGAHandler, simply because that
	//    handler appears in the list before TGA when you init them all at once.
	//wxImage::AddHandler( new wxJPEGHandler );
	//wxImage::AddHandler( new wxPNGHandler );
	wxInitAllImageHandlers();

	wxImageHandler* curHandler = wxImage::FindHandler( wxBITMAP_TYPE_CUR );
	if ( curHandler )
	{
		// Force the cursor handler to the end of the list so that it doesn't try to
		// open TGA files.
		wxImage::RemoveHandler( curHandler->GetName() );
		curHandler = NULL;
		wxImage::AddHandler( new wxCURHandler );
	}


	//---------------------------------------
	// make the task bar icon
	m_TrayIcon = new TrayIcon( this );


	m_CheckForUpdatesTimer.Start( s_CheckUpdatesEvery, wxTIMER_ONE_SHOT );

	return true;
}

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
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
		if ( m_Test )
		{
			// install the version currently running
			command = "\"" + s_DefaultLauncherInstallDir + LAUNCHER_VERSION_STRING"\\" +  s_DefaultLauncherInstallFile + "\" /SILENT"; //" /MERGETASKS=\"runastest\"";
		}

		Launcher::ExecuteCommand( command, "", false, false );
	}

	return __super::OnExit( );
}

///////////////////////////////////////////////////////////////////////////////
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
				//std::string newVersion = Launcher::GetFileVersionString( m_LauncherInstallPath );
				//wxString itemTitle = "New Update Available";
				//if ( !newVersion.empty() )
				//{
				//  itemTitle += wxString( " [v" ) + wxString( newVersion.c_str() ) + wxString( "]" );
				//}
				//m_TrayIcon->ShowBalloon( wxT("EShell Launcher"), itemTitle );

				wxCommandEvent pending( wxEVT_COMMAND_MENU_SELECTED, LauncherEventIDs::Redraw );
				m_TrayIcon->AddPendingEvent( pending );
			}
			m_TrayIcon->EndBusy();

			m_CheckForUpdatesTimer.Start( s_CheckUpdatesEvery * 1000, wxTIMER_ONE_SHOT );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Checks to see if there is a new version of the Launcher and prompts to install
// it.
void Application::OnMenuUpdateLauncher( wxCommandEvent& evt )
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

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
// Main entry point for the application.
//
#ifdef _DEBUG
long& g_BreakOnAlloc (_crtBreakAlloc);
#endif

#ifdef _DEBUG
IMPLEMENT_APP_CONSOLE( Application );
#else
IMPLEMENT_APP( Application );
#endif