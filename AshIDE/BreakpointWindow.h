//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef BREAKPOINTWINDOW_H
#define BREAKPOINTWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Tabs.H>
#include "TextEditor.h"
#include <string>

using namespace std;

namespace AshIDE
{
	class BreakpointWindow : public Fl_Double_Window
	{
	protected:
		friend class SimulatorWindow;

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		//Widgets for the tabs
		Fl_Tabs *pTabs;

		//Widgets for instruction breakpoints
		Fl_Group *pInstrTab;
		Fl_Round_Button *pLineInput, *pInstrInput;
		Fl_Input *pLineNumber, *pInstrAddress, *pInstrValue;
		Fl_Hold_Browser *pInstrBreakpoints;
		Fl_Return_Button *pInstrAdd;
		Fl_Button *pInstrRemove, *pInstrClear;

		//Widgets for data breakpoints
		Fl_Group *pDataTab;
		Fl_Input *pDataAddress, *pDataValue;
		Fl_Choice *pDataEvent, *pDataDataType;
		Fl_Hold_Browser *pDataBreakpoints;
		Fl_Return_Button *pDataAdd;
		Fl_Button *pDataRemove, *pDataClear;

		//Widgets for memory breakpoints
		Fl_Group *pMemTab;
		Fl_Input *pMemAddress, *pMemValue;
		Fl_Choice *pMemoryName, *pMemEvent, *pMemDataType;
		Fl_Hold_Browser *pMemBreakpoints;
		Fl_Return_Button *pMemAdd;
		Fl_Button *pMemRemove, *pMemClear;

		//Widgets for memory breakpoints
		Fl_Group *pRegTab;
		Fl_Input *pRegValue;
		Fl_Choice *pRegisterSet, *pRegister, *pRegEvent;
		Fl_Hold_Browser *pRegBreakpoints;
		Fl_Return_Button *pRegAdd;
		Fl_Button *pRegRemove, *pRegClear;

		//Widgets for quick breakpoint dialogs
		static Fl_Window *pDataDlg;
		static Fl_Input *pDataAddressDlg, *pDataValueDlg;
		static Fl_Choice *pDataEventDlg, *pDataDataTypeDlg;
		static Fl_Return_Button *pDataAddDlg;
		static Fl_Button *pDataCancelDlg;
		static Fl_Window *pRegDlg;
		static Fl_Input *pRegValueDlg, *pRegisterSetDlg, *pRegisterDlg;
		static Fl_Choice *pRegEventDlg;
		static Fl_Return_Button *pRegAddDlg;
		static Fl_Button *pRegCancelDlg;
		/**********************************************************************\
			InitBreakpointDlg( )
			CloseBreakpointDlg( )

			Constructs and destructs the quick breakpoint dialogs
		\******/
		static bool InitBreakpointDlg();
		static bool CloseBreakpointDlg();



		//Widgets for the status bar
		Fl_Output *pStatus;

	public:
		enum _EventEnum {_ReadEvent = 0, _WriteEvent, _ChangeEvent, _ValueEvent};
		static const char *const _sEventTypes[4];

		/**********************************************************************\
			BreakpointWindow( )

			Constructs the WriteData window.
			This window only displays text, there is no editing.
		\******/
		BreakpointWindow();

		/**********************************************************************\
			~BreakpointWindow( )

			Destructor
		\******/
		virtual ~BreakpointWindow();


		
	//*** Window Management Functions ***//
		static void HelpCB(Fl_Widget *pW, void *pV);

		/**********************************************************************\
			Close( )

			Closes the window and removes it from the SimulatorWindow's list.
		\******/
		static void CloseCB(Fl_Widget *pW, void *pV)	{	((BreakpointWindow *)pV)->Close();	}
		bool Close();

		/**********************************************************************\
			SetTitle( )

			Sets the title.
		\******/
		bool SetTitle();

		/**********************************************************************\
			Refresh( )

			Refreshes the breakpoint display.
		\******/
		bool Refresh();



	//*** Breakpoint Management Functions ***//

		/**********************************************************************\
			ChangeInput( )

			Changes whether it's inputing line number or data symbol.
		\******/
		static void ChangeInputCB(Fl_Widget *pW, void *pV)	{	((BreakpointWindow *)pV)->ChangeInput();	}
		bool ChangeInput();

		/**********************************************************************\
			ChangeEvent( )

			Changes whether it's a value event or not.
		\******/
		static void ChangeEventCB(Fl_Widget *pW, void *pV)	{	((BreakpointWindow *)pV)->ChangeEvent();	}
		bool ChangeEvent();
		static void ChangeEventDlgCB(Fl_Widget *pW, void *pV);

		/**********************************************************************\
			ChangeRegisterSet( )

			Changes whether it's inputting logical data or physical memory
			bytes.
		\******/
		static void ChangeRegisterSetCB(Fl_Widget *pW, void *pV)	{	((BreakpointWindow *)pV)->ChangeRegisterSet();	}
		bool ChangeRegisterSet();

		/**********************************************************************\
			Add( )

			Adds a breakpoint.
		\******/
		static void AddCB(Fl_Widget *pW, void *pV)	{	((BreakpointWindow *)pV)->Add();	}
		bool Add();

		/**********************************************************************\
			Remove( )

			Removes the selected breakpoint.
		\******/
		static void RemoveCB(Fl_Widget *pW, void *pV)	{	((BreakpointWindow *)pV)->Remove();	}
		bool Remove();

		/**********************************************************************\
			Clear( )

			Clears all breakpoints.
		\******/
		static void ClearCB(Fl_Widget *pW, void *pV)	{	((BreakpointWindow *)pV)->Clear();	}
		bool Clear();

	//*** File Breakpoint Functions ***//

		/**********************************************************************\
			InstructionBreakpoint( [in] program number, [in] line number )

			Adds a breakpoint for this file and line number.
		\******/
		static bool InstructionBreakpoint(unsigned int, unsigned int);

		/**********************************************************************\
			DataBreakpoint( [in] program number, [in] data identifier )

			Adds a breakpoint for a data variable in this file.
		\******/
		static bool DataBreakpoint(unsigned int, const string &);

		/**********************************************************************\
			RegisterBreakpoint( [in] register identifier )

			Adds a breakpoint for a register.
		\******/
		static bool RegisterBreakpoint(const string &);

	};
}

#endif
