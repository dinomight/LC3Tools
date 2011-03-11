//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "BreakpointWindow.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include "Project.h"
#include "MainWindow.h"
#include "FileWindow.h"
#include "SimulatorWindow.h"
#include <strstream>

using namespace std;
using namespace JMT;
using namespace Assembler;
using namespace LC3;
using namespace LC3b;

namespace AshIDE	{

const char *const BreakpointWindow::_sEventTypes[4] = {"READEVENT", "WRITEEVENT", "CHANGEEVENT", "VALUEEVENT"};


Fl_Menu_Item BreakpointWindow::MenuItems[] =
	{
		{ "&Breakpoints", 0, 0, 0, FL_SUBMENU },
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

Fl_Window *BreakpointWindow::pDataDlg = NULL;
Fl_Input *BreakpointWindow::pDataAddressDlg = NULL, *BreakpointWindow::pDataValueDlg = NULL;
Fl_Choice *BreakpointWindow::pDataEventDlg = NULL, *BreakpointWindow::pDataDataTypeDlg = NULL;
Fl_Return_Button *BreakpointWindow::pDataAddDlg = NULL;
Fl_Button *BreakpointWindow::pDataCancelDlg = NULL;
Fl_Window *BreakpointWindow::pRegDlg = NULL;
Fl_Input *BreakpointWindow::pRegValueDlg = NULL, *BreakpointWindow::pRegisterSetDlg = NULL, *BreakpointWindow::pRegisterDlg = NULL;
Fl_Choice *BreakpointWindow::pRegEventDlg = NULL;
Fl_Return_Button *BreakpointWindow::pRegAddDlg = NULL;
Fl_Button *BreakpointWindow::pRegCancelDlg = NULL;

const int Fudge = 16;

BreakpointWindow::BreakpointWindow() : Fl_Double_Window(308+Fudge*2, 304+Fudge*2)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		pMainMenu->copy(MenuItems, this);

		//Setup the status bar
		pStatus = new Fl_Output(0, h()-28, w(), 28);
		pStatus->box(FL_PLASTIC_UP_BOX);

		//Setup the tab display
		pTabs = new Fl_Tabs(0+Fudge, 28+Fudge, w()-Fudge*2, h()-28*2-Fudge*2);
		{
			int x = pTabs->x()+4, y = pTabs->y()+20+4, w = pTabs->w()-4*2, h = pTabs->h()-20-4*2;
			//Setup the instruction breakpoints
			pInstrTab = new Fl_Group(x-4, y-4, w+4*2, h+4*2, "Instruction");
			{
				//Setup the address query
				pLineInput = new Fl_Round_Button(x, y, 200, 20);//, "Line Of Code");
				pLineInput->type(FL_RADIO_BUTTON);
				pLineInput->callback(ChangeInputCB, this);
				pLineNumber = new Fl_Input(x+200, y, 100, 20, "Line Number");
				pLineNumber->deactivate();
				pLineNumber->value("1");

				pInstrInput = new Fl_Round_Button(x, y+20, 200, 20);
				pInstrInput->type(FL_RADIO_BUTTON);
				pInstrInput->callback(ChangeInputCB, this);
				pInstrInput->value(1);
				pInstrAddress = new Fl_Input(x+200, y+20, 100, 20, TheProject.SimISA == LangLC3 ? "Symbol or Word Address" : "Symbol or Byte Address");

				pInstrAdd = new Fl_Return_Button(x, y+20*2, 100, 20, "Add");
				pInstrAdd->callback(AddCB, this);
				pInstrRemove = new Fl_Button(x+100, y+20*2, 100, 20, "Remove");
				pInstrRemove->callback(RemoveCB, this);
				pInstrClear = new Fl_Button(x+200, y+20*2, 100, 20, "Clear");
				pInstrClear->callback(ClearCB, this);
				
				pInstrBreakpoints = new Fl_Hold_Browser(x, y+20*4, 300, 140, "Instruction Breakpoints:");
				pInstrBreakpoints->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			}
			pInstrTab->end();
			pInstrTab->resizable(pInstrBreakpoints);

			//Setup the data breakpoints
			pDataTab = new Fl_Group(x-4, y-4, w+4*2, h+4*2, "Data");
			{
				pDataAddress = new Fl_Input(x+200, y, 120, 20, TheProject.SimISA == LangLC3 ? "Symbol or Word Address" : "Symbol or Byte Address");

				pDataDataType = new Fl_Choice(x+200, y+20, 100, 20, "Data Type");
				pDataDataType->box(FL_PLASTIC_DOWN_BOX);
				pDataDataType->add("DATA1");
				pDataDataType->add("DATA2");
				pDataDataType->add("DATA4");
				pDataDataType->add("DATA8");
				pDataDataType->add("REAL1");
				pDataDataType->add("REAL2");
				pDataDataType->add("REAL4");
				pDataDataType->add("REAL8");
				pDataDataType->add("Auto");
				pDataDataType->value(STRUCT);

				pDataEvent = new Fl_Choice(x+200, y+20*2, 100, 20, "Event");
				pDataEvent->box(FL_PLASTIC_DOWN_BOX);
				pDataEvent->callback(ChangeEventCB, this);
				pDataEvent->add("Read");
				pDataEvent->add("Write");
				pDataEvent->add("Change");
				pDataEvent->add("Value");
				pDataEvent->value(2);

				pDataValue = new Fl_Input(x+200, y+20*3, 100, 20, "Symbol or Value");

				pDataAdd = new Fl_Return_Button(x, y+20*4, 100, 20, "Add");
				pDataAdd->callback(AddCB, this);
				pDataRemove = new Fl_Button(x+100, y+20*4, 100, 20, "Remove");
				pDataRemove->callback(RemoveCB, this);
				pDataClear = new Fl_Button(x+200, y+20*4, 100, 20, "Clear");
				pDataClear->callback(ClearCB, this);
				
				pDataBreakpoints = new Fl_Hold_Browser(x, y+20*6, 300, 100, "Data Breakpoints:");
				pDataBreakpoints->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			}
			pDataTab->end();
			pDataTab->resizable(pDataBreakpoints);

			//Setup the memory breakpoints
			pMemTab = new Fl_Group(x-4, y-4, w+4*2, h+4*2, "Memory");
			{
				pMemoryName = new Fl_Choice(x+200, y, 100, 20, "Memory");
				pMemoryName->box(FL_PLASTIC_DOWN_BOX);
				for(Architecture::MemoryMap::iterator MemIter = TheArch(Memories.begin()); MemIter != TheArch(Memories.end()); MemIter++)
					pMemoryName->add(MemIter->second.sName.c_str());
				pMemoryName->value(0);

				pMemAddress = new Fl_Input(x+200, y+20, 100, 20, "Symbol or Byte Address");

				pMemDataType = new Fl_Choice(x+200, y+20*2, 100, 20, "Data Type");
				pMemDataType->box(FL_PLASTIC_DOWN_BOX);
				pMemDataType->add("DATA1");
				pMemDataType->add("DATA2");
				pMemDataType->add("DATA4");
				pMemDataType->add("DATA8");
				pMemDataType->add("REAL1");
				pMemDataType->add("REAL2");
				pMemDataType->add("REAL4");
				pMemDataType->add("REAL8");
				pMemDataType->add("Auto");
				pMemDataType->value(STRUCT);

				pMemEvent = new Fl_Choice(x+200, y+20*3, 100, 20, "Event");
				pMemEvent->box(FL_PLASTIC_DOWN_BOX);
				pMemEvent->callback(ChangeEventCB, this);
				pMemEvent->add("Read");
				pMemEvent->add("Write");
				pMemEvent->add("Change");
				pMemEvent->add("Value");
				pMemEvent->value(2);

				pMemValue = new Fl_Input(x+200, y+20*4, 100, 20, "Symbol or Value");

				pMemAdd = new Fl_Return_Button(x, y+20*5, 100, 20, "Add");
				pMemAdd->callback(AddCB, this);
				pMemRemove = new Fl_Button(x+100, y+20*5, 100, 20, "Remove");
				pMemRemove->callback(RemoveCB, this);
				pMemClear = new Fl_Button(x+200, y+20*5, 100, 20, "Clear");
				pMemClear->callback(ClearCB, this);
				
				pMemBreakpoints = new Fl_Hold_Browser(x, y+20*7, 300, 80, "Memory Breakpoints:");
				pMemBreakpoints->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			}
			pMemTab->end();
			pMemTab->resizable(pMemBreakpoints);

			//Setup the register breakpoints
			pRegTab = new Fl_Group(x-4, y-4, w+4*2, h+4*2, "Register");
			{
				pRegisterSet = new Fl_Choice(x+200, y, 100, 20, "Register Set");
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
				pRegister = new Fl_Choice(x+200, y+20, 100, 20, "Register");
				pRegister->box(FL_PLASTIC_DOWN_BOX);
				ChangeRegisterSet();

				pRegEvent = new Fl_Choice(x+200, y+20*2, 100, 20, "Event");
				pRegEvent->box(FL_PLASTIC_DOWN_BOX);
				pRegEvent->callback(ChangeEventCB, this);
				pRegEvent->add("Read");
				pRegEvent->add("Write");
				pRegEvent->add("Change");
				pRegEvent->add("Value");
				pRegEvent->value(2);

				pRegValue = new Fl_Input(x+200, y+20*3, 100, 20, "Symbol or Value");

				pRegAdd = new Fl_Return_Button(x, y+20*4, 100, 20, "Add");
				pRegAdd->callback(AddCB, this);
				pRegRemove = new Fl_Button(x+100, y+20*4, 100, 20, "Remove");
				pRegRemove->callback(RemoveCB, this);
				pRegClear = new Fl_Button(x+200, y+20*4, 100, 20, "Clear");
				pRegClear->callback(ClearCB, this);
				
				pRegBreakpoints = new Fl_Hold_Browser(x, y+20*6, 300, 100, "Register Breakpoints:");
				pRegBreakpoints->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			}
			pRegTab->end();
			pRegTab->resizable(pRegBreakpoints);

			ChangeInput();
			ChangeEvent();
		}
		pTabs->end();
	}
	end();
	callback(CloseCB, this);
	resizable(pTabs);

	show();
	focus(pInstrAddress);
	SetTitle();
	Refresh();
}

BreakpointWindow::~BreakpointWindow()
{
	hide();
	TheSimulatorWindow.pBreakpointWindow = NULL;
	Fl::first_window()->show();
}

void BreakpointWindow::HelpCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.Help(AshIDEIndex, "BreakpointWindow");
}

bool BreakpointWindow::Close()
{
	Fl::delete_widget(this);
	return true;
}

bool BreakpointWindow::InitBreakpointDlg()
{
	//Setup the quick data breakpoint dialog
	int x = 4, y = 4, w = 300, h = 100;
	pDataDlg = new Fl_Window(x-4, y-4, w+4*2, h+4*2, "Data");
	{
		pDataAddressDlg = new Fl_Input(x+200, y, 120, 20, TheProject.SimISA == LangLC3 ? "Symbol or Word Address" : "Symbol or Byte Address");
		pDataAddressDlg->deactivate();

		pDataDataTypeDlg = new Fl_Choice(x+200, y+20, 100, 20, "Data Type");
		pDataDataTypeDlg->box(FL_PLASTIC_DOWN_BOX);
		pDataDataTypeDlg->add("DATA1");
		pDataDataTypeDlg->add("DATA2");
		pDataDataTypeDlg->add("DATA4");
		pDataDataTypeDlg->add("DATA8");
		pDataDataTypeDlg->add("REAL1");
		pDataDataTypeDlg->add("REAL2");
		pDataDataTypeDlg->add("REAL4");
		pDataDataTypeDlg->add("REAL8");
		pDataDataTypeDlg->add("Auto");
		pDataDataTypeDlg->value(STRUCT);

		pDataEventDlg = new Fl_Choice(x+200, y+20*2, 100, 20, "Event");
		pDataEventDlg->box(FL_PLASTIC_DOWN_BOX);
		pDataEventDlg->callback(ChangeEventDlgCB, NULL);
		pDataEventDlg->add("Read");
		pDataEventDlg->add("Write");
		pDataEventDlg->add("Change");
		pDataEventDlg->add("Value");
		pDataEventDlg->value(2);

		pDataValueDlg = new Fl_Input(x+200, y+20*3, 100, 20, "Symbol or Value");

		pDataAddDlg = new Fl_Return_Button(x, y+20*4, 100, 20, "Add");
		pDataCancelDlg = new Fl_Button(x+200, y+20*4, 100, 20, "Cancel");
	}
	pDataDlg->end();
	pDataDlg->set_modal();

	//Setup the quick register breakpoint dialog
	pRegDlg = new Fl_Window(x-4, y-4, w+4*2, h+4*2, "Register");
	{
		pRegisterSetDlg = new Fl_Input(x+200, y, 100, 20, "Register Set");
		pRegisterSetDlg->deactivate();
		pRegisterDlg = new Fl_Input(x+200, y+20, 100, 20, "Register");
		pRegisterDlg->deactivate();

		pRegEventDlg = new Fl_Choice(x+200, y+20*2, 100, 20, "Event");
		pRegEventDlg->box(FL_PLASTIC_DOWN_BOX);
		pRegEventDlg->callback(ChangeEventDlgCB, NULL);
		pRegEventDlg->add("Read");
		pRegEventDlg->add("Write");
		pRegEventDlg->add("Change");
		pRegEventDlg->add("Value");
		pRegEventDlg->value(2);

		pRegValueDlg = new Fl_Input(x+200, y+20*3, 100, 20, "Symbol or Value");

		pRegAddDlg = new Fl_Return_Button(x, y+20*4, 100, 20, "Add");
		pRegCancelDlg = new Fl_Button(x+200, y+20*4, 100, 20, "Cancel");
	}
	pRegDlg->end();
	pRegDlg->set_modal();

	ChangeEventDlgCB(NULL, NULL);

	return true;
}

bool BreakpointWindow::CloseBreakpointDlg()
{
	if(pDataDlg)
		delete pDataDlg;
	pDataDlg = NULL;
	if(pRegDlg)
		delete pRegDlg;
	pRegDlg = NULL;
	return true;
}

bool BreakpointWindow::Refresh()
{
	char sBuff[32];
	switch(TheProject.SimISA)
	{
	case LangLC3:
		{
		pInstrBreakpoints->clear();
		for(ArchSim<LC3ISA>::InstrMap::iterator InstrIter = TheProject.pLC3Sim->InstrBreakpoints.begin(); InstrIter != TheProject.pLC3Sim->InstrBreakpoints.end(); InstrIter++)
		{
			ostrstream strBreakpoints;
			if(InstrIter->second.second.first)
				strBreakpoints << " " << InputList[InstrIter->second.second.second].c_str() << "(" << InstrIter->second.second.third << "):\t";
			strBreakpoints << (const char *)*InstrIter->second.first;
			if(InstrIter->second.third == GotoEvent)
				strBreakpoints << " GOTO";
			strBreakpoints << ends;
			pInstrBreakpoints->add(strBreakpoints.str());
		}

		pDataBreakpoints->clear();
		for(ArchSim<LC3ISA>::DataMap::iterator DataIter = TheProject.pLC3Sim->DataBreakpoints.begin(); DataIter != TheProject.pLC3Sim->DataBreakpoints.end(); DataIter++)
		{
			ostrstream strBreakpoints;
			if(DataIter->second.third.first)
				strBreakpoints << InputList[DataIter->second.third.second].c_str() << "(" << DataIter->second.third.third << "):\t";
			strBreakpoints << (const char *)*DataIter->second.first << (const char *)*DataIter->second.fourth << ends;
			pDataBreakpoints->add(strBreakpoints.str());
		}

		pMemBreakpoints->clear();
		for(ArchSim<LC3ISA>::MemoryMap::iterator MemIter = TheProject.pLC3Sim->MemoryBreakpoints.begin(); MemIter != TheProject.pLC3Sim->MemoryBreakpoints.end(); MemIter++)
		{
			for(ArchSim<LC3ISA>::MemoryAddrMap::iterator MemAddrIter = MemIter->second.begin(); MemAddrIter != MemIter->second.end(); MemAddrIter++)
			{
				ostrstream strBreakpoints;
				#if defined _MSC_VER
					sprintf(sBuff, "[4x%I64X]", MemAddrIter->first);
				#elif defined GPLUSPLUS
					sprintf(sBuff, "[4x%llX]", MemAddrIter->first);
				#else
					#error "Only MSVC and GCC Compilers Supported"
				#endif
				strBreakpoints << MemIter->first.c_str() << sBuff << (const char *)*MemAddrIter->second.second << ends;
				pMemBreakpoints->add(strBreakpoints.str());
			}
		}

		pRegBreakpoints->clear();
		for(ArchSim<LC3ISA>::RegisterSetMap::iterator RegSetIter = TheProject.pLC3Sim->RegisterBreakpoints.begin(); RegSetIter != TheProject.pLC3Sim->RegisterBreakpoints.end(); RegSetIter++)
		{
			for(ArchSim<LC3ISA>::RegisterMap::iterator RegIter = RegSetIter->second.begin(); RegIter != RegSetIter->second.end(); RegIter++)
			{
				ostrstream strBreakpoints;
				strBreakpoints << RegSetIter->first.c_str() << "." << RegIter->first.c_str() << (const char *)*RegIter->second << ends;
				pRegBreakpoints->add(strBreakpoints.str());
			}
		}
		}
		break;
	case LangLC3b:
		{
		pInstrBreakpoints->clear();
		for(ArchSim<LC3bISA>::InstrMap::iterator InstrIter = TheProject.pLC3bSim->InstrBreakpoints.begin(); InstrIter != TheProject.pLC3bSim->InstrBreakpoints.end(); InstrIter++)
		{
			ostrstream strBreakpoints;
			if(InstrIter->second.second.first)
				strBreakpoints << " " << InputList[InstrIter->second.second.second].c_str() << "(" << InstrIter->second.second.third << "):\t";
			strBreakpoints << (const char *)*InstrIter->second.first;
			if(InstrIter->second.third == GotoEvent)
				strBreakpoints << " GOTO";
			strBreakpoints << ends;
			pInstrBreakpoints->add(strBreakpoints.str());
		}

		pDataBreakpoints->clear();
		for(ArchSim<LC3bISA>::DataMap::iterator DataIter = TheProject.pLC3bSim->DataBreakpoints.begin(); DataIter != TheProject.pLC3bSim->DataBreakpoints.end(); DataIter++)
		{
			ostrstream strBreakpoints;
			if(DataIter->second.third.first)
				strBreakpoints << InputList[DataIter->second.third.second].c_str() << "(" << DataIter->second.third.third << "):\t";
			strBreakpoints << (const char *)*DataIter->second.first << (const char *)*DataIter->second.fourth << ends;
			pDataBreakpoints->add(strBreakpoints.str());
		}

		pMemBreakpoints->clear();
		for(ArchSim<LC3bISA>::MemoryMap::iterator MemIter = TheProject.pLC3bSim->MemoryBreakpoints.begin(); MemIter != TheProject.pLC3bSim->MemoryBreakpoints.end(); MemIter++)
		{
			for(ArchSim<LC3bISA>::MemoryAddrMap::iterator MemAddrIter = MemIter->second.begin(); MemAddrIter != MemIter->second.end(); MemAddrIter++)
			{
				ostrstream strBreakpoints;
				#if defined _MSC_VER
					sprintf(sBuff, "[4x%I64X]", MemAddrIter->first);
				#elif defined GPLUSPLUS
					sprintf(sBuff, "[4x%llX]", MemAddrIter->first);
				#else
					#error "Only MSVC and GCC Compilers Supported"
				#endif
				strBreakpoints << MemIter->first.c_str() << sBuff << (const char *)*MemAddrIter->second.second << ends;
				pMemBreakpoints->add(strBreakpoints.str());
			}
		}

		pRegBreakpoints->clear();
		for(ArchSim<LC3bISA>::RegisterSetMap::iterator RegSetIter = TheProject.pLC3bSim->RegisterBreakpoints.begin(); RegSetIter != TheProject.pLC3bSim->RegisterBreakpoints.end(); RegSetIter++)
		{
			for(ArchSim<LC3bISA>::RegisterMap::iterator RegIter = RegSetIter->second.begin(); RegIter != RegSetIter->second.end(); RegIter++)
			{
				ostrstream strBreakpoints;
				strBreakpoints << RegSetIter->first.c_str() << "." << RegIter->first.c_str() << (const char *)*RegIter->second << ends;
				pRegBreakpoints->add(strBreakpoints.str());
			}
		}
		}
		break;
	}
	return true;
}

bool BreakpointWindow::SetTitle()
{
	string sTitle = "Breakpoints";

	label(sTitle.c_str());
	return true;
}

bool BreakpointWindow::ChangeInput()
{
	if(pLineInput->value())
	{
		pLineNumber->activate();
		pInstrAddress->deactivate();
	}
	else
	{
		pLineNumber->deactivate();
		pInstrAddress->activate();
	}

	return true;
}

bool BreakpointWindow::ChangeEvent()
{
	if(pDataEvent->value() == 3)
		pDataValue->activate();
	else
		pDataValue->deactivate();

	if(pMemEvent->value() == 3)
		pMemValue->activate();
	else
		pMemValue->deactivate();

	if(pRegEvent->value() == 3)
		pRegValue->activate();
	else
		pRegValue->deactivate();

	return true;
}

void BreakpointWindow::ChangeEventDlgCB(Fl_Widget *pW, void *pV)
{
	if(pDataEventDlg->value() == 3)
		pDataValueDlg->activate();
	else
		pDataValueDlg->deactivate();

	if(pRegEventDlg->value() == 3)
		pRegValueDlg->activate();
	else
		pRegValueDlg->deactivate();
}

bool BreakpointWindow::ChangeRegisterSet()
{
	const char *sRegisterSet = pRegisterSet->text();
	if(!sRegisterSet)
		return false;

	//Find the corresponding register set object for the chosen menu selection
	Architecture::RegisterSetMap::iterator RegSetIter = TheArch(RegisterSets.find(sRegisterSet));

	//Reset the register display
	pRegister->clear();
	for(RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.begin(); RegIter != RegSetIter->second.Registers.end(); RegIter++)
		pRegister->add(RegIter->second.sName.c_str());
	pRegister->value(0);

	return true;
}

bool BreakpointWindow::Add()
{
	bool fRetVal = true;

	if(pTabs->value() == pInstrTab)	//Instruction
	{
		if(pLineInput->value())
		{
			const char *sLineNumber = pLineNumber->value();
			if(sLineNumber[0] == 0)
			{
				fl_alert("You must provide a line number.");
				return false;
			}
			string sCommand;
			sCommand = string("bpl ")+sLineNumber;
			fRetVal = SIM_COMMAND(sCommand);
		}
		else
		{
			const char *sInstrAddress = pInstrAddress->value();
			if(sInstrAddress[0] == 0)
			{
				fl_alert(TheProject.SimISA == LangLC3 ?
					"You must provide a symbol or word address identifier." :
					"You must provide a symbol or byte address identifier.");
				return false;
			}
			string sCommand;
			sCommand = string("bpi ")+sInstrAddress;
			fRetVal = SIM_COMMAND(sCommand);
		}
	}
	else if(pTabs->value() == pDataTab)	//Data
	{
		const char *sDataAddress = pDataAddress->value();
		const char *sEvent = _sEventTypes[pDataEvent->value()];
		DataEnum DataType = (DataEnum)pDataDataType->value();
		if(sDataAddress[0] == 0)
		{
			fl_alert("You must provide a symbol or byte address identifier.");
			return false;
		}
		if(!stricmp(sEvent, "valueevent"))
		{
			const char *sValue = pDataValue->value();
			if(sValue[0] == 0)
			{
				fl_alert("You must provide a symbol or value for a VALUEEVENT breakpoint.");
				return false;
			}
			string sCommand;
			sCommand = string("bpd ")+sDataAddress+" ";
			if(DataType < STRUCT)
				(sCommand += sDataTypes[DataType]) += " ";
			((sCommand+=sEvent)+=" ")+=sValue;
			fRetVal = SIM_COMMAND(sCommand);
		}
		else
		{
			string sCommand;
			sCommand = string("bpd ")+sDataAddress+" ";
			if(DataType < STRUCT)
				(sCommand += sDataTypes[DataType]) += " ";
			sCommand += sEvent;
			fRetVal = SIM_COMMAND(sCommand);
		}
	}
	else if(pTabs->value() == pMemTab)	//Memory
	{
		const char *sMemoryName = pMemoryName->text();
		const char *sDataAddress = pMemAddress->value();
		const char *sEvent = _sEventTypes[pMemEvent->value()];
		DataEnum DataType = (DataEnum)pMemDataType->value();
		if(!sMemoryName)
		{
			fl_alert("Architecture does not contain any memories.");
			return false;
		}
		if(sDataAddress[0] == 0)
		{
			fl_alert("You must provide a memory name and symbol or byte address identifier.");
			return false;
		}
		if(!stricmp(sEvent, "valueevent"))
		{
			const char *sValue = pMemValue->value();
			if(sValue[0] == 0)
			{
				fl_alert("You must provide a symbol or value for a VALUEEVENT breakpoint.");
				return false;
			}
			string sCommand;
			sCommand = string("bpm ")+sMemoryName+" "+sDataAddress+" ";
			if(DataType < STRUCT)
				(sCommand += sDataTypes[DataType]) += " ";
			((sCommand+=sEvent)+=" ")+=sValue;
			fRetVal = SIM_COMMAND(sCommand);
		}
		else
		{
			string sCommand;
			sCommand = string("bpm ")+sMemoryName+" "+sDataAddress+" ";
			if(DataType < STRUCT)
				(sCommand += sDataTypes[DataType]) += " ";
			sCommand += sEvent;
			fRetVal = SIM_COMMAND(sCommand);
		}
	}
	else if(pTabs->value() == pRegTab)	//Register
	{
		const char *sRegisterSet = pRegisterSet->text();
		const char *sRegister = pRegister->text();
		const char *sEvent = _sEventTypes[pRegEvent->value()];
		if(!sRegisterSet || !sRegister)
		{
			fl_alert("Architecture does not contain any registers.");
			return false;
		}
		if(!stricmp(sEvent, "valueevent"))
		{
			const char *sValue = pRegValue->value();
			if(sValue[0] == 0)
			{
				fl_alert("You must provide a symbol or value for a VALUEEVENT breakpoint.");
				return false;
			}
			string sCommand;
			sCommand = string("bpr ")+sRegisterSet+"."+sRegister+" "+sEvent+" "+sValue;
			fRetVal = SIM_COMMAND(sCommand);
		}
		else
		{
			string sCommand;
			sCommand = string("bpr ")+sRegisterSet+"."+sRegister+" "+sEvent;
			fRetVal = SIM_COMMAND(sCommand);
		}
	}

	if(!fRetVal)
	{
		fl_alert("Error inserting the breakpoint.");
		TheSimulatorWindow.show();
	}
	else
		Refresh();

	return fRetVal;
}

bool BreakpointWindow::Remove()
{
	bool fRetVal = true;
	char sBuff[16];

	switch(TheProject.SimISA)
	{
	case LangLC3:
		if(pTabs->value() == pInstrTab)	//Instruction
		{
			int i = 1, Index = pInstrBreakpoints->value();
			ArchSim<LC3ISA>::InstrMap::iterator InstrIter;
			if(Index == 0)
			{
				fl_alert("You must select a breakpoint to remove.");
				return false;
			}

			for(InstrIter = TheProject.pLC3Sim->InstrBreakpoints.begin(); InstrIter != TheProject.pLC3Sim->InstrBreakpoints.end(); InstrIter++)
				if(i++ == Index)
					break;

			string sCommand;
			if(InstrIter->second.second.first)
			{
				sprintf(sBuff, "%u", InstrIter->second.second.second);
				sCommand = string("bpi ")+" {"+sBuff+"} "+(const char *)*InstrIter->second.first+" "+sEventTypes[NoEvent];
			}
			else
				sCommand = string("bpi ")+(const char *)*InstrIter->second.first+" "+sEventTypes[NoEvent];
			fRetVal = SIM_COMMAND(sCommand);
		}
		else if(pTabs->value() == pDataTab)	//Data
		{
			int i = 1, Index = pDataBreakpoints->value();
			ArchSim<LC3ISA>::DataMap::iterator DataIter;
			if(Index == 0)
			{
				fl_alert("You must select a breakpoint to remove.");
				return false;
			}

			for(DataIter = TheProject.pLC3Sim->DataBreakpoints.begin(); DataIter != TheProject.pLC3Sim->DataBreakpoints.end(); DataIter++)
				if(i++ == Index)
					break;

			string sCommand;
			if(DataIter->second.third.first)
			{
				sprintf(sBuff, "%u", DataIter->second.third.second);
				sCommand = string("bpd ")+" {"+sBuff+"} "+(const char *)*DataIter->second.first+" "+sEventTypes[NoEvent];
			}
			else
				sCommand = string("bpd ")+(const char *)*DataIter->second.first+" "+sEventTypes[NoEvent];
			fRetVal = SIM_COMMAND(sCommand);
		}
		else if(pTabs->value() == pMemTab)	//Memory
		{
			int i = 1, Index = pMemBreakpoints->value();
			ArchSim<LC3ISA>::MemoryMap::iterator MemIter;
			ArchSim<LC3ISA>::MemoryAddrMap::iterator MemAddrIter;
			if(Index == 0)
			{
				fl_alert("You must select a breakpoint to remove.");
				return false;
			}

			for(MemIter = TheProject.pLC3Sim->MemoryBreakpoints.begin(); MemIter != TheProject.pLC3Sim->MemoryBreakpoints.end(); MemIter++)
			{
				for(MemAddrIter = MemIter->second.begin(); MemAddrIter != MemIter->second.end(); MemAddrIter++);
					if(i++ == Index)
						break;
				if(i > Index)
					break;
			}

			string sCommand;
			#if defined _MSC_VER
				sprintf(sBuff, "4x%I64X", MemAddrIter->first);
			#elif defined GPLUSPLUS
				sprintf(sBuff, "4x%llX", MemAddrIter->first);
			#else
				#error "Only MSVC and GCC Compilers Supported"
			#endif
			sCommand = string("bpm ")+MemIter->first.c_str()+" "+sBuff+" "+sEventTypes[NoEvent];
			fRetVal = SIM_COMMAND(sCommand);
		}
		else if(pTabs->value() == pRegTab)	//Register
		{
			int i = 1, Index = pRegBreakpoints->value();
			ArchSim<LC3ISA>::RegisterSetMap::iterator RegSetIter;
			ArchSim<LC3ISA>::RegisterMap::iterator RegIter;
			if(Index == 0)
			{
				fl_alert("You must select a breakpoint to remove.");
				return false;
			}

			for(RegSetIter = TheProject.pLC3Sim->RegisterBreakpoints.begin(); RegSetIter != TheProject.pLC3Sim->RegisterBreakpoints.end(); RegSetIter++)
			{
				for(RegIter = RegSetIter->second.begin(); RegIter != RegSetIter->second.end(); RegIter++)
					if(i++ == Index)
						break;
				if(i > Index)
					break;
			}

			string sCommand;
			sCommand = string("bpr ")+RegSetIter->first.c_str()+"."+RegIter->first.c_str()+" "+sEventTypes[NoEvent];
			fRetVal = SIM_COMMAND(sCommand);
		}
		break;
	case LangLC3b:
		if(pTabs->value() == pInstrTab)	//Instruction
		{
			int i = 1, Index = pInstrBreakpoints->value();
			ArchSim<LC3bISA>::InstrMap::iterator InstrIter;
			if(Index == 0)
			{
				fl_alert("You must select a breakpoint to remove.");
				return false;
			}

			for(InstrIter = TheProject.pLC3bSim->InstrBreakpoints.begin(); InstrIter != TheProject.pLC3bSim->InstrBreakpoints.end(); InstrIter++)
				if(i++ == Index)
					break;

			string sCommand;
			if(InstrIter->second.second.first)
			{
				sprintf(sBuff, "%u", InstrIter->second.second.second);
				sCommand = string("bpi ")+" {"+sBuff+"} "+(const char *)*InstrIter->second.first+" "+sEventTypes[NoEvent];
			}
			else
				sCommand = string("bpi ")+(const char *)*InstrIter->second.first+" "+sEventTypes[NoEvent];
			fRetVal = SIM_COMMAND(sCommand);
		}
		else if(pTabs->value() == pDataTab)	//Data
		{
			int i = 1, Index = pDataBreakpoints->value();
			ArchSim<LC3bISA>::DataMap::iterator DataIter;
			if(Index == 0)
			{
				fl_alert("You must select a breakpoint to remove.");
				return false;
			}

			for(DataIter = TheProject.pLC3bSim->DataBreakpoints.begin(); DataIter != TheProject.pLC3bSim->DataBreakpoints.end(); DataIter++)
				if(i++ == Index)
					break;

			string sCommand;
			if(DataIter->second.third.first)
			{
				sprintf(sBuff, "%u", DataIter->second.third.second);
				sCommand = string("bpd ")+" {"+sBuff+"} "+(const char *)*DataIter->second.first+" "+sEventTypes[NoEvent];
			}
			else
				sCommand = string("bpd ")+(const char *)*DataIter->second.first+" "+sEventTypes[NoEvent];
			fRetVal = SIM_COMMAND(sCommand);
		}
		else if(pTabs->value() == pMemTab)	//Memory
		{
			int i = 1, Index = pMemBreakpoints->value();
			ArchSim<LC3bISA>::MemoryMap::iterator MemIter;
			ArchSim<LC3bISA>::MemoryAddrMap::iterator MemAddrIter;
			if(Index == 0)
			{
				fl_alert("You must select a breakpoint to remove.");
				return false;
			}

			for(MemIter = TheProject.pLC3bSim->MemoryBreakpoints.begin(); MemIter != TheProject.pLC3bSim->MemoryBreakpoints.end(); MemIter++)
			{
				for(MemAddrIter = MemIter->second.begin(); MemAddrIter != MemIter->second.end(); MemAddrIter++);
					if(i++ == Index)
						break;
				if(i > Index)
					break;
			}

			string sCommand;
			#if defined _MSC_VER
				sprintf(sBuff, "4x%I64X", MemAddrIter->first);
			#elif defined GPLUSPLUS
				sprintf(sBuff, "4x%llX", MemAddrIter->first);
			#else
				#error "Only MSVC and GCC Compilers Supported"
			#endif
			sCommand = string("bpm ")+MemIter->first.c_str()+" "+sBuff+" "+sEventTypes[NoEvent];
			fRetVal = SIM_COMMAND(sCommand);
		}
		else if(pTabs->value() == pRegTab)	//Register
		{
			int i = 1, Index = pRegBreakpoints->value();
			ArchSim<LC3bISA>::RegisterSetMap::iterator RegSetIter;
			ArchSim<LC3bISA>::RegisterMap::iterator RegIter;
			if(Index == 0)
			{
				fl_alert("You must select a breakpoint to remove.");
				return false;
			}

			for(RegSetIter = TheProject.pLC3bSim->RegisterBreakpoints.begin(); RegSetIter != TheProject.pLC3bSim->RegisterBreakpoints.end(); RegSetIter++)
			{
				for(RegIter = RegSetIter->second.begin(); RegIter != RegSetIter->second.end(); RegIter++)
					if(i++ == Index)
						break;
				if(i > Index)
					break;
			}

			string sCommand;
			sCommand = string("bpr ")+RegSetIter->first.c_str()+"."+RegIter->first.c_str()+" "+sEventTypes[NoEvent];
			fRetVal = SIM_COMMAND(sCommand);
		}
		break;
	}

	if(!fRetVal)
	{
		fl_alert("Error removing the breakpoint.");
		TheSimulatorWindow.show();
	}
	else
		Refresh();

	return fRetVal;
}

bool BreakpointWindow::Clear()
{
	bool fRetVal;
	if(pTabs->value() == pInstrTab)	//Instruction
	{
		fRetVal = SIM_COMMAND("bpic");
	}
	else if(pTabs->value() == pDataTab)	//Data
	{
		fRetVal = SIM_COMMAND("bpdc");
	}
	else if(pTabs->value() == pMemTab)	//Memory
	{
		fRetVal = SIM_COMMAND("bpmc");
	}
	else if(pTabs->value() == pRegTab)	//Register
	{
		fRetVal = SIM_COMMAND("bprc");
	}

	Refresh();
	return fRetVal;
}

bool BreakpointWindow::InstructionBreakpoint(unsigned int ProgramNumber, unsigned int LineNumber)
{
	char sProgramLine[32];
	sprintf(sProgramLine, "{%u} %u", ProgramNumber, LineNumber);
	string sCommand = string("bpl ")+sProgramLine;
	bool fRetVal = SIM_COMMAND(sCommand);

	if(!fRetVal)
	{
		fl_alert("Error inserting the breakpoint.");
		TheSimulatorWindow.show();
	}
	else if(TheSimulatorWindow.pBreakpointWindow)
		TheSimulatorWindow.pBreakpointWindow->Refresh();

	return fRetVal;
}

bool BreakpointWindow::DataBreakpoint(unsigned int ProgramNumber, const string &sIdentifier)
{
	char sProgramNumber[16];
	sprintf(sProgramNumber, "{%u} ", ProgramNumber);
	string sDataAddress = string(sProgramNumber) + sIdentifier;
	pDataAddressDlg->value(sDataAddress.c_str());

	//Run this dialog until enter or cancel is chosen, block all other operations.
	pDataDlg->show();
	bool fData = false;
	while(true)
	{
		Fl_Widget *pW = Fl::readqueue();
		if(!pW)
			Fl::wait();
		else if(pW == pDataAddDlg)
		{
			fData = true;
			break;
		}
		else if(pW == pDataCancelDlg)
			break;
		else if(pW == pDataDlg)
			break;
	}
	pDataDlg->hide();

	if(!fData)
		return false;

	bool fRetVal = true;
	const char *sEvent = _sEventTypes[pDataEventDlg->value()];
	DataEnum DataType = (DataEnum)pDataDataTypeDlg->value();
	if(sDataAddress[0] == 0)
	{
		fl_alert("You must provide a symbol or byte address identifier.");
		return false;
	}
	if(!stricmp(sEvent, "valueevent"))
	{
		const char *sValue = pDataValueDlg->value();
		if(sValue[0] == 0)
		{
			fl_alert("You must provide a symbol or value for a VALUEEVENT breakpoint.");
			return false;
		}
		string sCommand;
		sCommand = string("bpd ")+sDataAddress+" ";
		if(DataType < STRUCT)
			(sCommand += sDataTypes[DataType]) += " ";
		((sCommand+=sEvent)+=" ")+=sValue;
		fRetVal = SIM_COMMAND(sCommand);
	}
	else
	{
		string sCommand;
		sCommand = string("bpd ")+sDataAddress+" ";
		if(DataType < STRUCT)
			(sCommand += sDataTypes[DataType]) += " ";
		sCommand += sEvent;
		fRetVal = SIM_COMMAND(sCommand);
	}

	if(!fRetVal)
	{
		fl_alert("Error inserting the breakpoint.");
		TheSimulatorWindow.show();
	}
	else if(TheSimulatorWindow.pBreakpointWindow)
		TheSimulatorWindow.pBreakpointWindow->Refresh();

	return fRetVal;
}

bool BreakpointWindow::RegisterBreakpoint(const string &sRegister)
{
	const char *sRegisterSet = NULL;
	for(Architecture::RegisterSetMap::iterator RegSetIter = TheArch(RegisterSets.begin()); RegSetIter != TheArch(RegisterSets.end()) && !sRegisterSet; RegSetIter++)
	{
		for(RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.begin(); RegIter != RegSetIter->second.Registers.end() && !sRegisterSet; RegIter++)
			if(ToLower(RegIter->first) == ToLower(sRegister))
				sRegisterSet = RegSetIter->first.c_str();
	}
	if(!sRegisterSet)
	{
		fl_alert("Architecture does not contain this register.");
		return false;
	}
	pRegisterSetDlg->value(sRegisterSet);
	pRegisterDlg->value(sRegister.c_str());

	//Run this dialog until enter or cancel is chosen, block all other operations.
	pRegDlg->show();
	bool fReg = false;
	while(true)
	{
		Fl_Widget *pW = Fl::readqueue();
		if(!pW)
			Fl::wait();
		else if(pW == pRegAddDlg)
		{
			fReg = true;
			break;
		}
		else if(pW == pRegCancelDlg)
			break;
		else if(pW == pRegDlg)
			break;
	}
	pRegDlg->hide();

	if(!fReg)
		return false;

	bool fRetVal = true;
	const char *sEvent = _sEventTypes[pRegEventDlg->value()];
	if(!stricmp(sEvent, "valueevent"))
	{
		const char *sValue = pRegValueDlg->value();
		if(sValue[0] == 0)
		{
			fl_alert("You must provide a symbol or value for a VALUEEVENT breakpoint.");
			return false;
		}
		string sCommand;
		sCommand = string("bpr ")+sRegisterSet+"."+sRegister+" "+sEvent+" "+sValue;
		fRetVal = SIM_COMMAND(sCommand);
	}
	else
	{
		string sCommand;
		sCommand = string("bpr ")+sRegisterSet+"."+sRegister+" "+sEvent;
		fRetVal = SIM_COMMAND(sCommand);
	}

	if(!fRetVal)
	{
		fl_alert("Error inserting the breakpoint.");
		TheSimulatorWindow.show();
	}
	else if(TheSimulatorWindow.pBreakpointWindow)
		TheSimulatorWindow.pBreakpointWindow->Refresh();

	return fRetVal;
}

}	//namespace AshIDE
