#include "stdafx.h"
#include "TrayIcon.h"

#include "Application.h"
#include "Shortcut.h"
#include "Helper.h"
#include "Version.h"
#include "resource.h"

#include <shellapi.h>

using namespace Launcher;

TrayIcon::TrayIcon( Application* application ) 
	: m_Application( application ) 
	, m_Menu( NULL )
	, m_UpdateMenuItem( NULL )
	, m_BusyCount( 0 )
	, m_IsMenuShowing( false )
{
	SetIcon( wxICON( LAUNCHER_ICON ), "Initializing Launcher..." );

	// Connect Events
	Connect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( LauncherEventIDs::Exit, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuExit ), NULL, this );
	Connect( LauncherEventIDs::Help, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuHelp ), NULL, this );
	Connect( LauncherEventIDs::Refresh, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRefresh ), NULL, this );
	Connect( LauncherEventIDs::Reload, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuReload ), NULL, this );
	Connect( LauncherEventIDs::Add, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuAdd ), NULL, this );
}

TrayIcon::~TrayIcon() 
{ 
	// Disconnect Events
	Disconnect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( LauncherEventIDs::Exit, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuExit ), NULL, this );
	Disconnect( LauncherEventIDs::Help, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuHelp ), NULL, this );
	Disconnect( LauncherEventIDs::Refresh, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRefresh ), NULL, this );
	Disconnect( LauncherEventIDs::Reload, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuReload ), NULL, this );
	Disconnect( LauncherEventIDs::Add, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuAdd ), NULL, this );

	// Dynamically added
	Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );
}

void TrayIcon::Initialize()
{
	Refresh( true );
}

void TrayIcon::Cleanup()
{
	m_ProjectShortcuts.clear();

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
	if ( m_Application->m_UpdateLauncherNow || wxYES == wxMessageBox( wxT( "Are you sure you would like to exit the Launcher?" ), wxT( "Exit EShell Launcher?" ), wxYES_NO | wxCENTER | wxICON_QUESTION ) )
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
		wxT( "About EShell Launcher" ),
		wxOK | wxICON_INFORMATION );

	dialog.ShowModal();
}

void TrayIcon::OnMenuReload( wxCommandEvent& evt )
{
	wxBusyCursor wait;

	Refresh( true );
}

void TrayIcon::OnMenuRefresh( wxCommandEvent& evt )
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
		Shortcut* shortcut = static_cast< Shortcut* >( shortcutMenuItem->GetRefData() );

		if ( wxIsCtrlDown() )
		{
			shortcut->CopyToClipboard();
		}
		else if ( wxIsShiftDown() )
		{
			if ( shortcutMenuItem->GetMenu() == m_Menu )
			{
				m_Application->RemoveFavorite( shortcut->m_Command );
			}
			else
			{
				m_Application->AddFavorite( shortcut->m_Command );
			}

			wxCommandEvent pending( wxEVT_COMMAND_MENU_SELECTED, LauncherEventIDs::Refresh );
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

void TrayIcon::OnMenuAdd( wxCommandEvent& evt )
{
	wxFileDialog dlg ( NULL, "Open Eshell Settings file", wxEmptyString, "eshell.xml", "*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST );
	if ( wxID_OK == dlg.ShowModal() )
	{
		m_Application->AddProject( std::string( dlg.GetPath().c_str() ) );
		Refresh( true );
	}
}

void TrayIcon::BeginBusy()
{
	if ( ++m_BusyCount > 1 )
		return;

	// Disconnect events
	Disconnect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( LauncherEventIDs::Reload, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuReload ), NULL, this );

	// Clear the current menu and change the icon to notify the user that things are happening
	SetIcon( wxICON( LAUNCHER_BUSY_ICON ), wxString( m_Application->m_Title.c_str() ) + " Refreshing..." );
}

void TrayIcon::EndBusy()
{  
	if ( --m_BusyCount > 0 )
		return;

	assert( m_BusyCount == 0 );

	// Re-connect events
	Connect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( LauncherEventIDs::Reload, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuReload ), NULL, this );

	// Set the icon back
	SetIcon( m_Application->IsUpdateAvailable() ? wxICON( UPDATE_ICON ) : wxICON( LAUNCHER_ICON ), m_Application->m_Title.c_str() );
}

void TrayIcon::Refresh( bool reload )
{
	BeginBusy();
	{  
		if ( reload )
		{
			m_Settings.clear();
			m_ProjectShortcuts.clear();

			m_Application->LoadState();

			for ( std::set< std::string >::const_iterator itr = m_Application->m_Projects.begin(), end = m_Application->m_Projects.end(); itr != end; ++itr )
			{
				m_Settings.push_back( Settings() );
				if ( !m_Settings.back().LoadFile( *itr ) )
				{
					m_Settings.resize( m_Settings.size() - 1 );
				}
				else
				{
					for ( M_Config::const_iterator itr = m_Settings.back().m_Configs.begin(), end = m_Settings.back().m_Configs.end(); itr != end; ++itr )
					{
						M_EnvVar copyEnvVars = itr->second.m_EnvVar;

						V_ShortcutInfo::const_iterator ssItr = itr->second.m_Shortcuts.begin();
						V_ShortcutInfo::const_iterator ssEnd = itr->second.m_Shortcuts.end();
						for ( ; ssItr != ssEnd; ++ssItr )
						{
							const ShortcutInfo& shortcutInfo = (*ssItr);

							// create a Shortcut
							wxObjectDataPtr< Shortcut > shortcut ( new Shortcut () );
							shortcut->m_Name = shortcutInfo.m_Name;
							Settings::ProcessValue( shortcut->m_Name, copyEnvVars );

							shortcut->m_Folder = shortcutInfo.m_Folder;
							Settings::ProcessValue( shortcut->m_Folder, copyEnvVars );

							// Icon
							shortcut->m_Icon = shortcutInfo.m_IconPath;
							Settings::ProcessValue( shortcut->m_Icon, copyEnvVars );

							// Description
							shortcut->m_Description = shortcutInfo.m_Description;
							Settings::ProcessValue( shortcut->m_Description, copyEnvVars );

							// SettingsFile Path
							std::string settingsFile = m_Settings.back().m_File;

							// Build the Command  
							shortcut->m_Command = std::string ("\"") + m_Application->m_PerlExePath + "\"";
							shortcut->m_Command += "-I\"" + m_Application->m_PerlLibPath + "\"";
							shortcut->m_Command += " \"" + m_Application->m_EShellPlPath + "\"";
							shortcut->m_Command += " -settingsFile \"" + settingsFile + "\"";
							shortcut->m_Command += " -config " + itr->second.m_Name;

							if ( !shortcutInfo.m_Args.empty() )
							{
								shortcut->m_Command += " " + shortcutInfo.m_Args;
							}

							Settings::ProcessValue( shortcut->m_Command, copyEnvVars );

							// Create the FavoriteName
							shortcut->m_FavoriteName = m_Settings.back().m_Title + " - " + shortcutInfo.m_Name;

							m_ProjectShortcuts[ m_Settings.back().m_Title ].push_back( shortcut );
						}
					}
				}
			}
		}

		if ( !m_Menu )
		{
			m_Menu = new wxMenu();
			m_Menu->AppendSeparator();
			wxMenuItem* refreshMenuItem = new wxMenuItem( m_Menu, LauncherEventIDs::Reload, wxT( "Refresh" ), wxEmptyString, wxITEM_NORMAL );
			refreshMenuItem->SetBitmap( wxIcon( "REFRESH_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16 ) );
			m_Menu->Append( refreshMenuItem );

			m_UpdateMenuItem = new wxMenuItem( m_Menu, LauncherEventIDs::Update, wxT( "Update" ), wxEmptyString, wxITEM_NORMAL );
			m_Menu->Append( m_UpdateMenuItem );

			m_Menu->Append( new wxMenuItem( m_Menu, LauncherEventIDs::Add, wxString("Add..."), wxEmptyString, wxITEM_NORMAL ) );
			m_Menu->Append( new wxMenuItem( m_Menu, LauncherEventIDs::Help, wxString("Help"), wxEmptyString, wxITEM_NORMAL ) );
			m_Menu->Append( new wxMenuItem( m_Menu, LauncherEventIDs::Exit, wxString( wxT("Exit EShell Launcher v"LAUNCHER_VERSION_STRING) ) , wxEmptyString, wxITEM_NORMAL ) );
		}
		else
		{
			wxMenuItemList list = m_Menu->GetMenuItems();
			for ( wxMenuItemList::const_iterator itr = list.begin(), end = list.end(); itr != end; ++itr )
			{
				if ( (*itr)->GetId() < LauncherEventIDs::First || (*itr)->GetId() > LauncherEventIDs::Last )
				{
					m_Menu->Remove( *itr );
					delete *itr;
				}
			}
			m_Menu->PrependSeparator();
		}

		if ( !m_ProjectShortcuts.empty() )
		{
			CreateProjectsMenu( m_Menu );
		}

		bool update = m_Application->IsUpdateAvailable();
		m_UpdateMenuItem->Enable( update );
		if ( update )
		{
			m_UpdateMenuItem->SetBitmap( wxIcon( "UPDATE_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16 ) );
		}
	}
	EndBusy();
}

void TrayIcon::DetectAndSetIcon( Shortcut& shortcut, wxMenuItem* shortcutMenuItem )
{
	wxFileName name ( shortcut.m_Icon );

	if ( shortcut.m_Icon.empty() )
	{
		shortcutMenuItem->SetBitmap( wxIcon( "PROMPT_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16 ) );
	}
	else
	{
		wxImageHandler* handler = wxImage::FindHandler( name.GetExt() );
		if ( handler )
		{
			wxBitmap bitmap;
			bitmap.LoadFile( shortcut.m_Icon, wxBITMAP_TYPE_ANY );
			if ( bitmap.IsOk() )
			{
				shortcutMenuItem->SetBitmap( bitmap );
			}
		}
		else
		{
			// get the icon data from the shell associations
			SHFILEINFO info;
			ZeroMemory( &info, sizeof( info ) );
			SHGetFileInfo( shortcut.m_Icon.c_str(), 0, &info, sizeof( info ), SHGFI_ICON | SHGFI_SMALLICON );
			if ( info.hIcon )
			{
				wxIcon icon;
				icon.SetHICON( info.hIcon );
				icon.SetSize( 16, 16 );
				shortcutMenuItem->SetBitmap( icon );
			}
		}
	}
}

void TrayIcon::CreateProjectsMenu( wxMenu* parentMenu )
{
	V_Shortcut favoriteShortcuts;

	M_Shortcut::reverse_iterator projItr = m_ProjectShortcuts.rbegin();
	M_Shortcut::reverse_iterator projEnd = m_ProjectShortcuts.rend();
	for ( ; projItr != projEnd; ++projItr )
	{
		const std::string& title = projItr->first;
		const V_Shortcut& shortcuts = projItr->second;

		if ( shortcuts.empty() )
			continue;

		wxMenu* projectMenu = new wxMenu();

		typedef std::map< std::string, wxMenu* > M_SubMenues;
		M_SubMenues subMenus;

		V_Shortcut::const_iterator shortcutItr = shortcuts.begin();
		V_Shortcut::const_iterator shortcutEnd = shortcuts.end();
		for ( ; shortcutItr != shortcutEnd; ++shortcutItr )
		{
			const wxObjectDataPtr< Shortcut >& shortcut = (*shortcutItr);

			wxMenuItem* shortcutMenuItem = new wxMenuItem( projectMenu, wxID_ANY,
				wxString( wxT( shortcut->m_Name.c_str() ) ),
				wxString( wxT( shortcut->m_Description.c_str() ) ),
				wxITEM_NORMAL );

			if ( shortcut->m_Disable )
			{
				wxString name( "INVALID: " );
				name += shortcut->m_Name.c_str();
				shortcutMenuItem->SetText( name );
				shortcutMenuItem->Enable( false );
			}

			DetectAndSetIcon( *shortcut, shortcutMenuItem );

			shortcut.get()->IncRef();
			shortcutMenuItem->SetRefData( shortcut.get() );

			if ( shortcut->m_Folder.empty() )
			{
				projectMenu->Append( shortcutMenuItem );
			}
			else
			{
				wxMenu* menuToInsert = new wxMenu();
				std::pair< M_SubMenues::iterator, bool > insertedSubMenu = subMenus.insert( M_SubMenues::value_type( shortcut->m_Folder, menuToInsert ) );
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

		M_SubMenues::reverse_iterator subMenuItr = subMenus.rbegin();
		M_SubMenues::reverse_iterator subMenuEnd = subMenus.rend();
		for ( ; subMenuItr != subMenuEnd; ++subMenuItr )
		{
			wxMenuItem* subMenuItem = new wxMenuItem( projectMenu, wxID_ANY,
				wxString( wxT( subMenuItr->first.c_str() ) ),
				wxString( wxT( subMenuItr->first.c_str() ) ),
				wxITEM_NORMAL,
				subMenuItr->second );

			subMenuItem->SetBitmap( wxIcon( "FOLDER_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16 ) );
			projectMenu->Prepend( subMenuItem );
		}

		wxMenuItem* projectItem = parentMenu->Prepend( wxID_ANY, title, projectMenu );
		projectItem->SetBitmap( wxIcon( "FOLDER_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16 ) );
	}

	if ( !favoriteShortcuts.empty() )
	{
		parentMenu->PrependSeparator();

		V_Shortcut::reverse_iterator favItr = favoriteShortcuts.rbegin();
		V_Shortcut::reverse_iterator favEnd = favoriteShortcuts.rend();
		for( ; favItr != favEnd; ++favItr )
		{
			wxObjectDataPtr< Shortcut > shortcut = (*favItr);

			wxMenuItem* favoritesMenuItem = new wxMenuItem( parentMenu, wxID_ANY,
				wxString( wxT( shortcut->m_FavoriteName.c_str() ) ),
				wxString( wxT( shortcut->m_Description.c_str() ) ),
				wxITEM_NORMAL );

			DetectAndSetIcon( *shortcut, favoritesMenuItem );

			shortcut.get()->IncRef();
			favoritesMenuItem->SetRefData( shortcut.get() );

			parentMenu->Prepend( favoritesMenuItem );

			Connect( favoritesMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );
		}
	}
}
