#pragma once

#include <stdint.h>

namespace Launcher
{
	namespace Preferences
	{
		void LoadFavorites( std::set< std::string >& favorites );
		bool SaveFavorites( const std::set< std::string >& favorites );
	}
}