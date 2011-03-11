//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef MEMORYBYTESWINDOW_H
#define MEMORYBYTESWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Choice.H>
#include <string>
#include "ReadOnlyEditor.h"

using namespace std;

namespace AshIDE
{
	class MemoryBytesWindow : public Fl_Double_Window
	{
	protected:
		friend class SimulatorWindow;

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		//Widgets for the address query
		Fl_Round_Button *pLineInput, *pDataInput, *pMemoryInput;
		Fl_Choice *pMemoryName;
		Fl_Input *pLineNumber, *pDataAddress, *pLength;
		Fl_Return_Button *pEnter;

		//Widgets for memory bytes view
		ReadOnlyEditor *pTextDisplay;
		//Stores the preferred console width for the display
		unsigned int MemoryBytesCW;

		//Stuff for syntax highlighting
		static Fl_Text_Display::Style_Table_Entry StyleTable[];

		//Widgets for the status bar
		Fl_Output *pStatus;

	public:
		/**********************************************************************\
			MemoryBytesWindow( )

			Constructs the memory bytes window.
			This window only displays text, there is no editing.
		\******/
		MemoryBytesWindow();

		/**********************************************************************\
			~MemoryBytesWindow( )

			Destructor
		\******/
		virtual ~MemoryBytesWindow();



		
	//*** Window Management Functions ***//
		static void HelpCB(Fl_Widget *pW, void *pV);

		/**********************************************************************\
			Close( )

			Closes the window and removes it from the SimulatorWindow's list.
		\******/
		static void CloseCB(Fl_Widget *pW, void *pV)	{	((MemoryBytesWindow *)pV)->Close();	}
		bool Close();


		/**********************************************************************\
			SetTitle( )

			Sets the title.
		\******/
		bool SetTitle();

		/**********************************************************************\
			resize( [in] x, [in] y, [in] width, [in] height )

			Called when the memorybytes window changes size.
		\******/
		virtual void resize(int, int, int, int);



	//*** Memory Bytes Management Functions ***//
		static void SaveCB(Fl_Widget *pW, void *pV)	{	((MemoryBytesWindow *)pV)->pTextDisplay->Save();	}
		static void CopyCB(Fl_Widget *pW, void *pV)	{	((MemoryBytesWindow *)pV)->pTextDisplay->Copy();	}
		static void FindCB(Fl_Widget *pW, void *pV)	{	((MemoryBytesWindow *)pV)->pTextDisplay->Find();	}
		static void FindAgainCB(Fl_Widget *pW, void *pV)	{	((MemoryBytesWindow *)pV)->pTextDisplay->FindAgain();	}

		/**********************************************************************\
			ChangeInput( )

			Changes whether it's inputting line number, logical data, or
			physical memory bytes.
		\******/
		static void ChangeInputCB(Fl_Widget *pW, void *pV)	{	((MemoryBytesWindow *)pV)->ChangeInput();	}
		bool ChangeInput();

		/**********************************************************************\
			Enter( )

			Displays the data per the parameters.
		\******/
		static void EnterCB(Fl_Widget *pW, void *pV)	{	((MemoryBytesWindow *)pV)->Enter();	}
		bool Enter();

		/**********************************************************************\
			ViewMemory( [in] program number, [in] identifier )

			Displays the specified memory bytes.
		\******/
		bool ViewMemory(unsigned int, const string &);



	//*** Style Management Functions ***//
		/**********************************************************************\
			UpdateStyle( )

			Called when the text changes. Updates the syntax highlighting
			of the changed text.
		\******/
		static void UpdateStyleCB(int Pos, int nInserted, int nDeleted, int nRestyled, const char *pszDeleted, void *pV)
			{	((MemoryBytesWindow *)pV)->UpdateStyle(Pos, nInserted, nDeleted, nRestyled, pszDeleted);	}
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
