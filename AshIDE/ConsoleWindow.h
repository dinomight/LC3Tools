//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef CONSOLEWINDOW_H
#define CONSOLEWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Return_Button.H>
#include <string>
#include "ReadOnlyEditor.h"

using namespace std;

namespace AshIDE
{
	class ConsoleWindow : public Fl_Double_Window
	{
	protected:
		friend class SimulatorWindow;

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		//Widgets for the status bar
		Fl_Output *pStatus;

		//Widgets for console I/O
		ReadOnlyEditor *pTextDisplay;
		Fl_Input *pInput, *pInputAscii;
		Fl_Return_Button *pEnter;
		Fl_Button *pEnterAscii;
		string sConsoleInput;	//Stores buffered console input
//		bool fWaitForInput;	//set when the console is waiting for user input (SimReadConsole is controlling the event loop)

	public:
		/**********************************************************************\
			ConsoleWindow( )

			Constructs the console window.
			This window only displays text, there is no editing.
		\******/
		ConsoleWindow();

		/**********************************************************************\
			~ConsoleWindow( )

			Destructor
		\******/
		virtual ~ConsoleWindow();



	//*** Window Management Functions ***//
		static void HelpCB(Fl_Widget *pW, void *pV);

		/**********************************************************************\
			SetTitle( )

			Sets the title. If the console is expecting input, it mentions
			it in the title.
		\******/
		bool SetTitle();

		/**********************************************************************\
			Show( )

			Shows the console window.
		\******/
		static void ShowCB(Fl_Widget *pW, void *pV)	{	((ConsoleWindow *)pV)->Show();	}
		bool Show();

		/**********************************************************************\
			Hide( )

			Hides the console window.
		\******/
		static void HideCB(Fl_Widget *pW, void *pV)	{	((ConsoleWindow *)pV)->Hide();	}
		bool Hide();

		/**********************************************************************\
			handle( [in] event )

			All events pass through here.
		\******/
		virtual int handle(int);



	//*** Console Management Functions ***//
		static void SaveCB(Fl_Widget *pW, void *pV)	{	((ConsoleWindow *)pV)->pTextDisplay->Save();	}
		static void CopyCB(Fl_Widget *pW, void *pV)	{	((ConsoleWindow *)pV)->pTextDisplay->Copy();	}
		static void ClearCB(Fl_Widget *pW, void *pV)	{	((ConsoleWindow *)pV)->pTextDisplay->Clear();	}
		static void FindCB(Fl_Widget *pW, void *pV)	{	((ConsoleWindow *)pV)->pTextDisplay->Find();	}
		static void FindAgainCB(Fl_Widget *pW, void *pV)	{	((ConsoleWindow *)pV)->pTextDisplay->FindAgain();	}

		/**********************************************************************\
			Enter( )

			Copies input console text to an internal buffer
		\******/
		static void EnterCB(Fl_Widget *pW, void *pV)	{	((ConsoleWindow *)pV)->Enter();	}
		bool Enter();

		/**********************************************************************\
			EnterAscii( )

			Copies input ascii code to an internal buffer
		\******/
		static void EnterAsciiCB(Fl_Widget *pW, void *pV)	{	((ConsoleWindow *)pV)->EnterAscii();	}
		bool EnterAscii();

		/**********************************************************************\
			ReadConsole( [out] input buffer, [in] characters to read,
				[out] characters read)

			Reads the number of characters from the console buffer, places it
			in the input buffer, returns the number of characters actually read.
			Removes the input characters from the console buffer.

			Returns false if EOF or an error occured.
			Returns true with charsread < charstoread if EOI occured (no input)
			Returns true with charsread = charstoread if normal operation.
		\******/
		bool SimReadConsole(string &, unsigned int, unsigned int &);

		/**********************************************************************\
			WriteConsole( [in] output buffer, [in] characters to write,
				[out] characters written)

			Writes the number of characters from the output buffer, places it
			in the console display, returns the number of characters actually
			written.
		\******/
		bool SimWriteConsole(const string &, unsigned int, unsigned int &);
	};
}

#endif
