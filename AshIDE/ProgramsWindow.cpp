//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "ProgramsWindow.h"
#include <FL/Fl.H>
#include "Project.h"
#include "MainWindow.h"
#include "FileWindow.h"
#include "SimulatorWindow.h"

using namespace std;
using namespace JMT;
using namespace LC3;
using namespace LC3b;

namespace AshIDE	{

Fl_Menu_Item ProgramsWindow::MenuItems[] =
	{
		{ "&Programs", 0, 0, 0, FL_SUBMENU },
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
Fl_Text_Display::Style_Table_Entry ProgramsWindow::StyleTable[] = 
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

		{ FL_DARK_CYAN,		FL_COURIER,			14 }	// R - Filename
	};

ProgramsWindow::ProgramsWindow() : Fl_Double_Window(500, 200)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		pMainMenu->copy(MenuItems, this);
		//Setup the Programs display
		pTextDisplay = new ReadOnlyEditorX(0, 28, w(), h()-28*2, StyleTable, sizeof(StyleTable)/sizeof(StyleTable[0]));
		pTextDisplay->pTextBuffer->add_modify_callback(UpdateStyleCB, this);
		//Setup the status bar
		pStatus = new Fl_Output(0, h()-28, w(), 28);
		pStatus->box(FL_PLASTIC_UP_BOX);
	}
	end();
	resizable(pTextDisplay);
	callback(CloseCB, this);

	show();
	SetTitle();
	Enter();
}

ProgramsWindow::~ProgramsWindow()
{
	hide();
	TheSimulatorWindow.pProgramsWindow = NULL;
	Fl::first_window()->show();
}

ProgramsWindow::ReadOnlyEditorX::ReadOnlyEditorX(int X, int Y, int Width, int Height, Fl_Text_Display::Style_Table_Entry *pStyleTable, unsigned int nStyles) : ReadOnlyEditor(X, Y, Width, Height, pStyleTable, nStyles)
{
}

int ProgramsWindow::ReadOnlyEditorX::handle(int Event)
{
	unsigned int LineNumber;
	string sFileName;
	FileWindow *pFW;

	int RetVal = ReadOnlyEditor::handle(Event);
	switch(Event)
	{
	case FL_RELEASE:
		if(Fl::event_clicks())	//Double click
		{
			if(!GetFileLine(sFileName, LineNumber))
				break;

			//Open the file
			TheMainWindow.OpenFile(sFileName);
			//Highlight the line
			if(pFW = FileWindow::IsOpen(sFileName))
			{
				pFW->SelectLine(LineNumber);
			}
		}
		break;
	}
	return RetVal;
}

bool ProgramsWindow::ReadOnlyEditorX::GetFileLine(string &sFileName, unsigned int &LineNumber)
{
	unsigned int Start, End;
	char *sText;

	//Find the filename in this line's message
	End = Start = pTextBuffer->line_start(insert_position());
	while(pStyleBuffer->character(End) != 'F')
		End = ++Start;
	while(pStyleBuffer->character(End) == 'F')
		End++;

	if(End-Start <= 2)
		//There was no file name on this line
		return false;

	//Create a full-path version of the filename.
	//All file names in a message should be relative to the project, or absolute.
	sText = pTextBuffer->text_range(Start+1, End-1);
	FileName sFile = sFileName = CreateFileNameRelative(TheProject.sWorkingDir, sText);
	delete [] sText;

	LineNumber = 0;
	if(ToLower(sFile.Ext) == "obj" || ToLower(sFile.Ext) == "bin")
		return false;
	return true;
}

void ProgramsWindow::HelpCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.Help(AshIDEIndex, "ProgramsWindow");
}

bool ProgramsWindow::Close()
{
	Fl::delete_widget(this);
	return true;
}

bool ProgramsWindow::SetTitle()
{
	string sTitle = "Programs";

	label(sTitle.c_str());
	return true;
}

bool ProgramsWindow::GetFileLine(string &sFileName, unsigned int &LineNumber)
{
	return pTextDisplay->GetFileLine(sFileName, LineNumber);
}

bool ProgramsWindow::Enter()
{
	TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimProgramsWindow;
	TheSimulatorWindow.pRedirectWindow = this;
	pTextDisplay->Clear();
	bool fRetVal = SIM_COMMAND("dp");
	TheSimulatorWindow.RedirectWindow = SimulatorWindow::SimNoWindow;

	return fRetVal;
}

bool ProgramsWindow::UpdateStyle(int Pos, int Inserted, int Deleted, int Restyled, const char *pszDeleted)
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

bool ProgramsWindow::ParseStyle(const string &sText, string &sStyle)
{
	string::size_type TempLoc;

	//look for colons
	TempLoc = sText.find(':');
	if(TempLoc != string::npos && TempLoc != 0 && sStyle[TempLoc-1] != 'A')
		//This line was already styled
		return true;

	//Highlight the filename
	string sFileNameStyle, sAssemblyStyle;
	TempLoc = sText.find('(');
	if(TempLoc == string::npos)
		TempLoc = 0;

	if(TheProject.SimISA == LangLC3)
		SimulatorWindow::TheLC3HighlightLexer.Lex(sText.substr(TempLoc), sAssemblyStyle);
	else if(TheProject.SimISA == LangLC3b)
		SimulatorWindow::TheLC3bHighlightLexer.Lex(sText.substr(TempLoc), sAssemblyStyle);

	sStyle = string(TempLoc, 'R') + sAssemblyStyle;
	return true;
}

}	//namespace AshIDE
