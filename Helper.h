#pragma once

#include <stdint.h>

namespace EShellMenu
{
	bool FileExists( const tstring& fileName );
	bool DirectoryExists( const tstring& fileName );
	bool ExecuteCommand( const tstring& command, bool showWindow = true, bool block = false );

	tstring Capitalize( const tstring& str, const bool isGameName = false );
	tstring RemoveSlashes( const tstring& str, const tstring& replace = tstring( wxT(" ") ) );

	bool GetFileVersion( const tstring& path, uint64_t& version );
	tstring GetFileVersionString( uint64_t version );

	void ConsolePrint(const tchar_t *fmt,...);

	bool GetEnvVar( const tstring& envVarName, tstring& envVarValue );

	tstring GetUserFile( const tstring& basename, const tstring& ext );
	void LoadTextFile( const tstring& file, std::set< tstring >& contents );
	bool SaveTextFile( const tstring& file, const std::set< tstring >& contents );
}
