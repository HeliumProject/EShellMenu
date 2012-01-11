#include "stdafx.h"
#include "Application.h"

#include "Helper.h"
#include "Project.h"
#include "Version.h"

#include <regex>

using namespace EShellMenu;

static const tstring g_DefaultInstallDir  = wxT("\\\\eshell\\installs\\");

#ifdef _DEBUG
static const int g_UpdateIntervalInSeconds = 60 * 1;
#else
static const int g_UpdateIntervalInSeconds = 60 * 5;
#endif

Application::Application()
	: m_MutexHandle( NULL )
	, m_Title( wxT("EShell Menu v") LAUNCHER_VERSION_STRING )
	, m_TrayIcon( NULL )
	, m_CurrentVersion( 0 )
	, m_NetworkVersion( 0 )
	, m_UpdateNow( false )
	, m_UpdateTimer( this )
{
	// Figure out the current version
	uint32_t versionHi = ( LAUNCHER_VERSION_MAJOR << 16 ) | LAUNCHER_VERSION_MINOR;
	uint32_t versionLo = ( LAUNCHER_VERSION_PATCH << 16 ) | 0;
	m_CurrentVersion = ( ( uint64_t )versionHi << 32 ) | versionLo;

	wxStandardPaths sp;
	wxFileName installDirTxt ( sp.GetExecutablePath() );
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

	m_InstallPath = dir + wxT( "EShellMenuSetup.exe" );

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

	parser.AddOption( wxT("perl"), wxT("PerlLocation"), wxT("The location of the perl distribution (containing /bin)") );
	parser.AddOption( wxT("eshell"), wxT("EShellLocation"), wxT("The location of the directory containing eshell.pl") );

	return __super::OnInitCmdLine( parser );
}

bool Application::OnCmdLineParsed( wxCmdLineParser& parser )
{
	wxString perl;
	if ( parser.Found( wxT("perl"), &perl ) )
	{
		wxFileName name ( perl, wxT("") );
		name.AppendDir( wxT("bin") );
		name.SetName( wxT("perl") );
		name.SetExt( wxT("exe") );
		m_PerlExePath = name.GetFullPath();
	}
	else
	{
		wxStandardPaths sp;
		wxFileName name ( sp.GetExecutablePath() );
		name.AppendDir( wxT("StrawberryPerl") );
		name.AppendDir( wxT("perl") );
		name.AppendDir( wxT("bin") );
		name.SetName( wxT("perl") );
		name.SetExt( wxT("exe") );
		m_PerlExePath = name.GetFullPath();
	}

	wxString eshell;
	if ( parser.Found( wxT("eshell"), &eshell ) )
	{
		wxFileName name ( eshell, wxT("") );
		name.SetName( wxT("eshell") );
		name.SetExt( wxT("pl") );
		m_EShellPlPath = name.GetFullPath();
	}
	else
	{
		wxStandardPaths sp;
		wxFileName name ( sp.GetExecutablePath() );
		name.SetName( wxT("eshell") );
		name.SetExt( wxT("pl") );
		m_EShellPlPath = name.GetFullPath();
	}

	if ( !FileExists( m_PerlExePath ) )
	{
		wxMessageBox( tstring( wxT("Perl doesn't exist at the expected location:\n") ) + m_PerlExePath, wxT("Error"), wxOK | wxICON_ERROR );
		return false;
	}

	if ( !FileExists( m_EShellPlPath ) )
	{
		wxMessageBox( tstring( wxT("EShell.pl doesn't exist at the expected location:\n") ) + m_EShellPlPath, wxT("Error"), wxOK | wxICON_ERROR );
		return false;
	}

	wxFileName cBin ( m_PerlExePath.c_str() );
	cBin.RemoveLastDir(); // pop /bin
	cBin.RemoveLastDir(); // pop /perl
	cBin.AppendDir( wxT("c") );
	cBin.AppendDir( wxT("bin") );
	::SetCurrentDirectory( cBin.GetPath() );

	wxFileName lib ( m_PerlExePath.c_str() );
	lib.RemoveLastDir(); // pop /bin
	lib.RemoveLastDir(); // pop /perl
	lib.AppendDir( wxT("site") );
	lib.AppendDir( wxT("lib") );
	m_PerlLibPath = lib.GetPath();

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

	m_UpdateTimer.Start( g_UpdateIntervalInSeconds * 1000, wxTIMER_ONE_SHOT );

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

	return __super::OnExit( );
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

			EShellMenu::GetFileVersion( m_InstallPath, m_NetworkVersion );

			// only refresh if the network version has changed
			if ( IsUpdateAvailable() )
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

			m_UpdateTimer.Start( g_UpdateIntervalInSeconds * 1000, wxTIMER_ONE_SHOT );
		}
	}
}

void Application::OnMenuUpdate( wxCommandEvent& evt )
{
	m_UpdateNow = false;

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