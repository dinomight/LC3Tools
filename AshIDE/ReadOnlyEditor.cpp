//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "ReadOnlyEditor.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <fstream>

using namespace std;

namespace AshIDE	{

ReadOnlyEditor::ReadOnlyEditor(int X, int Y, int Width, int Height, Fl_Text_Display::Style_Table_Entry *pStyleTable, unsigned int nStyles) : Fl_Text_Editor(X, Y, Width, Height)
{
	{
//		box(FL_PLASTIC_DOWN_FRAME);
		//Setup the texteditor to use the textbuffer and stylebuffer
		pTextBuffer = new Fl_Text_Buffer();
		buffer(pTextBuffer);
		pTextBuffer->tab_distance(4);
		textfont(FL_COURIER);
		if(pStyleTable)
		{
			pStyleBuffer = new Fl_Text_Buffer();
			highlight_data(pStyleBuffer, pStyleTable, nStyles, 'A', UnfinishedStyleCB, this);
		}
		else
		{
			pStyleBuffer = NULL;
		}
	}
	end();

	//Setup the find dialog
	//Make sure no windows are current otherwise the dialog will become embedded in that window
	Fl_Group *pCurrent = current();
	current(NULL);
	pFindDlg = new Fl_Window(300, 105, "Find");
	{
		pFindFind = new Fl_Input(70, 10, 220, 25, "Find:");
		pFindCase = new Fl_Check_Button(10, 40, 105, 25, "Match Case");

		pFindDirection = new Fl_Group(120, 40, 170, 25, "Search");
		{
			pFindDirection->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
			pFindDirection->box(FL_PLASTIC_DOWN_FRAME);

			pFindUp = new Fl_Round_Button(180, 40, 40, 25, "Up");
			pFindUp->type(FL_RADIO_BUTTON);

			pFindDown = new Fl_Round_Button(230, 40, 60, 25, "Down");
			pFindDown->type(FL_RADIO_BUTTON);
			pFindDown->value(1);
		}
		pFindDirection->end();

		pFindNext = new Fl_Return_Button(120, 70, 105, 25, "Find Next");
		pFindNext->callback(FindAgainCB, this);

		pFindCancel = new Fl_Button(230, 70, 60, 25, "Cancel");
		pFindCancel->callback(FindCancelCB, this);
	}
	pFindDlg->end();
	pFindDlg->callback(FindCancelCB, this);
	pFindDlg->set_non_modal();
	current(pCurrent);
	fSupressAlert = false;
}

ReadOnlyEditor::~ReadOnlyEditor()
{
	//*NOTE: TextDisplay doesn't delete its text buffer,
	buffer(NULL);
	delete pTextBuffer;	
	if(pStyleBuffer)
		delete pStyleBuffer;
	delete pFindDlg;
}

int ReadOnlyEditor::handle(int Event)
{
	switch(Event)
	{
	case FL_KEYBOARD:
		return 0;
	}
	return Fl_Text_Editor::handle(Event);
}

bool ReadOnlyEditor::Save()
{
	char *sNewFile;
	do
	{
		sNewFile = fl_file_chooser("Save File As?", NULL, "");
		if(sNewFile == NULL)
			return false;
		ifstream TestNew(sNewFile);
		if(!TestNew.good())
			break;
		fl_ask("File already exists, overwrite '%s'?", sNewFile);
	}
	while(true);

	if(pTextBuffer->savefile(sNewFile))
	{
		fl_alert("Error writing to file '%s'", sNewFile);
		return false;
	}
	return true;
}

bool ReadOnlyEditor::Load()
{
	char *sNewFile = fl_file_chooser("Load File?", NULL, "");
	if(sNewFile == NULL)
		return false;
	if(pTextBuffer->loadfile(sNewFile))
	{
		fl_alert("Error reading from file '%s'", sNewFile);
		return false;
	}
	return true;
}

bool ReadOnlyEditor::Copy()
{
	kf_copy(0, this);
	return true;
}

bool ReadOnlyEditor::Clear()
{
	pTextBuffer->remove(0, pTextBuffer->length());
	pStyleBuffer->remove(0, pStyleBuffer->length());
	return true;
}

bool ReadOnlyEditor::Find()
{
	pFindDlg->show();
	pFindFind->take_focus();
	return true;
}

bool ReadOnlyEditor::FindAgain()
{
	const char *sFind = pFindFind->value();

	if(sFind[0] == 0)
	{	// Search string is blank
		pFindDlg->show();
		pFindFind->take_focus();
		return false;
	}

	int Pos = insert_position();
	if(	(	pFindDown->value() ?
		pTextBuffer->search_forward(Pos, sFind, &Pos, pFindCase->value()) :
		pTextBuffer->search_backward(Pos, sFind, &Pos, pFindCase->value())	)	)
	{
		// Found a match; select and update the position...
		pTextBuffer->select(Pos, Pos + strlen(sFind));
		
		insert_position(Pos + (pFindDown->value() ? strlen(sFind) : 0) );
		show_insert_position();
		UpdateSelection();
	}
	else
	{
		if(!fSupressAlert)
		{
//			if(!pFindDlg->shown())
//			{
				//If the dialog isn't shown, automatically wrap and peform one more search.
				insert_position((pFindDown->value() ? 0 : ((unsigned int)-1 >> 1)));
				fSupressAlert = true;
				bool fRetVal = FindAgain();
				fSupressAlert = false;
				//If the search string still wasn't found, that means it doesn't exist at all
				if(!fRetVal)
					fl_message("'%s' not found.", sFind);
				return fRetVal;
//			}
//			fl_beep(FL_BEEP_ERROR);
//			if(fl_ask("'%s' not found!\nRestart from the %s?", sFind, (pFindDown->value() ? "beginning" : "end")))
//			{
//				insert_position((pFindDown->value() ? 0 : ((unsigned int)-1 >> 1)));
//				return FindAgain();
//			}
		}
		return false;
	}

	return true;
}

bool ReadOnlyEditor::FindCancel()
{
	pFindDlg->hide();
	parent()->show();
	return true;
}

}	//namespace AshIDE
