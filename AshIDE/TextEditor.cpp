//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "TextEditor.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>

namespace AshIDE	{

TextEditor::TextEditor(int X, int Y, int Width, int Height, Fl_Text_Display::Style_Table_Entry *pStyleTable, unsigned int nStyles) : ReadOnlyEditor(X, Y, Width, Height, pStyleTable, nStyles)
{
	{
	}
	end();

	//Setup the replace dialog
	//Make sure no windows are current otherwise the dialog will become embedded in that window
	Fl_Group *pCurrent = current();
	current(NULL);
	pReplaceDlg = new Fl_Window(300, 135, "Replace");
	{
		pReplaceFind = new Fl_Input(70, 10, 220, 25, "Find:");
		pReplaceWith = new Fl_Input(70, 40, 220, 25, "Replace:");
		pReplaceCase = new Fl_Check_Button(10, 70, 105, 25, "Match Case");

		pReplaceDirection = new Fl_Group(120, 70, 170, 25, "Search");
		{
			pReplaceDirection->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
			pReplaceDirection->box(FL_PLASTIC_DOWN_FRAME);

			pReplaceUp = new Fl_Round_Button(180, 70, 40, 25, "Up");
			pReplaceUp->type(FL_RADIO_BUTTON);

			pReplaceDown = new Fl_Round_Button(230, 70, 60, 25, "Down");
			pReplaceDown->type(FL_RADIO_BUTTON);
			pReplaceDown->value(1);
		}
		pReplaceDirection->end();

		pReplaceAll = new Fl_Button(10, 100, 90, 25, "Replace All");
		pReplaceAll->callback(ReplaceAllCB, this);

		pReplaceNext = new Fl_Return_Button(105, 100, 120, 25, "Replace Next");
		pReplaceNext->callback(ReplaceAgainCB, this);

		pReplaceCancel = new Fl_Button(230, 100, 60, 25, "Cancel");
		pReplaceCancel->callback(ReplaceCancelCB, this);
	}
	pReplaceDlg->end();
	pReplaceDlg->callback(ReplaceCancelCB, this);
	pReplaceDlg->set_non_modal();
	current(pCurrent);
}

TextEditor::~TextEditor()
{
	delete pReplaceDlg;
}

int TextEditor::handle(int Event)
{
	return Fl_Text_Editor::handle(Event);
}

bool TextEditor::Cut()
{
	kf_cut(0, this);
	return true;
}

bool TextEditor::Paste()
{
	kf_paste(0, this);
	return true;
}

bool TextEditor::Delete()
{
	kf_delete(0, this);
	return true;
}

bool TextEditor::Replace()
{
	pReplaceDlg->show();
	pReplaceFind->take_focus();
	return true;
}

bool TextEditor::ReplaceAgain()
{
	const char *sFind = pReplaceFind->value();
	const char *sReplace = pReplaceWith->value();

	if(sFind[0] == 0)
	{	// Search string is blank
		pReplaceDlg->show();
		pReplaceFind->take_focus();
		return false;
	}

	int Pos = insert_position();
	if(	(	pReplaceDown->value() ?
		pTextBuffer->search_forward(Pos, sFind, &Pos, pReplaceCase->value()) :
		pTextBuffer->search_backward(Pos, sFind, &Pos, pReplaceCase->value())	)	)
	{
		// Found a match; update the position and replace text...
		pTextBuffer->select(Pos, Pos+strlen(sFind));
		pTextBuffer->remove_selection();
		pTextBuffer->insert(Pos, sReplace);
		pTextBuffer->select(Pos, Pos+strlen(sReplace));
		insert_position(Pos + (pReplaceDown->value() ? strlen(sFind) : 0) );
		show_insert_position();
		UpdateSelection();
	}
	else
	{
		if(!fSupressAlert)
		{
//			if(!pReplaceDlg->shown())
//			{
				//If the dialog isn't shown, automatically wrap and peform one more search.
				insert_position((pFindDown->value() ? 0 : ((unsigned int)-1 >> 1)));
				fSupressAlert = true;
				bool fRetVal = ReplaceAgain();
				fSupressAlert = false;
				//If the search string still wasn't found, that means it doesn't exist at all
				if(!fRetVal)
					fl_message("'%s' not found.", sFind);
				return fRetVal;
//			}
//			fl_beep(FL_BEEP_ERROR);
//			if(fl_ask("'%s' not found!\nRestart from the %s?", sFind, (pFindDown->value() ? "beginning" : "end")))
//			{
//				insert_position((pReplaceDown->value() ? 0 : ((int)-1 >> 1)));
//				return ReplaceAgain();
//			}
		}
		return false;
	}

	return true;
}

bool TextEditor::ReplaceAll()
{
	insert_position(0);
	int Replaces = 0;

	fSupressAlert = true;
	while(ReplaceAgain())
		Replaces++;
	fSupressAlert = false;

	fl_message("Replaced %d occurrence%s", Replaces, Replaces == 1 ? "" : "s");

	return Replaces ? true : false;
}

bool TextEditor::ReplaceCancel()
{
	pReplaceDlg->hide();
	parent()->show();
	return true;
}

}	//namespace AshIDE
