//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "DisassemblyWindow.h"
#include <FL/Fl.H>
#include "Project.h"
#include "MainWindow.h"
#include "FileWindow.h"
#include "SimulatorWindow.h"
#include <strstream>

using namespace std;
using namespace JMT;
using namespace LC3;
using namespace LC3b;

namespace AshIDE	{

Fl_Menu_Item DisassemblyWindow::MenuItems[] =
	{
		{ "&Disassembly", 0, 0, 0, FL_SUBMENU },
			{ "Show &Binary Values",	FL_CTRL + FL_SHIFT + 'b',	ShowValuesCB, 0, FL_MENU_TOGGLE | FL_MENU_DIVIDER },
			{ "&Save",					FL_CTRL + 's',				SaveCB, 0, FL_MENU_DIVIDER },
			{ "&Copy",					FL_CTRL + 'c',				CopyCB, 0, FL_MENU_DIVIDER },
			{ "&Help",					FL_F + 1,					HelpCB, 0, FL_MENU_DIVIDER },
			{ "C&lose",					FL_Escape,					CloseCB },
			{ 0 },

		{ "&Search", 0, 0, 0, FL_SUBMENU },
			{ "&Find...",		FL_CTRL + 'f',	FindCB },
			{ "F&ind Again",	FL_CTRL + 'g',	FindAgainCB },
			{ 0 },

		{ "S&imulate", 0, 0, 0, FL_SUBMENU },
			{ "&Go",							FL_F + 5,		SimulatorWindow::GoCB },
			{ "Go One &Instruction",			FL_F + 6,		SimulatorWindow::GoOneCB },
			{ "Run To &Cursor",					FL_F + 7,		RunToCursorCB, 0, FL_MENU_DIVIDER },
			{ "Step I&nto Next Function",		FL_F + 8,		SimulatorWindow::StepInCB },
			{ "Step O&ver Next Function",		FL_F + 9,		SimulatorWindow::StepOverCB },
			{ "Step &Out Of Current Function",	FL_F + 10,		SimulatorWindow::StepOutCB, 0, FL_MENU_DIVIDER },
			{ "&Signal Interrupt",				0,				SimulatorWindow::InterruptCB },
			{ "Brea&k",							FL_F + 11,		SimulatorWindow::BreakCB },
			{ "&Reset",							FL_F + 12,		SimulatorWindow::ResetCB },
			{ "&End Simulation",				FL_CTRL + 'q',	SimulatorWindow::StopCB, 0, FL_MENU_DIVIDER },

			{ "&Breakpoints", 0, 0, 0, FL_SUBMENU },
				{ "&Breakpoints",	FL_CTRL + 'b',	SimulatorWindow::BreakpointsCB, 0, FL_MENU_DIVIDER },
				{ "Instruction Breakpoint At &Cursor",	FL_CTRL + FL_SHIFT + 'i',	InstructionBreakpointCB },
				{ "&Data Breakpoint At Cursor",			FL_CTRL + FL_SHIFT + 'd',	DataBreakpointCB },
				{ "Regis&ter Breakpoint At Cursor",		FL_CTRL + FL_SHIFT + 'r',	RegisterBreakpointCB },
				{ 0 },

			{ "Vie&w", 0, 0, 0, FL_SUBMENU },
				{ "&Console I/O",	FL_CTRL + 'i',	SimulatorWindow::ConsoleCB, 0, FL_MENU_DIVIDER },
				{ "&Disassembly",	FL_CTRL + 'd',	SimulatorWindow::DisassemblyCB },
				{ "Call &Stack",	FL_CTRL + 'k',	SimulatorWindow::CallStackCB, 0, FL_MENU_DIVIDER },
				{ "View Instructions At Cursor",	FL_CTRL + FL_SHIFT + '1',	ViewInstructionCB },
				{ "View Data At Cursor",			FL_CTRL + FL_SHIFT + '2',	ViewDataCB },
				{ "View Memory At Cursor",			FL_CTRL + FL_SHIFT + '3',	ViewMemoryCB },
				{ "View Register At Cursor",		FL_CTRL + FL_SHIFT + '4',	ViewRegisterCB, 0, FL_MENU_DIVIDER },
				{ "&Instructions",	FL_CTRL + '1',	SimulatorWindow::InstructionsCB },
				{ "D&ata Values",	FL_CTRL + '2',	SimulatorWindow::DataValuesCB },
				{ "&Memory Bytes",	FL_CTRL + '3',	SimulatorWindow::MemoryBytesCB },
				{ "&Registers",		FL_CTRL + '4',	SimulatorWindow::RegistersCB },
				{ "Pipe&lines",		FL_CTRL + '5',	SimulatorWindow::PipelinesCB },
				{ "&Programs",		FL_CTRL + '6',	SimulatorWindow::ProgramsCB },
				{ 0 },

			{ "&Edit && Continue", 0, 0, 0, FL_SUBMENU },
				{ "Edit &Instruction/Data/Memory",		FL_F + 2,	SimulatorWindow::WriteDataCB },
				{ "Edit &Register",						FL_F + 3,	SimulatorWindow::WriteRegisterCB, 0, FL_MENU_DIVIDER },
				{ "Edit Instructions/Data At Cursor",	0,			EditInstructionCB },
				{ "Edit Data At Cursor",				0,			EditDataCB },
				{ "Edit Register At Cursor",			0,			EditRegisterCB },
				{ 0 },

			{ 0 },

		{ 0 }
	};

Fl_Menu_Item DisassemblyWindow::PopupItems[] =
	{
		{ "Run To This &Line",						FL_F + 7,		RunToCursorCB, 0, FL_MENU_DIVIDER },
		{ "&Instruction Breakpoint At This Line",	FL_CTRL + FL_SHIFT + 'i',	InstructionBreakpointCB },
		{ "&Data Breakpoint At This Variable",		FL_CTRL + FL_SHIFT + 'd',	DataBreakpointCB },
		{ "&Register Breakpoint At This Register",	FL_CTRL + FL_SHIFT + 'r',	RegisterBreakpointCB, 0, FL_MENU_DIVIDER },
		{ "View Instructions At This Line",			FL_CTRL + FL_SHIFT + '1',	ViewInstructionCB },
		{ "View Value Of This Variable",			FL_CTRL + FL_SHIFT + '2',	ViewDataCB },
		{ "View Memory Bytes At This Variable",		FL_CTRL + FL_SHIFT + '3',	ViewMemoryCB },
		{ "View Value Of This Register",			FL_CTRL + FL_SHIFT + '4',	ViewRegisterCB, 0, FL_MENU_DIVIDER },
		{ "Edit Instructions/Data At This Line",	0,			EditInstructionCB },
		{ "Edit This Variable",						0,			EditDataCB },
		{ "Edit This Register",						0,			EditRegisterCB },
		{ 0 },
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
Fl_Text_Display::Style_Table_Entry DisassemblyWindow::StyleTable[] = 
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

DisassemblyWindow::DisassemblyWindow() : Fl_Double_Window(480, 560)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		pMainMenu->copy(MenuItems, this);
		//Setup the disassembly display
		pTextDisplay = new ReadOnlyEditorX(0, 28, w(), h()-28*2, StyleTable, sizeof(StyleTable)/sizeof(StyleTable[0]));
		pTextDisplay->pTextBuffer->add_modify_callback(UpdateStyleCB, this);
		//Setup the status bar
		pStatus = new Fl_Output(0, h()-28, w(), 28);
		pStatus->box(FL_PLASTIC_UP_BOX);
		//Setup the pop-up menu
		pPopupMenu = new Fl_Menu_Button(0, 28, 0, 0);
		pPopupMenu->copy(PopupItems, this);
		pPopupMenu->type(Fl_Menu_Button::POPUP3);
	}
	end();
	resizable(pTextDisplay);
	callback(CloseCB, this);

	show();
	SetTitle();
	Init();
}

DisassemblyWindow::~DisassemblyWindow()
{
	hide();
	TheSimulatorWindow.pDisassemblyWindow = NULL;
	Fl::first_window()->show();
}

DisassemblyWindow::ReadOnlyEditorX::ReadOnlyEditorX(int X, int Y, int Width, int Height, Fl_Text_Display::Style_Table_Entry *pStyleTable, unsigned int nStyles) : ReadOnlyEditor(X, Y, Width, Height, pStyleTable, nStyles)
{
}

int DisassemblyWindow::ReadOnlyEditorX::handle(int Event)
{
	unsigned int LineNumber;
	string sFileName;
	FileWindow *pFW;

	int RetVal = ReadOnlyEditor::handle(Event);
	switch(Event)
	{
	case FL_RELEASE:
		if(Fl::event_clicks() && Fl::event_button() == FL_LEFT_MOUSE)	//Double click
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
		if(Fl::event_button() == FL_RIGHT_MOUSE)
			((DisassemblyWindow *)parent())->pPopupMenu->popup();
		break;
	}
	return RetVal;
}

bool DisassemblyWindow::ReadOnlyEditorX::GetFileLine(string &sFileName, unsigned int &LineNumber)
{
	unsigned int Start, End;
	char *sText;

	//Find the filename in this line's message
	End = Start = pTextBuffer->line_start(insert_position());
	while(pStyleBuffer->character(End) == 'R')
		End++;
	if(End == Start)
		//There was no file name on this line
		return false;

	//Create a full-path version of the filename.
	//All file names in a message should be relative to the project, or absolute.
	sText = pTextBuffer->text_range(Start, End);
	FileName sFile = sFileName = CreateFileNameRelative(TheProject.sWorkingDir, sText);
	delete [] sText;

	//Find the line number
	if(pStyleBuffer->character(End) != 'J' || pStyleBuffer->character(End+1) != 'G')
	{
		//There was no line number
		LineNumber = 0;
	}
	else
	{
		End = Start = End+1;
		while(pStyleBuffer->character(End) == 'G')
			End++;
		sText = pTextBuffer->text_range(Start, End);
		sscanf(sText, "%u", &LineNumber);
		delete [] sText;
	}

	if(sFile.Name == "NoFile" && LineNumber == 0 || ToLower(sFile.Ext) == "obj" || ToLower(sFile.Ext) == "bin")
		return false;
	return true;
}

void DisassemblyWindow::HelpCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.Help(AshIDEIndex, "DisassemblyWindow");
}

bool DisassemblyWindow::Close()
{
	Fl::delete_widget(this);
	return true;
}

bool DisassemblyWindow::SetTitle()
{
	string sTitle = "Disassembly";

	label(sTitle.c_str());
	return true;
}

bool DisassemblyWindow::UnHighlight()
{
	pTextDisplay->pTextBuffer->unhighlight();
	return true;
}

bool DisassemblyWindow::SelectLine(unsigned int LineNumber, bool fHighlight, bool fShow)
{
	int Start = 0, End;
	unsigned int Line;
	//Find the start of the line
	if(LineNumber > 1)
	{
		Line = 1;
		while(Line < LineNumber)
		{
			if(!pTextDisplay->pTextBuffer->findchar_forward(Start, '\n', &Start))
				break;
			Start++;
			Line++;
		}
	}
	if(Start == pTextDisplay->pTextBuffer->length())
		//no such line
		return false;

	//Find the end of the line
	if(pTextDisplay->pTextBuffer->findchar_forward(Start, '\n', &End))
		End++;

	if(fHighlight)
		pTextDisplay->pTextBuffer->highlight(Start, End);
	else
		pTextDisplay->pTextBuffer->select(Start, End);
	pTextDisplay->insert_position(Start);
	pTextDisplay->show_insert_position();
	if(fShow)
		show();
//	pTextDisplay->take_focus();
	return true;
}

bool DisassemblyWindow::GetFileLine(string &sFileName, unsigned int &LineNumber)
{
	return pTextDisplay->GetFileLine(sFileName, LineNumber);
}

bool DisassemblyWindow::GetIdentifier(string &sIdentifier) const
{
	if(pTextDisplay->pStyleBuffer->character(pTextDisplay->insert_position()) == 'I')
	{
		char *sID = pTextDisplay->pTextBuffer->text_range(pTextDisplay->pTextBuffer->word_start(pTextDisplay->insert_position()), pTextDisplay->pTextBuffer->word_end(pTextDisplay->insert_position()));
		sIdentifier = sID;
		delete sID;
		if(sIdentifier == "")
			return false;
		return true;
	}
	return false;
}

bool DisassemblyWindow::GetRegister(string &sRegister) const
{
	if(pTextDisplay->pStyleBuffer->character(pTextDisplay->insert_position()) == 'Q')
	{
		char *sID = pTextDisplay->pTextBuffer->text_range(pTextDisplay->pTextBuffer->word_start(pTextDisplay->insert_position()), pTextDisplay->pTextBuffer->word_end(pTextDisplay->insert_position()));
		sRegister = sID;
		delete sID;
		if(sRegister == "")
			return false;
		return true;
	}
	return false;
}

bool DisassemblyWindow::ShowValues()
{
	pTextDisplay->Clear();
	return Init();
}

bool DisassemblyWindow::Init()
{
	vector<Program *> &Programs = TheProject.Programs[TheProject.SimISA];
	list<Segment *>::iterator SegmentIter;
	list<Element *>::iterator SequenceIter;
	Element *pElement;
	const char *sInstruction;
	unsigned int i, LineCount = 1;

	//Reorder the programs so that they are printed in order of increasing origin address
	typedef pair<unsigned int, uint64> OrderInfo;
	vector<OrderInfo> Order;
	vector<OrderInfo>::iterator OrderIter;
	for(i = 0; i < Programs.size(); i++)
	{
		for(OrderIter = Order.begin(); OrderIter != Order.end(); OrderIter++)
			if(Programs[i]->Address < OrderIter->second)
				break;
		Order.insert( OrderIter, OrderInfo(i, Programs[i]->Address) );
	}

	//Go over each program
	for(i = 0; i < Order.size(); i++)
	{
		//Go over each segment
		for(SegmentIter = Programs[Order[i].first]->Segments.begin(); SegmentIter != Programs[Order[i].first]->Segments.end(); SegmentIter++)
		{
			//Go over each element in the segment
			for(SequenceIter = (*SegmentIter)->Sequence.begin(); SequenceIter != (*SegmentIter)->Sequence.end(); SequenceIter++)
			{
				pElement = *SequenceIter;
				ostrstream strInstr;
				if(!TheSim(PrintInstruction(strInstr, pElement, pElement->Address, true, false, pMainMenu->menu()[1].value() != 0)))
					continue;
				strInstr << endl << ends;
				sInstruction = strInstr.str();
				pTextDisplay->pTextBuffer->append(strInstr.str());
				pElement->LocationID = LineCount;
				while(sInstruction[0])
				{
					LineCount++;
					sInstruction = strchr(sInstruction, '\n')+1;
				}
			}
		}
	}

	TheSim(fFirstBreak) = true;

	return true;
}

void DisassemblyWindow::RunToCursorCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::RunToCursorCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

void DisassemblyWindow::InstructionBreakpointCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::InstructionBreakpointCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

void DisassemblyWindow::DataBreakpointCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::DataBreakpointCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}
void DisassemblyWindow::RegisterBreakpointCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::RegisterBreakpointCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

void DisassemblyWindow::ViewInstructionCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::ViewInstructionCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

void DisassemblyWindow::ViewDataCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::ViewDataCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

void DisassemblyWindow::ViewMemoryCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::ViewMemoryCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

void DisassemblyWindow::ViewRegisterCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::ViewRegisterCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

void DisassemblyWindow::EditInstructionCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::EditInstructionCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

void DisassemblyWindow::EditDataCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::EditDataCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

void DisassemblyWindow::EditRegisterCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::EditRegisterCB(SimulatorWindow::SimDisassemblyWindow, (Fl_Window *)pV);
}

bool DisassemblyWindow::UpdateStyle(int Pos, int Inserted, int Deleted, int Restyled, const char *pszDeleted)
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
	string sStyle;//(pText = pTextDisplay->pStyleBuffer->text_range(Start, End));
	//delete [] pText;

	ParseStyle(sText, sStyle);

	pTextDisplay->pStyleBuffer->append(sStyle.c_str());
	pTextDisplay->redisplay_range(Start, End);
	return true;
}

bool DisassemblyWindow::ParseStyle(const string &sText, string &sStyle)
{
	string::size_type TempLoc;

	//look for colons
//	TempLoc = sText.find(':');
//	if(TempLoc != string::npos && TempLoc != 0 && sStyle[TempLoc-1] != 'A')
		//This line was already styled
//		return true;

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
