#include "stdafx.h"
#include "Application.h"

#include "Helper.h"
#include "Project.h"
#include "resource.h"

#include <regex>

using namespace EShellMenu;

static const tstring g_DefaultInstallDir  = wxT("\\\\eshell\\installs\\");

#ifdef _DEBUG
static int g_UpdateIntervalMS = 1000 * 60;
#else
static int g_UpdateIntervalMS = 1000 * 60 * 5;
#endif

Application::Application()
	: m_MutexHandle( NULL )
	, m_Title( wxT("EShell Menu v") VERSION_STRING )
	, m_TrayIcon( NULL )
	, m_CurrentVersion( 0 )
	, m_NetworkVersion( 0 )
	, m_UpdateNow( false )
	, m_UpdateTimer( this )
{
	// Figure out the current version
	uint32_t versionHi = ( VERSION_MAJOR << 16 ) | VERSION_MINOR;
	uint32_t versionLo = ( VERSION_PATCH << 16 ) | 0;
	m_CurrentVersion = ( ( uint64_t )versionHi << 32 ) | versionLo;

	wxFileName installDirTxt ( wxStandardPaths::Get().GetExecutablePath() );
	installDirTxt.SetName( "install_dir" );
	installDirTxt.SetExt( "txt" );
	
	tstring dir = g_DefaultInstallDir;
	if ( wxFileName::FileExists( installDirTxt.GetFullPath() ) )
	{
		tifstream in( static_cast< const tchar_t* >( installDirTxt.GetFullPath().c_str() ) );
		if ( in.good() )
		{
			tstring line;
			if ( getline( in, line ) )
			{
				in.close();
				dir = line;

				if ( *dir.rbegin() != wxT('\\') && *dir.rbegin() != wxT('/') )
				{
					dir.append( wxT("\\") );
				}
			}
		}
	}

	// just like handling -install below
	wxFileName name;
	name.SetPath( dir );
	name.SetName( wxT( "EShellMenuSetup.exe" ) );
	m_InstallPath = name.GetFullPath();
	EShellMenu::GetFileVersion( m_InstallPath, m_NetworkVersion );

#ifdef _DEBUG
	m_MutexName = wxT("EShellMenu_DEBUG");
#else
	m_MutexName = wxT("EShellMenu");
#endif

	Connect( wxEVT_TIMER, wxTimerEventHandler( Application::OnUpdateTimer ), NULL, this );
	Connect( EventIDs::Update, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( Application::OnMenuUpdate ), NULL, this ); 
}

Application::~Application()
{
	Disconnect( wxEVT_TIMER, wxTimerEventHandler( Application::OnUpdateTimer ), NULL, this );
	Disconnect( EventIDs::Update, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( Application::OnMenuUpdate ), NULL, this );
}

void Application::AddProject( const tstring& project )
{
	m_Projects.insert( project );

	SaveTextFile( EShellMenu::GetUserFile( wxT("projects"), wxT("txt") ), m_Projects );
}

void Application::RemoveProject( const tstring& project )
{
	m_Projects.erase( project );

	SaveTextFile( EShellMenu::GetUserFile( wxT("projects"), wxT("txt") ), m_Projects );
}

void Application::AddFavorite( const tstring& command )
{
	m_Favorites.insert( command );

	SaveTextFile( EShellMenu::GetUserFile( wxT("favorites"), wxT("txt") ), m_Favorites );
}

void Application::RemoveFavorite( const tstring& command )
{
	m_Favorites.erase( command );

	SaveTextFile( EShellMenu::GetUserFile( wxT("favorites"), wxT("txt") ), m_Favorites );
}

bool Application::IsFavorite( const tstring& command )
{
	return m_Favorites.find( command ) != m_Favorites.end();
}

void Application::OnInitCmdLine( wxCmdLineParser& parser )
{
	SetVendorName( wxT("Helium Project") );
	parser.SetLogo( wxT("EShell Menu (c) 20xx - Helium Project\n") );
	parser.AddSwitch( wxT("u"), wxT("update"), wxT("Update to the latest version and exit") );
	parser.AddOption( wxT("i"), wxT("install"), wxT("Location of the directory containing the installer") );
	parser.AddOption( wxT("e"), wxT("eshell"), wxT("Location of the directory containing eshell.bat") );

	return __super::OnInitCmdLine( parser );
}

bool Application::OnCmdLineParsed( wxCmdLineParser& parser )
{
	if ( parser.FoundSwitch( wxT("update" ) ) )
	{
		m_UpdateNow = IsUpdateAvailable();
	}

	wxString install;
	if ( parser.Found( wxT("install"), &install ) )
	{
		// just in the ctor above
		wxFileName name;
		name.SetPath( install );
		name.SetName( wxT( "EShellMenuSetup.exe" ) );
		EShellMenu::GetFileVersion( m_InstallPath, m_NetworkVersion );
	}

	wxString eshell;
	if ( parser.Found( wxT("eshell"), &eshell ) )
	{
		wxFileName name ( eshell, wxT("") );
		name.SetName( wxT("eshell") );
		name.SetExt( wxT("bat") );
		m_EShellPath = name.GetFullPath();
	}
	else
	{
		wxFileName name ( wxStandardPaths::Get().GetExecutablePath() );
		name.SetName( wxT("eshell") );
		name.SetExt( wxT("bat") );
		while ( name.GetDirCount() )
		{
			if ( name.FileExists() )
			{
				m_EShellPath = name.GetFullPath();
				break;
			}
			else
			{
				name.RemoveLastDir();
			}
		}
	}

	if ( !FileExists( m_EShellPath ) )
	{
		wxMessageBox( tstring( wxT("eshell.bat doesn't exist at the expected location:\n") ) + m_EShellPath, wxT("Error"), wxOK | wxICON_ERROR );
		return false;
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
				wxT( "EShell Menu is currently running.\n\nPlease close all instances of it now, then click OK to continue, or Cancel to exit." ),
				wxT( "EShell Menu" ),
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

	if ( m_UpdateNow )
	{
		m_TrayIcon->AddPendingEvent( wxCommandEvent ( wxEVT_COMMAND_MENU_SELECTED, EventIDs::Exit ) );
	}
	else
	{
		m_UpdateTimer.Start( g_UpdateIntervalMS, wxTIMER_ONE_SHOT );
	}

	return true;
}

int Application::OnRun()
{
	try
	{
		m_TrayIcon->Initialize();

		return __super::OnRun();
	}
	catch ( std::exception& ex )
	{
		wxString error( "EShell Menu failed to run.\n\n  Reason:\n  ");
		error += ex.what();
		error += "\n\nExiting...\n"; 

		wxMessageDialog dialog(
			NULL,
			error,
			wxT("EShell Menu Error"), wxOK | wxICON_INFORMATION );
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
	if ( m_UpdateNow && FileExists( m_InstallPath ) )
	{
		tstring command = "\"" + m_InstallPath + "\" /SILENT";
		EShellMenu::ExecuteCommand( command, false, false );
	}

	return __super::OnExit();
}

void Application::OnUpdateTimer( wxTimerEvent& evt )
{
	if( evt.GetId() == m_UpdateTimer.GetId() )
	{
		if ( m_TrayIcon->IsMenuShowing() )
		{
			// the menu was open, start the timer but make it sooner than usual
			m_UpdateTimer.Start( 10 * 1000, wxTIMER_ONE_SHOT );
		}
		else
		{
			m_TrayIcon->BeginBusy();

			bool success = EShellMenu::GetFileVersion( m_InstallPath, m_NetworkVersion );

			// only refresh if the network version has changed
			if ( success && IsUpdateAvailable() )
			{
				tstring newVersion = EShellMenu::GetFileVersionString( m_NetworkVersion );
				wxString text = "New Update Available";
				if ( !newVersion.empty() )
				{
					text += wxString( ": v" ) + wxString( newVersion.c_str() );
				}
				m_TrayIcon->ShowBalloon( wxT("EShell Menu"), text );

				m_TrayIcon->Refresh( false );
			}
			else
			{
				m_TrayIcon->Refresh( true );
			}

			m_TrayIcon->EndBusy();

			m_UpdateTimer.Start( g_UpdateIntervalMS, wxTIMER_ONE_SHOT );
		}
	}
}

void Application::OnMenuUpdate( wxCommandEvent& evt )
{
	const tchar_t* title = wxT("Update EShell Menu?");
	const tchar_t* msg = wxT("There is an update available for Eshell Menu.  Would you like to exit and update now?");
	m_UpdateNow = wxYES == wxMessageBox( msg, title, wxYES_NO | wxICON_QUESTION );
	if ( m_UpdateNow )
	{
		m_TrayIcon->AddPendingEvent( wxCommandEvent ( wxEVT_COMMAND_MENU_SELECTED, EventIDs::Exit ) );
	}
}

void Application::LoadState()
{
	m_Projects.clear();
	LoadTextFile( EShellMenu::GetUserFile( wxT("projects"), wxT("txt") ), m_Projects );

	m_Favorites.clear();
	LoadTextFile( EShellMenu::GetUserFile( wxT("favorites"), wxT("txt") ), m_Favorites );
}

#ifdef _DEBUG
long& g_BreakOnAlloc (_crtBreakAlloc);
#endif

#ifdef _DEBUG
IMPLEMENT_APP_CONSOLE( Application );
#else
IMPLEMENT_APP( Application );
#endif