#include "stdafx.h"
#include "MenuItem.h"

#include "Helper.h"
#include "Project.h"

using namespace EShellMenu;

MenuItem::MenuItem()
	: m_Disable( false )
{
}

MenuItem::~MenuItem()
{
}

bool MenuItem::Execute()
{
#ifdef _DEBUG
	if ( !EShellMenu::ExecuteCommand( m_Command ) ) 
#else
	if ( !EShellMenu::ExecuteCommand( m_Command, false ) ) 
#endif
	{
		tstring error = wxT("Unable to create eshell, with command:\n  ") + m_Command;
		wxMessageDialog dialog( NULL, error.c_str(), wxT("Error"), wxOK | wxICON_INFORMATION );
		dialog.ShowModal();
		return false;
	}

	return true;
}

bool MenuItem::CopyToClipboard()
{
	bool result = false;

	if ( wxTheClipboard->Open() )
	{
		if ( wxTheClipboard->SetData( new wxTextDataObject( m_Command.c_str() ) ) )
		{
			result = true;
		}
		wxTheClipboard->Close();
	}

	return result;
}