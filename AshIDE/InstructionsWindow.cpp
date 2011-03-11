//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "InstructionsWindow.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include "Project.h"
#include "MainWindow.h"
#include "FileWindow.h"
#include "SimulatorWindow.h"

using namespace std;
using namespace JMT;
using namespace LC3;
using namespace LC3b;

namespace AshIDE	{

Fl_Menu_Item InstructionsWindow::MenuItems[] =
	{
		{ "&Instructions", 0, 0, 0, FL_SUBMENU },
			{ "&Save",			FL_CTRL + 's',	SaveCB, 0, FL_MENU_DIVIDER },
			{ "&Copy",			FL_CTRL + 'c',	CopyCB, 0, FL_MENU_DIVIDER },
			{ "&Help",			FL_F + 1,		HelpCB, 0, FL_MENU_DIVIDER },
			{ "C&lose",			FL_Escape,		CloseCB },
			{ 0 },

		{ "&Search", 0, 0, 0, FL_SUBMENU },
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
			{ "&End Simulation",				FL_CTRL + 'q',	SimulatorWindow::StopCB, 0, FL_MENU_DIVIDER },

			{ "&Breakpoints", 0, 0, 0, FL_SUBMENU },
				{ "&Breakpoints",	FL_CTRL + 'b',	SimulatorWindow::BreakpointsCB },
				{ 0 },

			{ "Vie&w", 0, 0, 0, FL_SUBMENU },
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

			{ 0 },

		{ 0 }
	};

/*
FL_WHITE
FL_BLACK
FL_CYAN
FL_DARK_CYAN
FL_BLUE
FL_DARK_BLUE
FL_GREEN
FL_DARK_GREEN
FL_YELLOW
FL_DARK_YELLOW
FL_MAGENTA
FL_DARK_MAGENTA
FL_RED
FL_DARK_RED
enum TokenEnum {TUnknown, TBad, TComment, TCharConst, TCharacter, TString, TInteger, TReal, TIdentifier, TOperator, TDirective, TAttribute, TExpand, TData, TISA, TOpcode, TRegister};
*/
Fl_Text_Display::Style_Table_Entry InstructionsWindow::StyleTable[] = 
	{	// Style table
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

InstructionsWindow::InstructionsWindow() : Fl_Double_Window(340, 300)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		pMainMenu->copy(MenuItems, this);

		//Setup the instrucitons display
		pTextDisplay = new ReadOnlyEditor(0, 28+20*3+16, w(), h()-28*2-20*3-16, StyleTable, sizeof(StyleTable)/sizeof(StyleTable[0]));
		pTextDisplay->pTextBuffer->add_modify_callback(UpdateStyleCB, this);

		//Setup the status bar
		pStatus = new Fl_Output(0, h()-28, w(), 28);
		pStatus->box(FL_PLASTIC_UP_BOX);

		//Setup the address query
		pLineInput = new Fl_Round_Button(0, 28, 120, 20, "Line Of Code");
		pLineInput->type(FL_RADIO_BUTTON);
		pLineInput->callback(ChangeInputCB, this);
		pLineNumber = new Fl_Input(220, 28, 80, 20, "Line Number");
		pLineNumber->deactivate();
		pLineNumber->value("1");

		pDataInput = new Fl_Round_Button(0, 28+20, 120, 20, "Instruction Address");
		pDataInput->type(FL_RADIO_BUTTON);
		pDataInput->callback(ChangeInputCB, this);
		pDataInput->value(1);

		pDataAddress = new Fl_Input(0, 28+20*2+16, 160, 20, TheProject.SimISA == LangLC3 ? "Symbol or Word Address" : "Symbol or Byte Address");
		pDataAddress->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
		pDataAddress->value("0");
		pLength = new Fl_Input(160, 28+20*2+16, 60, 20, "Length");
		pLength->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
		pLength->value("10");
		pEnter = new Fl_Return_Button(220, 28+20*2+16, 80, 20, "Display");
		pEnter->callback(EnterCB, this);

		ChangeInput();
	}
	end();
	resizable(pTextDisplay);
	callback(CloseCB, this);

	show();
	focus(pDataAddress);
	SetTitle();
	Enter();
}

InstructionsWindow::~InstructionsWindow()
{
	hide();
	SimulatorWindow::InstructionsMap::iterator InstructionIter = TheSimulatorWindow.Instructions.find(this);
	if(InstructionIter == TheSimulatorWindow.Instructions.end())
		throw "Instructions does not contain this window!";
	TheSimulatorWindow.Instructions.erase(InstructionIter);
	Fl::first_window()->show();
}

void InstructionsWindow::HelpCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.Help(AshIDEIndex, "InstructionsWindow");
}

bool InstructionsWindow::Close()
{
	Fl::delete_widget(this);
	return true;
}

bool InstructionsWindow::SetTitle()
{
	string sTitle = "Instructions";

	label(sTitle.c_str());
	return true;
}

bool InstructionsWindow::ChangeInput()
{
	if(pLineInput->value())
	{
		pLineNumber->activate();
		pDataAddress->deactivate();
	}
	else
	{
		pLineNumber->deactivate();
		pDataAddress->activate();
	}

	return true;
}

bool InstructionsWindow::Enter()
{
	bool fRetVal = true;

	if(pLineInput->value())
	{
		const char *sLineNumber = pLineNumber->value();
		const char *sLength = pLength->value();
		if(sLineNumber[0] == 0 || sLength[0] == 0)
		{
			fl_alert("You must provide a line number and display length.");
			return false;
		}
		string sCommand;
		sCommand = string("dil ")+sLineNumber+" "+sLength;
		TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimInstructionsWindow;
		TheSimulatorWindow.pRedirectWindow = this;
		pTextDisplay->Clear();
		fRetVal = SIM_COMMAND(sCommand);
		TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimNoWindow;
	}
	else
	{
		const char *sDataAddress = pDataAddress->value();
		const char *sLength = pLength->value();
		if(sDataAddress[0] == 0 || sLength[0] == 0)
		{
			fl_alert(TheProject.SimISA == LangLC3 ?
				"You must provide a symbol or word address identifier, and display length." :
				"You must provide a symbol or byte address identifier, and display length.");
			return false;
		}
		string sCommand;
		sCommand = string("di ")+sDataAddress+" "+sLength;
		TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimInstructionsWindow;
		TheSimulatorWindow.pRedirectWindow = this;
		pTextDisplay->Clear();
		fRetVal = SIM_COMMAND(sCommand);
		TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimNoWindow;
	}

	if(!fRetVal)
	{
		fl_alert("Error displaying instructions.");
		show();
	}
	return fRetVal;
}

bool InstructionsWindow::ViewInstruction(unsigned int ProgramNumber, unsigned int LineNumber)
{
	pLineInput->value(1);
	pDataInput->value(0);
	ChangeInput();
	char sProgramLine[32];
	sprintf(sProgramLine, "{%u} %u", ProgramNumber, LineNumber);
	pLineNumber->value(sProgramLine);
	return Enter();
}

bool InstructionsWindow::UpdateStyle(int Pos, int Inserted, int Deleted, int Restyled, const char *pszDeleted)
{
	int	Start, End;

	char *pText;

	//If this is just a selection change, then unselect the style buffer
	if(Inserted == 0 && Deleted == 0)
	{
		pTextDisplay->pStyleBuffer->unselect();
		return true;
	}

	//Select the area that was just updated to avoid unnecessary callbacks
	pTextDisplay->pStyleBuffer->select(Pos, Pos + Inserted - Deleted);

	//Re-parse the changed region; we do this by parsing from the
	//beginning of the line of the changed region to the end of
	//the line of the changed region.
	Start = pTextDisplay->pTextBuffer->line_start(Pos);
	End   = pTextDisplay->pTextBuffer->line_end(Pos + Inserted);
	string sText(pText = pTextDisplay->pTextBuffer->text_range(Start, End));
	delete [] pText;
	string sStyle(pText = pTextDisplay->pStyleBuffer->text_range(Start, End));
	delete [] pText;

	ParseStyle(sText, sStyle);

	pTextDisplay->pStyleBuffer->replace(Start, End, sStyle.c_str());
	pTextDisplay->redisplay_range(Start, End);
	return true;
}

bool InstructionsWindow::ParseStyle(const string &sText, string &sStyle)
{
	string::size_type TempLoc;

	//look for colons
	TempLoc = sText.find(':');
	if(TempLoc != string::npos && TempLoc != 0 && sStyle[TempLoc-1] != 'A')
		//This line was already styled
		return true;

	if(TheProject.SimISA == LangLC3)
		SimulatorWindow::TheLC3HighlightLexer.Lex(sText, sStyle);
	else if(TheProject.SimISA == LangLC3b)
		SimulatorWindow::TheLC3bHighlightLexer.Lex(sText, sStyle);

	return true;
}

}	//namespace AshIDE
