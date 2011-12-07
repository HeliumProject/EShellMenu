#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace Launcher
{
	class EnvVar
	{
	public:
		EnvVar()
			: m_IsPath( false )
			, m_IsOverride( true )
		{

		}

		tstring m_Name;
		tstring m_Value;
		bool m_IsPath;
		bool m_IsOverride;
	};
	typedef std::map<tstring, EnvVar> M_EnvVar;

	class IncludeFile
	{
	public:
		IncludeFile()
			: m_Optional( false )
		{

		}

		tstring m_Path;
		bool m_Optional;
	};
	typedef std::vector< IncludeFile > V_IncludeFiles;

	class ShortcutInfo
	{
	public:
		tstring m_Name;             // Display name: "prompt", etc...
		tstring m_Folder;			// Folder to put it in (under the project menu)
		tstring m_Args;             // Args to pass to eshell
		tstring m_Description;      // Mouse over description
		tstring m_IconPath;         // Path to the .png file
	};
	typedef std::vector< ShortcutInfo > V_ShortcutInfo;

	class Config
	{
	public:
		Config()
			: m_Hidden( false )
		{

		}

		tstring m_Name;
		tstring m_Parent;
		tstring m_Description;
		bool m_Hidden;		// used to hide from displayed configs

		M_EnvVar m_EnvVar;
		V_ShortcutInfo m_Shortcuts;
		V_IncludeFiles m_IncludeFiles;
	};
	typedef std::vector< Config > V_Config;
	typedef std::map< tstring, Config > M_Config;

	class Settings
	{
	public:
		Settings();
		~Settings();

		bool LoadFile( const tstring& settingsFile, bool includeFile = false );

		static void ProcessValue( tstring& value, const M_EnvVar& envVar );
		static void SetEnvVar( const tstring& envVarName, const tstring& envVarValue, M_EnvVar& envVars, bool isPath = true );
		static void GetEnvVarAliasValue( const tstring& envVarName, const M_EnvVar& envVars, tstring& aliasVar, const tchar_t* defaultValue = NULL );

	protected:
		static void ParseEnvVar( wxXmlNode* elem, M_EnvVar& envVars, bool includeFile = false );
		static void ParseInclude( wxXmlNode* elem, V_IncludeFiles& includes );
		static void ParseConfig( wxXmlNode* elem, M_Config& configs, M_EnvVar& globalEnvVar );
		static void ParseShortcut( wxXmlNode* elem, V_ShortcutInfo& shortcuts, M_EnvVar& envVars );

		static tstring ProcessEnvVar( const EnvVar& envVar, const M_EnvVar& envVars, std::set< tstring >& currentlyProcessing = std::set< tstring >() );

	public:
		tstring m_Title;
		tstring m_File;
		M_EnvVar m_EnvVar;
		M_Config m_Configs;
		V_IncludeFiles m_IncludeFiles;
	};

	typedef std::vector< Settings > V_Settings;
}
