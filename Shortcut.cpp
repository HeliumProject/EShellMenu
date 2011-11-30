#include "stdafx.h"
#include "Shortcut.h"

#include "Config.h"
#include "Helper.h"
#include "Settings.h"

using namespace Launcher;

Shortcut::Shortcut()
	: m_Disable( false )
	, m_DisableReason("")
{
}

Shortcut::Shortcut( const Shortcut& rhs )
{
	m_Name = rhs.m_Name;
	m_FavoriteName = rhs.m_FavoriteName;
	m_Icon = rhs.m_Icon;       
	m_Description = rhs.m_Description;
	m_Folder = rhs.m_Folder;     
	m_Command = rhs.m_Command;    
	m_StartIn = rhs.m_StartIn;
	m_Root = rhs.m_Root;
	m_ProjectName = rhs.m_ProjectName;
	m_Disable = rhs.m_Disable;
	m_DisableReason = rhs.m_DisableReason;
}

Shortcut::~Shortcut()
{
}

bool Shortcut::Execute()
{
	if ( !Launcher::ExecuteCommand( m_Command, m_StartIn ) ) 
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