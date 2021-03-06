#include "stdafx.h"
#include "Project.h"

#include "Exception.h"
#include "Helper.h"

#include <regex>

using namespace EShellMenu;

Project::Project()
{
}

Project::~Project()
{
}

void Project::SetEnvVar( const tstring& envVarName, const tstring& envVarValue, M_EnvVar& envVars, bool isPath )
{
	if ( !envVarName.empty() && !envVarValue.empty() )
	{
		envVars[envVarName].m_Value = envVarValue;
		envVars[envVarName].m_IsPath = isPath;
	}
}

void Project::GetEnvVarAliasValue( const tstring& envVarName, const M_EnvVar& envVars, tstring& aliasVar, const tchar_t* defaultValue )
{
	if ( !envVarName.empty() )
	{
		M_EnvVar::const_iterator foundEnvVar = envVars.find( envVarName );
		if ( foundEnvVar != envVars.end() )
		{
			aliasVar = foundEnvVar->second.m_Value;

			Project::ProcessValue( aliasVar, envVars );
		}
		else if ( defaultValue )
		{
			aliasVar = defaultValue;
		}
	}
}

void Project::ProcessValue( tstring& value, const M_EnvVar& envVars )
{
	const tregex grepTokens( wxT("%(.*?)%"), std::regex::icase );

	bool failed = false;
	while ( !failed && std::regex_search( value, grepTokens ) )
	{
		tstring tempValue = value;
		for ( tsregex_iterator itr( tempValue.begin(), tempValue.end(), grepTokens ), end; itr != end; ++itr )
		{
			const std::match_results<tstring::const_iterator>& resultTokens = *itr;

			tstring varName ( resultTokens[1].first, resultTokens[1].second );
			if ( !varName.empty() )
			{
				M_EnvVar::const_iterator foundVar = envVars.find( varName );
				if ( foundVar != envVars.end() )
				{
					tstring replaceTokenStr = wxT("%") + varName + wxT("%");
					tregex replaceToken( replaceTokenStr, std::regex::icase );
					value = std::regex_replace( value, replaceToken, ProcessEnvVar( foundVar->second, envVars ) );
				}
				else
				{
					failed = true;
				}
			}
		}
	}
}

tstring Project::ProcessEnvVar( const EnvVar& envVar, const M_EnvVar& envVars, std::set< tstring >& currentlyProcessing )
{
	std::pair< std::set< tstring >::const_iterator, bool > inserted = currentlyProcessing.insert( envVar.m_Name );
	if ( !inserted.second )
	{
		throw Exception( wxT("Cyclical environment variable reference found for: %s"), envVar.m_Name.c_str() );
	}

	// we are now processing this EnvVar
	tstring processedValue = envVar.m_Value;

	const tregex grepTokens( wxT("%([A-Z_\\-]*)%"), std::regex::icase );
	for ( tsregex_iterator itr( envVar.m_Value.begin(), envVar.m_Value.end(), grepTokens ), end; itr != end; ++itr )
	{
		const std::match_results<tstring::const_iterator>& resultTokens = *itr;

		tstring varName ( resultTokens[1].first, resultTokens[1].second );
		tstring varValue;
		if ( varName == envVar.m_Name 
			&& EShellMenu::GetEnvVar( varName, varValue ) )
		{
			// do nothing
		}
		else if ( currentlyProcessing.find( varName ) != currentlyProcessing.end() )
		{
			throw Exception( wxT("Cyclical environment variable reference found for: %s and %s"), envVar.m_Name.c_str(), varName.c_str() );
		}
		else
		{
			M_EnvVar::const_iterator foundVar = envVars.find( varName );
			if ( foundVar != envVars.end() )
			{
				varValue = ProcessEnvVar( foundVar->second, envVars );
			}
			else if ( !EShellMenu::GetEnvVar( varName, varValue ) )
			{
				throw Exception( wxT("Unknown environment variable reference found: %s"), varName.c_str() );
			}
		}

		tstring replaceTokenStr = wxT("%") + varName + wxT("%");
		tregex replaceToken( replaceTokenStr, std::regex::icase ); 
		processedValue = std::regex_replace( processedValue, replaceToken, varValue );
	}

	currentlyProcessing.erase( envVar.m_Name );

	if ( envVar.m_IsPath )
	{
		wxFileName name( processedValue );

		// handle relpathing
		name.Normalize();

		// wx doesn't handle trailing relpaths
		if ( name.GetFullName() == wxT( ".." ) )
		{
			// strip the ".." off and be explicit that it's a folder path, not a file path
			name = wxFileName( name.GetPath() + wxT( "\\" ) );

			// remove the folder specified by the ".." stripped above
			name.RemoveLastDir();
		}

		bool trailingSlash = *processedValue.rbegin() == '\\';

		processedValue = name.GetFullPath().c_str();

		if ( trailingSlash && *processedValue.rbegin() != '\\' )
		{
			processedValue += '\\';
		}
		else if ( !trailingSlash && *processedValue.rbegin() == '\\' )
		{
			processedValue.resize( processedValue.size() - 1 );
		}
	}

	return processedValue;
}

//<EnvVar variableName="P4PORT" value="perforce:1666" override="0" />
void Project::ParseEnvVar( wxXmlNode* elem, M_EnvVar& envVars, bool includeFile )
{
	tstring varName = elem->GetAttribute( wxT("variableName") );

	// when override is false, we don't override existing variables; defaults to true
	bool overrideAttrib;
	{
		wxString value;
		if ( elem->GetAttribute( wxT("override"), &value ) )
		{
			overrideAttrib = value == wxT("1");
		}
		else
		{
			overrideAttrib = true;
		}
	}
	overrideAttrib = includeFile ? false : overrideAttrib;

	// 'if' conditional information
	tstring ifStatement;
	{
		wxString value;
		if ( elem->GetAttribute( wxT("if"), &value ) )
		{
			ifStatement = value;

			const tregex ifPattern ( wxT("(.*?)([!=]+)(.*)") );

			tsmatch ifResults; 
			if ( std::regex_match( ifStatement, ifResults, ifPattern ) )
			{
				const tstring& var = ifResults[1];
				const tstring& op = ifResults[2];
				const tstring& val = ifResults[3];

				tstring existing;
				bool gotten = EShellMenu::GetEnvVar( var, existing );

				M_EnvVar::const_iterator found = envVars.find( var );
				if ( found != envVars.end() )
				{
					existing = found->second.m_Value;
					gotten = true;
				}

				if ( op == wxT("=") || op == wxT("==") )
				{
					if ( !gotten )
					{
						return;
					}

					if ( existing != val )
					{
						return;
					}
				}
				else if ( op == wxT("!=") )
				{
					if ( gotten && existing == val )
					{
						return;
					}
				}
			}
			else
			{
				tstring processed = ifStatement;
				Project::ProcessValue( processed, envVars );

				tstring existing;
				if ( !EShellMenu::GetEnvVar( ifStatement, existing ) && !FileExists( processed ) )
				{
					return;
				}
			}
		}
	}

	std::pair< std::map<tstring, EnvVar>::iterator, bool > inserted = envVars.insert( M_EnvVar::value_type( varName, EnvVar() ) );

	// early out if we didn't insert and the existing variable override is false
	EnvVar& envVar = inserted.first->second;
	if ( !inserted.second && !overrideAttrib )
	{
		return;
	}

	// Update/Fillout the EnvVar
	envVar.m_Name = varName;
	envVar.m_IsOverride = overrideAttrib;

	// Type
	envVar.m_IsPath = false;
	{
		wxString type;
		if ( elem->GetAttribute( wxT("type"), &type ) )
		{
			std::transform( type.begin(), type.end(), type.begin(), tolower);
			envVar.m_IsPath = ( type == wxT("path") );
		}
	}

	// Value
	envVar.m_Value.clear();
	{
		wxString value;
		if ( elem->GetAttribute( wxT("value"), &value ) )
		{
			envVar.m_Value = value;
		}
	}

	// if we're not overriding, make sure the variable is not already defined
	if ( !envVar.m_IsOverride )
	{
		tstring varValue;
		if ( EShellMenu::GetEnvVar( envVar.m_Name, varValue ) )
		{
			EShellMenu::ConsolePrint( wxT("EnvVar %s's override is '0'; using machine value \"%s\" (rather than settings file value \"%s\").\n"), varName.c_str(), varValue.c_str(), envVar.m_Value.c_str() );
			envVar.m_Value = varValue;
		}
	}

}

//<MenuItem>
//  <Name>Luna</Name>
//  <Args>-exec Luna</Args>
//  <Description>Luna</Description>
//  <Icon location="%IG_PROJECT_BIN%\Luna.exe" number="0" />
//  <IconPath>%IG_PROJECT_DATA%\luna\themes\default\moon_16.png</IconPath>
//</MenuItem>
void Project::ParseShortcut( wxXmlNode* elem, std::vector< Shortcut >& shortcuts, M_EnvVar& envVars )
{
	Shortcut shortcut;

	wxString optional;
	if ( elem->GetAttribute( wxT("optional"), &optional ) )
	{
		shortcut.m_Optional = optional == wxT("1");
	}

	for ( wxXmlNode* shortcutElem = elem->GetChildren(); shortcutElem != NULL; shortcutElem = shortcutElem->GetNext() )
	{
		tstring shortcutElemString = shortcutElem->GetName();

		if ( shortcutElemString.compare( wxT("Name") ) == 0 )
		{
			shortcut.m_Name = shortcutElem->GetNodeContent();
		}
		else if ( shortcutElemString.compare( wxT("Folder") ) == 0 )
		{
			shortcut.m_Folder = shortcutElem->GetNodeContent();
		}
		else if ( shortcutElemString.compare( wxT("Description") ) == 0 )
		{
			shortcut.m_Description = shortcutElem->GetNodeContent();
		}
		else if ( shortcutElemString.compare( wxT("Icon") ) == 0 && shortcutElem->GetNodeContent().length() )
		{
			shortcut.m_IconPath = shortcutElem->GetNodeContent();
		}
		else if ( shortcutElemString.compare( wxT("Args") ) == 0 )
		{
			shortcut.m_Args = shortcutElem->GetNodeContent();
		}
		else if ( shortcutElemString.compare( wxT("Target") ) == 0 )
		{
			shortcut.m_Target = shortcutElem->GetNodeContent();
			shortcut.m_TargetWorkingDir = shortcutElem->GetAttribute("workingDir");
			shortcut.m_TargetInstaller = shortcutElem->GetAttribute("installer");
		}
	}

	shortcuts.push_back( shortcut );
}

//<Include>%IG_PROJECT_CODE%\config\SDKSubscription.xml</Include>
void Project::ParseInclude( wxXmlNode* elem, std::vector< Include >& includes )
{
	Include includeFile;

	wxString path;
	if ( elem->GetNodeContent().length() )
	{
		includeFile.m_Path = elem->GetNodeContent();
	}
	else if ( elem->GetAttribute( wxT("path"), &path ) )
	{
		includeFile.m_Path = path;
	}

	wxString optionalAttrib;
	if ( elem->GetAttribute( wxT("optional"), &optionalAttrib ) && optionalAttrib == wxT("1") )
	{
		includeFile.m_Optional = true;
	}

	includes.push_back( includeFile );
}

//<Config name="gameplay" parent="production" description="SP and MP Gameplay programmers">
void Project::ParseConfig( wxXmlNode* elem, std::map< tstring, Config >& configs, M_EnvVar& globalEnvVar )
{
	wxString hiddenAttrib;
	if ( elem->GetAttribute( wxT("hidden"), &hiddenAttrib ) && hiddenAttrib == wxT("1") )
	{
		// hidden config... skip it
		return;
	}

	Config config;
	config.m_Name = elem->GetAttribute( wxT("name") );
	config.m_Parent = elem->GetAttribute( wxT("parent"), wxEmptyString );

	std::map< tstring, Config >::iterator foundParent = configs.find( config.m_Parent );
	if ( foundParent != configs.end() )
	{
		config.m_EnvVar = foundParent->second.m_EnvVar;
	}
	else
	{
		config.m_EnvVar = globalEnvVar;
	}

	config.m_Description = elem->GetAttribute( wxT("description"), wxEmptyString );

	for ( wxXmlNode* configElem = elem->GetChildren(); configElem != NULL; configElem = configElem->GetNext() )
	{
		tstring configElemString = configElem->GetName();

		//<EnvVar>
		if ( configElemString.compare( wxT("EnvVar") ) == 0 )
		{
			ParseEnvVar( configElem, config.m_EnvVar );
		}
		//<MenuItem>
		else if ( configElemString.compare( wxT("Shortcut") ) == 0 )
		{
			ParseShortcut( configElem, config.m_Shortcuts, config.m_EnvVar );
		}
		//<Include>
		else if ( configElemString.compare( wxT("Include") ) == 0 )
		{
			ParseInclude( configElem, config.m_Includes );
		}
	}

	configs.insert( std::map< tstring, Config >::value_type( config.m_Name, config ) );
}

bool Project::LoadFile( const tstring& file, bool includeFile )
{
	if ( m_File.empty() )
	{
		m_File = file;
	}

	// open the config file
	wxXmlDocument doc;
	if ( !doc.Load( file.c_str() ) )
	{
		return false;
	}

	wxFileName name( file );
	EnvVar settingsFile;
	settingsFile.m_IsPath = true;
	settingsFile.m_Name = wxT( "ESHELL_SETTINGS_FILE" );
	settingsFile.m_Value = name.GetFullPath().c_str();
	m_EnvVar[wxT( "ESHELL_SETTINGS_FILE" )] = settingsFile;

	EnvVar settingsDir;
	settingsDir.m_IsPath = true;
	settingsDir.m_Name = wxT( "ESHELL_SETTINGS_DIR" );
	settingsDir.m_Value = name.GetPath().c_str();
	m_EnvVar[wxT( "ESHELL_SETTINGS_DIR" )] = settingsDir;

	// <Project>
	wxXmlNode* projectSettings = doc.GetRoot();
	if ( projectSettings && tstring (projectSettings->GetName()) == wxT("Settings") )
	{
		for ( wxXmlNode* elem = projectSettings->GetChildren(); elem != NULL; elem = elem->GetNext() )
		{
			tstring elemString = elem->GetName();

			//<EnvVar>
			if ( elemString.compare( wxT("EnvVar") ) == 0 )
			{
				ParseEnvVar( elem, m_EnvVar, includeFile );
			}
			//<Include>
			else if ( elemString.compare( wxT("Include") ) == 0 )
			{
				ParseInclude( elem, m_Includes );
			}
			//<Config>
			else if ( elemString.compare( wxT("Config") ) == 0 )
			{
				ParseConfig( elem, m_Configs, m_EnvVar );
			}
		}
	}

	// We can't ProcessInculdeFiles here because part of the path may
	// have been defined in the with EvnVars in the EnvironmentVariableAlias,
	// so we can't ProcessValue on the include path or we will get the
	// defaults values."
	std::vector< Include >::iterator includeFileItr = m_Includes.begin();
	std::vector< Include >::iterator includeFileEnd = m_Includes.end();
	for ( ; includeFileItr != includeFileEnd; ++includeFileItr )
	{
		if ( !includeFileItr->m_Loaded )
		{
			includeFileItr->m_Loaded = true;

			tstring includeFile = includeFileItr->m_Path;

			// This would be bad because it's going to use all the default values for env vars that have aliases
			ProcessValue( includeFile, m_EnvVar );

			if ( includeFile.find( wxT(':') ) == tstring::npos )
			{
				wxFileName name ( file );
				tstring relFile = name.GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR ).c_str();
				includeFile = relFile + includeFile;
			}

			if ( FileExists( includeFile ) && !LoadFile( includeFile, true ) )
			{
				return false;
			}
		}
	}

	m_Title = wxT("%ESHELL_TITLE%");
	ProcessValue( m_Title, m_EnvVar );

	return true;
}