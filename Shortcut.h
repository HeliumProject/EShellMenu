#pragma once

#include <wx/object.h>

namespace Launcher
{
	///////////////////////////////////////////////////////////////////////////////
	/// Class Shortcut
	//
	// What does the command look like:
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\Settings.xml" -config production -exec Luna
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\Settings.xml" -config production -run Maya
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\Settings.xml" -config tools_builder -build "release" -code "devel" 
	//
	///////////////////////////////////////////////////////////////////////////////
	class Shortcut;
	typedef std::vector< Shortcut > V_Shortcut;
	typedef std::map< std::string, V_Shortcut > M_Shortcuts;

	class Shortcut : public wxObjectRefData
	{
	public:
		Shortcut();
		Shortcut( const Shortcut& rhs );
		virtual ~Shortcut();

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
}