#include "stdafx.h"
#include "TrayIcon.h"

#include "Application.h"
#include "Config.h"
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
	icon.SetHICON( ::LoadIcon( ::GetModuleHandle( NULL ), MAKEINTRESOURCE( LAUNCHER_ICON ) ) );
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
	Refresh();
}

void TrayIcon::Cleanup()
{
	m_Shortcuts.clear();

	RemoveIcon();

	delete m_Menu;
	m_Menu = NULL;
}

///////////////////////////////////////////////////////////////////////////////
void TrayIcon::OnTrayIconClick( wxTaskBarIconEvent& evt ) 
{ 
	if ( m_Menu )
	{
		m_IsMenuShowing = true;

		PopupMenu( m_Menu );

		m_IsMenuShowing = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
// Displays the Help dialog
//
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
	aboutLauncher += 
		"\nThis version of the Launcher is compatible with EShell [v"ESHELL_VERSION"] \n" \
		"and 32bit Perl [v"PERL_VERSION"].\n"; // "\n  Copyright 2009 EShell Games, Inc.\n";

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

	Refresh();
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
				invalidShortcut += "   is not defined in the ProjectSettings file.\n";
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
	icon.SetHICON( ::LoadIcon( ::GetModuleHandle( NULL ), MAKEINTRESOURCE( LAUNCHER_BUSY_ICON ) ) );
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
	icon.SetHICON( ::LoadIcon( ::GetModuleHandle( NULL ), MAKEINTRESOURCE( LAUNCHER_ICON ) ) );
	SetIcon( icon, m_Application->m_Title.c_str() );
}

#if 0

void CreateShortcut( const Project& project, V_Shortcut& shortcuts, const Install& install, const Config& config, const std::string& buildConfig, const std::string& assetsBranch )
{
	////////////////////////////////////////
	// Setup Aliased EnvVars
	M_EnvVarAlias envVarAlias = project.m_ProjectSettings.m_EnvVarAlias; 
	M_EnvVar copyEnvVars = config.m_EnvVar;

	ProjectSettings::SetEnvVar( envVarAlias[EnvVarAliasNames::Assets], assetsBranch, copyEnvVars, true );
	ProjectSettings::SetEnvVar( envVarAlias[EnvVarAliasNames::Build], buildConfig, copyEnvVars, true );
	ProjectSettings::SetEnvVar( envVarAlias[EnvVarAliasNames::Code], install.m_Code, copyEnvVars, true );
	ProjectSettings::SetEnvVar( envVarAlias[EnvVarAliasNames::Game], install.m_Game, copyEnvVars, true );
	ProjectSettings::SetEnvVar( envVarAlias[EnvVarAliasNames::Tools], install.m_Tools, copyEnvVars, true );


	////////////////////////////////////////
	// Get and Process the Aliased EnvVars Values
	std::string assets;
	ProjectSettings::GetEnvVarAliasValue( envVarAlias[EnvVarAliasNames::Assets], copyEnvVars, assets, s_DefaultAssetBranch );

	std::string build;
	ProjectSettings::GetEnvVarAliasValue( envVarAlias[EnvVarAliasNames::Build], copyEnvVars, build, s_DefaultBuildConfig );

	std::string code;
	ProjectSettings::GetEnvVarAliasValue( envVarAlias[EnvVarAliasNames::Code], copyEnvVars, code, s_DefaultCodeBranch );

	std::string game;
	ProjectSettings::GetEnvVarAliasValue( envVarAlias[EnvVarAliasNames::Game], copyEnvVars, game, s_DefaultGame );

	std::string tools;
	ProjectSettings::GetEnvVarAliasValue( envVarAlias[EnvVarAliasNames::Tools], copyEnvVars, tools, s_DefaultToolsRelease );


	////////////////////////////////////////
	// Create the Shortcuts
	V_ShortcutInfo::const_iterator ssItr = config.m_EShell.m_Shortcuts.begin();
	V_ShortcutInfo::const_iterator ssEndItr = config.m_EShell.m_Shortcuts.end();
	for ( ; ssItr != ssEndItr; ++ssItr )
	{
		const ShortcutInfo& shortcut = (*ssItr);

		// create a Shortcut
		Shortcut shortcut;
		shortcut.m_Name = shortcut.m_Name;
		ProjectSettings::ProcessValue( shortcut.m_Name, copyEnvVars );

		// ProjectName
		shortcut.m_ProjectName = project.m_Name;

		// Icon
		shortcut.m_Icon = shortcut.m_IconPath;
		ProjectSettings::ProcessValue( shortcut.m_Icon, copyEnvVars );

		// Description
		shortcut.m_Description = shortcut.m_Description;
		ProjectSettings::ProcessValue( shortcut.m_Description, copyEnvVars );

		// StartIn
		if ( copyEnvVars.find( "IG_PROJECT_ROOT" ) != copyEnvVars.end() )
		{
			shortcut.m_StartIn = copyEnvVars["IG_PROJECT_ROOT"].m_Value;
			ProjectSettings::ProcessValue( shortcut.m_StartIn, copyEnvVars );
		}
		else
		{
			shortcut.m_Disable = true;
			shortcut.m_DisableReason = "The required variable \"IG_PROJECT_ROOT\" was not defined in the ProjectSettings file.";
		}

		// IG_ROOT
		if ( copyEnvVars.find( "IG_ROOT" ) != copyEnvVars.end() )
		{
			shortcut.m_Root = copyEnvVars["IG_ROOT"].m_Value;
			ProjectSettings::ProcessValue( shortcut.m_Root, copyEnvVars );
		}
		else
		{
			shortcut.m_Disable = true;
			shortcut.m_DisableReason = "The required variable \"IG_ROOT\" was not defined in the ProjectSettings file or environment.";
		}

		// Get EShell path
		std::string eshellPath("");
		if ( copyEnvVars.find( "ESHELL_PATH" ) != copyEnvVars.end() )
		{
			eshellPath = copyEnvVars["ESHELL_PATH"].m_Value;
			ProjectSettings::ProcessValue( eshellPath, copyEnvVars );
		}
		else
		{
			shortcut.m_Disable = true;
			shortcut.m_DisableReason = "The required variable \"ESHELL_PATH\" was not defined in the ProjectSettings file.";
		}

		if ( !FileExists( eshellPath ) )
		{
			shortcut.m_Disable = true;
			shortcut.m_DisableReason = "EShell.pl did not exist in the expected location:\n  - \"" + eshellPath + "\"";
		}

		// SettingsFile Path
		std::string settingsFile = project.m_SettingsFile;

		// Build the Command  
		shortcut.m_Command = PERL_EXE" \"" + eshellPath + "\"";
		shortcut.m_Command += " -settingsFile \"" + settingsFile + "\"";
		shortcut.m_Command += " -config " + install.m_Config;

		shortcut.m_Command += " -set \"IG_ROOT=";
		shortcut.m_Command += shortcut.m_Root; 
		shortcut.m_Command += "\"";

		if ( !envVarAlias[EnvVarAliasNames::Assets].empty() )
		{
			shortcut.m_Command += " -";
			shortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Assets ); 
			shortcut.m_Command += " " + assets;
		}

		if ( !envVarAlias[EnvVarAliasNames::Code].empty() )
		{
			shortcut.m_Command += " -";
			shortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Code ); 
			shortcut.m_Command += " " + code;
		}

		if ( !envVarAlias[EnvVarAliasNames::Game].empty() && game != project.m_Name )
		{
			shortcut.m_Command += " -";
			shortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Game ); 
			shortcut.m_Command += " " + game;
		}

		if ( !envVarAlias[EnvVarAliasNames::Build].empty() && !buildConfig.empty() )
		{
			shortcut.m_Command += " -";
			shortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Build ); 
			shortcut.m_Command += " " + buildConfig;
		}

		if ( !envVarAlias[EnvVarAliasNames::Tools].empty() && !install.m_Tools.empty() )
		{
			shortcut.m_Command += " -";
			shortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Tools ); 
			shortcut.m_Command += " " + install.m_Tools;
		}

		if ( !shortcut.m_Args.empty() )
		{
			shortcut.m_Command += " " + shortcut.m_Args;
		}

		ProjectSettings::ProcessValue( shortcut.m_Command, copyEnvVars );


		// Create the Display Folder
		shortcut.m_Folder = "";
		if ( !game.empty() && game != project.m_Name )
		{
			shortcut.m_Folder = Capitalize( game, true );
		}

		if ( install.m_Config == Launcher::InstallConfigNames[InstallTypes::Code] )
		{
			shortcut.m_Folder += ( shortcut.m_Folder.empty() ) ? "" : " ";
			shortcut.m_Folder += "Tools Builder";
		}
		else if ( ( install.m_InstallType == InstallTypes::Game ) || ( install.m_InstallType == InstallTypes::Release ) )
		{
			shortcut.m_Folder += ( shortcut.m_Folder.empty() ) ? "" : " ";

			if ( !tools.empty() && tools != s_DefaultToolsRelease )
			{
				if ( !code.empty() && code != s_DefaultCodeBranch )
				{
					shortcut.m_Folder += Capitalize( tools ) + " Tools (" + code + ")";
				}
				else
				{
					shortcut.m_Folder += Capitalize( tools ) + " Tools";
				}
			}
			else if ( !code.empty() && code != s_DefaultCodeBranch )
			{
				shortcut.m_Folder += Capitalize( RemoveSlashes( code ) ) + " Tools";
			}
		}

		// Special folder names for non-devel and non-game assets branch
		if ( !assets.empty() && game == project.m_Name && assets != s_DefaultAssetBranch )
		{
			if ( shortcut.m_Folder.empty() )
			{
				shortcut.m_Folder += Capitalize( assets, true ) + " Assets";
			}
			else
			{
				shortcut.m_Folder += " (" + Capitalize( assets, true ) + " Assets)";
			}
		}

		// Create the FavoriteName
		shortcut.m_FavoriteName = ""; //shortcut.m_Name;
		shortcut.m_FavoriteName = Capitalize( shortcut.m_ProjectName, true ) + " - " + shortcut.m_Name;

		bool hasSpecialString = false;
		if ( ( install.m_InstallType == InstallTypes::Game ) || ( install.m_InstallType == InstallTypes::Release ) )
		{
			if ( !tools.empty() && tools != s_DefaultToolsRelease )
			{
				if ( !code.empty() && code != s_DefaultCodeBranch )
				{
					shortcut.m_FavoriteName += hasSpecialString ? "/" : " (";
					shortcut.m_FavoriteName += Capitalize( tools ) + " Tools [" + code + "]";
					hasSpecialString = true;
				}
				else
				{
					shortcut.m_FavoriteName += hasSpecialString ? "/" : " (";
					shortcut.m_FavoriteName += Capitalize( tools ) + " Tools";
					hasSpecialString = true;
				}
			}
			else if ( !code.empty() && code != s_DefaultCodeBranch )
			{
				shortcut.m_FavoriteName += hasSpecialString ? "/" : " (";
				shortcut.m_FavoriteName += Capitalize( RemoveSlashes( code ) ) + " Tools";
				hasSpecialString = true;
			}
		}

		if ( !assets.empty() && game == project.m_Name && assets != s_DefaultAssetBranch )
		{
			shortcut.m_FavoriteName += hasSpecialString ? "/" : " (";
			shortcut.m_FavoriteName += Capitalize( assets, true ) + " Assets";
			hasSpecialString = true;
		}

		if ( hasSpecialString )
		{
			shortcut.m_FavoriteName += ")";
		}

		shortcuts.push_back( shortcut );
	}
}

void CreateShortcut( const Project& project, V_Shortcut& shortcuts, const Install& install, const Config& config, const std::string& buildConfig )
{
	// if there's more than one asset branch, iterate over all of them
	if ( !install.m_Game.empty() && install.m_Game == project.m_Name && !project.m_Assets.empty() )
	{
		S_string::const_iterator itr = project.m_Assets.begin();
		S_string::const_iterator end = project.m_Assets.end();
		for( ; itr != end; ++itr )
		{
			CreateShortcut( project, shortcuts, install, config, buildConfig, (*itr) );
		}
	}
	else
	{
		CreateShortcut( project, shortcuts, install, config, buildConfig, "" );
	}
}

void CreateShortcuts( const M_Projects& projects, M_Shortcuts& displayShortcuts )
{
	V_string buildConfigs;
	buildConfigs.push_back( "debug" );
	buildConfigs.push_back( "release" );

	M_Projects::const_iterator projItr = projects.begin();
	M_Projects::const_iterator projEnd = projects.end();
	for ( ; projItr != projEnd; ++projItr )
	{
		const std::string& projName = projItr->first;
		const Project& project = projItr->second;

		// Make the shortcuts
		Nocturnal::Insert<M_Shortcuts>::Result insertedShortcuts = displayShortcuts.insert( M_Shortcuts::value_type( projName, V_Shortcut() ) );
		if ( insertedShortcuts.second )
		{
			V_Shortcut& shortcuts = insertedShortcuts.first->second;

			V_Install allInstall;
			allInstall.insert( allInstall.end(), project.m_ReleaseInstalls.begin(), project.m_ReleaseInstalls.end() );
			allInstall.insert( allInstall.end(), project.m_GameInstalls.begin(), project.m_GameInstalls.end() );
			allInstall.insert( allInstall.end(), project.m_CodeInstalls.begin(), project.m_CodeInstalls.end() );

			V_Install::const_iterator installItr = allInstall.begin();
			V_Install::const_iterator installEnd = allInstall.end();
			for ( ; installItr != installEnd; ++installItr )
			{
				const Install& install = (*installItr);

				M_Config::const_iterator foundConfig = project.m_ProjectSettings.m_Configs.find( install.m_Config );
				if ( foundConfig != project.m_ProjectSettings.m_Configs.end() )
				{
					const Config& config = foundConfig->second;

					M_EnvVarAlias::const_iterator foundBuildAlias = project.m_ProjectSettings.m_EnvVarAlias.find( EnvVarAliasNames::Build );
					if ( install.m_InstallType == InstallTypes::Code
						&& ( foundBuildAlias != project.m_ProjectSettings.m_EnvVarAlias.end() && !foundBuildAlias->second.empty() ) )
					{
						V_string::iterator buildItr = buildConfigs.begin();
						V_string::iterator buildEnd = buildConfigs.end();
						for ( ; buildItr != buildEnd; ++buildItr )
						{
							CreateShortcut( project, shortcuts, install, config, (*buildItr) );
						}
					}
					else
					{
						CreateShortcut( project, shortcuts, install, config, "" );
					}
				}
			}
		}
	}
}

#endif

void TrayIcon::Refresh( bool reloadProjects )
{
	BeginBusy();
	{  
		if ( reloadProjects )
		{
			if ( !m_Application->LoadSettings() )
			{
				wxCommandEvent pending( wxEVT_COMMAND_MENU_SELECTED, LauncherEventIDs::Exit );
				AddPendingEvent( pending );
				return;
			}
			else
			{
				m_Shortcuts.clear();
#if 0
				CreateShortcuts( m_Application->m_Projects, m_Shortcuts );
#endif
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
		HICON hIcon = (HICON)::LoadImage( ::GetModuleHandle( NULL ), MAKEINTRESOURCE( REFRESH_ICON ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );
		wxIcon refreshIcon;
		refreshIcon.SetHICON( hIcon );
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

	M_Shortcuts::iterator projItr = m_Shortcuts.begin();
	M_Shortcuts::iterator projEnd = m_Shortcuts.end();
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
			Shortcut& shortcut = (*shortcutItr);

			wxMenuItem* shortcutMenuItem;
			shortcutMenuItem = new wxMenuItem(
				projectMenu,
				wxID_ANY,
				wxString( wxT( shortcut.m_Name.c_str() ) ),
				wxString( wxT( shortcut.m_Description.c_str() ) ),
				wxITEM_NORMAL );

			if ( shortcut.m_Disable )
			{
				shortcutMenuItem->Enable( false );

				wxString name( "INVALID: " );
				name += shortcut.m_Name.c_str();
				shortcutMenuItem->SetText( name );
			}

			DetectAndSetIcon( shortcut, shortcutMenuItem );

			shortcutMenuItem->SetRefData( new Shortcut( shortcut ) );

			if ( shortcut.m_Folder.empty() )
			{
				projectMenu->Append( shortcutMenuItem );
			}
			else
			{
				wxMenu* menuToInsert = new wxMenu();
				std::pair< M_SubMenues::iterator, bool > insertedSubMenu = subMenues.insert( M_SubMenues::value_type( shortcut.m_Folder, menuToInsert ) );
				insertedSubMenu.first->second->Append( shortcutMenuItem );
				if ( !insertedSubMenu.second )
				{
					// New menu was not inserted, you have to clean it up
					delete menuToInsert;
				}
			}

			Connect( shortcutMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );

			if ( m_Application->FindFavorite( shortcut.m_Command ) )
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
			Shortcut& shortcut = (*favItr);

			wxMenuItem* favoritesMenuItem = new wxMenuItem(
				parentMenu,
				wxID_ANY,
				wxString( wxT( shortcut.m_FavoriteName.c_str() ) ),
				wxString( wxT( shortcut.m_Description.c_str() ) ),
				wxITEM_NORMAL
				);

			DetectAndSetIcon( shortcut, favoritesMenuItem );

			favoritesMenuItem->SetRefData( new Shortcut( shortcut ) );

			parentMenu->Prepend( favoritesMenuItem );

			Connect( favoritesMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );
		}
	}
}
