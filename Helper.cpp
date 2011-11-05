#include "stdafx.h"
#include "Helper.h"

#include <io.h>
#include <assert.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <regex>

const static uint32_t MAX_PRINT_SIZE = 8192;

bool Launcher::FileExists(const std::string& fileName)
{
	DWORD fileAttr;
	fileAttr = GetFileAttributes(fileName.c_str());
	switch( fileAttr )
	{
	case 0xFFFFFFFF:
		return false;

	case FILE_ATTRIBUTE_DIRECTORY:
		return false;

	default: // its a file
		return true;
	}
}

bool Launcher::DirectoryExists(const std::string& fileName)
{
	DWORD fileAttr;
	fileAttr = GetFileAttributes(fileName.c_str());
	switch( fileAttr )
	{
	case 0xFFFFFFFF:
		return false;

	case FILE_ATTRIBUTE_DIRECTORY:
		return true;

	default: // its a file
		return false;
	}
}

bool Launcher::MakePath( const std::string& fileName, bool stripFile )
{
	std::string path = fileName;

	if ( stripFile )
	{
		size_t pos = std::string::npos;
		pos = path.find_last_of( "\\/" );
		path = path.substr( 0, pos-1 );
	}

	assert( false ); // need a loop here to mkpath
	_mkdir( path.c_str() );

	return DirectoryExists( path );
}

bool Launcher::ExecuteCommand( const std::string& command, const std::string& startIn, bool showWindow, bool block )
{
	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	si.lpReserved = "";

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	bool success = true;
	if ( !::CreateProcess(
		NULL,                                                 // No module name (use command line)
		(LPTSTR) command.c_str(),                             // Command line
		NULL,                                                 // Process handle not inheritable
		NULL,                                                 // Thread handle not inheritable
		FALSE,                                                // Set handle inheritance to FALSE
		showWindow ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW,   // Creation flags
		NULL,                                                 // Use parent's environment block
		startIn.empty() ? NULL : (LPTSTR) startIn.c_str(),    // Use parent's starting directory
		&si,                                                  // Pointer to STARTUPINFO structure
		&pi ) )                                               // Pointer to PROCESS_INFORMATION structure
	{
		success = false;
	}

	if ( block )
	{
		DWORD waitResult = ::WaitForSingleObject( pi.hProcess, INFINITE );

		DWORD error = 0x00;
		BOOL getError = ::GetExitCodeProcess( pi.hProcess, &error );

		if ( success && getError && error != 0x00 )
		{
			success = false;
		}
	}

	::CloseHandle( pi.hProcess );
	::CloseHandle( pi.hThread );

	return success;
}


///////////////////////////////////////////////////////////////////////////////
std::string Launcher::Capitalize( const std::string& str, const bool isGameName )
{
	const std::regex matchGameNamePattern( "^.*[0-9]+.*$", std::regex::icase );

	std::string newStr = str;

	if ( !newStr.empty() )
	{
		std::smatch matchResults; 
		if ( !isGameName || !std::regex_match( str, matchResults, matchGameNamePattern ) || newStr.length() > 4 )
		{
			// capitolize the first letter
			std::transform( newStr.begin(), newStr.begin() + 1, newStr.begin(), toupper);
		}
		else
		{
			// capitolize the whole string
			std::transform( newStr.begin(), newStr.end(), newStr.begin(), toupper);
		}
	}
	return newStr;
}

///////////////////////////////////////////////////////////////////////////////
std::string Launcher::RemoveSlashes( const std::string& str, const std::string& replace )
{
	const std::regex slash("[\\\\/]+"); 
	return std::regex_replace( str, slash, replace );
}



///////////////////////////////////////////////////////////////////////////////
// Returns true if the version of the specified file could be found and passes back the 
// version number.  The version number is a 64-bit number that is made up of four parts:
// Compatible/Feature/Patch/Build.  This app does not use the build number so it will
// always be zero.
bool Launcher::GetFileVersion( const std::string& path, uint64_t& version )
{
	bool succeeded = false;
	DWORD dwHandle = 0;
	DWORD size = GetFileVersionInfoSize( ( LPTSTR )path.c_str(), &dwHandle );

	if ( size > 0 )
	{
		unsigned char* versionBuf = new unsigned char[size];
		if ( GetFileVersionInfo( ( LPTSTR )path.c_str(), dwHandle, size, ( LPVOID )versionBuf ) )
		{
			LPVOID buffer = NULL;
			uint32_t length = 0;

			if ( VerQueryValue( ( LPVOID )versionBuf, "\\", &buffer, &length ) )
			{
				VS_FIXEDFILEINFO* fileInfo = ( VS_FIXEDFILEINFO* )buffer;
				version = ( ( ( uint64_t )fileInfo->dwFileVersionMS << 32 ) | fileInfo->dwFileVersionLS );
				succeeded = true;
			}
		}
		delete [] versionBuf;
	}

	return succeeded;
}

///////////////////////////////////////////////////////////////////////////////
std::string Launcher::GetFileVersionString( const std::string& path )
{
	std::stringstream versionStr;

	// Version number is 4 parts, but we are going to skip the build number.
	uint64_t version = 0;
	if ( GetFileVersion( path, version ) )
	{
		versionStr 
			<< ( uint16_t )( version >> ( 16 * 3 ) ) << "." 
			<< ( uint16_t )( version >> ( 16 * 2 ) ) << "." 
			<< ( uint16_t )( version >> ( 16 * 1 ) );

	}
	return versionStr.str();
}

///////////////////////////////////////////////////////////////////////////////
void Launcher::ConsolePrint(const char *fmt,...) 
{
	va_list args;
	va_start(args, fmt); 
	static char string[MAX_PRINT_SIZE];
	int size = _vsnprintf(string, sizeof(string), fmt, args);
	string[ sizeof(string) - 1] = 0; 
	assert(size >= 0);

	fprintf(stdout, "%s", string);
	fflush(stdout);

	va_end(args);      
}

///////////////////////////////////////////////////////////////////////////////
bool Launcher::GetEnvVar( const std::string& envVarName, std::string& envVarValue )
{
	char *envVarSetting = getenv( envVarName.c_str() );

	if ( envVarSetting )
	{
		envVarValue = envVarSetting;
		return true;
	}

	return false;
}
