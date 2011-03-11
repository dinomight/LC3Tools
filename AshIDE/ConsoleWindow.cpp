//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "ConsoleWindow.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include "Project.h"
#include "SimulatorWindow.h"
#include "MainWindow.h"
#include "../AsmConvertLC3/AsmConvertLC3.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace AshIDE	{

Fl_Menu_Item ConsoleWindow::MenuItems[] =
	{
		{ "&Console", 0, 0, 0, FL_SUBMENU },
			{ "&Save",			FL_CTRL + 's',	SaveCB, 0, FL_MENU_DIVIDER },
			{ "&Copy",			FL_CTRL + 'c',	CopyCB },
			{ "&Clear",			0,				ClearCB, 0, FL_MENU_DIVIDER },
			{ "&Help",			FL_F + 1,		HelpCB, 0, FL_MENU_DIVIDER },
			{ "C&lose",			FL_Escape,		HideCB },
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

ConsoleWindow::ConsoleWindow() : Fl_Double_Window(320, 200)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), 28);
		pMainMenu->copy(MenuItems, this);
		//Setup the console output
		pTextDisplay = new ReadOnlyEditor(0, 28, w(), h()-28*4);
		//Setup the status bar
		pStatus = new Fl_Output(0, h()-28, w(), 28);
		pStatus->box(FL_PLASTIC_UP_BOX);
		//Setup the console input
		Fl_Box *pB = new Fl_Box(0, h()-28*3, 80, 28, "Input Text:");
		pB->box(FL_PLASTIC_UP_BOX);
		pInput = new Fl_Input(80, h()-28*3, w()-160, 28);
		pEnter = new Fl_Return_Button(w()-80, h()-28*3, 80, 28, "Enter");
		pEnter->callback(EnterCB, this);
		//Setup the ascii input
		pB = new Fl_Box(0, h()-28*2, 80, 28, "ASCII Code:");
		pB->box(FL_PLASTIC_UP_BOX);
		pInputAscii = new Fl_Input(80, h()-28*2, w()-160, 28);
		pEnterAscii = new Fl_Button(w()-80, h()-28*2, 80, 28, "Enter Code");
		pEnterAscii->callback(EnterAsciiCB, this);
	}
	end();
	resizable(pTextDisplay);
	callback(HideCB, this);

//	fWaitForInput = false;
}

ConsoleWindow::~ConsoleWindow()
{
	hide();
	TheSimulatorWindow.pConsoleWindow = NULL;
	Fl::first_window()->show();
}

void ConsoleWindow::HelpCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.Help(AshIDEIndex, "ConsoleWindow");
}

bool ConsoleWindow::SetTitle()
{
	string sTitle = "Console";

//	if(fWaitForInput)
//		sTitle += " (Waiting For User Input)";

	label(sTitle.c_str());
	return true;
}

bool ConsoleWindow::Show()
{
	show();
	SetTitle();
	return true;
}

bool ConsoleWindow::Hide()
{
//	if(fWaitForInput)
//	{
//		fl_message("The console cannot close while it is waiting for user input.");
//		return false;
//	}
//	else
//	{
		hide();
		Fl::first_window()->show();
		return true;
//	}
}

int ConsoleWindow::handle(int Event)
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

bool ConsoleWindow::Enter()
{
	const char *sInput = pInput->value();
	pInput->value("");
	if(sInput[0] == 0)
	{	//string is blank
		if(sConsoleInput.size() == 0 || sConsoleInput[sConsoleInput.size()-1] != 4)
			sConsoleInput += 4;	//EOI
		return false;
	}
	if(sConsoleInput.size() && sConsoleInput[sConsoleInput.size()-1] == 4)
		sConsoleInput.resize(sConsoleInput.size()-1);	//remove EOI
	sConsoleInput += sInput;
	for(unsigned int i = 0; i < strlen(sInput); i++)
		TheArch(Interrupt(TheArch(KeyboardInterruptVector)));
	return true;
}

bool ConsoleWindow::EnterAscii()
{
	const char *sInput = pInputAscii->value();
	string sAscii;
	AsmConvertLC3Line(sInput, sAscii, LocationVector(), MainWindow::SimMessageCallBack);
	list<Token *> TokenList;
	LC3ISA::Lexer TheLexer(TokenList, MainWindow::MessageCallBack);
	TheLexer.LexLine(LocationVector(), sAscii);
	if(TokenList.empty() || (*TokenList.begin())->TokenType != TInteger)
	{
		//get rid of the used tokens
		for(list<Token *>::iterator TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
 			delete *TokenIter;
		fl_alert("You must provide a number for the ASCII code.");
		return false;
	}
	if(((IntegerToken *)(*TokenList.begin()))->Integer >= 256)
	{
		//get rid of the used tokens
		for(list<Token *>::iterator TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
 			delete *TokenIter;
		fl_alert("ASCII code must be less than 256.");
		return false;
	}

	if(sConsoleInput.size() && sConsoleInput[sConsoleInput.size()-1] == 4)
		sConsoleInput.resize(sConsoleInput.size()-1);	//remove EOI
	sConsoleInput += (char)((IntegerToken *)(*TokenList.begin()))->Integer;
	TheArch(Interrupt(TheArch(KeyboardInterruptVector)));
	for(list<Token *>::iterator TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
 		delete *TokenIter;
	return true;
}

bool ConsoleWindow::SimReadConsole(string &sBuffer, unsigned int CharsToRead, unsigned int &CharsRead)
{
	if(!shown())
		Show();
	CharsRead = 0;
	sBuffer = "";
	fflush(NULL);
	if(CharsToRead == 0)
		return true;

/*Blocking read code
	while(TheProject.fSimulating)
	{
		if(sConsoleInput.size() > 0)
		{
			fWaitForInput = false;
			if(sConsoleInput[0] == 4)
			{
				return true;
			}
			else if(sConsoleInput[0] == -1)
			{
				return false;
			}
			else
			{
				sBuffer += sConsoleInput[0];
				sConsoleInput = sConsoleInput.substr(1);
				CharsRead++;
				if(CharsRead == CharsToRead)
					return true;
			}
		}
		else
		{
			fWaitForInput = true;
			Fl::check();
		}
	}
	fWaitForInput = false;
*/

	//Non-blocking read code
	for(unsigned int i = 0; i < CharsToRead; i++)
	{
		if(sConsoleInput.size() == 0)
			return true;
		if(sConsoleInput[0] == 4)
			return true;
		else if(sConsoleInput[0] == -1)
			return false;
		sBuffer += sConsoleInput[0];
		sConsoleInput = sConsoleInput.substr(1);
		CharsRead++;
	}
	return true;
}

bool ConsoleWindow::SimWriteConsole(const string &sBuffer, unsigned int CharsToWrite, unsigned int &CharsWritten)
{
	if(!shown())
		Show();
	CharsWritten = 0;
	char Buff[2] = {0,0};
	for(unsigned int i = 0; i < sBuffer.size(); i++)
	{
		if(i == CharsToWrite)
			break;
		Buff[0] = sBuffer[i];
		pTextDisplay->pTextBuffer->append(Buff);
		CharsWritten++;
	}
	pTextDisplay->scroll((unsigned int)-1 >> 1, 0);
	return true;
}

}	//namespace AshIDE
