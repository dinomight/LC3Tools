//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef FILEWINDOW_H
#define FILEWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Output.H>
#include <string>
#include <list>
#include "../Assembler/Base.h"
#include "Project.h"
#include "TextEditor.h"

using namespace std;

namespace AshIDE
{
	class FileWindow : public Fl_Double_Window
	{
	protected:
		//I made my own version of this class so that I can get the key and mouse
		//release events and selection change events  and update the status bar.
		//*NOTE: Some events are only sent to sub-widgets, so I can't intercept
		//them at the Window level.
		class File_Text_Editor : public TextEditor
		{
		public:
			File_Text_Editor(int, int, int, int, Fl_Text_Display::Style_Table_Entry *pStyleTable = 0, unsigned int nStyles = 0);
			virtual int handle(int);
			virtual void UpdateSelection();
		};
		friend class File_Text_Editor;

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		//The text editor object
		File_Text_Editor *pTextEditor;
		static Fl_Text_Display::Style_Table_Entry StyleTable[];
		string sFileNameFull;
		bool fLoading, fChanged;

		//Widgets for the status bar
		Fl_Output *pStatus;

		//Widgets for pop-up menu
		Fl_Menu_Button *pPopupMenu;
		static Fl_Menu_Item PopupItems[];

		//A list of all windows that are not child to the main window
		static list<Fl_Window *> WindowList;

		/**********************************************************************\
			~FileWindow( )

			Destructor
		\******/
		virtual ~FileWindow();

	public:
		/**********************************************************************\
			FileWindow( [in] width, [in] height, [in] file name,
				[in] inherited style (optional), [in] number of styles )

			Constructs the file window. It can either open an existing file
			or create a new file.

			An inherited class can pass its own menu and style table to the
			constructor.
		\******/
		FileWindow(int, int, const string &, Fl_Text_Display::Style_Table_Entry *pInheritedStyle = NULL, unsigned int InheritedStyleCount = 0);



	//*** Window Management Functions ***//
		/**********************************************************************\
			SaveAll( )

			Saves all open file windows.
		\******/
		static bool SaveAll();
		
		/**********************************************************************\
			CloseAll( )

			Closes all open file windows.
		\******/
		static bool CloseAll();
		
		/**********************************************************************\
			IsOpen( [in] file name )

			Checks to see if a file window is already open.
			Returns a pointe to the window if so, else NULL.
		\******/
		static FileWindow *IsOpen(const string &);

		/**********************************************************************\
			UnHighlight( )

			Unhighlights all open files.
		\******/
		static bool UnHighlight();

		/**********************************************************************\
			SelectLine( [in] line number, [in] true if select by highlight,
				[in] true if show window )

			Highlights the specified line by selecting it (2nd param false)
			or highlighting it (2nd param true)..
			(If you highlight it, the highlighting doesn't go away
			with a change in selection).
		\******/
		virtual bool SelectLine(unsigned int, bool fHighLight = false, bool fShow = true);

		/**********************************************************************\
			GetFileLine( [out] filename, [out] line number )

			Returns the filename of this file and the line number of the cursor
			location.
		\******/
		virtual bool GetFileLine(string &, unsigned int &) const;

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

		/**********************************************************************\
			handle( [in] event )

			All events pass through here.
		\******/
		virtual int handle(int);



	//*** File Management Functions ***//
		/**********************************************************************\
			Save( )

			Saves the file. If the file does not yet have a name, it performs
			SaveAs().
		\******/
		static void SaveCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->Save();	}
		virtual bool Save();

		/**********************************************************************\
			SaveAs( )

			Saves the file. Prompts the user for a file name.
		\******/
		static void SaveAsCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->SaveAs();	}
		virtual bool SaveAs();

		/**********************************************************************\
			Close( )
			CloseNow( )

			Sees if the user wants to save the file before closing the file.
			This also closes this window.
			CloseNow closes immediately, while Close uses the delete_widget()
			method which might not close it for a while.
		\******/
		static void CloseCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->Close();	}
		virtual bool Close();
		virtual bool CloseNow();

		/**********************************************************************\
			CheckSave( )

			Sees if the user wants to save the file before closing the file.
			Returns true if it is OK to close the file, else false.
		\******/
		virtual bool CheckSave();

		/**********************************************************************\
			Help( )

			Gets the identifier at the cursor position and opens up Help about
			that identifier.
		\******/
		static void HelpCB(Fl_Widget *pW, void *pV)	{	((FileWindow*)pV)->Help();	}
		virtual bool Help() const;



	//*** Projet Management Functions ***//
		/**********************************************************************\
			AddToProject( [in] ask )

			Adds this file to the current project.
			If ask is true, the user will be asked first if they want to add
			the file to the current project.
		\******/
		static void AddToProjectCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->AddToProject(false);	}
		virtual bool AddToProject(bool);

		/**********************************************************************\
			RemoveFromProject( [in] ask )

			Removes this file from the current project.
			If ask is true, the user will be asked first if they want to remove
			the file from the current project.
		\******/
		static void RemoveFromProjectCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->RemoveFromProject(false);	}
		virtual bool RemoveFromProject(bool);

		/**********************************************************************\
			SimulatorOptionsAll( )
			SimulatorOptions( )

			Checks if the simulator has been started or stopped, and if this
			file is being simulated.
			This allows updating of some menu choices for simulation.

			The *All() version will call the function for all open windows.
		\******/
		static bool SimulatorOptionsAll();
		virtual bool SimulatorOptions();

		/**********************************************************************\
			AssembleFile()

			Assembles the file.
		\******/
		static void AssembleFileCB(Fl_Widget *pW, void *pV)	{	TheProject.AssembleFile(((FileWindow *)pV)->sFileNameFull);	}



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


		
	//*** Title Management Functions ***//
		/**********************************************************************\
			SetTitle( )

			Sets the title to the filename. If text has changed, the filename
			is appended with (modified).
		\******/
		virtual bool SetTitle();

		/**********************************************************************\
			TextChanged( )

			Called when the text changes. Updates the (modified) marker
			on the filename display
		\******/
		static void TextChangedCB(int Pos, int nInserted, int nDeleted, int nRestyled, const char *pszDeleted, void* pThis)
		{	((FileWindow *)pThis)->TextChanged(Pos, nInserted, nDeleted, nRestyled, pszDeleted);	}
		virtual bool TextChanged(int, int, int, int, const char *);



	//*** Status Management Functions ***//
		/**********************************************************************\
			SetStatus( )

			Sets the status to the current cursor line and column numbers.
		\******/
		virtual bool SetStatus();



	//*** Text Editing Functions ***//
		static void CutCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->pTextEditor->Cut();	}
		static void CopyCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->pTextEditor->Copy();	}
		static void PasteCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->pTextEditor->Paste();	}
		static void DeleteCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->pTextEditor->Delete();	}
		static void FindCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->pTextEditor->Find();	}
		static void FindAgainCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->pTextEditor->FindAgain();	}
		static void ReplaceCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->pTextEditor->Replace();	}
		static void ReplaceAgainCB(Fl_Widget *pW, void *pV)	{	((FileWindow *)pV)->pTextEditor->ReplaceAgain();	}



	//*** Style Management Functions ***//
		/**********************************************************************\
			UpdateStyle( )

			Called when the text changes. Updates the syntax highlighting
			of the changed text.
		\******/
		static void UpdateStyleCB(int Pos, int nInserted, int nDeleted, int nRestyled, const char *pszDeleted, void *pV)
			{	((FileWindow *)pV)->UpdateStyle(Pos, nInserted, nDeleted, nRestyled, pszDeleted);	}
		virtual bool UpdateStyle(int, int, int, int, const char *);

		/**********************************************************************\
			ParseStyle( [in] updated text, [out] updated style )

			Parses the updated text and updates the style of that text
			character by character.
		\******/
		virtual bool ParseStyle(const string &, string &);
	};
}

#endif
