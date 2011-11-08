#include "stdafx.h"
#include "TrayIcon.h"

#include "Application.h"
#include "Config.h"
#include "DisplayShortcut.h"
#include "Helper.h"
#include "Version.h"

#include "icons/folder.xpm"
#include "icons/install.xpm"
#include "icons/launcher.xpm"
#include "icons/launcher_busy.xpm"
#include "icons/prompt.xpm"
#include "icons/setup.xpm"
#include "icons/sln.xpm"
#include "icons/throbber1.xpm"

using namespace Launcher;

TrayIcon::TrayIcon( Application* application ) 
	: m_Application( application ) 
	, m_Menu( NULL )
	, m_BusyCount( 0 )
	, m_IsMenuShowing( false )
{
	wxIcon tempIcon;
	tempIcon.CopyFromBitmap( wxBitmap( ( const char** )g_LauncherBusyIconXpm ) );  
	SetIcon( tempIcon, "Initializing Launcher..." );

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
	m_DisplayShortcuts.clear();

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
		"EShell's tools in the correct project environment. \n";

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
		DisplayShortcut* displayShortcut = reinterpret_cast<DisplayShortcut*> ( shortcutMenuItem->GetRefData() );

		if ( wxIsCtrlDown() )
		{
			displayShortcut->CopyToClipboard();
		}
		else if ( wxIsShiftDown() )
		{
			m_Application->AddFavorite( displayShortcut->m_Command );

			wxCommandEvent pending( wxEVT_COMMAND_MENU_SELECTED, LauncherEventIDs::Redraw );
			AddPendingEvent( pending );
		}
		else if ( displayShortcut->m_Disable )
		{
			wxString invalidShortcut( "" );

			if ( !displayShortcut->m_DisableReason.empty() )
			{
				invalidShortcut += "The Launcher was unable to create valid settings for this shortcut\n";
				invalidShortcut += "for the following reason(s):\n";
				invalidShortcut += displayShortcut->m_DisableReason + "\n";
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
				displayShortcut->Execute();
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
	wxIcon tempIcon;
	tempIcon.CopyFromBitmap( wxBitmap( ( const char** )g_LauncherBusyIconXpm ) );
	SetIcon( tempIcon, wxString( m_Application->m_Title.c_str() ) + " Refreshing..." );
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
	wxIcon tempIcon;
	tempIcon.CopyFromBitmap( wxBitmap( ( const char** )g_LauncherIconXpm ) );
	SetIcon( tempIcon, m_Application->m_Title.c_str() );
}

#if 0

void CreateShortcut( const Project& project, V_DisplayShortcut& shortcuts, const Install& install, const Config& config, const std::string& buildConfig, const std::string& assetsBranch )
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
	// Create the DisplayShortcuts
	V_Shortcut::const_iterator ssItr = config.m_EShell.m_Shortcuts.begin();
	V_Shortcut::const_iterator ssEndItr = config.m_EShell.m_Shortcuts.end();
	for ( ; ssItr != ssEndItr; ++ssItr )
	{
		const Shortcut& shortcut = (*ssItr);

		// create a DisplayShortcut
		DisplayShortcut displayShortcut;
		displayShortcut.m_Name = shortcut.m_Name;
		ProjectSettings::ProcessValue( displayShortcut.m_Name, copyEnvVars );

		// ProjectName
		displayShortcut.m_ProjectName = project.m_Name;

		// Icon
		displayShortcut.m_Icon = shortcut.m_IconPath;
		ProjectSettings::ProcessValue( displayShortcut.m_Icon, copyEnvVars );

		// Description
		displayShortcut.m_Description = shortcut.m_Description;
		ProjectSettings::ProcessValue( displayShortcut.m_Description, copyEnvVars );

		// StartIn
		if ( copyEnvVars.find( "IG_PROJECT_ROOT" ) != copyEnvVars.end() )
		{
			displayShortcut.m_StartIn = copyEnvVars["IG_PROJECT_ROOT"].m_Value;
			ProjectSettings::ProcessValue( displayShortcut.m_StartIn, copyEnvVars );
		}
		else
		{
			displayShortcut.m_Disable = true;
			displayShortcut.m_DisableReason = "The required variable \"IG_PROJECT_ROOT\" was not defined in the ProjectSettings file.";
		}

		// IG_ROOT
		if ( copyEnvVars.find( "IG_ROOT" ) != copyEnvVars.end() )
		{
			displayShortcut.m_Root = copyEnvVars["IG_ROOT"].m_Value;
			ProjectSettings::ProcessValue( displayShortcut.m_Root, copyEnvVars );
		}
		else
		{
			displayShortcut.m_Disable = true;
			displayShortcut.m_DisableReason = "The required variable \"IG_ROOT\" was not defined in the ProjectSettings file or environment.";
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
			displayShortcut.m_Disable = true;
			displayShortcut.m_DisableReason = "The required variable \"ESHELL_PATH\" was not defined in the ProjectSettings file.";
		}

		if ( !FileExists( eshellPath ) )
		{
			displayShortcut.m_Disable = true;
			displayShortcut.m_DisableReason = "EShell.pl did not exist in the expected location:\n  - \"" + eshellPath + "\"";
		}

		// SettingsFile Path
		std::string settingsFile = project.m_SettingsFile;

		// Build the Command  
		displayShortcut.m_Command = PERL_EXE" \"" + eshellPath + "\"";
		displayShortcut.m_Command += " -settingsFile \"" + settingsFile + "\"";
		displayShortcut.m_Command += " -config " + install.m_Config;

		displayShortcut.m_Command += " -set \"IG_ROOT=";
		displayShortcut.m_Command += displayShortcut.m_Root; 
		displayShortcut.m_Command += "\"";

		if ( !envVarAlias[EnvVarAliasNames::Assets].empty() )
		{
			displayShortcut.m_Command += " -";
			displayShortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Assets ); 
			displayShortcut.m_Command += " " + assets;
		}

		if ( !envVarAlias[EnvVarAliasNames::Code].empty() )
		{
			displayShortcut.m_Command += " -";
			displayShortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Code ); 
			displayShortcut.m_Command += " " + code;
		}

		if ( !envVarAlias[EnvVarAliasNames::Game].empty() && game != project.m_Name )
		{
			displayShortcut.m_Command += " -";
			displayShortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Game ); 
			displayShortcut.m_Command += " " + game;
		}

		if ( !envVarAlias[EnvVarAliasNames::Build].empty() && !buildConfig.empty() )
		{
			displayShortcut.m_Command += " -";
			displayShortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Build ); 
			displayShortcut.m_Command += " " + buildConfig;
		}

		if ( !envVarAlias[EnvVarAliasNames::Tools].empty() && !install.m_Tools.empty() )
		{
			displayShortcut.m_Command += " -";
			displayShortcut.m_Command += Launcher::GetEnvVarAliasNameString( EnvVarAliasNames::Tools ); 
			displayShortcut.m_Command += " " + install.m_Tools;
		}

		if ( !shortcut.m_Args.empty() )
		{
			displayShortcut.m_Command += " " + shortcut.m_Args;
		}

		ProjectSettings::ProcessValue( displayShortcut.m_Command, copyEnvVars );


		// Create the Display Folder
		displayShortcut.m_Folder = "";
		if ( !game.empty() && game != project.m_Name )
		{
			displayShortcut.m_Folder = Capitalize( game, true );
		}

		if ( install.m_Config == Launcher::InstallConfigNames[InstallTypes::Code] )
		{
			displayShortcut.m_Folder += ( displayShortcut.m_Folder.empty() ) ? "" : " ";
			displayShortcut.m_Folder += "Tools Builder";
		}
		else if ( ( install.m_InstallType == InstallTypes::Game ) || ( install.m_InstallType == InstallTypes::Release ) )
		{
			displayShortcut.m_Folder += ( displayShortcut.m_Folder.empty() ) ? "" : " ";

			if ( !tools.empty() && tools != s_DefaultToolsRelease )
			{
				if ( !code.empty() && code != s_DefaultCodeBranch )
				{
					displayShortcut.m_Folder += Capitalize( tools ) + " Tools (" + code + ")";
				}
				else
				{
					displayShortcut.m_Folder += Capitalize( tools ) + " Tools";
				}
			}
			else if ( !code.empty() && code != s_DefaultCodeBranch )
			{
				displayShortcut.m_Folder += Capitalize( RemoveSlashes( code ) ) + " Tools";
			}
		}

		// Special folder names for non-devel and non-game assets branch
		if ( !assets.empty() && game == project.m_Name && assets != s_DefaultAssetBranch )
		{
			if ( displayShortcut.m_Folder.empty() )
			{
				displayShortcut.m_Folder += Capitalize( assets, true ) + " Assets";
			}
			else
			{
				displayShortcut.m_Folder += " (" + Capitalize( assets, true ) + " Assets)";
			}
		}

		// Create the FavoriteName
		displayShortcut.m_FavoriteName = ""; //displayShortcut.m_Name;
		displayShortcut.m_FavoriteName = Capitalize( displayShortcut.m_ProjectName, true ) + " - " + displayShortcut.m_Name;

		bool hasSpecialString = false;
		if ( ( install.m_InstallType == InstallTypes::Game ) || ( install.m_InstallType == InstallTypes::Release ) )
		{
			if ( !tools.empty() && tools != s_DefaultToolsRelease )
			{
				if ( !code.empty() && code != s_DefaultCodeBranch )
				{
					displayShortcut.m_FavoriteName += hasSpecialString ? "/" : " (";
					displayShortcut.m_FavoriteName += Capitalize( tools ) + " Tools [" + code + "]";
					hasSpecialString = true;
				}
				else
				{
					displayShortcut.m_FavoriteName += hasSpecialString ? "/" : " (";
					displayShortcut.m_FavoriteName += Capitalize( tools ) + " Tools";
					hasSpecialString = true;
				}
			}
			else if ( !code.empty() && code != s_DefaultCodeBranch )
			{
				displayShortcut.m_FavoriteName += hasSpecialString ? "/" : " (";
				displayShortcut.m_FavoriteName += Capitalize( RemoveSlashes( code ) ) + " Tools";
				hasSpecialString = true;
			}
		}

		if ( !assets.empty() && game == project.m_Name && assets != s_DefaultAssetBranch )
		{
			displayShortcut.m_FavoriteName += hasSpecialString ? "/" : " (";
			displayShortcut.m_FavoriteName += Capitalize( assets, true ) + " Assets";
			hasSpecialString = true;
		}

		if ( hasSpecialString )
		{
			displayShortcut.m_FavoriteName += ")";
		}

		shortcuts.push_back( displayShortcut );
	}
}

void CreateShortcut( const Project& project, V_DisplayShortcut& shortcuts, const Install& install, const Config& config, const std::string& buildConfig )
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

void CreateShortcuts( const M_Projects& projects, M_DisplayShortcuts& displayShortcuts )
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
		Nocturnal::Insert<M_DisplayShortcuts>::Result insertedShortcuts = displayShortcuts.insert( M_DisplayShortcuts::value_type( projName, V_DisplayShortcut() ) );
		if ( insertedShortcuts.second )
		{
			V_DisplayShortcut& shortcuts = insertedShortcuts.first->second;

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
				m_DisplayShortcuts.clear();
#if 0
				CreateShortcuts( m_Application->m_Projects, m_DisplayShortcuts );
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
	if ( !m_DisplayShortcuts.empty() )
	{
		CreateProjectsMenu( m_Menu );
		m_Menu->AppendSeparator();
	}

	////////////////////////////////////////
	// Create the Setup submenu  
	wxMenu* setupMenu = new wxMenu();

	////////////////////////////////////////
	// Refresh projects
	wxIcon refreshIcon;
	refreshIcon.CopyFromBitmap( wxBitmap( ( const char** )g_Throbber1IconXpm ) );

	wxMenuItem* refreshMenuItem;
	refreshMenuItem = new wxMenuItem( m_Menu, LauncherEventIDs::Refresh, wxT( "Refresh Projects" ), wxEmptyString, wxITEM_NORMAL );
	refreshMenuItem->SetBitmap( refreshIcon );
	refreshMenuItem->SetText( wxT( "Refresh Projects" ) );
	refreshMenuItem->Enable( true );
	setupMenu->Append( refreshMenuItem );

	setupMenu->AppendSeparator();

	////////////////////////////////////////
	// About...
	wxMenuItem* aboutMenuItem;
	aboutMenuItem = new wxMenuItem( setupMenu, LauncherEventIDs::Help, wxString("About ") + m_Application->m_Title.c_str() , wxEmptyString, wxITEM_NORMAL );
	setupMenu->Append( aboutMenuItem );

	wxIcon setupIcon;
	setupIcon.CopyFromBitmap( wxBitmap( ( const char** )g_SetupIconXpm ) );

	wxMenuItem* setupMenuItem = new wxMenuItem(
		m_Menu,
		wxID_ANY,
		wxT( "Setup..." ),
		wxT( "Setup..." ),
		wxITEM_NORMAL,
		setupMenu );
	setupMenuItem->SetBitmap( setupIcon );
	m_Menu->Append( setupMenuItem );

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

void TrayIcon::DetectAndSetIcon( DisplayShortcut& displayShortcut, wxMenuItem* shortcutMenuItem )
{
	if ( !displayShortcut.m_Icon.empty() && FileExists( displayShortcut.m_Icon ) )
	{
		wxBitmap icon;
		icon.LoadFile( displayShortcut.m_Icon, wxBITMAP_TYPE_PNG );
		shortcutMenuItem->SetBitmap( icon );
	}
	else
	{
		//TODO: try to get a good default icon
		wxIcon icon;
		if ( displayShortcut.m_Name.find( "sln" ) != std::string::npos )
		{
			icon.CopyFromBitmap( wxBitmap( ( const char** )g_SlnIconXpm ) );
			shortcutMenuItem->SetBitmap( icon );
		}
		else if ( displayShortcut.m_Name.find( "ompt" ) != std::string::npos )
		{
			icon.CopyFromBitmap( wxBitmap( ( const char** )g_PromptIconXpm ) );
			shortcutMenuItem->SetBitmap( icon );
		}
	}
}

void TrayIcon::CreateProjectsMenu( wxMenu* parentMenu )
{
	wxIcon subMenuIcon;
	subMenuIcon.CopyFromBitmap( wxBitmap( ( const char** )g_FolderIconXpm ) );

	V_DisplayShortcut favoriteShortcuts;

	M_DisplayShortcuts::iterator projItr = m_DisplayShortcuts.begin();
	M_DisplayShortcuts::iterator projEnd = m_DisplayShortcuts.end();
	for ( ; projItr != projEnd; ++projItr )
	{
		const std::string& projName = projItr->first;
		V_DisplayShortcut& shortcuts = projItr->second;

		if ( shortcuts.empty() )
			continue;

		wxMenu* projectMenu = new wxMenu();

		typedef std::map< std::string, wxMenu* > M_SubMenues;
		M_SubMenues subMenues;

		V_DisplayShortcut::iterator shortcutItr = shortcuts.begin();
		V_DisplayShortcut::iterator shortcutEnd = shortcuts.end();
		for ( ; shortcutItr != shortcutEnd; ++shortcutItr )
		{
			DisplayShortcut& displayShortcut = (*shortcutItr);

			wxMenuItem* shortcutMenuItem;
			shortcutMenuItem = new wxMenuItem(
				projectMenu,
				wxID_ANY,
				wxString( wxT( displayShortcut.m_Name.c_str() ) ),
				wxString( wxT( displayShortcut.m_Description.c_str() ) ),
				wxITEM_NORMAL );

			if ( displayShortcut.m_Disable )
			{
				shortcutMenuItem->Enable( false );

				wxString name( "INVALID: " );
				name += displayShortcut.m_Name.c_str();
				shortcutMenuItem->SetText( name );
			}

			DetectAndSetIcon( displayShortcut, shortcutMenuItem );

			shortcutMenuItem->SetRefData( new DisplayShortcut( displayShortcut ) );

			if ( displayShortcut.m_Folder.empty() )
			{
				projectMenu->Append( shortcutMenuItem );
			}
			else
			{
				wxMenu* menuToInsert = new wxMenu();
				std::pair< M_SubMenues::iterator, bool > insertedSubMenu = subMenues.insert( M_SubMenues::value_type( displayShortcut.m_Folder, menuToInsert ) );
				insertedSubMenu.first->second->Append( shortcutMenuItem );
				if ( !insertedSubMenu.second )
				{
					// New menu was not inserted, you have to clean it up
					delete menuToInsert;
				}
			}

			Connect( shortcutMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );

			if ( m_Application->FindFavorite( displayShortcut.m_Command ) )
			{
				favoriteShortcuts.push_back( displayShortcut );
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
			subMenuItem->SetBitmap( subMenuIcon );
			projectMenu->Prepend( subMenuItem );
		}

		parentMenu->Append( wxID_ANY, wxT( Capitalize( projName, true ).c_str() ), projectMenu );
	}

	if ( !favoriteShortcuts.empty() )
	{
		parentMenu->PrependSeparator();

		V_DisplayShortcut::iterator favItr = favoriteShortcuts.begin();
		V_DisplayShortcut::iterator favEnd = favoriteShortcuts.end();
		for( ; favItr != favEnd; ++favItr )
		{
			DisplayShortcut& displayShortcut = (*favItr);

			wxMenuItem* favoritesMenuItem = new wxMenuItem(
				parentMenu,
				wxID_ANY,
				wxString( wxT( displayShortcut.m_FavoriteName.c_str() ) ),
				wxString( wxT( displayShortcut.m_Description.c_str() ) ),
				wxITEM_NORMAL
				);

			DetectAndSetIcon( displayShortcut, favoritesMenuItem );

			favoritesMenuItem->SetRefData( new DisplayShortcut( displayShortcut ) );

			parentMenu->Prepend( favoritesMenuItem );

			Connect( favoritesMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( TrayIcon::OnMenuShortcut ), NULL, this );
		}
	}
}
