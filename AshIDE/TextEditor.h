//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#pragma warning (disable:4786)
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Round_Button.H>
#include <string>
#include "ReadOnlyEditor.h"

using namespace std;

namespace AshIDE
{
	class TextEditor : public ReadOnlyEditor
	{
	public:
		//Widgets for replace functions 
		Fl_Window *pReplaceDlg;
		Fl_Input *pReplaceFind;
		Fl_Input *pReplaceWith;
		Fl_Check_Button *pReplaceCase;
		Fl_Group *pReplaceDirection;
		Fl_Round_Button *pReplaceUp;
		Fl_Round_Button *pReplaceDown;
		Fl_Button *pReplaceAll;
		Fl_Return_Button *pReplaceNext;
		Fl_Button *pReplaceCancel;
		string sSearch;

	protected:
		/**********************************************************************\
			~TextEditor( )

			Destructor
		\******/
		virtual ~TextEditor();

	public:
		/**********************************************************************\
			TextEditor( [in] x, [in] y, [in] width, [in] height,
				[in] style table, [in] number of styles )

			Constructs the message window.
			This window only displays text, there is no editing.
		\******/
		TextEditor(int, int, int, int, Fl_Text_Display::Style_Table_Entry *pStyleTable = 0, unsigned int nStyles = 0);

		/**********************************************************************\
			handle( [in] event )

			All events pass through here.
		\******/
		virtual int handle(int);



	//*** Text Editing Functions ***//
		static void CutCB(Fl_Widget *pW, void *pV)	{	((TextEditor *)pV)->Cut();	}
		virtual bool Cut();
		static void PasteCB(Fl_Widget *pW, void *pV)	{	((TextEditor *)pV)->Paste();	}
		virtual bool Paste();
		static void DeleteCB(Fl_Widget *pW, void *pV)	{	((TextEditor *)pV)->Delete();	}
		virtual bool Delete();


	//*** Search Functions ***//
		static void ReplaceCB(Fl_Widget *pW, void *pV)	{	((TextEditor *)pV)->Replace();	}
		virtual bool Replace();
		static void ReplaceAgainCB(Fl_Widget *pW, void *pV)	{	((TextEditor *)pV)->ReplaceAgain();	}
		virtual bool ReplaceAgain();
		static void ReplaceAllCB(Fl_Widget *pW, void *pV)	{	((TextEditor *)pV)->ReplaceAll();	}
		virtual bool ReplaceAll();
		static void ReplaceCancelCB(Fl_Widget *pW, void *pV)	{	((TextEditor *)pV)->ReplaceCancel();	}
		virtual bool ReplaceCancel();

	};
}

#endif
