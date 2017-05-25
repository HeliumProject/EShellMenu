#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace EShellMenu
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

	class Include
	{
	public:
		Include()
			: m_Optional( false )
			, m_Loaded( false )
		{

		}

		tstring m_Path;
		bool m_Optional;
		bool m_Loaded;
	};

	class Shortcut
	{
	public:
		Shortcut()
			: m_Optional( false )
		{

		}

		tstring m_Name;             // Display name: "prompt", etc...
		tstring m_Folder;			// Folder to put it in (under the project menu)
		tstring m_Description;      // Mouse over description
		tstring m_IconPath;         // Path to the .png file
		tstring m_Args;             // Args to pass to eshell, overrides
		tstring m_Target;			// If no Args specified, use -run <blah> to this file
		tstring m_TargetWorkingDir;	// The working directory to launch the target in
		tstring m_TargetInstaller;	// If target is defined, but doesn't exist, kick into this installer
		bool	m_Optional;
	};

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
		std::vector< Shortcut > m_Shortcuts;
		std::vector< Include > m_Includes;
	};

	class Project
	{
	public:
		Project();
		~Project();

		bool LoadFile( const tstring& project, bool includeFile = false );

		static void ProcessValue( tstring& value, const M_EnvVar& envVar );
		static void SetEnvVar( const tstring& envVarName, const tstring& envVarValue, M_EnvVar& envVars, bool isPath = true );
		static void GetEnvVarAliasValue( const tstring& envVarName, const M_EnvVar& envVars, tstring& aliasVar, const tchar_t* defaultValue = NULL );

	protected:
		static void ParseEnvVar( wxXmlNode* elem, M_EnvVar& envVars, bool includeFile = false );
		static void ParseInclude( wxXmlNode* elem, std::vector< Include >& includes );
		static void ParseConfig( wxXmlNode* elem, std::map< tstring, Config >& configs, M_EnvVar& globalEnvVar );
		static void ParseShortcut( wxXmlNode* elem, std::vector< Shortcut >& shortcuts, M_EnvVar& envVars );

		static tstring ProcessEnvVar( const EnvVar& envVar, const M_EnvVar& envVars, std::set< tstring >& currentlyProcessing = std::set< tstring >() );

	public:
		tstring m_Title;
		tstring m_File;
		M_EnvVar m_EnvVar;
		std::map< tstring, Config > m_Configs;
		std::vector< Include > m_Includes;
	};

	class StringRefData : public wxObjectRefData
	{
	public:
		StringRefData( const tstring& str )
			: m_Value( str )
		{

		}

		tstring m_Value;
	};
}
