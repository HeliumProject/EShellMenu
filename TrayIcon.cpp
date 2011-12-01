#include "stdafx.h"
#include "TrayIcon.h"

#include "Application.h"
#include "Shortcut.h"
#include "Helper.h"
#include "Version.h"
#include "resource.h"

using namespace Launcher;

TrayIcon::TrayIcon( Application* application ) 
	: m_Application( application ) 
	, m_Menu( NULL )
	, m_BusyCount( 0 )
	, m_IsMenuShowing( false )
{
	wxIcon icon;
	icon.SetHICON( (HICON)::LoadImage( ::GetModuleHandle( NULL ), MAKEINTRESOURCE( LAUNCHER_ICON ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR ) );
	SetIcon( icon, "Initializing Launcher..." );

	// Connect Events
	Connect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( LauncherEventIDs::Exit, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuExit ), NULL, this );
	Connect( LauncherEventIDs::Help, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuHelp ), NULL, this );
	Connect( LauncherEventIDs::Redraw, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRedraw ), NULL, this );
	Connect( LauncherEventIDs::Refresh, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRefresh ), NULL, this );
}

TrayIcon::~TrayIcon() 
{ 
	// Disconnect Events
	Disconnect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( LauncherEventIDs::Exit, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuExit ), NULL, this );
	Disconnect( LauncherEventIDs::Help, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuHelp ), NULL, this );
	Disconnect( LauncherEventIDs::Redraw, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRedraw ), NULL, this );
	Disconnect( LauncherEventIDs::Refresh, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRefresh ), NULL, this );

	// Dynamically added
	Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );
}

void TrayIcon::Initialize()
{
	Refresh( true );
}

void TrayIcon::Cleanup()
{
	m_Shortcuts.clear();

	RemoveIcon();

	delete m_Menu;
	m_Menu = NULL;
}

void TrayIcon::OnTrayIconClick( wxTaskBarIconEvent& evt ) 
{ 
	if ( m_Menu )
	{
		m_IsMenuShowing = true;

		PopupMenu( m_Menu );

		m_IsMenuShowing = false;
	}
}

void TrayIcon::OnMenuExit( wxCommandEvent& evt )
{
	std::string uninstallProject = "Are you sure you would like to exit the Launcher?";
	if ( m_Application->m_UpdateLauncherNow 
		|| ( wxMessageBox( wxT( uninstallProject.c_str() ),
		wxT( "Exit EShell Launcher?" ),
		wxYES_NO | wxCENTER | wxICON_QUESTION ) == wxYES ) )
	{
		wxExit();
	}
}

void TrayIcon::OnMenuHelp( wxCommandEvent& evt )
{
	std::string aboutLauncher("");
	aboutLauncher += 
		"The EShell Launcher is a system tray applicaiton used to launch \n" \
		"EShell's tools in the correct process environment. \n";
	aboutLauncher +=
		"\nFeatures:\n" \
		"  Shift+Click - add/remove a 'favorite' shortcut.\n" \
		"  Ctrl+Click  - copy a shortcut to the clipboard.\n";

	wxMessageDialog dialog(
		NULL,
		wxT( aboutLauncher.c_str() ),
		wxT( " About EShell Launcher" ),
		wxOK | wxICON_INFORMATION );

	dialog.ShowModal();
}

void TrayIcon::OnMenuRefresh( wxCommandEvent& evt )
{
	wxBusyCursor wait;

	Refresh( true );
}

void TrayIcon::OnMenuRedraw( wxCommandEvent& evt )
{
	wxBusyCursor wait;

	Refresh( false );
}

void TrayIcon::OnMenuShortcut( wxCommandEvent& evt )
{
	wxMenu* projectMenu = wxDynamicCast( evt.GetEventObject(), wxMenu );

	if ( projectMenu && projectMenu->FindItem( evt.GetId() ) )
	{
		wxMenuItem* shortcutMenuItem = projectMenu->FindItem( evt.GetId() ); 
		Shortcut* shortcut = reinterpret_cast<Shortcut*> ( shortcutMenuItem->GetRefData() );

		if ( wxIsCtrlDown() )
		{
			shortcut->CopyToClipboard();
		}
		else if ( wxIsShiftDown() )
		{
			m_Application->AddFavorite( shortcut->m_Command );

			wxCommandEvent pending( wxEVT_COMMAND_MENU_SELECTED, LauncherEventIDs::Redraw );
			AddPendingEvent( pending );
		}
		else if ( shortcut->m_Disable )
		{
			wxString invalidShortcut( "" );

			if ( !shortcut->m_DisableReason.empty() )
			{
				invalidShortcut += "The Launcher was unable to create valid settings for this shortcut\n";
				invalidShortcut += "for the following reason(s):\n";
				invalidShortcut += shortcut->m_DisableReason + "\n";
			}
			else
			{
				invalidShortcut += "The Launcher was unable to create valid settings for this shortcut\n";
				invalidShortcut += "for one or more of the following reasons:\n";
				invalidShortcut += " - EShell.pl did not exist in the expected location.\n";
				invalidShortcut += " - One of the required settings (ESHELL_PATH, or IG_PROJECT_ROOT)\n";
				invalidShortcut += "   is not defined in the Settings file.\n";
			} 
			invalidShortcut += "\n\nYou can copy the shortcut to your clipboard by holding Ctrl and clicking on the shortcut.\n";
			invalidShortcut += "This may give you additional useful information.\n";

			wxMessageDialog dialog(
				NULL,
				invalidShortcut,
				wxT( "Invalid Shortcut" ),
				wxOK | wxICON_ERROR );

			dialog.ShowModal();
		}
		else
		{
			BeginBusy();
			{
				shortcut->Execute();
			}
			EndBusy();
		}
	}
}

void TrayIcon::BeginBusy()
{
	if ( ++m_BusyCount > 1 )
		return;

	// Disconnect events
	Disconnect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( LauncherEventIDs::Refresh, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRefresh ), NULL, this );

	// Clear the current menu and change the icon to notify the user that things are happening
	wxIcon icon;
	icon.SetHICON( (HICON)::LoadImage( ::GetModuleHandle( NULL ), MAKEINTRESOURCE( LAUNCHER_BUSY_ICON ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR ) );
	SetIcon( icon, wxString( m_Application->m_Title.c_str() ) + " Refreshing..." );
}

void TrayIcon::EndBusy()
{  
	if ( --m_BusyCount > 0 )
		return;

	assert( m_BusyCount == 0 );

	// Re-connect events
	Connect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( LauncherEventIDs::Refresh, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRefresh ), NULL, this );

	// Set the icon back
	wxIcon icon;
	icon.SetHICON( (HICON)::LoadImage( ::GetModuleHandle( NULL ), MAKEINTRESOURCE( LAUNCHER_ICON ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR ) );
	SetIcon( icon, m_Application->m_Title.c_str() );
}

void CreateShortcuts( const Settings& settings, const Config& config, M_Shortcut& shortcuts )
{
	M_EnvVar copyEnvVars = config.m_EnvVar;

	V_ShortcutInfo::const_iterator ssItr = config.m_Shortcuts.begin();
	V_ShortcutInfo::const_iterator ssEndItr = config.m_Shortcuts.end();
	for ( ; ssItr != ssEndItr; ++ssItr )
	{
		const ShortcutInfo& shortcutInfo = (*ssItr);

		// create a Shortcut
		wxObjectDataPtr< Shortcut > shortcut ( new Shortcut () );
		shortcut->m_Name = shortcutInfo.m_Name;
		Settings::ProcessValue( shortcut->m_Name, copyEnvVars );

		// Icon
		shortcut->m_Icon = shortcutInfo.m_IconPath;
		Settings::ProcessValue( shortcut->m_Icon, copyEnvVars );

		// Description
		shortcut->m_Description = shortcut->m_Description;
		Settings::ProcessValue( shortcut->m_Description, copyEnvVars );

		// Get EShell path
		std::string eshellPath("");
		if ( copyEnvVars.find( "ESHELL_PATH" ) != copyEnvVars.end() )
		{
			eshellPath = copyEnvVars["ESHELL_PATH"].m_Value;
			Settings::ProcessValue( eshellPath, copyEnvVars );
		}
		else
		{
			shortcut->m_Disable = true;
			shortcut->m_DisableReason = "The required variable \"ESHELL_PATH\" was not defined in the Settings file.";
		}

		if ( !FileExists( eshellPath ) )
		{
			shortcut->m_Disable = true;
			shortcut->m_DisableReason = "eshell.pl did not exist in the expected location:\n  - \"" + eshellPath + "\"";
		}

		// SettingsFile Path
		std::string settingsFile = settings.m_File;

		// Build the Command  
		shortcut->m_Command = "perl.exe \"" + eshellPath + "\"";
		shortcut->m_Command += " -settingsFile \"" + settingsFile + "\"";
		shortcut->m_Command += " -config " + config.m_Name;

		if ( !shortcutInfo.m_Args.empty() )
		{
			shortcut->m_Command += " " + shortcutInfo.m_Args;
		}

		Settings::ProcessValue( shortcut->m_Command, copyEnvVars );

		// Create the Display Folder
		shortcut->m_ProjectName = settings.m_Project;

		// Create the FavoriteName
		shortcut->m_FavoriteName = ""; //shortcut->m_Name;
		shortcut->m_FavoriteName = Capitalize( shortcut->m_Folder, true ) + " - " + shortcut->m_Name;
		
		shortcuts[ settings.m_Project ].push_back( shortcut );
	}
}

void TrayIcon::Refresh( bool reload )
{
	BeginBusy();
	{  
		if ( reload )
		{
			m_Settings.clear();
			m_Shortcuts.clear();

			m_Application->LoadState();

			for ( std::set< std::string >::const_iterator itr = m_Application->m_Projects.begin(), end = m_Application->m_Projects.end(); itr != end; ++itr )
			{
				m_Settings.push_back( Settings() );
				if ( !m_Settings.back().LoadFile( *itr ) )
				{
					m_Settings.resize( m_Settings.size() - 1 );
				}

				for ( M_Config::const_iterator itr = m_Settings.back().m_Configs.begin(), end = m_Settings.back().m_Configs.end(); itr != end; ++itr )
				{
					CreateShortcuts( m_Settings.back(), itr->second, m_Shortcuts );
				}
			}
		}

		CreateMenu();
	}
	EndBusy();
}

void TrayIcon::CreateMenu()
{
	delete m_Menu;
	m_Menu = NULL;
	m_Menu = new wxMenu();

	////////////////////////////////////////
	// add Project SubMenus and separater
	if ( !m_Shortcuts.empty() )
	{
		CreateProjectsMenu( m_Menu );
		m_Menu->AppendSeparator();
	}

	////////////////////////////////////////
	// Refresh
	wxMenuItem* refreshMenuItem = new wxMenuItem( m_Menu, LauncherEventIDs::Refresh, wxT( "Refresh" ), wxEmptyString, wxITEM_NORMAL );
	{
		wxIcon refreshIcon;
		refreshIcon.SetHICON( (HICON)::LoadImage( ::GetModuleHandle( NULL ), MAKEINTRESOURCE( REFRESH_ICON ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR ) );
		refreshIcon.SetSize( 16, 16 );
		refreshMenuItem->SetBitmap( refreshIcon );
	}
	m_Menu->Append( refreshMenuItem );

	////////////////////////////////////////
	// Help
	m_Menu->Append( new wxMenuItem( m_Menu, LauncherEventIDs::Help, wxString("Help"), wxEmptyString, wxITEM_NORMAL ) );
	m_Menu->AppendSeparator();

	////////////////////////////////////////
	// Exit
	wxMenuItem* exitMenuItem;
	exitMenuItem = new wxMenuItem( m_Menu, LauncherEventIDs::Exit, wxString( wxT("Exit") ) , wxEmptyString, wxITEM_NORMAL );
	m_Menu->Append( exitMenuItem );

	////////////////////////////////////////
	// Title
	m_Menu->PrependSeparator();

	wxMenuItem* projectMenuItem = new wxMenuItem(
		m_Menu,
		wxID_ANY,
		wxT( m_Application->m_Title.c_str() ),
		wxT( m_Application->m_Title.c_str() ),
		wxITEM_NORMAL );
	m_Menu->Prepend( projectMenuItem );
	projectMenuItem->Enable( false );
}

void TrayIcon::DetectAndSetIcon( Shortcut& shortcut, wxMenuItem* shortcutMenuItem )
{
	if ( !shortcut.m_Icon.empty() && FileExists( shortcut.m_Icon ) )
	{
		wxBitmap icon;
		icon.LoadFile( shortcut.m_Icon, wxBITMAP_TYPE_PNG );
		shortcutMenuItem->SetBitmap( icon );
	}
	else
	{
		// fetch icon from Windows Shell API
	}
}

void TrayIcon::CreateProjectsMenu( wxMenu* parentMenu )
{
	V_Shortcut favoriteShortcuts;

	M_Shortcut::iterator projItr = m_Shortcuts.begin();
	M_Shortcut::iterator projEnd = m_Shortcuts.end();
	for ( ; projItr != projEnd; ++projItr )
	{
		const std::string& projName = projItr->first;
		V_Shortcut& shortcuts = projItr->second;

		if ( shortcuts.empty() )
			continue;

		wxMenu* projectMenu = new wxMenu();

		typedef std::map< std::string, wxMenu* > M_SubMenues;
		M_SubMenues subMenues;

		V_Shortcut::iterator shortcutItr = shortcuts.begin();
		V_Shortcut::iterator shortcutEnd = shortcuts.end();
		for ( ; shortcutItr != shortcutEnd; ++shortcutItr )
		{
			wxObjectDataPtr< Shortcut > shortcut = (*shortcutItr);

			wxMenuItem* shortcutMenuItem;
			shortcutMenuItem = new wxMenuItem(
				projectMenu,
				wxID_ANY,
				wxString( wxT( shortcut->m_Name.c_str() ) ),
				wxString( wxT( shortcut->m_Description.c_str() ) ),
				wxITEM_NORMAL );

			if ( shortcut->m_Disable )
			{
				shortcutMenuItem->Enable( false );

				wxString name( "INVALID: " );
				name += shortcut->m_Name.c_str();
				shortcutMenuItem->SetText( name );
			}

			DetectAndSetIcon( *shortcut, shortcutMenuItem );

			shortcutMenuItem->SetRefData( shortcut.get() );

			if ( shortcut->m_Folder.empty() )
			{
				projectMenu->Append( shortcutMenuItem );
			}
			else
			{
				wxMenu* menuToInsert = new wxMenu();
				std::pair< M_SubMenues::iterator, bool > insertedSubMenu = subMenues.insert( M_SubMenues::value_type( shortcut->m_Folder, menuToInsert ) );
				insertedSubMenu.first->second->Append( shortcutMenuItem );
				if ( !insertedSubMenu.second )
				{
					// New menu was not inserted, you have to clean it up
					delete menuToInsert;
				}
			}

			Connect( shortcutMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );

			if ( m_Application->IsFavorite( shortcut->m_Command ) )
			{
				favoriteShortcuts.push_back( shortcut );
			}
		}

		M_SubMenues::reverse_iterator subMenuItr = subMenues.rbegin();
		M_SubMenues::reverse_iterator subMenuEnd = subMenues.rend();
		for ( ; subMenuItr != subMenuEnd; ++subMenuItr )
		{
			wxMenuItem* subMenuItem = new wxMenuItem(
				projectMenu,
				wxID_ANY,
				wxString( wxT( subMenuItr->first.c_str() ) ),
				wxString( wxT( subMenuItr->first.c_str() ) ),
				wxITEM_NORMAL,
				subMenuItr->second );

			wxIcon icon;
			icon.SetHICON( ::LoadIcon( ::GetModuleHandle( NULL ), MAKEINTRESOURCE( FOLDER_ICON ) ) );
			subMenuItem->SetBitmap( wxBitmap( icon ) );

			projectMenu->Prepend( subMenuItem );
		}

		parentMenu->Append( wxID_ANY, wxT( Capitalize( projName, true ).c_str() ), projectMenu );
	}

	if ( !favoriteShortcuts.empty() )
	{
		parentMenu->PrependSeparator();

		V_Shortcut::iterator favItr = favoriteShortcuts.begin();
		V_Shortcut::iterator favEnd = favoriteShortcuts.end();
		for( ; favItr != favEnd; ++favItr )
		{
			wxObjectDataPtr< Shortcut > shortcut = (*favItr);

			wxMenuItem* favoritesMenuItem = new wxMenuItem(
				parentMenu,
				wxID_ANY,
				wxString( wxT( shortcut->m_FavoriteName.c_str() ) ),
				wxString( wxT( shortcut->m_Description.c_str() ) ),
				wxITEM_NORMAL
				);

			DetectAndSetIcon( *shortcut, favoritesMenuItem );

			favoritesMenuItem->SetRefData( shortcut.get() );

			parentMenu->Prepend( favoritesMenuItem );

			Connect( favoritesMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );
		}
	}
}
