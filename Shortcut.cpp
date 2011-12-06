#include "stdafx.h"
#include "Shortcut.h"

#include "Helper.h"
#include "Settings.h"

using namespace Launcher;

Shortcut::Shortcut()
	: m_Disable( false )
	, m_DisableReason("")
{
}

Shortcut::~Shortcut()
{
}

bool Shortcut::Execute()
{
#ifdef _DEBUG
	if ( !Launcher::ExecuteCommand( m_Command ) ) 
#else
	if ( !Launcher::ExecuteCommand( m_Command, false ) ) 
#endif
	{
		std::string error = "Unable to create eshell, with command:\n  " + m_Command;
		wxMessageDialog dialog( NULL, wxT( error.c_str() ), wxT("Error"), wxOK | wxICON_INFORMATION );
		dialog.ShowModal();
		return false;
	}

	return true;
}

bool Shortcut::CopyToClipboard()
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