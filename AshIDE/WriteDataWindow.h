//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef WRITEDATAWINDOW_H
#define WRITEDATAWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include "TextEditor.h"
#include <string>

using namespace std;

namespace AshIDE
{
	class WriteDataWindow : public Fl_Double_Window
	{
	protected:
		friend class SimulatorWindow;

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		//Widgets for the address query
		Fl_Round_Button *pLineInput, *pDataInput, *pMemoryInput;
		Fl_Choice *pMemoryName;
		Fl_Input *pLineNumber, *pDataAddress;
		Fl_Return_Button *pEnter;

		//Widgets for WriteData view
		TextEditor *pTextEditor;

		//Stuff for syntax highlighting
		static Fl_Text_Display::Style_Table_Entry StyleTable[];

		//Widgets for the status bar
		Fl_Output *pStatus;

	public:
		/**********************************************************************\
			WriteDataWindow( )

			Constructs the WriteData window.
			This window only displays text, there is no editing.
		\******/
		WriteDataWindow();

		/**********************************************************************\
			~WriteDataWindow( )

			Destructor
		\******/
		virtual ~WriteDataWindow();

		/**********************************************************************\
			handle( [in] event )

			All events pass through here.
		\******/
		virtual int handle(int);



		
	//*** Window Management Functions ***//
		static void HelpCB(Fl_Widget *pW, void *pV);

		/**********************************************************************\
			Close( )

			Closes the window and removes it from the SimulatorWindow's list.
		\******/
		static void CloseCB(Fl_Widget *pW, void *pV)	{	((WriteDataWindow *)pV)->Close();	}
		bool Close();

		/**********************************************************************\
			SetTitle( )

			Sets the title.
		\******/
		bool SetTitle();



	//*** Data Values Management Functions ***//
		static void SaveCB(Fl_Widget *pW, void *pV)	{	((WriteDataWindow *)pV)->pTextEditor->Save();	}
		static void LoadCB(Fl_Widget *pW, void *pV)	{	((WriteDataWindow *)pV)->pTextEditor->Load();	}
		static void CopyCB(Fl_Widget *pW, void *pV)	{	((WriteDataWindow *)pV)->pTextEditor->Copy();	}
		static void FindCB(Fl_Widget *pW, void *pV)	{	((WriteDataWindow *)pV)->pTextEditor->Find();	}
		static void FindAgainCB(Fl_Widget *pW, void *pV)	{	((WriteDataWindow *)pV)->pTextEditor->FindAgain();	}

		/**********************************************************************\
			ChangeInput( )

			Changes whether it's inputting line number, logical data, or
			physical memory bytes.
		\******/
		static void ChangeInputCB(Fl_Widget *pW, void *pV)	{	((WriteDataWindow *)pV)->ChangeInput();	}
		bool ChangeInput();

		/**********************************************************************\
			Enter( )

			Displays the data per the parameters.
		\******/
		static void EnterCB(Fl_Widget *pW, void *pV)	{	((WriteDataWindow *)pV)->Enter();	}
		bool Enter();

		/**********************************************************************\
			EditInstruction( [in] program number, [in] line number )

			Edits the specified instruction.
		\******/
		bool EditInstruction(unsigned int, unsigned int);

		/**********************************************************************\
			EditData( [in] program number, [in] identifier )

			Edits the specified data.
		\******/
		bool EditData(unsigned int, const string &);



	//*** Style Management Functions ***//
		/**********************************************************************\
			UpdateStyle( )

			Called when the text changes. Updates the syntax highlighting
			of the changed text.
		\******/
		static void UpdateStyleCB(int Pos, int nInserted, int nDeleted, int nRestyled, const char *pszDeleted, void *pV)
			{	((WriteDataWindow *)pV)->UpdateStyle(Pos, nInserted, nDeleted, nRestyled, pszDeleted);	}
		bool UpdateStyle(int, int, int, int, const char *);

		/**********************************************************************\
			ParseStyle( [in] updated text, [out] updated style )

			Parses the updated text and updates the style of that text
			character by character.
		\******/
		virtual bool ParseStyle(const string &, string &);
	};
}

#endif
