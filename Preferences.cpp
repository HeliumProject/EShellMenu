#include "stdafx.h"

#include "Preferences.h"
#include "ProjectSettings.h"
#include "Helper.h"

using namespace Launcher;

bool GetFavoritesPath( std::string& favoritesPath )
{
	if ( !Launcher::GetEnvVar( "USERPROFILE", favoritesPath ) )
	{
		return false;
	}

	favoritesPath += "/.eshell/favorites.txt";

	return true;
}

void Preferences::LoadFavorites( std::set< std::string >& favorites )
{
	std::string favoritesPath;
	if ( !GetFavoritesPath( favoritesPath ) )
	{
		return;
	}

	std::ifstream in( favoritesPath.c_str() );

	if ( !in.good() )
	{
		return;
	}

	std::string line;
	while( getline( in, line ) )
	{
		favorites.insert( line );
	}

	in.close();
}

bool Preferences::SaveFavorites( const std::set< std::string >& favorites )
{
	std::string favoritesPath;
	if ( !GetFavoritesPath( favoritesPath ) )
	{
		return false;
	}

	if ( !MakePath( favoritesPath, true ) )
	{
		return false;
	}

	std::ofstream out( favoritesPath.c_str() );

	if ( !out.good() )
	{
		return false;
	}

	std::set< std::string >::const_iterator itr = favorites.begin();
	std::set< std::string >::const_iterator end = favorites.end();
	for( ; itr != end; ++itr )
	{
		out << (*itr) << "\n";
	}

	out.close();

	return true;
}