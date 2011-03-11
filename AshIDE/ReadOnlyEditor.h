//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef READONLYEDITOR_H
#define READONLYEDITOR_H

#pragma warning (disable:4786)
#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Round_Button.H>

namespace AshIDE
{
	//*NOTE: This should inherit from Fl_Text_Display as this is a display-only message window.
	//However, Fl_Text_Display has a bug where forced selection does not get redrawn correctly.
	//It only works with a Fl_Text_Editor. To force the Text_Editor to be read only, I prevent
	//all keyboard messages from getting through.
	class ReadOnlyEditor : public Fl_Text_Editor
	{
	public:
		//Widgets for find functions 
		Fl_Window *pFindDlg;
		Fl_Input *pFindFind;
		Fl_Check_Button *pFindCase;
		Fl_Group *pFindDirection;
		Fl_Round_Button *pFindUp;
		Fl_Round_Button *pFindDown;
		Fl_Return_Button *pFindNext;
		Fl_Button *pFindCancel;
		bool fSupressAlert;

		//Holds the actual file text and its style
		Fl_Text_Buffer *pTextBuffer;
		Fl_Text_Buffer *pStyleBuffer;

	protected:
		/**********************************************************************\
			~ReadOnlyEditor( )

			Destructor
		\******/
		virtual ~ReadOnlyEditor();

	public:
		/**********************************************************************\
			ReadOnlyEditor( [in] x, [in] y, [in] width, [in] height,
				[in] style table, [in] number of styles )

			Constructs the message window.
			This window only displays text, there is no editing.
		\******/
		ReadOnlyEditor(int, int, int, int, Fl_Text_Display::Style_Table_Entry *pStyleTable = 0, unsigned int nStyles = 0);

		/**********************************************************************\
			handle( [in] event )

			All events pass through here.
		\******/
		virtual int handle(int);



	//*** Text Management Functions ***//
		/**********************************************************************\
			Save( )

			Saves the to file. Always prompts for a file to save to.
		\******/
		virtual bool Save();

		/**********************************************************************\
			Save( )

			Loads a file.
		\******/
		virtual bool Load();

		/**********************************************************************\
			Copy( )

			Copies selected text to the clipboard.
		\******/
		virtual bool Copy();

		/**********************************************************************\
			Clear()

			Clears the text from the display.
		\******/
		virtual bool Clear();



	//*** Search Functions ***//
		static void FindCB(Fl_Widget *pW, void *pV)	{	((ReadOnlyEditor *)pV)->Find();	}
		virtual bool Find();
		static void FindAgainCB(Fl_Widget *pW, void *pV)	{	((ReadOnlyEditor *)pV)->FindAgain();	}
		virtual bool FindAgain();
		static void FindCancelCB(Fl_Widget *pW, void *pV)	{	((ReadOnlyEditor *)pV)->FindCancel();	}
		virtual bool FindCancel();
		//This function is called after the selection has changed due to a find/replace function.
		//Override it to do something.
		virtual void UpdateSelection()	{	};



	//*** Style Management Functions ***//
		/**********************************************************************\
			UnfinishedStyleCB( )

			This callback is not used.
		\******/
		static void UnfinishedStyleCB(int, void*) {	}
	};
}

#endif
