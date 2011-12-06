#include "stdafx.h"
#include "Settings.h"

#include "Exception.h"
#include "Helper.h"

#include <regex>

using namespace Launcher;

Settings::Settings()
{
}

Settings::~Settings()
{
}

void Settings::SetEnvVar( const std::string& envVarName, const std::string& envVarValue, M_EnvVar& envVars, bool isPath )
{
	if ( !envVarName.empty() && !envVarValue.empty() )
	{
		envVars[envVarName].m_Value = envVarValue;
		envVars[envVarName].m_IsPath = isPath;
	}
}

void Settings::GetEnvVarAliasValue( const std::string& envVarName, const M_EnvVar& envVars, std::string& aliasVar, const char* defaultValue )
{
	if ( !envVarName.empty() )
	{
		M_EnvVar::const_iterator foundEnvVar = envVars.find( envVarName );
		if ( foundEnvVar != envVars.end() )
		{
			aliasVar = foundEnvVar->second.m_Value;

			Settings::ProcessValue( aliasVar, envVars );
		}
		else if ( defaultValue )
		{
			aliasVar = defaultValue;
		}
	}
}

void Settings::ProcessValue( std::string& value, const M_EnvVar& envVars )
{
	const std::regex grepTokens( "%(.*?)%", std::regex::icase  );

	std::string tempValue = value;
	std::sregex_iterator itr( tempValue.begin(), tempValue.end(), grepTokens );
	std::sregex_iterator end;

	for ( ; itr != end; ++itr )
	{
		const std::match_results<std::string::const_iterator>& resultTokens = *itr;

		std::string varName ( resultTokens[1].first, resultTokens[1].second );
		if ( !varName.empty() )
		{
			M_EnvVar::const_iterator foundVar = envVars.find( varName );
			if ( foundVar != envVars.end() )
			{
				std::string replaceTokenStr = "%" + varName + "%";
				std::regex replaceToken( replaceTokenStr, std::regex::icase );
				value = std::regex_replace( value, replaceToken, ProcessEnvVar( foundVar->second, envVars ) );
			}
		}
	}
}

std::string Settings::ProcessEnvVar( const EnvVar& envVar, const M_EnvVar& envVars, std::set< std::string >& currentlyProcessing )
{
	std::pair< std::set< std::string >::const_iterator, bool > inserted = currentlyProcessing.insert( envVar.m_Name );
	if ( !inserted.second )
	{
		throw Exception( "Cyclical environment variable reference found for: %s", envVar.m_Name.c_str() );
	}

	// we are now processing this EnvVar
	std::string processedValue = envVar.m_Value;

	const std::regex grepTokens( "%([A-Z_\\-]*)%", std::regex::icase  );
	std::sregex_iterator itr( envVar.m_Value.begin(), envVar.m_Value.end(), grepTokens );
	std::sregex_iterator end;

	for ( ; itr != end; ++itr )
	{
		const std::match_results<std::string::const_iterator>& resultTokens = *itr;

		std::string varName ( resultTokens[1].first, resultTokens[1].second );
		std::string varValue;
		if ( varName == envVar.m_Name 
			&& Launcher::GetEnvVar( varName, varValue ) )
		{
			// do nothing
		}
		else if ( currentlyProcessing.find( varName ) != currentlyProcessing.end() )
		{
			throw Exception( "Cyclical environment variable reference found for: %s and %s", envVar.m_Name.c_str(), varName.c_str() );
		}
		else
		{
			M_EnvVar::const_iterator foundVar = envVars.find( varName );
			if ( foundVar != envVars.end() )
			{          
				varValue = ProcessEnvVar( foundVar->second, envVars );
			}
			else if ( !Launcher::GetEnvVar( varName, varValue ) )
			{
				throw Exception( "Unknown environment variable reference found: %s", varName.c_str() );
			}
		}

		std::string replaceTokenStr = "%" + varName + "%";
		std::regex replaceToken( replaceTokenStr, std::regex::icase ); 
		processedValue = std::regex_replace( processedValue, replaceToken, varValue );
	}

	currentlyProcessing.erase( envVar.m_Name );

	return processedValue;
}

//<EnvVar variableName="P4PORT" value="perforce.insomniacgames.com:60606" override="0" />
void Settings::ParseEnvVar( TiXmlElement* elem, M_EnvVar& envVars, bool includeFile )
{
	std::string varName = elem->Attribute( "variableName" );
	//toUpper( varName );

	// when override is false, we don't override existing variables; defaults to true
	int intValue;
	bool overrideAttrib = ( ( elem->Attribute( "override", &intValue ) == NULL ) || ( intValue == 1 ) ) ? true : false ;
	overrideAttrib = includeFile ? false : overrideAttrib;

	std::pair< std::map<std::string, EnvVar>::iterator, bool > inserted = envVars.insert( M_EnvVar::value_type( varName, EnvVar() ) );

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
	if ( elem->Attribute( "type" ) )
	{
		std::string type = elem->Attribute( "type" );
		std::transform( type.begin(), type.end(), type.begin(), tolower);
		envVar.m_IsPath = ( type == "path" );
	}

	// Value
	envVar.m_Value = "";
	if ( elem->Attribute( "value" ) )
	{
		envVar.m_Value = elem->Attribute( "value" );
	}

	// if we're not overriding, make sure the variable is not already defined
	if ( !envVar.m_IsOverride )
	{
		std::string varValue("");
		if ( Launcher::GetEnvVar( envVar.m_Name, varValue ) )
		{
			Launcher::ConsolePrint( "EnvVar %s's override is '0'; using machine value \"%s\" (rather than settings file value \"%s\").\n", varName.c_str(), varValue.c_str(), envVar.m_Value.c_str() );
			envVar.m_Value = varValue;
		}
	}

}

//<Shortcut>
//  <Name>Luna</Name>
//  <Args>-exec Luna</Args>
//  <Description>Luna</Description>
//  <Icon location="%IG_PROJECT_BIN%\Luna.exe" number="0" />
//  <IconPath>%IG_PROJECT_DATA%\luna\themes\default\moon_16.png</IconPath>
//</Shortcut>
void Settings::ParseShortcut( TiXmlElement* elem, V_ShortcutInfo& shortcuts, M_EnvVar& envVars )
{
	ShortcutInfo shortcut;

	for ( TiXmlElement* shortcutElem = elem->FirstChildElement(); shortcutElem != NULL; shortcutElem = shortcutElem->NextSiblingElement() )
	{
		std::string shortcutElemString = shortcutElem->Value();

		if ( shortcutElemString.compare( "Name" ) == 0 )
		{
			shortcut.m_Name = shortcutElem->GetText();
		}
		else if ( shortcutElemString.compare( "Folder" ) == 0 )
		{
			shortcut.m_Folder = shortcutElem->GetText();
		}
		else if ( shortcutElemString.compare( "Run" ) == 0 )
		{
			shortcut.m_Args = std::string ( "-run \"" ) + shortcutElem->GetText() + "\"";
			shortcut.m_IconPath = shortcutElem->GetText();
		}
		else if ( shortcutElemString.compare( "Args" ) == 0 )
		{
			shortcut.m_Args = shortcutElem->GetText();
		}
		else if ( shortcutElemString.compare( "Description" ) == 0 )
		{
			shortcut.m_Description = shortcutElem->GetText();
		}
		else if ( shortcutElemString.compare( "Icon" ) == 0 && shortcutElem->GetText() )
		{
			shortcut.m_IconPath = shortcutElem->GetText();
		}
	}

	shortcuts.push_back( shortcut );
}

//<Include>%IG_PROJECT_CODE%\config\SDKSubscription.xml</Include>
void Settings::ParseInclude( TiXmlElement* elem, V_IncludeFiles& includes )
{
	IncludeFile includeFile;

	if ( elem->GetText() )
	{
		includeFile.m_Path = elem->GetText();
	}
	else if ( elem->Attribute( "path" ) )
	{
		includeFile.m_Path = elem->Attribute( "path" );
	}

	int intValue;
	if ( ( elem->Attribute( "optional", &intValue ) != NULL ) && ( intValue == 1 ) )
	{
		includeFile.m_Optional = true;
	}

	includes.push_back( includeFile );
}

//<Config name="gameplay" parent="production" description="SP and MP Gameplay programmers">
void Settings::ParseConfig( TiXmlElement* elem, M_Config& configs, M_EnvVar& globalEnvVar )
{
	int intValue;
	if ( ( elem->Attribute( "hidden", &intValue ) != NULL ) && ( intValue == 1 ) )
	{
		// hidden config... skip it
		return;
	}

	Config config;
	config.m_Name = elem->Attribute( "name" );

	config.m_Parent = "";
	if ( elem->Attribute( "parent" ) )
	{
		config.m_Parent = elem->Attribute( "parent" );
	}

	M_Config::iterator foundParent = configs.find( config.m_Parent );
	if ( foundParent != configs.end() )
	{
		config.m_EnvVar = foundParent->second.m_EnvVar;
	}
	else
	{
		config.m_EnvVar = globalEnvVar;
	}

	config.m_Description = "";
	if ( elem->Attribute( "description" ) )
	{
		config.m_Description = elem->Attribute( "description" );
	}

	for ( TiXmlElement* configElem = elem->FirstChildElement(); configElem != NULL; configElem = configElem->NextSiblingElement() )
	{
		std::string configElemString = configElem->Value();

		//<EnvVar>
		if ( configElemString.compare( "EnvVar" ) == 0 )
		{
			ParseEnvVar( configElem, config.m_EnvVar );
		}
		//<Shortcut>
		else if ( configElemString.compare( "Shortcut" ) == 0 )
		{
			ParseShortcut( configElem, config.m_Shortcuts, config.m_EnvVar );
		}
		//<Include>
		else if ( configElemString.compare( "Include" ) == 0 )
		{
			ParseInclude( configElem, config.m_IncludeFiles );
		}
	}

	configs.insert( M_Config::value_type( config.m_Name, config ) );
}

bool Settings::LoadFile( const std::string& file, bool includeFile )
{
	m_File = file;

	//open the config file
	TiXmlDocument doc;
	if (!doc.LoadFile( file.c_str() ))
	{
		return false;
	}

	// <Settings>
	TiXmlElement* projectSettings = doc.FirstChildElement();
	if ( projectSettings && std::string (projectSettings->Value()) == "Settings" )
	{
		for ( TiXmlElement* elem = projectSettings->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement() )
		{
			std::string elemString =  elem->Value();

			//<EnvVar>
			if ( elemString.compare( "EnvVar" ) == 0 )
			{
				ParseEnvVar( elem, m_EnvVar, includeFile );
			}
			//<Include>
			else if ( elemString.compare( "Include" ) == 0 )
			{
				ParseInclude( elem, m_IncludeFiles );
			}
			//<Config>
			else if ( elemString.compare( "Config" ) == 0 )
			{
				ParseConfig( elem, m_Configs, m_EnvVar );
			}
		}
	}

	doc.Clear();

	// We can't ProcessInculdeFiles here because part of the path may
	// have been defined in the with EvnVars in the EnvironmentVariableAlias,
	// so we can't ProcessValue on the include path or we will get the
	// defaults values."
	V_IncludeFiles::iterator includeFileItr = m_IncludeFiles.begin();
	V_IncludeFiles::iterator includeFileEnd = m_IncludeFiles.end();
	for ( ; includeFileItr != includeFileEnd; ++includeFileItr )
	{
		std::string includeFile = includeFileItr->m_Path;

		// This would be bad because it's going to use all the default values for env vars that have aliases
		ProcessValue( includeFile, m_EnvVar ); 

		if ( FileExists( includeFile ) && !LoadFile( includeFile, true ) )
		{
			return false;
		}
	}

	m_Title = "%ESHELL_TITLE%";
	ProcessValue( m_Title, m_EnvVar );

	return true;
}