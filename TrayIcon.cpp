#include "stdafx.h"
#include "TrayIcon.h"

#include "Application.h"
#include "MenuItem.h"
#include "Helper.h"
#include "resource.h"

#include <shellapi.h>

using namespace EShellMenu;

TrayIcon::TrayIcon( Application* application ) 
	: m_Application( application ) 
	, m_Menu( NULL )
	, m_UpdateMenuItem( NULL )
	, m_BusyCount( 0 )
	, m_IsMenuShowing( false )
{
	SetIcon( wxICON( LOGO_ICON ), "Initializing EShell Menu..." );

	// Connect Events
	Connect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( EventIDs::Exit, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuExit ), NULL, this );
	Connect( EventIDs::Help, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuHelp ), NULL, this );
	Connect( EventIDs::Refresh, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRefresh ), NULL, this );
	Connect( EventIDs::Reload, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuReload ), NULL, this );
	Connect( EventIDs::Add, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuAdd ), NULL, this );
}

TrayIcon::~TrayIcon() 
{ 
	// Disconnect Events
	Disconnect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( EventIDs::Exit, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuExit ), NULL, this );
	Disconnect( EventIDs::Help, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuHelp ), NULL, this );
	Disconnect( EventIDs::Refresh, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRefresh ), NULL, this );
	Disconnect( EventIDs::Reload, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuReload ), NULL, this );
	Disconnect( EventIDs::Add, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuAdd ), NULL, this );

	// Dynamically added
	Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );
}

void TrayIcon::Initialize()
{
	Refresh( true );
}

void TrayIcon::Cleanup()
{
	m_MenuItems.clear();

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
	if ( m_Application->m_UpdateNow || wxYES == wxMessageBox( wxT( "Are you sure you would like to exit EShell Menu?" ), wxT( "Exit EShell?" ), wxYES_NO | wxCENTER | wxICON_QUESTION ) )
	{
		wxExit();
	}
}

void TrayIcon::OnMenuHelp( wxCommandEvent& evt )
{
	tstring about;
	about += 
		wxT("EShell Menu is a system tray applicaiton used to launch \n") \
		wxT("EShell's tools in the correct process environment. \n");
	about +=
		wxT("\nFeatures:\n") \
		wxT("  Shift+Click - add/remove a 'favorite' shortcut.\n") \
		wxT("  Ctrl+Click  - copy a shortcut to the clipboard.\n");

	wxMessageDialog dialog(
		NULL,
		about.c_str(),
		wxT( "About EShell Menu" ),
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
		wxMenuItem* menuItemMenuItem = projectMenu->FindItem( evt.GetId() ); 
		MenuItem* menuItem = static_cast< MenuItemRefData* >( menuItemMenuItem->GetRefData() )->m_MenuItem;

		if ( wxIsCtrlDown() )
		{
			menuItem->CopyToClipboard();
		}
		else if ( wxIsShiftDown() )
		{
			if ( menuItemMenuItem->GetMenu() == m_Menu )
			{
				m_Application->RemoveFavorite( menuItem->m_Command );
			}
			else
			{
				m_Application->AddFavorite( menuItem->m_Command );
			}

			wxCommandEvent pending( wxEVT_COMMAND_MENU_SELECTED, EventIDs::Refresh );
			AddPendingEvent( pending );
		}
		else if ( menuItem->m_Disable )
		{
			wxString invalidShortcut( "" );

			if ( !menuItem->m_DisableReason.empty() )
			{
				invalidShortcut += "The EShellMenu was unable to create valid settings for this shortcut\n";
				invalidShortcut += "for the following reason(s):\n";
				invalidShortcut += menuItem->m_DisableReason + "\n";
			}
			else
			{
				invalidShortcut += "The EShellMenu was unable to create valid settings for this shortcut\n";
				invalidShortcut += "for one or more of the following reasons:\n";
				invalidShortcut += " - EShell.pl did not exist in the expected location.\n";
			} 
			invalidShortcut += "\n\nYou can copy the shortcut to your clipboard by holding Ctrl and clicking on the menuItem.\n";
			invalidShortcut += "This may give you additional useful information.\n";

			wxMessageDialog dialog(
				NULL,
				invalidShortcut,
				wxT( "Invalid MenuItem" ),
				wxOK | wxICON_ERROR );

			dialog.ShowModal();
		}
		else
		{
			BeginBusy();
			{
				menuItem->Execute();
			}
			EndBusy();
		}
	}
}

void TrayIcon::OnMenuAdd( wxCommandEvent& evt )
{
	wxFileDialog dlg ( NULL, "Open Eshell Project file", wxEmptyString, "eshell.xml", "*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST );
	if ( wxID_OK == dlg.ShowModal() )
	{
		m_Application->AddProject( tstring( dlg.GetPath().c_str() ) );
		Refresh( true );
	}
}

void TrayIcon::OnMenuRemove( wxCommandEvent& evt )
{
	wxMenu* projectMenu = wxDynamicCast( evt.GetEventObject(), wxMenu );

	if ( projectMenu && projectMenu->FindItem( evt.GetId() ) )
	{
		wxMenuItem* menuItem = projectMenu->FindItem( evt.GetId() ); 
		StringRefData* refData = static_cast< StringRefData* >( menuItem->GetRefData() );
		m_Application->RemoveProject( refData->m_Value );
		Refresh( true );
	}
}

void TrayIcon::OnMenuEdit( wxCommandEvent& evt )
{
	wxMenu* projectMenu = wxDynamicCast( evt.GetEventObject(), wxMenu );

	if ( projectMenu && projectMenu->FindItem( evt.GetId() ) )
	{
		wxMenuItem* menuItem = projectMenu->FindItem( evt.GetId() ); 
		StringRefData* refData = static_cast< StringRefData* >( menuItem->GetRefData() );
		EShellMenu::ExecuteCommand( tstring ( wxT("cmd.exe /c start \"\" \"") ) + refData->m_Value + wxT("\""), false );
	}
}

void TrayIcon::BeginBusy()
{
	if ( ++m_BusyCount > 1 )
		return;

	// Disconnect events
	Disconnect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Disconnect( EventIDs::Reload, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuReload ), NULL, this );

	// Clear the current menu and change the icon to notify the user that things are happening
	SetIcon( wxICON( BUSY_ICON ), wxString( m_Application->m_Title.c_str() ) + " Refreshing..." );
}

void TrayIcon::EndBusy()
{  
	if ( --m_BusyCount > 0 )
		return;

	assert( m_BusyCount == 0 );

	// Re-connect events
	Connect( wxID_ANY, wxEVT_TASKBAR_CLICK, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( wxID_ANY, wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler( TrayIcon::OnTrayIconClick ), NULL, this );
	Connect( EventIDs::Reload, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuReload ), NULL, this );

	// Set the icon back
	SetIcon( m_Application->IsUpdateAvailable() ? wxICON( UPDATE_ICON ) : wxICON( LOGO_ICON ), m_Application->m_Title.c_str() );
}

void TrayIcon::Refresh( bool reload )
{
	BeginBusy();
	{  
		if ( reload )
		{
			m_Projects.clear();
			m_MenuItems.clear();

			m_Application->LoadState();

			for ( std::set< tstring >::const_iterator itr = m_Application->m_Projects.begin(), end = m_Application->m_Projects.end(); itr != end; ++itr )
			{
				if ( !FileExists( *itr ) )
				{
					continue;
				}

				m_Projects.push_back( Project() );

				Project& project ( m_Projects.back() );
				
				if ( !project.LoadFile( *itr ) )
				{
					m_Projects.resize( m_Projects.size() - 1 );
				}
				else
				{
					m_MenuItems[ project.m_Title ] = std::make_pair( static_cast<uint32_t>( m_Projects.size() - 1 ), std::vector< MenuItem > () );

					for ( std::map< tstring, Config >::const_iterator itr = m_Projects.back().m_Configs.begin(), end = m_Projects.back().m_Configs.end(); itr != end; ++itr )
					{
						M_EnvVar copyEnvVars = itr->second.m_EnvVar;

						std::vector< Shortcut >::const_iterator ssItr = itr->second.m_Shortcuts.begin();
						std::vector< Shortcut >::const_iterator ssEnd = itr->second.m_Shortcuts.end();
						for ( ; ssItr != ssEnd; ++ssItr )
						{
							const Shortcut& shortcut = (*ssItr);
							MenuItem menuItem;

							menuItem.m_Name = shortcut.m_Name;
							Project::ProcessValue( menuItem.m_Name, copyEnvVars );

							if ( shortcut.m_Folder != wxT("none") )
							{
								menuItem.m_Folder = shortcut.m_Folder;
								Project::ProcessValue( menuItem.m_Folder, copyEnvVars );
								if ( menuItem.m_Folder.empty() && itr->first != wxT("default") )
								{
									menuItem.m_Folder = itr->first;
								}
							}

							menuItem.m_Icon = shortcut.m_IconPath;
							Project::ProcessValue( menuItem.m_Icon, copyEnvVars );

							menuItem.m_Description = shortcut.m_Description;
							Project::ProcessValue( menuItem.m_Description, copyEnvVars );

							// SettingsFile Path
							tstring projectFile = m_Projects.back().m_File;

							// Build the Command  
							menuItem.m_Command = wxT("\"") + m_Application->m_EShellPath + wxT("\"");
							menuItem.m_Command += wxT(" -settingsFile \"") + projectFile + wxT("\"");
							menuItem.m_Command += wxT(" -config ") + itr->second.m_Name;

							if ( !shortcut.m_Args.empty() )
							{
								menuItem.m_Command += wxT(" ") + shortcut.m_Args;
							}
							else
							{
								tstring target = shortcut.m_Target;
								Project::ProcessValue( target, copyEnvVars );
								if ( FileExists( target ) )
								{
									menuItem.m_Icon = target;

									tstring workingDirectory = shortcut.m_WorkingDirectory;
									Project::ProcessValue( workingDirectory, copyEnvVars );
									if ( workingDirectory.empty() )
									{
										menuItem.m_Command += wxT(" -run \"") + shortcut.m_Target + wxT("\"");
									}
									else
									{
										menuItem.m_Command += wxT(" -run \"start \\\"") + shortcut.m_Name + wxT("\\\"");
										menuItem.m_Command += wxT(" /d\\\"") + workingDirectory + wxT("\\\"");
										menuItem.m_Command += wxT(" \\\"") + target + wxT("\\\"\"");
									}
								}
								else if ( !shortcut.m_Installer.empty() )
								{
									tstring installer = shortcut.m_Installer;
									Project::ProcessValue( installer, copyEnvVars );
									if ( FileExists( installer ) )
									{
										menuItem.m_Icon = installer;
										menuItem.m_Name = wxT("Install: ") + menuItem.m_Name;
										menuItem.m_Command += wxT(" -run \"") + installer + wxT("\"");
									}
									else
									{
										menuItem.m_Name = wxT("Missing installer for: ") + menuItem.m_Name;
										menuItem.m_Command.clear();
									}
								}
								else
								{
									continue;
								}
							}

							Project::ProcessValue( menuItem.m_Command, copyEnvVars );

							// Create the FavoriteName
							menuItem.m_FavoriteName = m_Projects.back().m_Title + " - " + shortcut.m_Name;

							m_MenuItems[ project.m_Title ].second.push_back( menuItem );
						}
					}
				}
			}
		}

		if ( !m_Menu )
		{
			m_Menu = new wxMenu();
			m_Menu->AppendSeparator();
			wxMenuItem* refreshMenuItem = new wxMenuItem( m_Menu, EventIDs::Reload, wxT( "Refresh" ), wxEmptyString, wxITEM_NORMAL );
			refreshMenuItem->SetBitmap( wxIcon( "REFRESH_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16 ) );
			m_Menu->Append( refreshMenuItem );

			m_UpdateMenuItem = new wxMenuItem( m_Menu, EventIDs::Update, wxT( "Update" ), wxEmptyString, wxITEM_NORMAL );
			m_Menu->Append( m_UpdateMenuItem );

			m_Menu->Append( new wxMenuItem( m_Menu, EventIDs::Add, wxString("Add..."), wxEmptyString, wxITEM_NORMAL ) );
			m_Menu->Append( new wxMenuItem( m_Menu, EventIDs::Help, wxString("Help"), wxEmptyString, wxITEM_NORMAL ) );
			m_Menu->Append( new wxMenuItem( m_Menu, EventIDs::Exit, wxString( wxT("Exit EShell Menu v"VERSION_STRING) ) , wxEmptyString, wxITEM_NORMAL ) );
		}
		else
		{
			wxMenuItemList list = m_Menu->GetMenuItems();
			for ( wxMenuItemList::const_iterator itr = list.begin(), end = list.end(); itr != end; ++itr )
			{
				if ( (*itr)->GetId() < EventIDs::First || (*itr)->GetId() > EventIDs::Last )
				{
					m_Menu->Remove( *itr );
					delete *itr;
				}
			}
			m_Menu->PrependSeparator();
		}

		if ( !m_MenuItems.empty() )
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

void TrayIcon::DetectAndSetIcon( MenuItem& menuItem, wxMenuItem* actualMenuItem )
{
	wxFileName name ( menuItem.m_Icon );

	if ( menuItem.m_Icon.empty() )
	{
		actualMenuItem->SetBitmap( wxIcon( "PROMPT_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16 ) );
	}
	else
	{
		wxImageHandler* handler = wxImage::FindHandler( name.GetExt() );
		if ( handler )
		{
			wxBitmap bitmap;
			bitmap.LoadFile( menuItem.m_Icon, wxBITMAP_TYPE_ANY );
			if ( bitmap.IsOk() )
			{
				actualMenuItem->SetBitmap( bitmap );
			}
		}
		else
		{
			// get the icon data from the shell associations
			SHFILEINFO info;
			ZeroMemory( &info, sizeof( info ) );
			SHGetFileInfo( menuItem.m_Icon.c_str(), 0, &info, sizeof( info ), SHGFI_ICON | SHGFI_SMALLICON );
			if ( info.hIcon )
			{
				wxIcon icon;
				icon.SetHICON( info.hIcon );
				icon.SetSize( 16, 16 );
				actualMenuItem->SetBitmap( icon );
			}
		}
	}
}

void TrayIcon::CreateProjectsMenu( wxMenu* parentMenu )
{
	std::vector< MenuItem* > favoriteShortcuts;

	std::map< tstring, std::pair< uint32_t, std::vector< MenuItem > > >::reverse_iterator projItr = m_MenuItems.rbegin();
	std::map< tstring, std::pair< uint32_t, std::vector< MenuItem > > >::reverse_iterator projEnd = m_MenuItems.rend();
	for ( ; projItr != projEnd; ++projItr )
	{
		const tstring& title = projItr->first;
		
		std::vector< MenuItem >& menuItems = projItr->second.second;
		if ( menuItems.empty() )
		{
			continue;
		}

		wxMenu* projectMenu = new wxMenu();

		typedef std::map< tstring, wxMenu* > M_SubMenues;
		M_SubMenues subMenus;

		std::vector< MenuItem >::iterator menuItemItr = menuItems.begin();
		std::vector< MenuItem >::iterator menuItemEnd = menuItems.end();
		for ( ; menuItemItr != menuItemEnd; ++menuItemItr )
		{
			MenuItem& menuItem = (*menuItemItr);

			wxMenuItem* actualMenuItem = new wxMenuItem( projectMenu, wxID_ANY,
				wxString( menuItem.m_Name.c_str() ),
				wxString( menuItem.m_Description.c_str() ),
				wxITEM_NORMAL );

			if ( menuItem.m_Disable )
			{
				wxString name( "INVALID: " );
				name += menuItem.m_Name.c_str();
				actualMenuItem->SetText( name );
				actualMenuItem->Enable( false );
			}

			DetectAndSetIcon( menuItem, actualMenuItem );

			actualMenuItem->SetRefData( new MenuItemRefData( &menuItem ) );

			if ( menuItem.m_Folder.empty() )
			{
				projectMenu->Append( actualMenuItem );
			}
			else
			{
				wxMenu* menuToInsert = new wxMenu();
				std::pair< M_SubMenues::iterator, bool > insertedSubMenu = subMenus.insert( M_SubMenues::value_type( menuItem.m_Folder, menuToInsert ) );
				insertedSubMenu.first->second->Append( actualMenuItem );
				if ( !insertedSubMenu.second )
				{
					// New menu was not inserted, you have to clean it up
					delete menuToInsert;
				}
			}

			Connect( actualMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );

			if ( m_Application->IsFavorite( menuItem.m_Command ) )
			{
				favoriteShortcuts.push_back( &menuItem );
			}
		}

		M_SubMenues::reverse_iterator subMenuItr = subMenus.rbegin();
		M_SubMenues::reverse_iterator subMenuEnd = subMenus.rend();
		for ( ; subMenuItr != subMenuEnd; ++subMenuItr )
		{
			wxMenuItem* subMenuItem = new wxMenuItem( projectMenu, wxID_ANY,
				wxString( subMenuItr->first.c_str() ),
				wxString( subMenuItr->first.c_str() ),
				wxITEM_NORMAL,
				subMenuItr->second );

			subMenuItem->SetBitmap( wxIcon( "FOLDER_OPEN_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16 ) );
			projectMenu->Prepend( subMenuItem );
		}

		wxMenuItem* projectItem = parentMenu->Prepend( wxID_ANY, title, projectMenu );
		projectItem->SetBitmap( wxIcon( "FOLDER_OPEN_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16 ) );

		projectMenu->AppendSeparator();

		wxMenuItem* remove = projectMenu->Append( wxID_ANY, "Remove" );
		remove->SetRefData( new StringRefData( m_Projects[ projItr->second.first ].m_File ) );
		Connect( remove->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuRemove ), NULL, this );

		wxMenuItem* edit = projectMenu->Append( wxID_ANY, "Edit" );
		edit->SetRefData( new StringRefData( m_Projects[ projItr->second.first ].m_File ) );
		Connect( edit->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuEdit ), NULL, this );
	}

	if ( !favoriteShortcuts.empty() )
	{
		parentMenu->PrependSeparator();

		std::vector< MenuItem* >::reverse_iterator favItr = favoriteShortcuts.rbegin();
		std::vector< MenuItem* >::reverse_iterator favEnd = favoriteShortcuts.rend();
		for( ; favItr != favEnd; ++favItr )
		{
			MenuItem* menuItem = (*favItr);

			wxMenuItem* favoritesMenuItem = new wxMenuItem( parentMenu, wxID_ANY,
				wxString( menuItem->m_FavoriteName.c_str() ),
				wxString( menuItem->m_Description.c_str() ),
				wxITEM_NORMAL );

			DetectAndSetIcon( *menuItem, favoritesMenuItem );

			favoritesMenuItem->SetRefData( new MenuItemRefData( menuItem ) );

			parentMenu->Prepend( favoritesMenuItem );

			Connect( favoritesMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );
		}
	}
}
