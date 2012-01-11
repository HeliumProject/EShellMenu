#pragma once

#include <wx/object.h>

namespace EShellMenu
{
	//
	// What does the command look like:
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\Project.xml" -config production -exec Luna
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\Project.xml" -config production -run Maya
	//   "C:\Program Files (x86)\eshell\eshell.pl" -settingsFile "X:\Project.xml" -config tools_builder -build "release" -code "devel" 
	//

	class MenuItem
	{
	public:
		MenuItem();
		virtual ~MenuItem();

		virtual bool Execute();
		virtual bool CopyToClipboard();

	public:
		tstring m_Name;             // Display name
		tstring m_FavoriteName;     // Display name for when the shortcut is in the favorites menu
		tstring m_Icon;             // Optional MenuIcon
		tstring m_Description;      // Mouse over description
		tstring m_Folder;           // Optional name of sub menu
		tstring m_Command;          // Command complete with Arguments

		bool m_Disable;				// true if we think the shortcut will not load
		tstring m_DisableReason;    // the reason this might fail
	};

	class MenuItemRefData : public wxObjectRefData
	{
	public:
		MenuItemRefData( MenuItem* item )
			: m_MenuItem( item )
		{

		}

		MenuItem* m_MenuItem;
	};
}