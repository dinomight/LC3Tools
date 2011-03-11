//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "FileWindow.h"
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include "MainWindow.h"
#include "SimulatorWindow.h"
#include <fstream>

using namespace std;

namespace AshIDE	{

Fl_Menu_Item FileWindow::MenuItems[] =
	{
		{ "&File", 0, 0, 0, FL_SUBMENU },	//0
			{ "&New File",		FL_CTRL + 'n',				MainWindow::NewFileCB },
			{ "&Open File",		FL_CTRL + 'o',				MainWindow::OpenFileCB, 0, FL_MENU_DIVIDER },
			{ "&Save File",		FL_CTRL + 's',				SaveCB },
			{ "Save File &As",	FL_CTRL + FL_SHIFT + 's',	SaveAsCB, 0, FL_MENU_DIVIDER },
			{ "&Close File",	0,							CloseCB, 0, FL_MENU_DIVIDER },
			{ "&Help",			FL_F + 1,					HelpCB, 0, FL_MENU_DIVIDER },
			{ "E&xit",			FL_CTRL + 'q',				MainWindow::ExitCB },
			{ 0 },

		{ "&Edit", 0, 0, 0, FL_SUBMENU },	//9
			{ "Cu&t",		FL_CTRL + 'x',	CutCB },
			{ "&Copy",		FL_CTRL + 'c',	CopyCB },
			{ "&Paste",		FL_CTRL + 'v',	PasteCB },
			{ "&Delete",	0,				DeleteCB },
			{ 0 },

		{ "&Search", 0, 0, 0, FL_SUBMENU },	//15
			{ "&Find...",		FL_CTRL + 'f',	FindCB },
			{ "F&ind Again",	FL_CTRL + 'g',	FindAgainCB, 0, FL_MENU_DIVIDER },
			{ "&Replace...",	FL_CTRL + 'r',	ReplaceCB },
			{ "Re&place Again",	FL_CTRL + 't',	ReplaceAgainCB },
			{ 0 },

		{ "&Project", 0, 0, 0, FL_SUBMENU },	//21
			{ "A&dd To Project",		0,						AddToProjectCB },
			{ "&Remove From Project",	0,						RemoveFromProjectCB, 0, FL_MENU_DIVIDER | FL_MENU_INVISIBLE },
			{ "&Assemble File",			FL_F + 2,				AssembleFileCB, 0, FL_MENU_INVISIBLE },
			{ "Assemble &Project",		FL_SHIFT + FL_F + 2,	MainWindow::AssembleCB, 0, FL_MENU_INVISIBLE },
			{ "&Build Project",			FL_F + 3,				MainWindow::BuildCB, 0, FL_MENU_INVISIBLE },
			{ "R&ebuild All",			0,						MainWindow::RebuildCB, 0, FL_MENU_INVISIBLE },
			{ "&Start Simulator",		FL_F + 4,				MainWindow::SimulateCB, 0, FL_MENU_INVISIBLE },
			{ 0 },

		{ "S&imulate", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//30
			{ "&Go",							FL_F + 5,		SimulatorWindow::GoCB },
			{ "Go One &Instruction",			FL_F + 6,		SimulatorWindow::GoOneCB },
			{ "Run To &Cursor",					FL_F + 7,		RunToCursorCB, 0, FL_MENU_DIVIDER },
			{ "Step I&nto Next Function",		FL_F + 8,		SimulatorWindow::StepInCB },
			{ "Step O&ver Next Function",		FL_F + 9,		SimulatorWindow::StepOverCB },
			{ "Step &Out Of Current Function",	FL_F + 10,		SimulatorWindow::StepOutCB, 0, FL_MENU_DIVIDER },
			{ "&Signal Interrupt",				0,				SimulatorWindow::InterruptCB, 0, FL_MENU_INVISIBLE },
			{ "Brea&k",							FL_F + 11,		SimulatorWindow::BreakCB, 0, FL_MENU_INVISIBLE },
			{ "&Reset",							FL_F + 12,		SimulatorWindow::ResetCB, 0, FL_MENU_INVISIBLE },
			{ "&End Simulation",				FL_CTRL + 'q',	SimulatorWindow::StopCB, 0, FL_MENU_DIVIDER | FL_MENU_INVISIBLE },

			{ "&Breakpoints", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//41
				{ "&Breakpoints",	FL_CTRL + 'b',	SimulatorWindow::BreakpointsCB, 0, FL_MENU_DIVIDER },
				{ "Instruction Breakpoint At &Cursor",	FL_CTRL + FL_SHIFT + 'i',	InstructionBreakpointCB },
				{ "&Data Breakpoint At Cursor",			FL_CTRL + FL_SHIFT + 'd',	DataBreakpointCB },
				{ "Regis&ter Breakpoint At Cursor",		FL_CTRL + FL_SHIFT + 'r',	RegisterBreakpointCB },
				{ 0 },

			{ "Vie&w", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//47
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

			{ "&Edit && Continue", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//62
				{ "Edit &Instruction/Data/Memory",		FL_F + 2,	SimulatorWindow::WriteDataCB },
				{ "Edit &Register",						FL_F + 3,	SimulatorWindow::WriteRegisterCB, 0, FL_MENU_DIVIDER },
				{ "Edit Instructions/Data At Cursor",	0,			EditInstructionCB },
				{ "Edit Data At Cursor",				0,			EditDataCB },
				{ "Edit Register At Cursor",			0,			EditRegisterCB },
				{ 0 },

			{ 0 },

		{ 0 }
	};

Fl_Menu_Item FileWindow::PopupItems[] =
	{
		{ "Run To This &Line",						FL_F + 7,		RunToCursorCB, 0, FL_MENU_DIVIDER },
		{ "&Instruction Breakpoint At This Line",	FL_CTRL + FL_SHIFT + 'i',	InstructionBreakpointCB, 0, FL_MENU_INVISIBLE },
		{ "&Data Breakpoint At This Variable",		FL_CTRL + FL_SHIFT + 'd',	DataBreakpointCB, 0, FL_MENU_INVISIBLE },
		{ "&Register Breakpoint At This Register",	FL_CTRL + FL_SHIFT + 'r',	RegisterBreakpointCB, 0, FL_MENU_DIVIDER | FL_MENU_INVISIBLE },
		{ "View Instructions At This Line",			FL_CTRL + FL_SHIFT + '1',	ViewInstructionCB, 0, FL_MENU_INVISIBLE },
		{ "View Value Of This Variable",			FL_CTRL + FL_SHIFT + '2',	ViewDataCB, 0, FL_MENU_INVISIBLE },
		{ "View Memory Bytes At This Variable",		FL_CTRL + FL_SHIFT + '3',	ViewMemoryCB, 0, FL_MENU_INVISIBLE },
		{ "View Value Of This Register",			FL_CTRL + FL_SHIFT + '4',	ViewRegisterCB, 0, FL_MENU_DIVIDER | FL_MENU_INVISIBLE },
		{ "Edit Instructions/Data At This Line",	0,			EditInstructionCB, 0, FL_MENU_INVISIBLE },
		{ "Edit This Variable",						0,			EditDataCB, 0, FL_MENU_INVISIBLE },
		{ "Edit This Register",						0,			EditRegisterCB, 0, FL_MENU_INVISIBLE },
		{ 0 },
	};

Fl_Text_Display::Style_Table_Entry FileWindow::StyleTable[] = 
	{	// Style table
		{ FL_BLACK,		FL_COURIER,	14 } // A - Unknown
	};

list<Fl_Window *> FileWindow::WindowList;

FileWindow::FileWindow(int Width, int Height, const string &sFN, Fl_Text_Display::Style_Table_Entry *pInheritedStyle, unsigned int InheritedStyleCount) : Fl_Double_Window(Width, Height)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		pMainMenu->copy(MenuItems, this);
		//Setup the status bar
		pStatus = new Fl_Output(0, h()-28, w(), 28);
		pStatus->box(FL_PLASTIC_UP_BOX);
		//Setup the texteditor to use the textbuffer and stylebuffer
		if(pInheritedStyle)
			pTextEditor = new File_Text_Editor(0, 28, w(), h()-56, pInheritedStyle, InheritedStyleCount);
		else
			pTextEditor = new File_Text_Editor(0, 28, w(), h()-56, StyleTable, sizeof(StyleTable) / sizeof(StyleTable[0]));
		pTextEditor->pTextBuffer->add_modify_callback(TextChangedCB, this);
		pTextEditor->pTextBuffer->add_modify_callback(UpdateStyleCB, this);
		//Setup the pop-up menu
		pPopupMenu = new Fl_Menu_Button(0, 28, 0, 0);
		pPopupMenu->copy(PopupItems, this);
		pPopupMenu->type(Fl_Menu_Button::POPUP3);
	}
	end();
	resizable(pTextEditor);
	callback(CloseCB, this);
	WindowList.push_back(this);

	//Add the file text to the file buffer
	sFileNameFull = sFN;
	fChanged = false;
	fLoading = true;
	if(pTextEditor->pTextBuffer->loadfile(sFileNameFull.c_str()))
	{
		fl_alert("Error reading from file '%s'", sFileNameFull.c_str());
		sFileNameFull = "";
	}
	fLoading = false;

	show();
	//*NOTE: There is a bug in FLTK where it doesn't save the label if the window isn't shown yet
	SetTitle();
	SetStatus();
}

FileWindow::~FileWindow()
{
	hide();
	for(list<Fl_Window *>::iterator WindowIter = WindowList.begin(); WindowIter != WindowList.end(); WindowIter++)
	{
		if((FileWindow *)(*WindowIter) == this)
		{
			WindowList.erase(WindowIter);
			break;
		}
	}

	if(Project::pProject)
		TheProject.RemoveWindow(sFileNameFull);

	Fl::first_window()->show();
}

FileWindow::File_Text_Editor::File_Text_Editor(int X, int Y, int Width, int Height, Fl_Text_Display::Style_Table_Entry *pStyleTable, unsigned int nStyles) : TextEditor(X, Y, Width, Height, pStyleTable, nStyles)
{
}

int FileWindow::File_Text_Editor::handle(int Event)
{
	int RetVal = TextEditor::handle(Event);
	switch(Event)
	{
	case FL_KEYUP:
		((FileWindow *)parent())->SetStatus();
		break;
	case FL_RELEASE:
		((FileWindow *)parent())->SetStatus();
		if(Fl::event_button() == FL_RIGHT_MOUSE)
			((FileWindow *)parent())->pPopupMenu->popup();
		break;
	}
	return RetVal;
}

void FileWindow::File_Text_Editor::UpdateSelection()
{
	((FileWindow *)parent())->SetStatus();
}

bool FileWindow::SaveAll()
{
	bool fRetVal = true;
	for(list<Fl_Window *>::iterator WindowIter = WindowList.begin(); WindowIter != WindowList.end(); WindowIter++)
	{
		if(!((FileWindow *)(*WindowIter))->Save())
			fRetVal = false;
	}

	return fRetVal;
}

bool FileWindow::CloseAll()
{
	for(list<Fl_Window *>::iterator WindowIter = WindowList.begin(); WindowIter != WindowList.end(); WindowIter = WindowList.begin())
	{
		//Close() erases the window from this list, which invalidates this pointer.
		if(!((FileWindow *)(*WindowIter))->CloseNow())
			return false;
	}

	return true;
}

FileWindow *FileWindow::IsOpen(const string &sFileName)
{
	for(list<Fl_Window *>::iterator WindowIter = WindowList.begin(); WindowIter != WindowList.end(); WindowIter++)
	{
		if(((FileWindow *)(*WindowIter))->sFileNameFull == sFileName)
			return (FileWindow *)(*WindowIter);
	}
	return NULL;
}

bool FileWindow::UnHighlight()
{
	//Unhighlight all other files
	for(list<Fl_Window *>::iterator WindowIter = WindowList.begin(); WindowIter != WindowList.end(); WindowIter++)
		((FileWindow *)(*WindowIter))->pTextEditor->pTextBuffer->unhighlight();
	return true;
}

bool FileWindow::SelectLine(unsigned int LineNumber, bool fHighlight, bool fShow)
{
	int Start = 0, End;
	unsigned int Line;
	//Find the start of the line
	if(LineNumber > 0)
	{
		Line = 1;
		while(Line < LineNumber)
		{
			if(!pTextEditor->pTextBuffer->findchar_forward(Start, '\n', &Start))
				break;
			Start++;
			Line++;
		}
	}
	else
	{
		//If they select line 0, just show the window
		show();
		return true;
	}
	if(Start == pTextEditor->pTextBuffer->length())
		//no such line
		return false;

	//Find the end of the line
	if(pTextEditor->pTextBuffer->findchar_forward(Start, '\n', &End))
		End++;

	if(fHighlight)
		pTextEditor->pTextBuffer->highlight(Start, End);
	else
		pTextEditor->pTextBuffer->select(Start, End);
	pTextEditor->insert_position(Start);
	pTextEditor->show_insert_position();
	if(fShow)
		show();
	SetStatus();
//	pTextEditor->take_focus();
	return true;
}

bool FileWindow::GetFileLine(string &sFileName, unsigned int &LineNumber) const
{
	sFileName = sFileNameFull;
	LineNumber = pTextEditor->pTextBuffer->count_lines(0, pTextEditor->insert_position())+1;
	return true;
}

bool FileWindow::GetIdentifier(string &sIdentifier) const
{
	char *sID = pTextEditor->pTextBuffer->text_range(pTextEditor->pTextBuffer->word_start(pTextEditor->insert_position()), pTextEditor->pTextBuffer->word_end(pTextEditor->insert_position()));
	sIdentifier = sID;
	delete sID;
	if(sIdentifier == "")
		return false;
	return true;
}

bool FileWindow::GetRegister(string &sRegister) const
{
	return false;
}

int FileWindow::handle(int Event)
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

bool FileWindow::Save()
{
	if(sFileNameFull == "")
		return SaveAs();
	if(pTextEditor->pTextBuffer->savefile(sFileNameFull.c_str()))
	{
		show();
		fl_alert("Error writing to file '%s'", sFileNameFull.c_str());
		return false;
	}
	if(fChanged)
	{
		Project::FileData *pFD = TheProject.GetFile(sFileNameFull);
		if(pFD)
			pFD->fChanged = true;
	}
	fChanged = false;
	SetTitle();
	return true;
}

bool FileWindow::SaveAs()
{
	show();
	char *sNewFile;
	string sFileNameSave = sFileNameFull;
	do
	{
		sNewFile = fl_file_chooser("Save File As?", NULL, sFileNameFull.c_str());
		if(sNewFile == NULL)
			return false;
		sFileNameFull = CreateStandardPath(sNewFile);
		ifstream TestNew(sFileNameFull.c_str());
		if(!TestNew.good())
			break;
		fl_ask("File already exists, overwrite '%s'?", sFileNameFull.c_str());
	}
	while(true);

	if(!Save())
	{
		//Restore the original file name since the save-as did not succeed.
		sFileNameFull = sFileNameSave;
		return false;
	}
	else
	{
		if(Project::pProject)
		{
			//Check to see if this file is in the project
			if(TheProject.GetFile(sFileNameFull))
			{
				//Remove the window for the old file
				TheProject.RemoveWindow(sFileNameSave);
				//OpenFile will decide whether to add this to the project or not.
				TheMainWindow.OpenFile(sFileNameFull);
			}
		}
		return true;
	}
}

bool FileWindow::Close()
{
	if(Fl::event_key() == FL_Escape)
		// ignore Escape
		return false;

	if(!CheckSave())
		return false;

	Fl::delete_widget(this);
	return true;
}

bool FileWindow::CloseNow()
{
	if(Fl::event_key() == FL_Escape)
		// ignore Escape
		return false;

	if(!CheckSave())
		return false;

	delete this;
	return true;
}

bool FileWindow::Help() const
{
	return TheMainWindow.Help(AshIDEIndex, "FileWindow");
}

bool FileWindow::CheckSave()
{
	if(!fChanged) return true;

	show();
	int Choice = fl_choice("The current file has changes that have not been saved",
		"Cancel", "Save", "Discard");

	switch(Choice)
	{
	case 0:
		return false;
	case 1:
		return Save();
	case 2:
		return true;
	}

	return true;
}

bool FileWindow::AddToProject(bool fAsk)
{
	if(!Project::pProject)
	{
		//There is no project open yet.
		if(!TheMainWindow.OpenProject())
			return false;
	}
	if(sFileNameFull == "")
	{
		//This file does not yet have a name.
		if(!SaveAs())
			return false;
	}
	Project::FileData *pFileData = TheProject.GetFile(sFileNameFull);
	if(pFileData)
		//This file is already in the current project
		fAsk = false;

	if(fAsk && !fl_ask("Add file '%s' to project '%s'?", sFileNameFull.substr(sFileNameFull.find_last_of("\\/:")+1).c_str(), TheProject.sFileName.Bare.c_str()))
		return false;

	TheProject.AddFile(sFileNameFull);
	//Make the project menu items visible
	pMainMenu->mode(22, pMainMenu->mode(22) | FL_MENU_INVISIBLE);
	for(unsigned int i = 23; i <= 28; i++)
		pMainMenu->mode(i, pMainMenu->mode(i) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(30, pMainMenu->mode(30) & ~FL_MENU_INVISIBLE);
	//Possibly make the simulator menu items visible
	SimulatorOptions();
	pMainMenu->redraw();
	return true;
}

bool FileWindow::RemoveFromProject(bool fAsk)
{
	if(!Project::pProject || sFileNameFull == "" || !TheProject.GetFile(sFileNameFull))
		//It's not in the current project
		return true;
	if(fAsk && !fl_ask("Remove file '%s' from project '%s'?", sFileNameFull.substr(sFileNameFull.find_last_of("\\/:")+1).c_str(), TheProject.sFileName.Bare.c_str()))
		return false;
	TheProject.RemoveFile(sFileNameFull);
	//Make the project menu items invisable
	pMainMenu->mode(22, pMainMenu->mode(22) & ~FL_MENU_INVISIBLE);
	for(unsigned int i = 23; i <= 28; i++)
		pMainMenu->mode(i, pMainMenu->mode(i) | FL_MENU_INVISIBLE);
	pMainMenu->mode(30, pMainMenu->mode(30) | FL_MENU_INVISIBLE);
	pMainMenu->redraw();
	return true;
}

bool FileWindow::SimulatorOptionsAll()
{
	//Unhighlight all other files
	for(list<Fl_Window *>::iterator WindowIter = WindowList.begin(); WindowIter != WindowList.end(); WindowIter++)
		((FileWindow *)(*WindowIter))->SimulatorOptions();
	return true;
}

bool FileWindow::SimulatorOptions()
{
	unsigned int i;
	if(TheProject.IsSimulating(sFileNameFull))
	{
		pMainMenu->mode(6, pMainMenu->mode(6) & ~FL_MENU_DIVIDER);
		pMainMenu->mode(7, pMainMenu->mode(7) | FL_MENU_INVISIBLE);
		pMainMenu->mode(21, pMainMenu->mode(21) | FL_MENU_INVISIBLE);
		pMainMenu->mode(36, pMainMenu->mode(36) | FL_MENU_DIVIDER);
		for(i = 37; i <= 40; i++)
			pMainMenu->mode(i, pMainMenu->mode(i) & ~FL_MENU_INVISIBLE);
		pMainMenu->mode(41, pMainMenu->mode(41) & ~FL_MENU_INVISIBLE);
		pMainMenu->mode(47, pMainMenu->mode(47) & ~FL_MENU_INVISIBLE);
		pMainMenu->mode(62, pMainMenu->mode(62) & ~FL_MENU_INVISIBLE);
		pMainMenu->redraw();

		pPopupMenu->mode(0, pPopupMenu->mode(0) | FL_MENU_DIVIDER);
		for(i = 0; i <= 10; i++)
			pPopupMenu->mode(i, pPopupMenu->mode(i) & ~FL_MENU_INVISIBLE);
	}
	else if(TheProject.fSimulating)
	{
		//If it is simulating, but this file is not part of the simulation, just disable the whole menu
		pMainMenu->mode(6, pMainMenu->mode(6) & ~FL_MENU_DIVIDER);
		pMainMenu->mode(7, pMainMenu->mode(7) | FL_MENU_INVISIBLE);
		pMainMenu->mode(21, pMainMenu->mode(21) | FL_MENU_INVISIBLE);
		pMainMenu->mode(30, pMainMenu->mode(30) | FL_MENU_INVISIBLE);
		pMainMenu->redraw();

		for(i = 0; i <= 10; i++)
			pPopupMenu->mode(i, pPopupMenu->mode(i) | FL_MENU_INVISIBLE);
	}
	else
	{
		pMainMenu->mode(6, pMainMenu->mode(6) | FL_MENU_DIVIDER);
		pMainMenu->mode(7, pMainMenu->mode(7) & ~FL_MENU_INVISIBLE);
		pMainMenu->mode(21, pMainMenu->mode(21) & ~FL_MENU_INVISIBLE);
		pMainMenu->mode(36, pMainMenu->mode(36) & ~FL_MENU_DIVIDER);
		for(i = 37; i <= 40; i++)
			pMainMenu->mode(i, pMainMenu->mode(i) | FL_MENU_INVISIBLE);
		pMainMenu->mode(41, pMainMenu->mode(41) | FL_MENU_INVISIBLE);
		pMainMenu->mode(47, pMainMenu->mode(47) | FL_MENU_INVISIBLE);
		pMainMenu->mode(62, pMainMenu->mode(62) | FL_MENU_INVISIBLE);
		pMainMenu->redraw();

		pPopupMenu->mode(0, pPopupMenu->mode(0) & ~FL_MENU_DIVIDER);
		for(i = 1; i <= 10; i++)
			pPopupMenu->mode(i, pPopupMenu->mode(i) | FL_MENU_INVISIBLE);
	}
	return true;
}

void FileWindow::RunToCursorCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::RunToCursorCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::InstructionBreakpointCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::InstructionBreakpointCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::DataBreakpointCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::DataBreakpointCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::RegisterBreakpointCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::RegisterBreakpointCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::ViewInstructionCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::ViewInstructionCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::ViewDataCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::ViewDataCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::ViewMemoryCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::ViewMemoryCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::ViewRegisterCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::ViewRegisterCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::EditInstructionCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::EditInstructionCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::EditDataCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::EditDataCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

void FileWindow::EditRegisterCB(Fl_Widget *pW, void *pV)
{
	SimulatorWindow::EditRegisterCB(SimulatorWindow::SimFileWindow, (Fl_Window *)pV);
}

bool FileWindow::TextChanged(int Pos, int nInserted, int nDeleted, int nRestyled, const char *pszDeleted)
{
	if((nInserted || nDeleted) && !fLoading)
	{
		if(!fChanged)
		{
			fChanged = true;
			//Updated the title when the file becomes modified
			SetTitle();
		}
	}

	if(fLoading)
		pTextEditor->show_insert_position();

	return true;
}

bool FileWindow::SetTitle()
{
	string sTitle = sFileNameFull;

	int LastSlash = sTitle.find_last_of("\\/:");
	sTitle = sTitle.substr(LastSlash+1);
	if(fChanged)
		sTitle += " (modified)";

	label(sTitle.c_str());
	return true;
}

bool FileWindow::SetStatus()
{
	char sBuffer[64];
	unsigned int Length;

	if(!shown())
		return false;

	Length = sprintf(sBuffer, "Line: %u ", pTextEditor->pTextBuffer->count_lines(0, pTextEditor->insert_position())+1);
	for(; Length < 14; Length++)
		sBuffer[Length] = ' ';
	sprintf(&sBuffer[Length], "Char: %u", pTextEditor->insert_position() - pTextEditor->pTextBuffer->line_start(pTextEditor->insert_position()));
	pStatus->value(sBuffer);

	return true;
}

bool FileWindow::UpdateStyle(int Pos, int Inserted, int Deleted, int Restyled, const char *pszDeleted)
{
	int	Start, End;

	char *pStyle, *pText;

	//*NOTE: Okay you'll never get this one.
	//The "this" pointer is not valid during construction, it looks like just an instance
	//of the base class, instead of an instance of the inherited class.
	//When the file is opened in the FileWindow constructor, it calls this function.
	//When this function call ParseStyle, it calls FileWindow::ParseStyle() even if this is an
	//inherited instance.
	//To get around that, the constructor of the inherited instance calls the callbacks again.
	//However, since no text has changed inbetween the calls, it would normally return.
	//To get around that, I call this pretending everything was deleted and reinserted..

	//If this is just a selection change, then unselect the style buffer
	if(Inserted == 0 && Deleted == 0)
	{
		pTextEditor->pStyleBuffer->unselect();
		return true;
	}

	//Track changes in the text buffer
	if(Inserted > 0)
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

bool FileWindow::ParseStyle(const string &sText, string &sStyle)
{
	sStyle.resize(sText.size(), 'A');
	return true;
}

}	//namespace AshIDE
