//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "WriteRegisterWindow.h"
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

Fl_Menu_Item WriteRegisterWindow::MenuItems[] =
	{
		{ "&Write Register", 0, 0, 0, FL_SUBMENU },
			{ "&Help",			FL_F + 1,		HelpCB, 0, FL_MENU_DIVIDER },
			{ "C&lose",			FL_Escape,		CloseCB },
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

WriteRegisterWindow::WriteRegisterWindow() : Fl_Double_Window(320, 28*2+20*2+16)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		pMainMenu->copy(MenuItems, this);

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

		pValue = new Fl_Input(0, 28+20+16, 170, 20, "Symbol or Value");
		pValue->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
		pValue->value("0");
		pEnter = new Fl_Return_Button(240, 28+20+16, 80, 20, "Edit");
		pEnter->callback(EnterCB, this);
	}
	end();
	callback(CloseCB, this);

	show();
	focus(pValue);
	SetTitle();
}

WriteRegisterWindow::~WriteRegisterWindow()
{
	hide();
	TheSimulatorWindow.pWriteRegisterWindow = NULL;
	Fl::first_window()->show();
}

void WriteRegisterWindow::HelpCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.Help(AshIDEIndex, "WriteRegisterWindow");
}

bool WriteRegisterWindow::Close()
{
	Fl::delete_widget(this);
	return true;
}

bool WriteRegisterWindow::SetTitle()
{
	string sTitle = "Write Register";

	label(sTitle.c_str());
	return true;
}

bool WriteRegisterWindow::ChangeRegisterSet()
{
	const char *sRegisterSet = pRegisterSet->text();
	if(!sRegisterSet)
		return false;

	//Find the corresponding register set object for the chosen menu selection
	Architecture::RegisterSetMap::iterator RegSetIter = TheArch(RegisterSets.find(sRegisterSet));

	//Reset the register display
	pRegister->clear();
	bool fPC = false;
	int i = 0;
	for(RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.begin(); RegIter != RegSetIter->second.Registers.end(); RegIter++)
	{
		pRegister->add(RegIter->second.sName.c_str());
		if(RegIter->second.sName == "pc")
		{
			pRegister->value(i);
			fPC = true;
		}
		i++;
	}
	if(!fPC)
		pRegister->value(0);

	return true;
}

bool WriteRegisterWindow::Enter()
{
	const char *sRegisterSet = pRegisterSet->text();
	const char *sRegister = pRegister->text();
	const char *sValue = pValue->value();
	if(!sRegisterSet || !sRegister)
	{
		fl_alert("Architecture does not contain any registers.");
		return false;
	}
	if(sValue[0] == 0)
	{
		fl_alert("You must provide a symbolic or numeric value.");
		return false;
	}

	string sCommand;
	sCommand = string("wr ")+sRegisterSet+"."+sRegister+" "+sValue;
	bool fRetVal = SIM_COMMAND_UPDATE(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error performing the edit.");
		TheSimulatorWindow.show();
	}
	return fRetVal;
}

bool WriteRegisterWindow::EditRegister(const string &sRegister)
{
	unsigned int RegisterSet = 0, Register;
	for(Architecture::RegisterSetMap::iterator RegSetIter = TheArch(RegisterSets.begin()); RegSetIter != TheArch(RegisterSets.end()); RegSetIter++)
	{
		Register = 0;
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

	return true;
}

}	//namespace AshIDE
