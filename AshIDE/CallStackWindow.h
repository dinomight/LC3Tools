//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef CALLSTACKWINDOW_H
#define CALLSTACKWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Output.H>
#include <string>
#include <list>
#include "ReadOnlyEditor.h"

using namespace std;

namespace AshIDE
{
	class CallStackWindow : public Fl_Double_Window
	{
	protected:
		friend class SimulatorWindow;

		class ReadOnlyEditorX : public ReadOnlyEditor
		{
		public:
			ReadOnlyEditorX(int, int, int, int, Fl_Text_Display::Style_Table_Entry *pStyleTable = 0, unsigned int nStyles = 0);
			virtual int handle(int);
			bool GetFileLine(string &, unsigned int &);
		};

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		//Widgets for CallStack view
		ReadOnlyEditorX *pTextDisplay;

		//Stuff for syntax highlighting
		static Fl_Text_Display::Style_Table_Entry StyleTable[];

		//Widgets for the status bar
		Fl_Output *pStatus;

	public:
		/**********************************************************************\
			CallStackWindow( )

			Constructs the CallStack window.
			This window only displays text, there is no editing.
		\******/
		CallStackWindow();

		/**********************************************************************\
			~CallStackWindow( )

			Destructor
		\******/
		virtual ~CallStackWindow();


		
	//*** Window Management Functions ***//
		static void HelpCB(Fl_Widget *pW, void *pV);

		/**********************************************************************\
			Close( )

			Closes the window and removes it from the SimulatorWindow.
		\******/
		static void CloseCB(Fl_Widget *pW, void *pV)	{	((CallStackWindow *)pV)->Close();	}
		bool Close();

		/**********************************************************************\
			SetTitle( )

			Sets the title.
		\******/
		bool SetTitle();

		/**********************************************************************\
			GetFileLine( [out] filename, [out] line number )

			Returns the filename and line number of the message at the cursor
			location. Returns false if there is no file/line info at the cursor.
		\******/
		bool GetFileLine(string &, unsigned int &);



	//*** Call Stack Management Functions ***//
		static void SaveCB(Fl_Widget *pW, void *pV)	{	((CallStackWindow *)pV)->pTextDisplay->Save();	}
		static void CopyCB(Fl_Widget *pW, void *pV)	{	((CallStackWindow *)pV)->pTextDisplay->Copy();	}
		static void FindCB(Fl_Widget *pW, void *pV)	{	((CallStackWindow *)pV)->pTextDisplay->Find();	}
		static void FindAgainCB(Fl_Widget *pW, void *pV)	{	((CallStackWindow *)pV)->pTextDisplay->FindAgain();	}

		/**********************************************************************\
			Enter( )

			Displays the call stack.
		\******/
		bool Enter();



	//*** Style Management Functions ***//
		/**********************************************************************\
			UpdateStyle( )

			Called when the text changes. Updates the syntax highlighting
			of the changed text.
		\******/
		static void UpdateStyleCB(int Pos, int nInserted, int nDeleted, int nRestyled, const char *pszDeleted, void *pV)
			{	((CallStackWindow *)pV)->UpdateStyle(Pos, nInserted, nDeleted, nRestyled, pszDeleted);	}
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
