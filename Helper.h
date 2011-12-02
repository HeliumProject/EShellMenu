#pragma once

#include <stdint.h>

namespace Launcher
{
	bool FileExists( const std::string& fileName );
	bool DirectoryExists( const std::string& fileName );
	bool ExecuteCommand( const std::string& command, const std::string& startIn = std::string(""), bool showWindow = true, bool block = false );

	std::string Capitalize( const std::string& str, const bool isGameName = false );
	std::string RemoveSlashes( const std::string& str, const std::string& replace = std::string(" ") );

	bool GetFileVersion( const std::string& path, uint64_t& version );
	std::string GetFileVersionString( uint64_t version );

	void ConsolePrint(const char *fmt,...);

	bool GetEnvVar( const std::string& envVarName, std::string& envVarValue );

	std::string GetUserFile( const std::string& basename, const std::string& ext );
	void LoadTextFile( const std::string& file, std::set< std::string >& contents );
	bool SaveTextFile( const std::string& file, const std::set< std::string >& contents );
}
