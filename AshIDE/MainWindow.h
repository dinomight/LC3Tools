//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Help_Dialog.H>
#include <string>
#include "../Assembler/Base.h"
#include "MessageWindow.h"
#include "FilesWindow.h"
#include "SettingsWindow.h"
#include "SimulatorWindow.h"
#include "Project.h"

using namespace std;

namespace AshIDE
{

	//*NOTE: FLTK text input widgets all used to pre-process these UNIX control keys
	//This has since been remedied, so this #define isn't needed anymore
#define NO_STUPID_UNIX_COMMANDS\
	switch(Event)\
	{\
	case FL_KEYBOARD:\
		switch(Fl::event_key())\
		{\
		case 'f':\
		case 'b':\
		case 'p':\
		case 'n':\
		case 'a':\
		case 'e':\
		case 'd':\
		case 'k':\
		case 'u':\
		case 'y':\
		case 'w':\
			if(Fl::event_state(FL_CTRL))\
				return 0;\
		}\
	}

	enum HelpEnum {NoIndex, AssemblerIndex, LC3InstructionIndex, LC3bInstructionIndex, SimulatorIndex, AshIDEIndex};

	class MainWindow : public Fl_Double_Window
	{
	protected:

		//Window sizes
		static unsigned int MainWidth, MainHeight, FileWidth, FileHeight, MenuHeight, FilesWidth, SettingsHeight;

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		/**********************************************************************\
			~MainWindow( )

			Destructor.
			Exit should have been called before the destructor.
		\******/
		~MainWindow();

	public:
		//The one and only instance of the main window
		#define TheMainWindow	(*MainWindow::pMainWindow)
		static MainWindow *pMainWindow;
		FilesWindow *pFilesWindow;
		SettingsWindow *pSettingsWindow;
		MessageWindow *pMessageWindow;

		//Widgets for the help display
		Fl_Help_Dialog *pHelpDlg;
		FileName sHelpLocation;

		/**********************************************************************\
			MainWindow( )

			Constructs the main IDE window
		\******/
		MainWindow();



	//*** Program/Main-Window Management Functions ***//
		static void ExitCB(Fl_Widget *pW, void *pV)	{	pMainWindow->Exit();	}
		bool Exit();

		/**********************************************************************\
			SetTitle( )

			Sets the title to the project name.
		\******/
		bool SetTitle();



	//*** Project Management Functions ***//
		static void NewProjectCB(Fl_Widget *pW, void *pV)	{	pMainWindow->NewProject();	}
		bool NewProject();
		static void OpenProjectCB(Fl_Widget *pW, void *pV)	{	pMainWindow->OpenProject();	}
		bool OpenProject();
		static void SaveProjectCB(Fl_Widget *pW, void *pV)	{	pMainWindow->SaveProject();	}
		bool SaveProject();
		static void SaveProjectAsCB(Fl_Widget *pW, void *pV)	{	pMainWindow->SaveProjectAs();	}
		bool SaveProjectAs();
		static void CloseProjectCB(Fl_Widget *pW, void *pV)	{	pMainWindow->CloseProject();	}
		bool CloseProject();

		/**********************************************************************\
			OpenProject( )

			Opens a project file.
		\******/
		bool OpenProject(const string &);

		/**********************************************************************\
			CheckSaveProject( [in] file name )

			Sees if the user wants to save the project before closing it.
			Returns true if it is OK to close the project, else false.

			Auto projects are not checked.
		\******/
		bool CheckSaveProject();

		/**********************************************************************\
			RemoveFromProject( )

			Remoes the currently selected file from the project.
		\******/
		static void RemoveFromProjectCB(Fl_Widget *pW, void *pV)	{	pMainWindow->RemoveFromProject();	}
		bool RemoveFromProject();


	//*** File Management Functions ***//
		static void NewFileCB(Fl_Widget *pW, void *pV)	{	pMainWindow->NewFile();	}
		bool NewFile();
		static void AddFileCB(Fl_Widget *pW, void *pV)	{	pMainWindow->AddFile();	}
		bool AddFile();
		static void OpenFileCB(Fl_Widget *pW, void *pV)	{	pMainWindow->OpenFile();	}
		bool OpenFile();

		/**********************************************************************\
			OpenFile( [in] file name )

			Opens a file.

			If a project is not already open, it creates a default auto-project.
			If the current project is an auto-project, the file is auto-inserted
			into the project. Otherwise, a dialog asks the user if they want
			to insert the file into the project.
		\******/
		bool OpenFile(const string &);



	//*** Project Files Management Functions ***//
		/**********************************************************************\
			SelectFile( )

			Tells the window that the file selection has changed in the project
			files window. It updates the file settings and menu choices as
			appropriate.
		\******/
		bool SelectFile();



	//*** Project Settings Management Functions ***//
		static void AssembleFileCB(Fl_Widget *pW, void *pV)	{	TheProject.AssembleFile(TheProject.pFile);	}
		static void AssembleCB(Fl_Widget *pW, void *pV)	{	TheProject.Assemble();	}
		static void BuildCB(Fl_Widget *pW, void *pV)	{	TheProject.Build();	}
		static void RebuildCB(Fl_Widget *pW, void *pV)	{	if(TheProject.ResetAssembler())	TheProject.Build();	}
		static void SimulateCB(Fl_Widget *pW, void *pV)	{	pMainWindow->Simulate();	}
		bool Simulate();



	//*** Message Management Functions ***//
		static void SaveMessagesCB(Fl_Widget *pW, void *pV)	{	pMainWindow->pMessageWindow->Save();	}
		static void CopyMessagesCB(Fl_Widget *pW, void *pV)	{	pMainWindow->pMessageWindow->Copy();	}
		static void FindMessagesCB(Fl_Widget *pW, void *pV)	{	pMainWindow->pMessageWindow->Find();	}
		static void FindMessagesAgainCB(Fl_Widget *pW, void *pV)	{	pMainWindow->pMessageWindow->FindAgain();	}

		//Lexing and parsing functions use this callback to display information and error messages.
		static bool MessageCallBack(MessageEnum MessageType, const string &sMessage, const LocationVector &LocationStack)
		{	return pMainWindow->pMessageWindow->Message(MessageType, sMessage, LocationStack);	}
		//Simulation functions use this callback to display information and error messages.
		static bool SimMessageCallBack(MessageEnum MessageType, const string &sMessage, const LocationVector &LocationStack)
		{	return TheSimulatorWindow.SimMessage(MessageType, sMessage);	}
		static bool SimCallBack(MessageEnum MessageType, const string &sMessage)
		{	return TheSimulatorWindow.SimMessage(MessageType, sMessage);	}
		static bool SimCommand(string &)
		{	return true;	}
		static bool SimReadConsole(string &sBuffer, unsigned int CharsToRead, unsigned int &CharsRead)
		{	return TheSimulatorWindow.pConsoleWindow->SimReadConsole(sBuffer, CharsToRead, CharsRead);	}
		static bool SimWriteConsole(const string &sBuffer, unsigned int CharsToWrite, unsigned int &CharsWritten)
		{	return TheSimulatorWindow.pConsoleWindow->SimWriteConsole(sBuffer, CharsToWrite, CharsWritten);	}



	//*** Help Management Functions ***//
		/**********************************************************************\
			Help( [in] index type [in] index string )

			If the index string is NULL or index type is NoIndex, it displays
			the help contents.
			If the index index type points to one
			of the tutorials, then it displays the help for the index word in
			that tutorial if available.
		\******/
		static void HelpCB(Fl_Widget *pW, void *pV)	{	pMainWindow->Help(NoIndex, "");	}
		bool Help(HelpEnum, const string &);

		/**********************************************************************\
			About( )

			Displays a standard about dialog box.
		\******/
		static void AboutCB(Fl_Widget *pW, void *pV)	{	pMainWindow->About();	}
		bool About();

	};
}

#endif
