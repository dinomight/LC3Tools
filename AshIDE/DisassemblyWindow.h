//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef DISASSEMBLYWINDOW_H
#define DISASSEMBLYWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Output.H>
#include <string>
#include <list>
#include "ReadOnlyEditor.h"

using namespace std;

namespace AshIDE
{
	class DisassemblyWindow : public Fl_Double_Window
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
		friend class ReadOnlyEditorX;

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		//Widgets for disassembly view
		ReadOnlyEditorX *pTextDisplay;

		//Stuff for syntax highlighting
		static Fl_Text_Display::Style_Table_Entry StyleTable[];

		//Widgets for the status bar
		Fl_Output *pStatus;

		//Widgets for pop-up menu
		Fl_Menu_Button *pPopupMenu;
		static Fl_Menu_Item PopupItems[];

		/**********************************************************************\
			Init( )

			Shows the console window and initializes the disassembly data.
		\******/
		bool Init();

	public:
		/**********************************************************************\
			DisassemblyWindow( )

			Constructs the disassembly window.
			This window only displays text, there is no editing.
		\******/
		DisassemblyWindow();

		/**********************************************************************\
			~DisassemblyWindow( )

			Destructor
		\******/
		virtual ~DisassemblyWindow();


		
	//*** Window Management Functions ***//
		static void HelpCB(Fl_Widget *pW, void *pV);

		/**********************************************************************\
			Close( )

			Closes the window and removes it from the SimulatorWindow.
		\******/
		static void CloseCB(Fl_Widget *pW, void *pV)	{	((DisassemblyWindow *)pV)->Close();	}
		bool Close();

		/**********************************************************************\
			SetTitle( )

			Sets the title.
		\******/
		bool SetTitle();

		/**********************************************************************\
			UnHighlight( )

			Unhighlights the disassembly window.
		\******/
		bool UnHighlight();

		/**********************************************************************\
			SelectLine( [in] line number, [in] true if select by highlight,
				[in] true if show window )

			Highlights the specified line by selecting it (2nd param false)
			or highlighting it (2nd param true)..
			(If you highlight it, the highlighting doesn't go away
			with a change in selection).
		\******/
		bool SelectLine(unsigned int, bool fHighLight = false, bool fShow = true);

		/**********************************************************************\
			GetFileLine( [out] filename, [out] line number )

			Returns the filename and line number of the message at the cursor
			location. Returns false if there is no file/line info at the cursor.
		\******/
		bool GetFileLine(string &, unsigned int &);

		/**********************************************************************\
			GetIdentifier( [out] identifier )

			Returns the identifier (data variable) at the cursor location
		\******/
		virtual bool GetIdentifier(string &) const;

		/**********************************************************************\
			GetRegister( [out] identifier )

			Returns the register name at the cursor location
		\******/
		virtual bool GetRegister(string &) const;



	//*** Disassembly Management Functions ***//
		static void ShowValuesCB(Fl_Widget *pW, void *pV)	{	((DisassemblyWindow *)pV)->ShowValues();	}
		bool ShowValues();
		static void SaveCB(Fl_Widget *pW, void *pV)	{	((DisassemblyWindow *)pV)->pTextDisplay->Save();	}
		static void CopyCB(Fl_Widget *pW, void *pV)	{	((DisassemblyWindow *)pV)->pTextDisplay->Copy();	}
		static void FindCB(Fl_Widget *pW, void *pV)	{	((DisassemblyWindow *)pV)->pTextDisplay->Find();	}
		static void FindAgainCB(Fl_Widget *pW, void *pV)	{	((DisassemblyWindow *)pV)->pTextDisplay->FindAgain();	}



	//*** Simulation Management Functions ***//
		static void RunToCursorCB(Fl_Widget *pW, void *pV);
		static void InstructionBreakpointCB(Fl_Widget *pW, void *pV);
		static void DataBreakpointCB(Fl_Widget *pW, void *pV);
		static void RegisterBreakpointCB(Fl_Widget *pW, void *pV);
		static void ViewInstructionCB(Fl_Widget *pW, void *pV);
		static void ViewDataCB(Fl_Widget *pW, void *pV);
		static void ViewMemoryCB(Fl_Widget *pW, void *pV);
		static void ViewRegisterCB(Fl_Widget *pW, void *pV);
		static void EditInstructionCB(Fl_Widget *pW, void *pV);
		static void EditDataCB(Fl_Widget *pW, void *pV);
		static void EditRegisterCB(Fl_Widget *pW, void *pV);



	//*** Style Management Functions ***//
		/**********************************************************************\
			UpdateStyle( )

			Called when the text changes. Updates the syntax highlighting
			of the changed text.
		\******/
		static void UpdateStyleCB(int Pos, int nInserted, int nDeleted, int nRestyled, const char *pszDeleted, void *pV)
			{	((DisassemblyWindow *)pV)->UpdateStyle(Pos, nInserted, nDeleted, nRestyled, pszDeleted);	}
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
