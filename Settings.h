#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

typedef std::set<std::string> S_string;
typedef std::vector<std::string> V_string;

//
// Forwards
//
class TiXmlElement;

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

		std::string m_Name;
		std::string m_Value;
		bool m_IsPath;
		bool m_IsOverride;
	};
	typedef std::map<std::string, EnvVar> M_EnvVar;

	class IncludeFile
	{
	public:
		IncludeFile()
			: m_Optional( false )
		{

		}

		std::string m_Path;
		bool m_Optional;
	};
	typedef std::vector< IncludeFile > V_IncludeFiles;

	class ShortcutInfo
	{
	public:
		std::string m_Name;             // Display name: "prompt", etc...
		std::string m_Args;             // Args to pass to eshell
		std::string m_Description;      // Mouse over description
		std::string m_IconPath;         // Path to the .png file
	};
	typedef std::vector< ShortcutInfo > V_ShortcutInfo;

	class Config
	{
	public:
		Config()
			: m_Hidden( false )
		{

		}

		std::string m_Name;
		std::string m_Parent;
		std::string m_Description;
		bool m_Hidden;		// used to hide from displayed configs

		M_EnvVar m_EnvVar;
		V_ShortcutInfo m_Shortcuts;
		V_IncludeFiles m_IncludeFiles;
	};
	typedef std::vector< Config > V_Config;
	typedef std::map< std::string, Config > M_Config;

	class Settings
	{
	public:
		Settings();
		~Settings();

		bool LoadFile( const std::string& settingsFile, bool includeFile = false );

		static void ProcessValue( std::string& value, const M_EnvVar& envVar );
		static void SetEnvVar( const std::string& envVarName, const std::string& envVarValue, M_EnvVar& envVars, bool isPath = true );
		static void GetEnvVarAliasValue( const std::string& envVarName, const M_EnvVar& envVars, std::string& aliasVar, const char* defaultValue = NULL );

	protected:
		static void ParseEnvVar( TiXmlElement* elem, M_EnvVar& envVars, bool includeFile = false );
		static void ParseInclude( TiXmlElement* elem, V_IncludeFiles& includes );
		static void ParseConfig( TiXmlElement* elem, M_Config& configs, M_EnvVar& globalEnvVar );
		static void ParseShortcuts( TiXmlElement* elem, V_ShortcutInfo& shortcuts, M_EnvVar& envVars );

		static std::string ProcessEnvVar( const EnvVar& envVar, const M_EnvVar& envVars, S_string& currentlyProcessing = S_string() );

	public:
		M_EnvVar m_EnvVar;
		M_Config m_Configs;
		V_IncludeFiles m_IncludeFiles;
	};
}
