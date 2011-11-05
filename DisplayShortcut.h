#pragma once

#include "DisplayBase.h"

namespace Launcher
{
	///////////////////////////////////////////////////////////////////////////////
	/// Class DisplayShortcut
	//
	// What does the command look like:
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\resistance\config\ProjectSettings.xml" -config production -exec Luna
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\rcf2\config\ProjectSettings.xml" -config production -run Maya
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\rcf2\config\ProjectSettings.xml" -config tools_builder -build "release" -code "devel" 
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "x:\core\config\ProjectSettings.xml" -config tools_builder -assets "rcf2" -game "rcf2" -build "develop" -code "projects/batfist" 
	//
	///////////////////////////////////////////////////////////////////////////////
	class DisplayShortcut;
	typedef std::vector< DisplayShortcut > V_DisplayShortcut;
	typedef std::map< std::string, V_DisplayShortcut > M_DisplayShortcuts;

	class DisplayShortcut : public DisplayBase
	{
	public:
		DisplayShortcut();
		DisplayShortcut( const DisplayShortcut& rhs );
		virtual ~DisplayShortcut();

		virtual bool Execute();
		virtual bool CopyToClipboard();

	public:
		std::string m_Name;             // Display name
		std::string m_FavoriteName;     // Display name for when the shortcut is in the favorites menu
		std::string m_Icon;             // Optional MenuIcon
		std::string m_Description;      // Mouse over description
		std::string m_Folder;           // Optional name of sub menu
		std::string m_Command;          // Command complete with Arguments
		std::string m_StartIn;          // Dir to start command from
		std::string m_Root;             // IG_ROOT
		std::string m_ProjectName;

		bool        m_Disable;           // true if we think the shortcut will not load
		std::string m_DisableReason;     // the reason this might fail
	};


	///////////////////////////////////////////////////////////////////////////////
	struct SortDisplayShortcuts
	{
		bool operator()( const DisplayShortcut& lhs, const DisplayShortcut& rhs ) const
		{
			return lhs.m_Name < rhs.m_Name;
		}
	};

}