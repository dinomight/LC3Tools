//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "WriteDataWindow.h"
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

Fl_Menu_Item WriteDataWindow::MenuItems[] =
	{
		{ "&Write Data", 0, 0, 0, FL_SUBMENU },
			{ "&Save",			FL_CTRL + 's',	SaveCB },
			{ "L&oad",			FL_CTRL + 'l',	LoadCB, 0, FL_MENU_DIVIDER },
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
Fl_Text_Display::Style_Table_Entry WriteDataWindow::StyleTable[] = 
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
		{ FL_DARK_RED,		FL_COURIER_BOLD,	14 }	// Q - Register
	};

WriteDataWindow::WriteDataWindow() : Fl_Double_Window(300, 340)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		pMainMenu->copy(MenuItems, this);

		//Setup the write data display
		pTextEditor = new TextEditor(0, 28+20*4+16*2, w(), h()-28*2-20*4-16*2, StyleTable, sizeof(StyleTable)/sizeof(StyleTable[0]));
		pTextEditor->pTextBuffer->add_modify_callback(UpdateStyleCB, this);

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

		pMemoryInput = new Fl_Round_Button(0, 28+20, 120, 20, "Physical Memory");
		pMemoryInput->type(FL_RADIO_BUTTON);
		pMemoryInput->callback(ChangeInputCB, this);
		pMemoryName = new Fl_Choice(220, 28+20, 80, 20, "Memory");
		pMemoryName->box(FL_PLASTIC_DOWN_BOX);
		pMemoryName->deactivate();
		for(Architecture::MemoryMap::iterator MemIter = TheArch(Memories.begin()); MemIter != TheArch(Memories.end()); MemIter++)
			pMemoryName->add(MemIter->second.sName.c_str());
		pMemoryName->value(0);

		pDataInput = new Fl_Round_Button(0, 28+20*2, 120, 20, "Logical Data");
		pDataInput->type(FL_RADIO_BUTTON);
		pDataInput->callback(ChangeInputCB, this);
		pDataInput->value(1);

		pDataAddress = new Fl_Input(0, 28+20*3+16, 160, 20);	//Symbol Or Byte Address
		pDataAddress->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
		pDataAddress->value("0");
		pEnter = new Fl_Return_Button(220, 28+20*3+16, 80, 20, "Edit");
		pEnter->callback(EnterCB, this);

		Fl_Box *pBox = new Fl_Box(0, 28+20*4+16, 300, 16, "Enter assembly syntax instructions and data:");
		pBox->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

		ChangeInput();
	}
	end();
	resizable(pTextEditor);
	callback(CloseCB, this);

	show();
	SetTitle();
}

WriteDataWindow::~WriteDataWindow()
{
	hide();
	TheSimulatorWindow.pWriteDataWindow = NULL;
	Fl::first_window()->show();
}

int WriteDataWindow::handle(int Event)
{
	int RetVal = Fl_Double_Window::handle(Event);
	switch(Event)
	{
	case FL_FOCUS:
		//*NOTE: Some bug/feature in FLTK causes the Status widget to always get
		//the focus instead of the text editor
		pTextEditor->take_focus();
	}
	return RetVal;
}

void WriteDataWindow::HelpCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.Help(AshIDEIndex, "WriteDataWindow");
}

bool WriteDataWindow::Close()
{
	Fl::delete_widget(this);
	return true;
}

bool WriteDataWindow::SetTitle()
{
	string sTitle = "Write Data";

	label(sTitle.c_str());
	return true;
}

bool WriteDataWindow::ChangeInput()
{
	if(pMemoryInput->value())
	{
		pMemoryName->activate();
		pDataAddress->label("Symbol or Byte Address");
	}
	else
	{
		pMemoryName->deactivate();
		pDataAddress->label(TheProject.SimISA == LangLC3 ? "Symbol or Word Address" : "Symbol or Byte Address");
	}

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

bool WriteDataWindow::Enter()
{
	bool fRetVal = true;

	if(pLineInput->value())
	{
		const char *sLineNumber = pLineNumber->value();
		const char *sWriteData = pTextEditor->pTextBuffer->text();
		if(sLineNumber[0] == 0 || sWriteData[0] == 0)
		{
			fl_alert("You must provide a line number and assembler instructions/data.");
			return false;
		}
		string sCommand;
		sCommand = string("wl ")+sLineNumber+" "+sWriteData;
		//Remove newlines from the command
		string::size_type Pos = 0;
		while((Pos = sCommand.find('\n', Pos)) != string::npos)
			sCommand[Pos] = ' ';
		fRetVal = SIM_COMMAND_UPDATE(sCommand);
	}
	else if(pMemoryInput->value())
	{
		const char *sMemoryName = pMemoryName->text();
		const char *sDataAddress = pDataAddress->value();
		const char *sWriteData = pTextEditor->pTextBuffer->text();
		if(!sMemoryName)
		{
			fl_alert("Architecture does not contain any memories.");
			return false;
		}
		if(sDataAddress[0] == 0 || sWriteData[0] == 0)
		{
			fl_alert("You must provide a memory name and symbol or byte address identifier and assembler instructions/data.");
			return false;
		}
		string sCommand;
		sCommand = string("wm ")+sMemoryName+" "+sDataAddress+" "+sWriteData;
		//Remove newlines from the command
		string::size_type Pos = 0;
		while((Pos = sCommand.find('\n', Pos)) != string::npos)
			sCommand[Pos] = ' ';
		fRetVal = SIM_COMMAND_UPDATE(sCommand);
	}
	else
	{
		const char *sDataAddress = pDataAddress->value();
		const char *sWriteData = pTextEditor->pTextBuffer->text();
		if(sDataAddress[0] == 0 || sWriteData[0] == 0)
		{
			fl_alert(TheProject.SimISA == LangLC3 ?
				"You must provide a symbol or word address identifier and assembler instructions/data." :
				"You must provide a symbol or byte address identifier and assembler instructions/data.");
			return false;
		}
		string sCommand;
		sCommand = string("wd ")+sDataAddress+" "+sWriteData;
		//Remove newlines from the command
		string::size_type Pos = 0;
		while((Pos = sCommand.find('\n', Pos)) != string::npos)
			sCommand[Pos] = ' ';
		fRetVal = SIM_COMMAND_UPDATE(sCommand);
	}

	if(!fRetVal)
	{
		fl_alert("Error performing the edit.");
		TheSimulatorWindow.show();
	}
	return fRetVal;
}

bool WriteDataWindow::EditInstruction(unsigned int ProgramNumber, unsigned int LineNumber)
{
	pLineInput->value(1);
	pDataInput->value(0);
	ChangeInput();
	char sProgramLine[32];
	sprintf(sProgramLine, "{%u} %u", ProgramNumber, LineNumber);
	pLineNumber->value(sProgramLine);
	return true;
}

bool WriteDataWindow::EditData(unsigned int ProgramNumber, const string &sIdentifier)
{
	pDataInput->value(1);
	ChangeInput();
	char sProgramNumber[16];
	sprintf(sProgramNumber, "{%u} ", ProgramNumber);
	string sDataAddress = string(sProgramNumber) + sIdentifier;
	pDataAddress->value(sDataAddress.c_str());
	return true;
}

bool WriteDataWindow::UpdateStyle(int Pos, int Inserted, int Deleted, int Restyled, const char *pszDeleted)
{
	int	Start, End;

	char *pStyle, *pText;

	//If this is just a selection change, then unselect the style buffer
	if(Inserted == 0 && Deleted == 0)
	{
		pTextEditor->pStyleBuffer->unselect();
		return true;
	}

	//Track changes in the text buffer
	if (Inserted > 0)
	{
		//Insert characters into the style buffer
		pStyle = new char[Inserted + 1];
		memset(pStyle, 'A', Inserted);
		pStyle[Inserted] = '\0';

		pTextEditor->pStyleBuffer->replace(Pos, Pos + Deleted, pStyle);
		delete[] pStyle;
	}
	else
	{
		//Just delete characters from the style buffer
		pTextEditor->pStyleBuffer->remove(Pos, Pos + Deleted);
	}

	//Select the area that was just updated to avoid unnecessary callbacks
	pTextEditor->pStyleBuffer->select(Pos, Pos + Inserted - Deleted);

	//Re-parse the changed region; we do this by parsing from the
	//beginning of the line of the changed region to the end of
	//the line of the changed region.
	Start = pTextEditor->pTextBuffer->line_start(Pos);
	End   = pTextEditor->pTextBuffer->line_end(Pos + Inserted);
	string sText(pText = pTextEditor->pTextBuffer->text_range(Start, End));
	delete [] pText;
	string sStyle;

	ParseStyle(sText, sStyle);

	pTextEditor->pStyleBuffer->replace(Start, End, sStyle.c_str());
	pTextEditor->redisplay_range(Start, End);
	return true;
}

bool WriteDataWindow::ParseStyle(const string &sText, string &sStyle)
{
	if(TheProject.SimISA == LangLC3)
		SimulatorWindow::TheLC3HighlightLexer.Lex(sText, sStyle);
	else if(TheProject.SimISA == LangLC3b)
		SimulatorWindow::TheLC3bHighlightLexer.Lex(sText, sStyle);

	return true;
}

}	//namespace AshIDE
