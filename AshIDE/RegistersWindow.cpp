//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "RegistersWindow.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include "Project.h"
#include "MainWindow.h"
#include "FileWindow.h"
#include "SimulatorWindow.h"

using namespace std;
using namespace JMT;
using namespace Assembler;
using namespace LC3;
using namespace LC3b;

namespace AshIDE	{

Fl_Menu_Item RegistersWindow::MenuItems[] =
	{
		{ "&Registers", 0, 0, 0, FL_SUBMENU },
			{ "List All &Registers",	FL_CTRL + 'r',	ListCB, 0, FL_MENU_DIVIDER },
			{ "&Save",					FL_CTRL + 's',	SaveCB, 0, FL_MENU_DIVIDER },
			{ "&Copy",					FL_CTRL + 'c',	CopyCB, 0, FL_MENU_DIVIDER },
			{ "&Help",					FL_F + 1,		HelpCB, 0, FL_MENU_DIVIDER },
			{ "C&lose",					FL_Escape,		CloseCB },
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
Fl_Text_Display::Style_Table_Entry RegistersWindow::StyleTable[] = 
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

RegistersWindow::RegistersWindow() : Fl_Double_Window(320, 230)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		pMainMenu->copy(MenuItems, this);

		//Setup the registers display
		pTextDisplay = new ReadOnlyEditor(0, 28+20, w(), h()-28*2-20, StyleTable, sizeof(StyleTable)/sizeof(StyleTable[0]));
		pTextDisplay->pTextBuffer->add_modify_callback(UpdateStyleCB, this);

		//Setup the status bar
		pStatus = new Fl_Output(0, h()-28, w(), 28);
		pStatus->box(FL_PLASTIC_UP_BOX);

		//Setup the address query
		pRegisterSet = new Fl_Choice(90, 28, 80, 20, "Register Set");
		pRegisterSet->box(FL_PLASTIC_DOWN_BOX);
		unsigned int i = 0;
		bool fRegFile = false;
		for(Architecture::RegisterSetMap::iterator RegSetIter = TheArch(RegisterSets.begin()); RegSetIter != TheArch(RegisterSets.end()); RegSetIter++)
		{
			pRegisterSet->add(RegSetIter->second.sName.c_str());
			if(RegSetIter->second.sName == "regfile")
			{
				pRegisterSet->value(i);
				fRegFile = true;
			}
			i++;
		}
		if(!fRegFile)
			pRegisterSet->value(0);
		pRegisterSet->callback(ChangeRegisterSetCB, this);
		pRegister = new Fl_Choice(240, 28, 80, 20, "Register");
		pRegister->box(FL_PLASTIC_DOWN_BOX);
		pRegister->callback(EnterCB, this);
		ChangeRegisterSet();
	}
	end();
	resizable(pTextDisplay);
	callback(CloseCB, this);

	//Setup the register display dialog
	pRegisterDlg = new Fl_Window(300, TheArch(RegisterSets.size())*20+20, "Registers");
	{
		Fl_Box *pBox = new Fl_Box(0, 0, 100, 20, "Register Set:");
		pBox->labelfont(pBox->labelfont() | FL_BOLD);
		pBox = new Fl_Box(100, 0, 200, 20, "Contains Registers:");
		pBox->labelfont(pBox->labelfont() | FL_BOLD);
		unsigned int i = 1;
		for(Architecture::RegisterSetMap::iterator RegSetIter = TheArch(RegisterSets.begin()); RegSetIter != TheArch(RegisterSets.end()); RegSetIter++)
		{
			pBox = new Fl_Box(0, i*20, 100, 20, RegSetIter->second.sName.c_str());
			sRegisterText.push_back(string());
			for(RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.begin(); RegIter != RegSetIter->second.Registers.end(); RegIter++)
				*sRegisterText.rbegin() += RegIter->second.sName+" ";
			pBox = new Fl_Box(100, i*20, 200, 20, sRegisterText.rbegin()->c_str());
			i++;
		}
	}
	pRegisterDlg->end();

	show();
	SetTitle();
}

RegistersWindow::~RegistersWindow()
{
	hide();
	SimulatorWindow::RegistersMap::iterator RegisterIter = TheSimulatorWindow.Registers.find(this);
	if(RegisterIter == TheSimulatorWindow.Registers.end())
		throw "Registers does not contain this window!";
	TheSimulatorWindow.Registers.erase(RegisterIter);
	delete pRegisterDlg;
	Fl::first_window()->show();
}

void RegistersWindow::HelpCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.Help(AshIDEIndex, "RegistersWindow");
}

bool RegistersWindow::Close()
{
	Fl::delete_widget(this);
	return true;
}

bool RegistersWindow::SetTitle()
{
	string sTitle = "Registers";

	label(sTitle.c_str());
	return true;
}

bool RegistersWindow::ChangeRegisterSet()
{
	const char *sRegisterSet = pRegisterSet->text();
	if(!sRegisterSet)
		return false;

	//Find the corresponding register set object for the chosen menu selection
	Architecture::RegisterSetMap::iterator RegSetIter = TheArch(RegisterSets.find(sRegisterSet));

	//Reset the register display
	pRegister->clear();
	pRegister->add("All");
	for(RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.begin(); RegIter != RegSetIter->second.Registers.end(); RegIter++)
		pRegister->add(RegIter->second.sName.c_str());
	pRegister->value(0);

	Enter();
	return true;
}

bool RegistersWindow::List()
{
	pRegisterDlg->show();
	return true;
}

bool RegistersWindow::Enter()
{
	bool fRetVal = true;

	const char *sRegisterSet = pRegisterSet->text();
	const char *sRegister = pRegister->text();
	if(!sRegisterSet || !sRegister)
	{
		fl_alert("Architecture does not contain any registers.");
		return false;
	}
	if(string(sRegister) == "All")
	{
		string sCommand;
		sCommand = string("drs ")+sRegisterSet;
		TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimRegistersWindow;
		TheSimulatorWindow.pRedirectWindow = this;
		pTextDisplay->Clear();
		fRetVal = SIM_COMMAND(sCommand);
		TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimNoWindow;
	}
	else
	{
		string sCommand;
		sCommand = string("dr ")+sRegisterSet+"."+sRegister;
		TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimRegistersWindow;
		TheSimulatorWindow.pRedirectWindow = this;
		pTextDisplay->Clear();
		fRetVal = SIM_COMMAND(sCommand);
		TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimNoWindow;
	}

	return fRetVal;
}

bool RegistersWindow::ViewRegister(const string &sRegister)
{
	unsigned int RegisterSet = 0, Register;
	for(Architecture::RegisterSetMap::iterator RegSetIter = TheArch(RegisterSets.begin()); RegSetIter != TheArch(RegisterSets.end()); RegSetIter++)
	{
		Register = 1;
		for(RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.begin(); RegIter != RegSetIter->second.Registers.end(); RegIter++)
		{
			if(ToLower(RegIter->first) == ToLower(sRegister))
				goto Done;
			Register++;
		}
		RegisterSet++;
	}
	fl_alert("Architecture does not contain this register.");
	return false;

Done:
	pRegisterSet->value(RegisterSet);
	ChangeRegisterSet();
	pRegister->value(Register);

	return Enter();
}

bool RegistersWindow::UpdateStyle(int Pos, int Inserted, int Deleted, int Restyled, const char *pszDeleted)
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

bool RegistersWindow::ParseStyle(const string &sText, string &sStyle)
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
