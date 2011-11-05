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

	///////////////////////////////////////////////////////////////////////////////
	//<EnvironmentVariableAlias aliasName="code" envVarName="IG_CODE_BRANCH_NAME" />
	///////////////////////////////////////////////////////////////////////////////
	namespace EnvVarAliasNames
	{
		enum EnvVarAliasName
		{
			Assets = 0,
			Build,
			Code,
			Game,
			Tools,

			// Last
			Unknown,
			Count
		};
	}
	typedef EnvVarAliasNames::EnvVarAliasName EnvVarAliasName;
	typedef std::map<EnvVarAliasName, std::string> M_EnvVarAlias;

	const char* GetEnvVarAliasNameString( EnvVarAliasName name );
	EnvVarAliasName GetEnvVarAliasNameEnum( const char* name );

	///////////////////////////////////////////////////////////////////////////////
	//<EnvVar variableName="P4PORT" value="perforce.insomniacgames.com:60606" override="0" />
	class EnvVar
	{
	public:
		std::string m_VariableName;
		std::string m_Value;

		// will have either value or a path
		bool m_IsPath;
		bool m_Override; // default: true
	};
	typedef std::map<std::string, EnvVar> M_EnvVar;

	///////////////////////////////////////////////////////////////////////////////
	//<Include path="%IG_PROJECT_CODE%\config\CodeSettings.xml"/>
	//<Include>%IG_PROJECT_CODE%\%IG_GAME%\config\GameSettings.xml</Include>
	//<Include path="%IG_PROJECT_TOOLS%\config\ReleaseSettingsWEWEWE.xml" optional="1" />
	typedef std::vector<std::pair< std::string, bool >> M_IncludeFiles;

	///////////////////////////////////////////////////////////////////////////////
	struct Shortcut
	{
		std::string m_Name;             // Display name: FXT, prompt, etc...
		std::string m_Args;             // Args to pass to eshell
		std::string m_Description;      // Mouse over description
		std::string m_IconPath;         // Path to the .png file
	};
	typedef std::vector< Shortcut > V_Shortcut; // Shortcuts

	///////////////////////////////////////////////////////////////////////////////
	struct EShell
	{
		V_Shortcut m_Shortcuts;
	};

	///////////////////////////////////////////////////////////////////////////////
	//<Config name="gameplay" parent="production" description="SP and MP Gameplay programmers">
	struct Config
	{
		std::string m_Name;
		std::string m_Parent;
		std::string m_Description;
		bool        m_Hidden;  // used to hide from displayed configs; default: false

		M_EnvVar  m_EnvVar;
		EShell    m_EShell;
		M_IncludeFiles  m_IncludeFiles;
	};
	typedef std::vector< Config > V_Config;
	typedef std::map< std::string, Config > M_Config;


	///////////////////////////////////////////////////////////////////////////////
	class ProjectSettings
	{
	public:
		ProjectSettings();
		~ProjectSettings();

		bool LoadFile( const std::string& settingsFile, bool includeFile = false );

		static void ProcessValue( std::string& value, const M_EnvVar& envVar );

		static void SetEnvVar( const std::string& envVarName, const std::string& envVarValue, M_EnvVar& envVars, bool isPath = true );
		static void GetEnvVarAliasValue( const std::string& envVarName, const M_EnvVar& envVars, std::string& aliasVar, const char* defaultValue = NULL );

	protected:
		static void ParseEnvVar( TiXmlElement* elem, M_EnvVar& envVars, bool includeFile = false );
		static void ParseEnvVarAlias( TiXmlElement* elem, M_EnvVarAlias& envVarAliases );
		static void ParseInclude( TiXmlElement* elem, M_IncludeFiles& includes );
		static void ParseConfig( TiXmlElement* elem, M_Config& configs, M_EnvVar& globalEnvVar );
		static void ParseEShell( TiXmlElement* elem, EShell& eshell, M_EnvVar& envVars );

		static std::string ProcessEnvVar( const EnvVar& envVar, const M_EnvVar& envVars, S_string& currentlyProcessing = S_string() );

	public:
		M_EnvVarAlias m_EnvVarAlias;
		M_EnvVar m_EnvVar;
		M_Config m_Configs;

		M_IncludeFiles m_IncludeFiles;
	};
}
