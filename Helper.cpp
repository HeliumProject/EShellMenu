#include "stdafx.h"
#include "Helper.h"

#include <io.h>
#include <assert.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <regex>

const static uint32_t MAX_PRINT_SIZE = 8192;

bool EShellMenu::FileExists(const tstring& fileName)
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

bool EShellMenu::DirectoryExists(const tstring& fileName)
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

bool EShellMenu::ExecuteCommand( const tstring& command, bool showWindow, bool block )
{
    STARTUPINFO si;
    memset( &si, 0, sizeof( si ) );
    si.lpReserved = wxT("");

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
        NULL,                                                 // Use parent's starting directory
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

tstring EShellMenu::Capitalize( const tstring& str, const bool isGameName )
{
    const tregex matchGameNamePattern( wxT("^.*[0-9]+.*$"), std::regex::icase );

    tstring newStr = str;

    if ( !newStr.empty() )
    {
        tsmatch matchResults; 
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

tstring EShellMenu::RemoveSlashes( const tstring& str, const tstring& replace )
{
    const tregex slash( wxT("[\\\\/]+") ); 
    return std::regex_replace( str, slash, replace );
}

// Returns true if the version of the specified file could be found and passes back the 
// version number.  The version number is a 64-bit number that is made up of four parts:
// Compatible/Feature/Patch/Build.  This app does not use the build number so it will
// always be zero.
bool EShellMenu::GetFileVersion( const tstring& path, uint64_t& version )
{
    bool succeeded = false;
    DWORD dwHandle = 0;

    if ( !FileExists( path ) )
    {
        return false;
    }

    DWORD size = GetFileVersionInfoSize( ( LPTSTR )path.c_str(), &dwHandle );

    if ( size > 0 )
    {
        tchar_t* versionBuf = new tchar_t[size];
        if ( GetFileVersionInfo( ( LPTSTR )path.c_str(), dwHandle, size, ( LPVOID )versionBuf ) )
        {
            LPVOID buffer = NULL;
            uint32_t length = 0;

            if ( VerQueryValue( ( LPVOID )versionBuf, wxT("\\"), &buffer, &length ) )
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

tstring EShellMenu::GetFileVersionString( uint64_t version )
{
    tstringstream versionStr;

    // Version number is 4 parts, but we are going to skip the build number.
    versionStr
        << ( uint16_t )( version >> ( 16 * 3 ) ) << "." 
        << ( uint16_t )( version >> ( 16 * 2 ) ) << "." 
        << ( uint16_t )( version >> ( 16 * 1 ) );

    return versionStr.str();
}

void EShellMenu::ConsolePrint(const tchar_t *fmt,...) 
{
    va_list args;
    va_start(args, fmt); 
    static tchar_t string[ MAX_PRINT_SIZE ];
    int size = _vsntprintf(string, sizeof(string), fmt, args);
    string[ ( sizeof(string) / sizeof(tchar_t) ) - 1 ] = 0; 
    assert(size >= 0);

    _ftprintf(stdout, wxT("%s"), string);
    fflush(stdout);

    va_end(args);      
}

bool EShellMenu::GetEnvVar( const tstring& envVarName, tstring& envVarValue )
{
    tchar_t envVarSetting[8192];
    if ( ::GetEnvironmentVariable( envVarName.c_str(), envVarSetting, sizeof(envVarSetting) / sizeof(tchar_t) ) )
    {
        envVarValue = envVarSetting;
        return true;
    }

    return false;
}

tstring EShellMenu::GetUserFile( const tstring& basename, const tstring& ext )
{
    wxFileName name ( wxFileName::GetHomeDir(), basename );
    name.AppendDir( ".eshell" );
    name.SetExt( ext );

    return tstring( name.GetFullPath().c_str() );
}

void EShellMenu::LoadTextFile( const tstring& file, std::set< tstring >& contents )
{
    tifstream in( file.c_str() );
    if ( !in.good() )
    {
        return;
    }

    tstring line;
    while( getline( in, line ) )
    {
        contents.insert( line );
    }

    in.close();
}

bool EShellMenu::SaveTextFile( const tstring& file, const std::set< tstring >& contents )
{
    wxFileName name ( file.c_str() );
    name.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

	tofstream out( file.c_str() );
    if ( !out.good() )
    {
        return false;
    }

    std::set< tstring >::const_iterator itr = contents.begin();
    std::set< tstring >::const_iterator end = contents.end();
    for( ; itr != end; ++itr )
    {
        out << (*itr) << "\n";
    }

    out.close();

    return true;
}