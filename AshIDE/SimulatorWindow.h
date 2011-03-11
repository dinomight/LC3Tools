//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef SIMULATORWINDOW_H
#define SIMULATORWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Output.H>
#include <string>
#include <map>
#include "ReadOnlyEditor.h"
#include "Project.h"
#include "ConsoleWindow.h"
#include "DisassemblyWindow.h"
#include "CallStackWindow.h"
#include "InstructionsWindow.h"
#include "DataValuesWindow.h"
#include "MemoryBytesWindow.h"
#include "RegistersWindow.h"
#include "ProgramsWindow.h"
#include "WriteDataWindow.h"
#include "WriteRegisterWindow.h"
#include "BreakpointWindow.h"
#include "../Assembler/Base.h"
#include "../LC3Assembler/LC3ISA.h"
#include "../LC3bAssembler/LC3bISA.h"
#include "../JMTLib/HighlightLexer.h"

using namespace std;
using namespace JMT;
using namespace LC3;
using namespace LC3b;

namespace AshIDE
{
	class SimulatorWindow : public Fl_Double_Window
	{
	public:
		enum SimGoEnum {SimNoGo, SimGo, SimGoOne, SimStepIn, SimStepOver, SimStepOut, SimRunToCursor};
		enum SimWindowEnum {SimNoWindow, SimFileWindow, SimDisassemblyWindow, SimCallStackWindow, SimInstructionsWindow, SimDataValuesWindow, SimMemoryBytesWindow, SimRegistersWindow, SimProgramsWindow};
		#define TheSimulatorWindow	(*SimulatorWindow::pSimulatorWindow)
		static SimulatorWindow *pSimulatorWindow;

	protected:
		friend class MainWindow;
		friend class Project;
		friend class ConsoleWindow;
		friend class DisassemblyWindow;
		friend class CallStackWindow;
		friend class InstructionsWindow;
		friend class DataValuesWindow;
		friend class MemoryBytesWindow;
		friend class RegistersWindow;
		friend class ProgramsWindow;
		friend class WriteDataWindow;
		friend class WriteRegisterWindow;
		friend class BreakpointWindow;

		//Widgets for main menu
		Fl_Menu_Bar *pMainMenu;
		static Fl_Menu_Item MenuItems[];

		//Widgets for the go buttons
		Fl_Group *pGoCommands;
		Fl_Input *pInstructionCount, *pInstruction, *pCycleCount, *pLineNumber;

		//Widgets for the simulator command display
		ReadOnlyEditor *pTextDisplay;
		static Fl_Text_Display::Style_Table_Entry StyleTable[];
		Fl_Input *pInput;
		Fl_Return_Button *pEnter;

		//Widgets for the Trace's register selection dialog
		Fl_Window *pTraceDlg;
		list<Fl_Check_Button *> pTraceRegisters;
		//*NOTE: a feature in FLTK widgets (or at least Fl_Box) is that the char * passed to
		//the constructor as a title is stored directoy as a pointer, and is not copied.
		//So I need to keep the titles I generate around permanently.
		list<string> sTraceRegisterText;
		Fl_Return_Button *pTraceEnter;
		Fl_Button *pTraceCancel;

		//Widgets for the Load/SaveData address dialog
		Fl_Window *pDataDlg;
		Fl_Input *pDataAddress, *pDataLength;
		Fl_Check_Button *pDataOpposite;
		Fl_Choice *pDataType;
		Fl_Return_Button *pDataEnter;
		Fl_Button *pDataCancel;

		//Widgets for the pipelines dialog
		Fl_Window *pPipelineDlg;
		list<string> sPipelineText;

		//Widgets for the status bar
		Fl_Output *pStatus;

		//True if don't update the highlighting of the currently executing line in the open file
		bool fSupressHighlight;
		//True if don't do the updates in Command();
		bool fUpdating;

		//Style stuff that's shared by all simulator windows
		static list<Token *> TheList;
		static LC3ISA::Lexer TheLC3Lexer;
		static LC3bISA::Lexer TheLC3bLexer;
		static HighlightLexer TheLC3HighlightLexer, TheLC3bHighlightLexer;

		//Additional simulator windows
		ConsoleWindow *pConsoleWindow;
		DisassemblyWindow *pDisassemblyWindow;
		CallStackWindow *pCallStackWindow;
		typedef map<InstructionsWindow *, InstructionsWindow *> InstructionsMap;
		InstructionsMap Instructions;
		typedef map<DataValuesWindow *, DataValuesWindow *> DataValuesMap;
		DataValuesMap DataValues;
		typedef map<MemoryBytesWindow *, MemoryBytesWindow *> MemoryBytesMap;
		MemoryBytesMap MemoryBytes;
		typedef map<RegistersWindow *, RegistersWindow *> RegistersMap;
		RegistersMap Registers;
		ProgramsWindow *pProgramsWindow;
		WriteDataWindow *pWriteDataWindow;
		WriteRegisterWindow *pWriteRegisterWindow;
		BreakpointWindow *pBreakpointWindow;

		//Redirect message output to one of the child simulator windows
		SimWindowEnum RedirectWindow;
		Fl_Window *pRedirectWindow;

		/**********************************************************************\
			PlainText( [in] text )
			MessageType( [in] message type )

			Common code used by Message to update both the text buffer
			and style buffer at the same time.
		\******/
		bool PlainText(const string &);
		bool MessageID(MessageEnum);

	public:
		/**********************************************************************\
			SimulatorWindow()

			Constructs the simulator window.
		\******/
		SimulatorWindow();

		/**********************************************************************\
			~SimulatorWindow( )

			Destructor
		\******/
		virtual ~SimulatorWindow();



	//*** Window Management Functions ***//
		static void HelpCB(Fl_Widget *pW, void *pV);

		/**********************************************************************\
			SetTitle( )

			Sets the title to the filename. If text has changed, the filename
			is appended with (modified).
		\******/
		bool SetTitle();

		/**********************************************************************\
			SetStatus( )

			Sets the status to the current simulator information.
		\******/
		bool SetStatus();

		/**********************************************************************\
			Clear( )

			Clears the message output.
		\******/
		static void ClearCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->pTextDisplay->Clear();	}

		/**********************************************************************\
			handle( [in] event )

			All events pass through here.
		\******/
		virtual int handle(int);

		/**********************************************************************\
			resize( [in] x, [in] y, [in] width, [in] height )

			Called when the simulator window changes size.
		\******/
		virtual void resize(int, int, int, int);

		/**********************************************************************\
			ChangeDataType( )

			Updates whether the Signed option is visable in the Load/SaveData
			address selection based on the chosen datatype.
		\******/
		static void ChangeDataTypeCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->ChangeDataType();	}
		bool ChangeDataType();



	//*** Simulator Command Functions ***//

		/**********************************************************************\
			Enter( )

			Executes simulator command
		\******/
		static void EnterCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->Enter();	}
		bool Enter();

		/**********************************************************************\
			Command( [in] command string,
				[in] true if update other displays after executing command )

			Executes simulator command
		\******/
		bool Command(const string &, bool fUpdate = false);



	//*** Simulator Management Functions ***//
		/**********************************************************************\
			Update( )

			Updates any displayed data with the results of current simulation.
			AlwaysStatus sets whether to update the status display on every
			clock cycle, or just on the first break.
		\******/
		bool Update();

		/**********************************************************************\
			Go( )
			GoOne( )
			RunToCursor( )
			StepIn( )
			StepOver( )
			StepOut( )
			Break( )
			Interrupt( )
			Reset( )
			Stop( )
			GoInstruction( )
			GotoInstruction( )
			GoCycle( )
			GotoLine( )

			Performs regular simulation.

			These calls can be made from file windows in order to start a
			simulation. If simulation hasn't started yet, then these functions
			will start the simulation. However, the call to begin simulation
			does not return until simulation terminates, so in order to
			submit this first "Go" command, they set a global flag that
			"Update" will see.
		\******/
		static SimGoEnum InitialGo;
		static SimWindowEnum InitialWindow;
		static Fl_Window *pInitialWindow;
		static void GoCB(Fl_Widget *pW, void *pV);
		bool Go();
		static void GoOneCB(Fl_Widget *pW, void *pV);
		bool GoOne();
		static void RunToCursorCB(SimWindowEnum, Fl_Window *pV);
		bool RunToCursor(SimWindowEnum, Fl_Window *);
		static void StepInCB(Fl_Widget *pW, void *pV);
		bool StepIn();
		static void StepOverCB(Fl_Widget *pW, void *pV);
		bool StepOver();
		static void StepOutCB(Fl_Widget *pW, void *pV);
		bool StepOut();
		static void BreakCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->Break();	}
		bool Break();
		static void InterruptCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->Interrupt();	}
		bool Interrupt();
		static void ResetCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->Reset();	}
		bool Reset();
		static void StopCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->Stop();	}
		bool Stop();
		static void GoInstructionCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->GoInstruction();	}
		bool GoInstruction();
		static void GotoInstructionCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->GotoInstruction();	}
		bool GotoInstruction();
		static void GoCycleCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->GoCycle();	}
		bool GoCycle();
		static void GotoLineCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->GotoLine();	}
		bool GotoLine();

		/**********************************************************************\
			RuntimeChecks( )
			Trace( )
			LoadState( )
			SaveState( )
			LoadData( )
			SaveData( )

			Perform simulator functions.

			These calls can be made from file windows in order to start a
			simulation. If simulation hasn't started yet, then these functions
			will start the simulation. However, the call to begin simulation
			does not return until simulation terminates, so in order to
			submit this first "Go" command, they set a global flag that
			"Update" will see.
		\******/
		static void RuntimeChecksCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->RuntimeChecks();	}
		bool RuntimeChecks();
		static void TraceCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->Trace();	}
		bool Trace();
		static void LoadStateCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->LoadState();	}
		bool LoadState();
		static void SaveStateCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->SaveState();	}
		bool SaveState();
		static void LoadDataCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->LoadData();	}
		bool LoadData();
		static void SaveDataCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->SaveData();	}
		bool SaveData();
		static void LoadObjectCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->LoadObject();	}
		bool LoadObject();
		static void SaveObjectCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->SaveObject();	}
		bool SaveObject();


	//*** Simulator View Functions ***//
		/**********************************************************************\
			Console( )
			Disassembly( )
			CallStack( )
			Instructions( )
			DataValues( )
			MemoryBytes( )
			Registers( )
			Pipelines( )
			Programs( )

			Displays the view window.

			ViewInstruction( )
			ViewData( )
			ViewMemory( )
			ViewRegister( )

			Views the data at the cursor position.
		\******/
		static void ConsoleCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->pConsoleWindow->Show();	}
		bool Console();
		static void DisassemblyCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->ShowDisassembly();	}
		bool ShowDisassembly();
		static void CallStackCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->ShowCallStack();	}
		bool ShowCallStack();
		static void InstructionsCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->AddInstructions();	}
		bool AddInstructions();
		static void DataValuesCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->AddDataValues();	}
		bool AddDataValues();
		static void MemoryBytesCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->AddMemoryBytes();	}
		bool AddMemoryBytes();
		static void RegistersCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->AddRegisters();	}
		bool AddRegisters();
		static void PipelinesCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->Pipelines();	}
		bool Pipelines();
		static void ProgramsCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->Programs();	}
		bool Programs();
		static void ViewInstructionCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->ViewInstruction(WindowType, pW);	}
		bool ViewInstruction(SimWindowEnum, Fl_Window *);
		static void ViewDataCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->ViewData(WindowType, pW);	}
		bool ViewData(SimWindowEnum, Fl_Window *);
		static void ViewMemoryCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->ViewMemory(WindowType, pW);	}
		bool ViewMemory(SimWindowEnum, Fl_Window *);
		static void ViewRegisterCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->ViewRegister(WindowType, pW);	}
		bool ViewRegister(SimWindowEnum, Fl_Window *);

		
		
	//*** Simulator Breakpoint Functions ***//
		/**********************************************************************\
			Breakpoints( )

			Displays the breakpoint window.

			InstructionBreakpoint( )
			DataBreakpoint( )
			RegisterBreakpoint( )

			Sets a breakpoint at the cursor position.
		\******/
		static void BreakpointsCB(Fl_Widget *pW, void *pV)	{	if(pSimulatorWindow)	pSimulatorWindow->Breakpoints();	}
		bool Breakpoints();
		static void InstructionBreakpointCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->InstructionBreakpoint(WindowType, pW);	}
		bool InstructionBreakpoint(SimWindowEnum, Fl_Window *);
		static void DataBreakpointCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->DataBreakpoint(WindowType, pW);	}
		bool DataBreakpoint(SimWindowEnum, Fl_Window *);
		static void RegisterBreakpointCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->RegisterBreakpoint(WindowType, pW);	}
		bool RegisterBreakpoint(SimWindowEnum, Fl_Window *);



	//*** Simulator Edit Functions ***//
		/**********************************************************************\
			WriteData( )
			WriteRegister( )

			Displays the edit window.

			EditInstruction( )
			EditData( )
			EditRegister( )

			Edits the data at the cursor position.
		\******/
		static void WriteDataCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->WriteData();	}
		bool WriteData();
		static void WriteRegisterCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->WriteRegister();	}
		bool WriteRegister();
		static void EditInstructionCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->EditInstruction(WindowType, pW);	}
		bool EditInstruction(SimWindowEnum, Fl_Window *);
		static void EditDataCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->EditData(WindowType, pW);	}
		bool EditData(SimWindowEnum, Fl_Window *);
		static void EditRegisterCB(SimWindowEnum WindowType, Fl_Window *pW)	{	pSimulatorWindow->EditRegister(WindowType, pW);	}
		bool EditRegister(SimWindowEnum, Fl_Window *);

		
		
	//*** Message Management Functions ***//
		static void SaveCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->pTextDisplay->Save();	}
		static void CopyCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->pTextDisplay->Copy();	}
		static void FindCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->pTextDisplay->Find();	}
		static void FindAgainCB(Fl_Widget *pW, void *pV)	{	pSimulatorWindow->pTextDisplay->FindAgain();	}

		/**********************************************************************\
			SimMessage( [in] Message type, [in] message )

			Adds a message to the window text.
		\******/
		bool SimMessage(MessageEnum, const string &);
	};
}

#endif
