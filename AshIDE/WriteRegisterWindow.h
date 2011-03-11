//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef WRITEREGISTERWINDOW_H
#define WRITEREGISTERWINDOW_H

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <string>

using namespace std;

namespace AshIDE
{
	class WriteRegisterWindow : public Fl_Double_Window
	{
	protected:
		friend class SimulatorWindow;

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		//Widgets for the address query
		Fl_Choice *pRegisterSet, *pRegister;
		Fl_Input *pValue;
		Fl_Return_Button *pEnter;

		//Widgets for the status bar
		Fl_Output *pStatus;

	public:
		/**********************************************************************\
			WriteRegisterWindow( )

			Constructs the WriteRegister window.
			This window only displays text, there is no editing.
		\******/
		WriteRegisterWindow();

		/**********************************************************************\
			~WriteRegisterWindow( )

			Destructor
		\******/
		virtual ~WriteRegisterWindow();



		
	//*** Window Management Functions ***//
		static void HelpCB(Fl_Widget *pW, void *pV);

		/**********************************************************************\
			Close( )

			Closes the window and removes it from the SimulatorWindow's list.
		\******/
		static void CloseCB(Fl_Widget *pW, void *pV)	{	((WriteRegisterWindow *)pV)->Close();	}
		bool Close();

		/**********************************************************************\
			SetTitle( )

			Sets the title.
		\******/
		bool SetTitle();



	//*** WriteRegister Management Functions ***//
		/**********************************************************************\
			ChangeWriteRegisteret( )

			Changes whether it's displaying logical data or physical memory
			bytes.
		\******/
		static void ChangeRegisterSetCB(Fl_Widget *pW, void *pV)	{	((WriteRegisterWindow *)pV)->ChangeRegisterSet();	}
		bool ChangeRegisterSet();

		/**********************************************************************\
			Enter( )

			Displays the data per the parameters.
		\******/
		static void EnterCB(Fl_Widget *pW, void *pV)	{	((WriteRegisterWindow *)pV)->Enter();	}
		bool Enter();

		/**********************************************************************\
			EditRegister( [in] register )

			Edit the specified register.
		\******/
		bool EditRegister(const string &);
	};
}

#endif
