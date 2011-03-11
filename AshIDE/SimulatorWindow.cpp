//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "SimulatorWindow.h"
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include "MainWindow.h"
#include "FileWindow.h"
#include "../AsmConvertLC3/AsmConvertLC3.h"

using namespace std;

namespace AshIDE	{

SimulatorWindow *SimulatorWindow::pSimulatorWindow = NULL;
SimulatorWindow::SimGoEnum SimulatorWindow::InitialGo = SimNoGo;
SimulatorWindow::SimWindowEnum SimulatorWindow::InitialWindow = SimNoWindow;
Fl_Window *SimulatorWindow::pInitialWindow = NULL;

list<Token *> SimulatorWindow::TheList;
LC3ISA::Lexer SimulatorWindow::TheLC3Lexer(SimulatorWindow::TheList, MainWindow::MessageCallBack);
LC3bISA::Lexer SimulatorWindow::TheLC3bLexer(SimulatorWindow::TheList, MainWindow::MessageCallBack);
HighlightLexer SimulatorWindow::TheLC3HighlightLexer(&SimulatorWindow::TheLC3Lexer, false);
HighlightLexer SimulatorWindow::TheLC3bHighlightLexer(&SimulatorWindow::TheLC3bLexer, false);

Fl_Menu_Item SimulatorWindow::MenuItems[] =
	{
		{ "&Simulator", 0, 0, 0, FL_SUBMENU },
			{ "&Update Status Every Cycle",	0,				0, 0, FL_MENU_TOGGLE },
			{ "Enable &Runtime Checks",		0,				SimulatorWindow::RuntimeChecksCB, 0, FL_MENU_TOGGLE },
			{ "Enable Execution &Trace",	0,				SimulatorWindow::TraceCB, 0, FL_MENU_TOGGLE | FL_MENU_DIVIDER },
			{ "&Load State",				0,				SimulatorWindow::LoadStateCB },
			{ "&Save State",				0,				SimulatorWindow::SaveStateCB },
			{ "Load D&ata",					0,				SimulatorWindow::LoadDataCB },
			{ "Save &Data",					0,				SimulatorWindow::SaveDataCB },
			{ "Load Program &Object",		0,				SimulatorWindow::LoadObjectCB },
			{ "Save Program O&bject",		0,				SimulatorWindow::SaveObjectCB, 0, FL_MENU_DIVIDER },
			{ "&Help",						FL_F + 1,		HelpCB, 0, FL_MENU_DIVIDER },
			{ "&End Simulation",			FL_CTRL + 'q',	StopCB },
			{ 0 },

		{ "&Messages", 0, 0, 0, FL_SUBMENU },
			{ "&Save",			0,				SaveCB, 0, FL_MENU_DIVIDER },
			{ "&Copy",			FL_CTRL + 'c',	CopyCB },
			{ "C&lear",			0,				ClearCB, 0, FL_MENU_DIVIDER },
			{ "&Find...",		FL_CTRL + 'f',	FindCB },
			{ "F&ind Again",	FL_CTRL + 'g',	FindAgainCB },
			{ 0 },

		{ "S&imulate", 0, 0, 0, FL_SUBMENU },
			{ "&Go",							FL_F + 5,		SimulatorWindow::GoCB },
			{ "Go One &Instruction",			FL_F + 6,		SimulatorWindow::GoOneCB, 0, FL_MENU_DIVIDER },
			{ "Step I&nto Next Function",		FL_F + 8,		SimulatorWindow::StepInCB },
			{ "Step O&ver Next Function",		FL_F + 9,		SimulatorWindow::StepOverCB },
			{ "Step &Out Of Current Function",	FL_F + 10,		SimulatorWindow::StepOutCB, 0, FL_MENU_DIVIDER },
			{ "&Signal Interrupt",				0,				SimulatorWindow::InterruptCB },
			{ "Brea&k",							FL_F + 11,		SimulatorWindow::BreakCB },
			{ "&Reset",							FL_F + 12,		SimulatorWindow::ResetCB },
			{ 0 },

		{ "&Breakpoints", 0, 0, 0, FL_SUBMENU },
			{ "&Breakpoints",	FL_CTRL + 'b',	SimulatorWindow::BreakpointsCB },
			{ 0 },

		{ "&View", 0, 0, 0, FL_SUBMENU },
				{ "&Console I/O",	FL_CTRL + 'i',	SimulatorWindow::ConsoleCB, 0, FL_MENU_DIVIDER },
				{ "&Disassembly",	FL_CTRL + 'd',	SimulatorWindow::DisassemblyCB },
				{ "Call &Stack",	FL_CTRL + 'k',	SimulatorWindow::CallStackCB, 0, FL_MENU_DIVIDER },
				{ "&Instructions",	FL_CTRL + '1',	SimulatorWindow::InstructionsCB },
				{ "D&ata Values",	FL_CTRL + '2',	SimulatorWindow::DataValuesCB },
				{ "&Memory Bytes",	FL_CTRL + '3',	SimulatorWindow::MemoryBytesCB },
				{ "&Registers",		FL_CTRL + '4',	SimulatorWindow::RegistersCB },
				{ "Pipe&lines",		FL_CTRL + '5',	SimulatorWindow::PipelinesCB },
				{ "&Programs",		FL_CTRL + '6',	SimulatorWindow::ProgramsCB },
			{ 0 },

		{ "&Edit && Continue", 0, 0, 0, FL_SUBMENU },
				{ "Edit &Instruction/Data/Memory",	FL_F + 2,	SimulatorWindow::WriteDataCB },
				{ "Edit &Register",					FL_F + 3,	SimulatorWindow::WriteRegisterCB },
			{ 0 },

		{ 0 }
	};

Fl_Text_Display::Style_Table_Entry SimulatorWindow::StyleTable[] = 
	{	// Style table
		//A-Q are here so there aren't conflicts with assembly syntax highlighting
		{ FL_BLACK,			FL_COURIER,			14 },	// A - Unknown
		{ FL_BLACK,			FL_COURIER,			14 },	// B - Bad
		{ FL_DARK_GREEN,	FL_COURIER,			14 },	// C - Comment
		{ FL_BLACK,			FL_COURIER,			14 },	// D - CharConst
		{ FL_MAGENTA,		FL_COURIER,			14 },	// E - Character
		{ FL_DARK_MAGENTA,	FL_COURIER,			14 },	// F - String
		{ FL_RED,			FL_COURIER,			14 },	// G - Integer
		{ FL_RED,			FL_COURIER_ITALIC,	14 },	// H - Real
		{ FL_BLACK,			FL_COURIER,			14 },	// I - Identifier
		{ FL_BLUE,			FL_COURIER,			14 },	// J - Operator
		{ FL_BLUE,			FL_COURIER,			14 },	// K - Directive
		{ FL_DARK_CYAN,		FL_COURIER_ITALIC,	14 },	// L - Attribute
		{ FL_BLACK,			FL_COURIER,			14 },	// M - Expand
		{ FL_DARK_CYAN,		FL_COURIER_BOLD,	14 },	// N - Data
		{ FL_BLACK,			FL_COURIER,			14 },	// O - ISA
		{ FL_DARK_BLUE,		FL_COURIER_BOLD,	14 },	// P - Opcode
		{ FL_DARK_RED,		FL_COURIER_BOLD,	14 },	// Q - Register

		{ FL_DARK_CYAN,		FL_COURIER,			14 },	// R - Filename 
		{ FL_BLACK,			FL_COURIER_BOLD,	14 },	// S - Info
		{ FL_DARK_BLUE,		FL_COURIER_BOLD,	14 },	// T - Warning
		{ FL_DARK_RED,		FL_COURIER_BOLD,	14 },	// U - Error
		{ FL_RED,			FL_COURIER_BOLD,	14 },	// V - Fatal
		{ FL_DARK_MAGENTA,	FL_COURIER_BOLD,	14 },	// W - Exception
		{ FL_DARK_GREEN,	FL_COURIER_BOLD,	14 }	// X - Breakpoint
	};

SimulatorWindow::SimulatorWindow() : Fl_Double_Window(480, 300)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		MenuItems[1].set();
		//*NOTE: Do we want runtime checks enabled or disabled by default?
//		MenuItems[2].set();
		pMainMenu->copy(MenuItems, this);

		//Setup the "go" buttons
		int B = 0, GCX = B, GCY = 28+B, GCW = w()-B*2, GCH = 20*4+8, MX = 0, MY = GCY+GCH+B, MW = w(), MH = h()-MY-28*2;
		pGoCommands = new Fl_Group(GCX, GCY, GCW, GCH);
		{
			Fl_Button *pButton;
			pButton = new Fl_Button(GCX+4, GCY+4, 59, 20, "Stop");
			pButton->callback(StopCB, this);
			pButton = new Fl_Button(GCX+8+59, GCY+4, 59, 20, "Reset");
			pButton->callback(ResetCB, this);
			pButton = new Fl_Button(GCX+12+118, GCY+4, 59, 20, "Break");
			pButton->callback(BreakCB, this);
			pButton = new Fl_Button(GCX+16+177, GCY+4, 59, 20, "Go");
			pButton->callback(GoCB, this);

			pButton = new Fl_Button(GCX+4, GCY+34, 80, 20, "Step In");
			pButton->callback(StepInCB, this);
			pButton = new Fl_Button(GCX+8+80, GCY+34, 80, 20, "Step Over");
			pButton->callback(StepOverCB, this);
			pButton = new Fl_Button(GCX+12+160, GCY+34, 80, 20, "Step Out");
			pButton->callback(StepOutCB, this);

			pButton = new Fl_Button(GCX+26+236, GCY+4, 134, 20, "Go # Cycles");
			pButton->callback(GoCycleCB, this);
			pCycleCount = new Fl_Input(GCX+26+370, GCY+4, 80, 20);
			pCycleCount->value("1");
			pButton = new Fl_Button(GCX+26+236, GCY+24, 134, 20, "Go # Instructions");
			pButton->callback(GoInstructionCB, this);
			pInstructionCount = new Fl_Input(GCX+26+370, GCY+24, 80, 20);
			pInstructionCount->value("1");

			pButton = new Fl_Button(GCX+26+236, GCY+44, 134, 20, "Goto Line Number");
			pButton->callback(GotoLineCB, this);
			pLineNumber = new Fl_Input(GCX+26+370, GCY+44, 80, 20);
			pLineNumber->value("1");
			pButton = new Fl_Button(GCX+4, GCY+64, 300, 20, TheProject.SimISA == LangLC3 ? "Goto Instruction (Symbol or Word Address)" : "Goto Instruction (Symbol or Byte Address)");
			pButton->callback(GotoInstructionCB, this);
			pInstruction = new Fl_Input(GCX+304, GCY+64, 172, 20);
		}
		pGoCommands->end();

		//Setup the status bar
		pStatus = new Fl_Output(0, h()-28, w(), 28);
		pStatus->box(FL_PLASTIC_UP_BOX);
		//Setup the texteditor to use the textbuffer and stylebuffer
		pTextDisplay = new ReadOnlyEditor(MX, MY, MW, MH, StyleTable, sizeof(StyleTable) / sizeof(StyleTable[0]));

		//Setup the command input
		Fl_Box *pB = new Fl_Box(0, h()-28*2, 80, 28, "Command:");
		pB->box(FL_PLASTIC_UP_BOX);
		pInput = new Fl_Input(80, h()-28*2, w()-160, 28);
		pInput->align(FL_ALIGN_LEFT);
		pEnter = new Fl_Return_Button(w()-80, h()-28*2, 80, 28, "Enter");
		pEnter->callback(EnterCB, this);
	}
	end();
	//Setup the Window features
	resizable(pTextDisplay);
	callback(StopCB, this);

	//Setup the Trace's register selection dialog
	pTraceDlg = new Fl_Window(300, TheArch(RegisterSets.size())*20+20+20, "Add Register Sets to the Execution Trace");
	{
		Fl_Box *pBox = new Fl_Box(0, 0, 100, 20, "Register Set:");
		pBox->labelfont(pBox->labelfont() | FL_BOLD);
		pBox = new Fl_Box(100, 0, 200, 20, "Contains Registers:");
		pBox->labelfont(pBox->labelfont() | FL_BOLD);
		unsigned int i = 1;
		for(Architecture::RegisterSetMap::iterator RegSetIter = TheArch(RegisterSets.begin()); RegSetIter != TheArch(RegisterSets.end()); RegSetIter++)
		{
			pTraceRegisters.push_back(new Fl_Check_Button(0, i*20, 100, 20, RegSetIter->second.sName.c_str()));
			(*pTraceRegisters.rbegin())->set();
			sTraceRegisterText.push_back(string());
			for(RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.begin(); RegIter != RegSetIter->second.Registers.end(); RegIter++)
				*sTraceRegisterText.rbegin() += RegIter->second.sName+" ";
			pBox = new Fl_Box(100, i*20, 200, 20, sTraceRegisterText.rbegin()->c_str());
			i++;
		}

		pTraceEnter = new Fl_Return_Button(300-200, i*20, 100, 20, "Enter");

		pTraceCancel = new Fl_Button(300-100, i*20, 100, 20, "Cancel");
	}
	pTraceDlg->end();
	pTraceDlg->set_modal();

	//Setup the Load/SaveData address dialog
	pDataDlg = new Fl_Window(300, 100, "Select Data Address, Length, and Type");
	{
		pDataAddress = new Fl_Input(0, 0, 120, 20, TheProject.SimISA == LangLC3 ? "Symbol or Word Address" : "Symbol or Byte Address");
		pDataAddress->align(FL_ALIGN_RIGHT);
		pDataAddress->value("0");
		pDataLength = new Fl_Input(0, 20, 120, 20, "Length");
		pDataLength->align(FL_ALIGN_RIGHT);
		pDataLength->value("256");
		pDataType = new Fl_Choice(0, 40, 120, 20, "Data Type");
		pDataType->box(FL_PLASTIC_DOWN_BOX);
		pDataType->align(FL_ALIGN_RIGHT);
		pDataType->add("DATA1");
		pDataType->add("DATA2");
		pDataType->add("DATA4");
		pDataType->add("DATA8");
		pDataType->add("REAL1");
		pDataType->add("REAL2");
		pDataType->add("REAL4");
		pDataType->add("REAL8");
		pDataType->add(TheProject.SimISA == LangLC3 ? "Words" : "Bytes");
		pDataType->callback(ChangeDataTypeCB, this);
		pDataType->value(STRUCT);
		pDataOpposite = new Fl_Check_Button(104, 60, 120, 20, "Store in Opposite Endian");
		pDataEnter = new Fl_Return_Button(300-200, 80, 100, 20, "Enter");
		pDataCancel = new Fl_Button(300-100, 80, 100, 20, "Cancel");
	}
	pDataDlg->end();
	pDataDlg->set_modal();

	//Setup the display pipelines dialog
	pPipelineDlg = new Fl_Window(300, TheArch(Pipelines.size())*20+20, "Pipelines");
	{
		Fl_Box *pBox = new Fl_Box(0, 0, 100, 20, "Pipeline:");
		pBox->labelfont(pBox->labelfont() | FL_BOLD);
		pBox = new Fl_Box(100, 0, 200, 20, "Contains Stages:");
		pBox->labelfont(pBox->labelfont() | FL_BOLD);
		unsigned int i = 1;
		for(Architecture::PipelineVector::iterator PipeIter = TheArch(Pipelines.begin()); PipeIter != TheArch(Pipelines.end()); PipeIter++)
		{
			pBox = new Fl_Box(0, i*20, 100, 20, PipeIter->sName.c_str());
			sPipelineText.push_back(string());
			for(Pipeline::PipelineStageVector::iterator PipeStageIter = PipeIter->PipelineStages.begin(); PipeStageIter != PipeIter->PipelineStages.end(); PipeStageIter++)
				*sPipelineText.rbegin() += PipeStageIter->sName+" ";
			pBox = new Fl_Box(100, i*20, 200, 20, sPipelineText.rbegin()->c_str());
			i++;
		}
	}
	pPipelineDlg->end();

	//Setup the quick breakpoint dialogs
	BreakpointWindow::InitBreakpointDlg();
	
	//Setup the console
	pConsoleWindow = new ConsoleWindow();

	//Set up view windows
	pDisassemblyWindow = NULL;
	pCallStackWindow = NULL;
	pProgramsWindow = NULL;
	pWriteDataWindow = NULL;
	pWriteRegisterWindow = NULL;
	pBreakpointWindow = NULL;

	RedirectWindow = SimNoWindow;
	fSupressHighlight = false;
	fUpdating = false;

	//turn on the simulator
	show();
	SetTitle();
}

SimulatorWindow::~SimulatorWindow()
{
	hide();
	InitialGo = SimNoGo;
	InitialWindow = SimNoWindow;
	pInitialWindow = NULL;
	FileWindow::UnHighlight();
	BreakpointWindow::CloseBreakpointDlg();
	delete pTraceDlg;
	delete pDataDlg;
	delete pPipelineDlg;
	delete pConsoleWindow;
	if(pDisassemblyWindow)
		delete pDisassemblyWindow;
	if(pCallStackWindow)
		delete pCallStackWindow;
	for(InstructionsMap::iterator InstructionIter = Instructions.begin(); InstructionIter != Instructions.end(); InstructionIter = Instructions.begin())
		delete InstructionIter->second;
	for(DataValuesMap::iterator DataIter = DataValues.begin(); DataIter != DataValues.end(); DataIter = DataValues.begin())
		delete DataIter->second;
	for(MemoryBytesMap::iterator MemoryIter = MemoryBytes.begin(); MemoryIter != MemoryBytes.end(); MemoryIter = MemoryBytes.begin())
		delete MemoryIter->second;
	for(RegistersMap::iterator RegisterIter = Registers.begin(); RegisterIter != Registers.end(); RegisterIter = Registers.begin())
		delete RegisterIter->second;
	if(pProgramsWindow)
		delete pProgramsWindow;
	if(pWriteDataWindow)
		delete pWriteDataWindow;
	if(pWriteRegisterWindow)
		delete pWriteRegisterWindow;
	if(pBreakpointWindow)
		delete pBreakpointWindow;
	Fl::first_window()->show();
	pSimulatorWindow = NULL;
}

void SimulatorWindow::HelpCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.Help(AshIDEIndex, "SimulatorWindow");
}

bool SimulatorWindow::SetTitle()
{
	string sTitle = "Simulator";

	sTitle += string(" - ") + TheProject.sFileName.Bare;

	label(sTitle.c_str());
	return true;
}

bool SimulatorWindow::SetStatus()
{
	char sBuffer[64];
	unsigned int Length = 0;

	#if defined _MSC_VER
		Length += sprintf(sBuffer + Length, "Cycle: %I64u", TheSim(SimCycle));
	#elif defined GPLUSPLUS
		Length += sprintf(sBuffer + Length, "Cycle: %llu", TheSim(SimCycle));
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif

	for(; Length < 14; Length++)
		sBuffer[Length] = ' ';

	#if defined _MSC_VER
		Length += sprintf(sBuffer + Length, "Instr: %I64u", TheSim(SimInstruction));
	#elif defined GPLUSPLUS
		Length += sprintf(sBuffer + Length, "Instr: %llu", TheSim(SimInstruction));
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif

	for(; Length < 28; Length++)
		sBuffer[Length] = ' ';

	if(TheProject.SimISA == LangLC3)
	{
		#if defined _MSC_VER
			Length += sprintf(sBuffer + Length, "Address: 4x%I64X", TheArch(NextInstruction()) >> LC3ISA::Addressability);
		#elif defined GPLUSPLUS
			Length += sprintf(sBuffer + Length, "Address: 4x%llX", TheArch(NextInstruction()) >> LC3ISA::Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
	}
	else
	{
		#if defined _MSC_VER
			Length += sprintf(sBuffer + Length, "Address: 4x%I64X", TheArch(NextInstruction()) >> LC3bISA::Addressability);
		#elif defined GPLUSPLUS
			Length += sprintf(sBuffer + Length, "Address: 4x%llX", TheArch(NextInstruction()) >> LC3bISA::Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
	}

	pStatus->value(sBuffer);
	pConsoleWindow->pStatus->value(sBuffer);
	if(pDisassemblyWindow)
		pDisassemblyWindow->pStatus->value(sBuffer);
	if(pCallStackWindow)
		pCallStackWindow->pStatus->value(sBuffer);
	for(InstructionsMap::iterator InstructionIter = Instructions.begin(); InstructionIter != Instructions.end(); InstructionIter++)
		InstructionIter->second->pStatus->value(sBuffer);
	for(DataValuesMap::iterator DataIter = DataValues.begin(); DataIter != DataValues.end(); DataIter++)
		DataIter->second->pStatus->value(sBuffer);
	for(MemoryBytesMap::iterator MemoryIter = MemoryBytes.begin(); MemoryIter != MemoryBytes.end(); MemoryIter++)
		MemoryIter->second->pStatus->value(sBuffer);
	for(RegistersMap::iterator RegisterIter = Registers.begin(); RegisterIter != Registers.end(); RegisterIter++)
		RegisterIter->second->pStatus->value(sBuffer);
	if(pProgramsWindow)
		pProgramsWindow->pStatus->value(sBuffer);
	if(pWriteDataWindow)
		pWriteDataWindow->pStatus->value(sBuffer);
	if(pWriteRegisterWindow)
		pWriteRegisterWindow->pStatus->value(sBuffer);
	if(pBreakpointWindow)
		pBreakpointWindow->pStatus->value(sBuffer);

	return true;
}

int SimulatorWindow::handle(int Event)
{
	int RetVal = Fl_Double_Window::handle(Event);
	switch(Event)
	{
	case FL_FOCUS:
		//*NOTE: Some bug/feature in FLTK causes the Status widget to always get
		//the focus instead of the text editor
		pInput->take_focus();
	}
	return RetVal;
}

void SimulatorWindow::resize(int x, int y, int width, int height)
{
	//Reset the console width so that data/memory array displays fit in the message window
	if(width-4 > 0)
		TheSim(SetConsoleWidth(MAX(MIN((width-4)/8, MAX_CONSOLE_WIDTH), MIN_CONSOLE_WIDTH)));
	Fl_Double_Window::resize(x, y, width, height);
}

bool SimulatorWindow::ChangeDataType()
{
	DataEnum DataType = (DataEnum)pDataType->value();
	if(DataType >= DATA1 && DataType <= DATA8)
	{
		pDataOpposite->activate();
	}
	else
	{
		pDataOpposite->deactivate();
	}
	return true;
}

bool SimulatorWindow::Update()
{
	if(pMainMenu->menu()[1].value() != 0 || TheSim(fBreak) && TheSim(fFirstBreak))
	{
		SetStatus();
	}

	switch(InitialGo)
	{
	case SimGo:
		InitialGo = SimNoGo;
		Go();
		break;
	case SimGoOne:
		InitialGo = SimNoGo;
		GoOne();
		break;
	case SimStepIn:
		InitialGo = SimNoGo;
		StepIn();
		break;
	case SimStepOver:
		InitialGo = SimNoGo;
		StepOver();
		break;
	case SimStepOut:
		InitialGo = SimNoGo;
		StepOut();
		break;
	case SimRunToCursor:
		InitialGo = SimNoGo;
		RunToCursor(InitialWindow, pInitialWindow);
		break;
	default:
		break;
	}

	if(TheSim(fBreak) && TheSim(fFirstBreak))
	{
		TheSim(fFirstBreak) = false;

		if(!fSupressHighlight)
		{
			FileWindow::UnHighlight();
			Element *pElement = TheSim(AddressToElement(TheArch(NextInstruction()), true));
			if(pElement)
			{
				string sFileName;
				FileName sFile = sFileName = CreateFileNameRelative(TheProject.sWorkingDir, InputList[pElement->LocationStack.rbegin()->first]);
				if(ToLower(sFile.Ext) != "obj" && ToLower(sFile.Ext) != "bin")
				{
					unsigned int LineNumber = pElement->LocationStack.rbegin()->second;
					FileWindow *pFW;
					if(!pDisassemblyWindow)
						TheMainWindow.OpenFile(sFileName);
					if(pFW = FileWindow::IsOpen(sFileName))
					{
						pFW->SelectLine(LineNumber, true, !pDisassemblyWindow);
					}
					if(pDisassemblyWindow)
					{
						pDisassemblyWindow->UnHighlight();
						pDisassemblyWindow->SelectLine(pElement->LocationID, true);
					}
				}
			}
		}

		fUpdating = true;
		if(pCallStackWindow)
			pCallStackWindow->Enter();
		for(InstructionsMap::iterator InstructionIter = Instructions.begin(); InstructionIter != Instructions.end(); InstructionIter++)
			InstructionIter->second->Enter();
		for(DataValuesMap::iterator DataIter = DataValues.begin(); DataIter != DataValues.end(); DataIter++)
			DataIter->second->Enter();
		for(MemoryBytesMap::iterator MemoryIter = MemoryBytes.begin(); MemoryIter != MemoryBytes.end(); MemoryIter++)
			MemoryIter->second->Enter();
		for(RegistersMap::iterator RegisterIter = Registers.begin(); RegisterIter != Registers.end(); RegisterIter++)
			RegisterIter->second->Enter();
		if(pProgramsWindow)
			pProgramsWindow->Enter();
		if(pBreakpointWindow)
			pBreakpointWindow->Refresh();
		fUpdating = false;
	}

	return true;
}

void SimulatorWindow::GoCB(Fl_Widget *pW, void *pV)
{
	if(pSimulatorWindow)
		pSimulatorWindow->Go();
	else
	{
		InitialGo = SimGo;
		MainWindow::SimulateCB(MainWindow::pMainWindow, NULL);
	}
}

bool SimulatorWindow::Go()
{
	bool fRetVal = SIM_COMMAND("go");
	if(pBreakpointWindow)
		pBreakpointWindow->Refresh();
	return fRetVal;
}

void SimulatorWindow::GoOneCB(Fl_Widget *pW, void *pV)
{
	if(pSimulatorWindow)
		pSimulatorWindow->GoOne();
	else
	{
		InitialGo = SimGoOne;
		MainWindow::SimulateCB(MainWindow::pMainWindow, NULL);
	}
}

bool SimulatorWindow::GoOne()
{
	static IntegerNumber One(NullLocationStack, 1);
	bool fRetVal = SIM_COMMAND("goi 1");
	if(pBreakpointWindow)
		pBreakpointWindow->Refresh();
	return fRetVal;
}

void SimulatorWindow::RunToCursorCB(SimWindowEnum WindowType, Fl_Window *pWindow)
{
	if(pSimulatorWindow)
		pSimulatorWindow->RunToCursor(WindowType, pWindow);
	else
	{
		InitialGo = SimRunToCursor;
		InitialWindow = WindowType;
		pInitialWindow = pWindow;
		MainWindow::SimulateCB(MainWindow::pMainWindow, NULL);
	}
}

bool SimulatorWindow::RunToCursor(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sFileName;
	unsigned int LineNumber;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		break;
	default:
		return false;
	}

	if(!TheProject.IsSimulating(sFileName))
	{
		fl_alert("File '%s' is not part of the current simulation.", sFileName.c_str());
		return false;
	}

	Project::FileData *pFileData = TheProject.GetFile(sFileName);
	char sProgramLine[32];
	sprintf(sProgramLine, "{%u} %u", pFileData->ProgramNumber, LineNumber);
	string sCommand = string("gotol ")+sProgramLine;
	bool fRetVal = SIM_COMMAND(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error inserting breakpoint.");
		show();
	}
	else if(pBreakpointWindow)
		pBreakpointWindow->Refresh();

	return fRetVal;
}

void SimulatorWindow::StepInCB(Fl_Widget *pW, void *pV)
{
	if(pSimulatorWindow)
		pSimulatorWindow->StepIn();
	else
	{
		InitialGo = SimStepIn;
		MainWindow::SimulateCB(MainWindow::pMainWindow, NULL);
	}
}

bool SimulatorWindow::StepIn()
{
	bool fRetVal = SIM_COMMAND("goin");
	if(pBreakpointWindow)
		pBreakpointWindow->Refresh();
	return fRetVal;
}

void SimulatorWindow::StepOutCB(Fl_Widget *pW, void *pV)
{
	if(pSimulatorWindow)
		pSimulatorWindow->StepOut();
	else
	{
		InitialGo = SimStepOut;
		MainWindow::SimulateCB(MainWindow::pMainWindow, NULL);
	}
}

bool SimulatorWindow::StepOver()
{
	bool fRetVal = SIM_COMMAND("goover");
	if(pBreakpointWindow)
		pBreakpointWindow->Refresh();
	return fRetVal;
}

void SimulatorWindow::StepOverCB(Fl_Widget *pW, void *pV)
{
	if(pSimulatorWindow)
		pSimulatorWindow->StepOver();
	else
	{
		InitialGo = SimStepOver;
		MainWindow::SimulateCB(MainWindow::pMainWindow, NULL);
	}
}

bool SimulatorWindow::StepOut()
{
	bool fRetVal = SIM_COMMAND("goout");
	if(pBreakpointWindow)
		pBreakpointWindow->Refresh();
	return fRetVal;
}

bool SimulatorWindow::Break()
{
	TheSim(fControlC) = -1;
	return true;
}

bool SimulatorWindow::Interrupt()
{
	const char *sVector = fl_input("Enter the interrupt vector (symbol or number):");
	if(!sVector || sVector[0] == 0)
		return false;

	string sCommand = string("int ")+sVector;
	bool fRetVal = SIM_COMMAND_UPDATE(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error performing interrupt.");
		show();
	}
	return fRetVal;
}

bool SimulatorWindow::Reset()
{
	bool fRetVal = true;
	return SIM_COMMAND_UPDATE("reset");
}

bool SimulatorWindow::Stop()
{
	if(Fl::event_key() == FL_Escape)
		// ignore Escape
		return false;

	TheProject.fSimulating = false;
	return true;
}

bool SimulatorWindow::GoInstruction()
{
	const char *sInstruction = pInstructionCount->value();
	string sCommand = string("goi ")+sInstruction;
	bool fRetVal = SIM_COMMAND(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error with go command.");
		show();
	}
	else if(pBreakpointWindow)
		pBreakpointWindow->Refresh();

	return fRetVal;
}

bool SimulatorWindow::GotoInstruction()
{
	const char *sInstruction = pInstruction->value();
	string sCommand = string("gotoi ")+sInstruction;
	bool fRetVal = SIM_COMMAND(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error with goto command.");
		show();
	}
	else if(pBreakpointWindow)
		pBreakpointWindow->Refresh();

	return fRetVal;
}

bool SimulatorWindow::GoCycle()
{
	const char *sCycle = pCycleCount->value();
	string sCommand = string("go ")+sCycle;
	bool fRetVal = SIM_COMMAND(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error with go command.");
		show();
	}
	else if(pBreakpointWindow)
		pBreakpointWindow->Refresh();

	return fRetVal;
}

bool SimulatorWindow::GotoLine()
{
	const char *sLineNumber = pLineNumber->value();
	string sCommand = string("gotol ")+sLineNumber;
	bool fRetVal = SIM_COMMAND(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error with goto command.");
		show();
	}
	else if(pBreakpointWindow)
		pBreakpointWindow->Refresh();

	return fRetVal;
}

bool SimulatorWindow::RuntimeChecks()
{
	if(pMainMenu->menu()[2].value() != 0)
		return SIM_COMMAND("checkon");
	else
		return SIM_COMMAND("checkoff");
}

bool SimulatorWindow::Trace()
{
	if(pMainMenu->menu()[3].value() != 0)
	{
		char *sBuffer;
		sBuffer = fl_file_chooser("New Trace?", NULL, "");
		if(sBuffer == NULL)
		{
			const_cast<Fl_Menu_Item *>(pMainMenu->menu())[3].clear();
			return false;
		}

		//Run this dialog until enter or cancel is chosen, block all other operations.
		pTraceDlg->show();
		bool fTrace = false;
		while(true)
		{
			Fl_Widget *pW = Fl::readqueue();
			if(!pW)
				Fl::wait();
			else if(pW == pTraceEnter)
			{
				fTrace = true;
				break;
			}
			else if(pW == pTraceCancel)
				break;
			else if(pW == pTraceDlg)
				break;
		}
		pTraceDlg->hide();

		if(!fTrace)
		{
			const_cast<Fl_Menu_Item *>(pMainMenu->menu())[3].clear();
			return false;
		}

		string sRegisterSetList;
		for(list<Fl_Check_Button *>::iterator RegSetIter = pTraceRegisters.begin(); RegSetIter != pTraceRegisters.end(); RegSetIter++)
		{
			if((*RegSetIter)->value())
				(sRegisterSetList += " ") += (*RegSetIter)->label();
		}

		string sCommand = string("traceon \"")+sBuffer+"\""+sRegisterSetList;
		bool fRetVal = SIM_COMMAND(sCommand);

		if(!fRetVal)
		{
			fl_alert("Error starting trace.");
			show();
			const_cast<Fl_Menu_Item *>(pMainMenu->menu())[3].clear();
		}
		return fRetVal;
	}
	else
		return SIM_COMMAND("traceoff");
}

bool SimulatorWindow::LoadState()
{
	if(!TheSim(fBreak))
	{
		int Choice = fl_choice("The program is still executing. Break before loading new state?",
			"Cancel", "Break", "Don't Break");

		switch(Choice)
		{
		case 0:
			return false;
		case 1:
			Break();
		}
	}

	char *sBuffer;
	sBuffer = fl_file_chooser("Load State?", NULL, "");
	if(sBuffer == NULL)
		return false;

	string sCommand = string("loads \"")+sBuffer+"\"";
	bool fRetVal = SIM_COMMAND_UPDATE(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error loading state.");
		show();
	}
	return fRetVal;
}

bool SimulatorWindow::SaveState()
{
	char *sBuffer;
	sBuffer = fl_file_chooser("Save State?", NULL, "");
	if(sBuffer == NULL)
		return false;

	string sCommand = string("saves \"")+sBuffer+"\"";
	bool fRetVal = SIM_COMMAND(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error saving state.");
		show();
	}
	return fRetVal;
}

bool SimulatorWindow::LoadData()
{
	char *Buffer;
	Buffer = fl_file_chooser("Load Data?", NULL, "");
	if(Buffer == NULL)
		return false;

	//Run this dialog until enter or cancel is chosen, block all other operations.
	pDataDlg->show();
	bool fData = false;
	while(true)
	{
		Fl_Widget *pW = Fl::readqueue();
		if(!pW)
			Fl::wait();
		else if(pW == pDataEnter)
		{
			fData = true;
			break;
		}
		else if(pW == pDataCancel)
			break;
		else if(pW == pDataDlg)
			break;
	}
	pDataDlg->hide();

	if(!fData)
		return false;

	const char *sDataAddress = pDataAddress->value();
	const char *sLength = pDataLength->value();
	if(sDataAddress[0] == 0 || sLength[0] == 0)
	{
		fl_alert(TheProject.SimISA == LangLC3 ?
			"You must provide a symbol or word address identifier, and display length." :
			"You must provide a symbol or byte address identifier, and display length.");
		return false;
	}
	DataEnum DataType = (DataEnum)pDataType->value();
	string sCommand;
	sCommand = string("loadd \"")+Buffer+"\" "+sDataAddress+" "+sLength+" ";
	if(DataType >= DATA1 && DataType <= DATA8 && pDataOpposite->value())
		sCommand += "- ";
	if(DataType < STRUCT)
		(sCommand += sDataTypes[DataType]) += " ";
	bool fRetVal = SIM_COMMAND_UPDATE(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error loading data.");
		show();
	}
	return fRetVal;
}

bool SimulatorWindow::SaveData()
{
	char *Buffer;
	Buffer = fl_file_chooser("Save Data?", NULL, "");
	if(Buffer == NULL)
		return false;

	//Run this dialog until enter or cancel is chosen, block all other operations.
	pDataDlg->show();
	bool fData = false;
	while(true)
	{
		Fl_Widget *pW = Fl::readqueue();
		if(!pW)
			Fl::wait();
		else if(pW == pDataEnter)
		{
			fData = true;
			break;
		}
		else if(pW == pDataCancel)
			break;
		else if(pW == pDataDlg)
			break;
	}
	pDataDlg->hide();

	if(!fData)
		return false;

	const char *sDataAddress = pDataAddress->value();
	const char *sLength = pDataLength->value();
	if(sDataAddress[0] == 0 || sLength[0] == 0)
	{
		fl_alert(TheProject.SimISA == LangLC3 ?
			"You must provide a symbol or word address identifier, and display length." :
			"You must provide a symbol or byte address identifier, and display length.");
		return false;
	}
	DataEnum DataType = (DataEnum)pDataType->value();
	string sCommand;
	sCommand = string("saved \"")+Buffer+"\" "+sDataAddress+" "+sLength+" ";
	if(DataType >= DATA1 && DataType <= DATA8 && pDataOpposite->value())
		sCommand += "- ";
	if(DataType < STRUCT)
		(sCommand += sDataTypes[DataType]) += " ";
	bool fRetVal = SIM_COMMAND(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error saving data.");
		show();
	}
	return fRetVal;
}

bool SimulatorWindow::LoadObject()
{
	char *Buffer;
	Buffer = fl_file_chooser("Load Program Object?", NULL, "");
	if(Buffer == NULL)
		return false;

	string sCommand;
	sCommand = string("loado \"")+Buffer+"\"";
	bool fRetVal = SIM_COMMAND_UPDATE(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error loading object.");
		show();
	}
	return fRetVal;
}

bool SimulatorWindow::SaveObject()
{
	char *Buffer;
	Buffer = fl_file_chooser("Save Program Object?", NULL, "");
	if(Buffer == NULL)
		return false;

	//Run this dialog until enter or cancel is chosen, block all other operations.
	pDataOpposite->hide();
	pDataType->hide();
	const char *pOldLabel = pDataDlg->label();
	pDataDlg->label("Select Data Address and Length");
	pDataDlg->show();
	bool fData = false;
	while(true)
	{
		Fl_Widget *pW = Fl::readqueue();
		if(!pW)
			Fl::wait();
		else if(pW == pDataEnter)
		{
			fData = true;
			break;
		}
		else if(pW == pDataCancel)
			break;
		else if(pW == pDataDlg)
			break;
	}
	pDataDlg->hide();
	pDataOpposite->show();
	pDataType->show();
	pDataDlg->label(pOldLabel);

	if(!fData)
		return false;

	const char *sDataAddress = pDataAddress->value();
	const char *sLength = pDataLength->value();
	if(sDataAddress[0] == 0 || sLength[0] == 0)
	{
		fl_alert(TheProject.SimISA == LangLC3 ?
			"You must provide a symbol or word address identifier, and display length." :
			"You must provide a symbol or byte address identifier, and display length.");
		return false;
	}
	string sCommand;
	sCommand = string("saveo \"")+Buffer+"\" "+sDataAddress+" "+sLength+" ";
	bool fRetVal = SIM_COMMAND(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error saving object.");
		show();
	}
	return fRetVal;
}

bool SimulatorWindow::ShowDisassembly()
{
	if(!pDisassemblyWindow)
		pDisassemblyWindow = new DisassemblyWindow();
	else
		pDisassemblyWindow->show();
	return true;
}

bool SimulatorWindow::ShowCallStack()
{
	if(!pCallStackWindow)
		pCallStackWindow = new CallStackWindow();
	else
		pCallStackWindow->show();
	return true;
}

bool SimulatorWindow::AddInstructions()
{
	InstructionsWindow *pI = new InstructionsWindow();
	Instructions.insert(InstructionsMap::value_type(pI, pI));
	return true;
}

bool SimulatorWindow::AddDataValues()
{
	DataValuesWindow *pDV = new DataValuesWindow();
	DataValues.insert(DataValuesMap::value_type(pDV, pDV));
	return true;
}

bool SimulatorWindow::AddMemoryBytes()
{
	MemoryBytesWindow *pMB = new MemoryBytesWindow();
	MemoryBytes.insert(MemoryBytesMap::value_type(pMB, pMB));
	return true;
}

bool SimulatorWindow::AddRegisters()
{
	RegistersWindow *pR = new RegistersWindow();
	Registers.insert(RegistersMap::value_type(pR, pR));
	return true;
}

bool SimulatorWindow::Pipelines()
{
	pPipelineDlg->show();
	return true;
}

bool SimulatorWindow::Programs()
{
	if(!pProgramsWindow)
		pProgramsWindow = new ProgramsWindow();
	else
		pProgramsWindow->show();
	return true;
}

bool SimulatorWindow::ViewInstruction(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sFileName;
	unsigned int LineNumber;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		break;
	default:
		return false;
	}

	Project::FileData *pFileData = TheProject.GetFile(sFileName);
	InstructionsWindow *pI = new InstructionsWindow();
	Instructions.insert(InstructionsMap::value_type(pI, pI));
	return pI->ViewInstruction(pFileData->ProgramNumber, LineNumber);
}

bool SimulatorWindow::ViewData(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sFileName, sIdentifier;
	unsigned int LineNumber;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		if(!((FileWindow *)pW)->GetIdentifier(sIdentifier))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		if(!((DisassemblyWindow *)pW)->GetIdentifier(sIdentifier))
			return false;
		break;
	default:
		return false;
	}

	Project::FileData *pFileData = TheProject.GetFile(sFileName);
	DataValuesWindow *pDV = new DataValuesWindow();
	DataValues.insert(DataValuesMap::value_type(pDV, pDV));
	return pDV->ViewData(pFileData->ProgramNumber, sIdentifier);
}

bool SimulatorWindow::ViewMemory(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sFileName, sIdentifier;
	unsigned int LineNumber;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		if(!((FileWindow *)pW)->GetIdentifier(sIdentifier))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		if(!((DisassemblyWindow *)pW)->GetIdentifier(sIdentifier))
			return false;
		break;
	default:
		return false;
	}

	Project::FileData *pFileData = TheProject.GetFile(sFileName);
	MemoryBytesWindow *pMB = new MemoryBytesWindow();
	MemoryBytes.insert(MemoryBytesMap::value_type(pMB, pMB));
	return pMB->ViewMemory(pFileData->ProgramNumber, sIdentifier);
}

bool SimulatorWindow::ViewRegister(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sRegister;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetRegister(sRegister))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetRegister(sRegister))
			return false;
		break;
	default:
		return false;
	}

	RegistersWindow *pR = new RegistersWindow();
	Registers.insert(RegistersMap::value_type(pR, pR));
	return pR->ViewRegister(sRegister);
}

bool SimulatorWindow::Breakpoints()
{
	if(!pBreakpointWindow)
		pBreakpointWindow = new BreakpointWindow();
	else
		pBreakpointWindow->show();
	return true;
}

bool SimulatorWindow::InstructionBreakpoint(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sFileName;
	unsigned int LineNumber;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		break;
	default:
		return false;
	}

	Project::FileData *pFileData = TheProject.GetFile(sFileName);
	return BreakpointWindow::InstructionBreakpoint(pFileData->ProgramNumber, LineNumber);
}

bool SimulatorWindow::DataBreakpoint(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sFileName, sIdentifier;
	unsigned int LineNumber;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		if(!((FileWindow *)pW)->GetIdentifier(sIdentifier))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		if(!((DisassemblyWindow *)pW)->GetIdentifier(sIdentifier))
			return false;
		break;
	default:
		return false;
	}

	Project::FileData *pFileData = TheProject.GetFile(sFileName);
	return BreakpointWindow::DataBreakpoint(pFileData->ProgramNumber, sIdentifier);
}

bool SimulatorWindow::RegisterBreakpoint(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sRegister;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetRegister(sRegister))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetRegister(sRegister))
			return false;
		break;
	default:
		return false;
	}

	return BreakpointWindow::RegisterBreakpoint(sRegister);
}

bool SimulatorWindow::WriteData()
{
	if(!pWriteDataWindow)
		pWriteDataWindow = new WriteDataWindow();
	else
		pWriteDataWindow->show();
	return true;
}

bool SimulatorWindow::WriteRegister()
{
	if(!pWriteRegisterWindow)
		pWriteRegisterWindow = new WriteRegisterWindow();
	else
		pWriteRegisterWindow->show();
	return true;
}

bool SimulatorWindow::EditInstruction(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sFileName;
	unsigned int LineNumber;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		break;
	default:
		return false;
	}

	Project::FileData *pFileData = TheProject.GetFile(sFileName);
	WriteData();
	return pWriteDataWindow->EditInstruction(pFileData->ProgramNumber, LineNumber);
}

bool SimulatorWindow::EditData(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sFileName, sIdentifier;
	unsigned int LineNumber;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		if(!((FileWindow *)pW)->GetIdentifier(sIdentifier))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetFileLine(sFileName, LineNumber))
			return false;
		if(!((DisassemblyWindow *)pW)->GetIdentifier(sIdentifier))
			return false;
		break;
	default:
		return false;
	}

	Project::FileData *pFileData = TheProject.GetFile(sFileName);
	WriteData();
	return pWriteDataWindow->EditData(pFileData->ProgramNumber, sIdentifier);
}

bool SimulatorWindow::EditRegister(SimWindowEnum WindowType, Fl_Window *pW)
{
	string sRegister;

	switch(WindowType)
	{
	case SimFileWindow:
		if(!((FileWindow *)pW)->GetRegister(sRegister))
			return false;
		break;
	case SimDisassemblyWindow:
		if(!((DisassemblyWindow *)pW)->GetRegister(sRegister))
			return false;
		break;
	default:
		return false;
	}

	WriteRegister();
	return pWriteRegisterWindow->EditRegister(sRegister);
}

bool SimulatorWindow::Enter()
{
	const char *sInput = pInput->value();
	if(sInput[0] == 0)
	{	//string is blank
		return false;
	}
	if(SIM_COMMAND_UPDATE(sInput))
	{
		pInput->value("");
		return true;
	}
	return false;
}

bool SimulatorWindow::Command(const string &scommand, bool fUpdate)
{
	string sCommand = scommand;

	//Check to see if we should run the translator on it
	if(TheProject.SimISA == LangLC3 && Flags.fOldLC3)
	{
		string sTemp;
		AsmConvertLC3Line(sCommand, sTemp, LocationVector(), MainWindow::SimMessageCallBack);
		sCommand = sTemp;
	}

	if(fUpdating)
		return TheSim(DoCommand(sCommand));

//	PlainText("sim> ");
	pTextDisplay->pStyleBuffer->append(string(5, 'N').c_str());
	pTextDisplay->pTextBuffer->append("sim> ");
//	PlainText(sCommand);
	pTextDisplay->pStyleBuffer->append(string(sCommand.size(), 'A').c_str());
	pTextDisplay->pTextBuffer->append(sCommand.c_str());
//	PlainText("\n");
	pTextDisplay->pStyleBuffer->append(string(1, 'A').c_str());
	pTextDisplay->pTextBuffer->append("\n");
	pTextDisplay->scroll((unsigned int)-1 >> 1, 0);

	uint64 OldIP = TheArch(NextInstruction());
	if(TheSim(DoCommand(sCommand)))
	{
		if(fUpdate)
		{
			TheSim(fFirstBreak) = true;
			if(OldIP == TheArch(NextInstruction()))
				fSupressHighlight = true;
			Update();
			fSupressHighlight = false;
		}
		return true;
	}
	return false;
}

bool SimulatorWindow::SimMessage(MessageEnum MessageType, const string &sMessage)
{
	ReadOnlyEditor *pTextDisplay = this->pTextDisplay;
	switch(RedirectWindow)
	{
	case SimCallStackWindow:
		pTextDisplay = ((CallStackWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimInstructionsWindow:
		pTextDisplay = ((InstructionsWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimDataValuesWindow:
		pTextDisplay = ((DataValuesWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimMemoryBytesWindow:
		pTextDisplay = ((MemoryBytesWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimRegistersWindow:
		pTextDisplay = ((RegistersWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimProgramsWindow:
		pTextDisplay = ((ProgramsWindow *)pRedirectWindow)->pTextDisplay;
		break;
	}

	switch(MessageType)
	{
	case Info:
		break;
	case Warning:
		MessageID(Warning);
		break;
	case Error:
		MessageID(Error);
		break;
	case Fatal:
		MessageID(Fatal);
		break;
	case Exception:
		MessageID(Exception);
		break;
	case Breakpoint:
		if(sMessage.find("Instruction limit.") == string::npos && sMessage.find("Cycle limit.") == string::npos)
			fl_beep(FL_BEEP_DEFAULT);
		MessageID(Breakpoint);
		break;
	case Check:
		MessageID(Check);
		break;
	}

	PlainText(sMessage);
	PlainText("\n");

	if(pTextDisplay == this->pTextDisplay)
		//*NOTE: The insert position is always at the beginning, even after appending
		//so show_insert_position() does nothing.
		pTextDisplay->scroll((unsigned int)-1 >> 1, 0);

	switch(MessageType)
	{
	case Exception:
		fl_alert(TheProject.SimISA == LangLC3 ? "LC3 processor exception." : "LC3b processor exception.");
		break;
	case Check:
		fl_alert("Runtime check failed. You can disable runtime checks from the main simulator window's Simulator menu.");
		break;
	}

	return true;
}

bool SimulatorWindow::PlainText(const string &sText)
{
	ReadOnlyEditor *pTextDisplay = this->pTextDisplay;
	switch(RedirectWindow)
	{
	case SimCallStackWindow:
		pTextDisplay = ((CallStackWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimInstructionsWindow:
		pTextDisplay = ((InstructionsWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimDataValuesWindow:
		pTextDisplay = ((DataValuesWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimMemoryBytesWindow:
		pTextDisplay = ((MemoryBytesWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimRegistersWindow:
		pTextDisplay = ((RegistersWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimProgramsWindow:
		pTextDisplay = ((ProgramsWindow *)pRedirectWindow)->pTextDisplay;
		break;
	}

	pTextDisplay->pStyleBuffer->append(string(sText.size(), 'A').c_str());
	pTextDisplay->pTextBuffer->append(sText.c_str());
	return true;
}

bool SimulatorWindow::MessageID(MessageEnum MessageType)
{
	ReadOnlyEditor *pTextDisplay = this->pTextDisplay;
	switch(RedirectWindow)
	{
	case SimCallStackWindow:
		pTextDisplay = ((CallStackWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimInstructionsWindow:
		pTextDisplay = ((InstructionsWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimDataValuesWindow:
		pTextDisplay = ((DataValuesWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimMemoryBytesWindow:
		pTextDisplay = ((MemoryBytesWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimRegistersWindow:
		pTextDisplay = ((RegistersWindow *)pRedirectWindow)->pTextDisplay;
		break;
	case SimProgramsWindow:
		pTextDisplay = ((ProgramsWindow *)pRedirectWindow)->pTextDisplay;
		break;
	}

	switch(MessageType)
	{
	case Info:
		pTextDisplay->pStyleBuffer->append(string(15, 'S').c_str());
		pTextDisplay->pTextBuffer->append("       Info:   ");
		break;
	case Warning:
		pTextDisplay->pStyleBuffer->append(string(15, 'T').c_str());
		pTextDisplay->pTextBuffer->append("    Warning:   ");
		break;
	case Error:
		pTextDisplay->pStyleBuffer->append(string(15, 'U').c_str());
		pTextDisplay->pTextBuffer->append("      Error:   ");
		break;
	case Fatal:
		pTextDisplay->pStyleBuffer->append(string(15, 'V').c_str());
		pTextDisplay->pTextBuffer->append("      Fatal:   ");
		break;
	case Exception:
		pTextDisplay->pStyleBuffer->append(string(15, 'W').c_str());
		pTextDisplay->pTextBuffer->append("  Exception:   ");
		break;
	case Breakpoint:
		pTextDisplay->pStyleBuffer->append(string(15, 'X').c_str());
		pTextDisplay->pTextBuffer->append(" Breakpoint:   ");
		break;
	case Check:
		pTextDisplay->pStyleBuffer->append(string(15, 'W').c_str());
		pTextDisplay->pTextBuffer->append("      Check:   ");
		break;
	}
	return true;
}

}	//namespace AshIDE
