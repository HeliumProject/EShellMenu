#pragma once

#include <wx/object.h>

namespace Launcher
{
	//
	// What does the command look like:
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\Settings.xml" -config production -exec Luna
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\Settings.xml" -config production -run Maya
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\Settings.xml" -config tools_builder -build "release" -code "devel" 
	//
	class Shortcut;
	typedef std::vector< wxObjectDataPtr< Shortcut > > V_Shortcut;
	typedef std::map< tstring, V_Shortcut > M_Shortcut;

	class Shortcut : public wxObjectRefData
	{
	public:
		Shortcut();
		virtual ~Shortcut();

		virtual bool Execute();
		virtual bool CopyToClipboard();

	public:
		tstring m_Name;             // Display name
		tstring m_FavoriteName;     // Display name for when the shortcut is in the favorites menu
		tstring m_Icon;             // Optional MenuIcon
		tstring m_Description;      // Mouse over description
		tstring m_Folder;           // Optional name of sub menu
		tstring m_Command;          // Command complete with Arguments

		bool        m_Disable;          // true if we think the shortcut will not load
		tstring m_DisableReason;    // the reason this might fail
	};
}