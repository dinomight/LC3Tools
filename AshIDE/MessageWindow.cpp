//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "MessageWindow.h"
#include <FL/Fl.H>
#include "Project.h"
#include "MainWindow.h"
#include "FileWindow.h"

using namespace std;
using namespace JMT;

namespace AshIDE	{

Fl_Text_Display::Style_Table_Entry MessageWindow::StyleTable[] = 
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

MessageWindow::MessageWindow(int X, int Y, int Width, int Height) : ReadOnlyEditor(X, Y, Width, Height, StyleTable, sizeof(StyleTable) / sizeof(StyleTable[0]))
{
	{
	}
	end();
}

MessageWindow::~MessageWindow()
{
}

int MessageWindow::handle(int Event)
{
	unsigned int Start, End, LineNumber;
	char *sText;
	string sFileName;
	FileWindow *pFW;

	int RetVal = ReadOnlyEditor::handle(Event);
	switch(Event)
	{
	case FL_RELEASE:
		if(Fl::event_clicks())	//Double click
		{
			//Find the filename in this line's message
			End = Start = pTextBuffer->line_start(insert_position());
			while(pStyleBuffer->character(End) == 'R')
				End++;
			if(End == Start)
				//There was no file name on this line
				break;

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

			if(ToLower(sFile.Ext) == "obj" || ToLower(sFile.Ext) == "bin")
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

bool MessageWindow::Message(MessageEnum MessageType, const string &sMessage, const LocationVector &LocationStack)
{
	unsigned int i, StackNumber = LocationStack.size()-1;
	//store the last locationstack so that we don't reprint a possibly long "From included file" list
	//when the current error happens in the same file as the one we just printed.
	static LocationVector LastLocation;

	if(!LocationStack.empty())
	{
		for(i = 0; i < StackNumber; i++)
		{
			if(LastLocation.size() > i+1 && LocationStack[i] == LastLocation[i])
				continue;
			else
				LastLocation.clear();
			FileNameLine(InputList[LocationStack[i].first], LocationStack[i].second);
			PlainText("    ");
			MessageID(Info);
			PlainText("From included file:\n");
		}
		LastLocation = LocationStack;
	}

	switch(MessageType)
	{
	case Info:
		if(!LocationStack.empty())
		{
			FileNameLine(InputList[LocationStack[i].first], LocationStack[i].second);
			MessageID(Info);
		}
		break;
	case Warning:
		TheProject.Warnings++;
		if(LocationStack.empty())
			throw "Empty location stack for Warning!";
		FileNameLine(InputList[LocationStack[i].first], LocationStack[i].second);
		MessageID(Warning);
		break;
	case Error:
		TheProject.Errors++;
		if(LocationStack.empty())
			throw "Empty location stack for Error!";
		FileNameLine(InputList[LocationStack[i].first], LocationStack[i].second);
		MessageID(Error);
		break;
	case Fatal:
		TheProject.Errors++;
		if(!LocationStack.empty())
			FileNameLine(InputList[LocationStack[i].first], LocationStack[i].second);
		MessageID(Fatal);
		break;
	}

	PlainText(sMessage);
	PlainText("\n");

	//*NOTE: The insert position is always at the beginning, even after appending
	//so show_insert_position() does nothing.
	scroll((unsigned int)-1 >> 1, 0);

	return true;
}

bool MessageWindow::FileNameLine(const string &sFileName, unsigned int LineNumber)
{
	char sNumber[16];

	//Append the filename
	pTextBuffer->append(sFileName.c_str());
	pStyleBuffer->append(string(sFileName.size(), 'R').c_str());

	//append the line number
	pTextBuffer->append("(");
	pStyleBuffer->append(string(1, 'J').c_str());

	sprintf(sNumber, "%u", LineNumber);
	pTextBuffer->append(sNumber);
	pStyleBuffer->append(string(strlen(sNumber), 'G').c_str());

	pTextBuffer->append("):");
	pStyleBuffer->append(string(2, 'J').c_str());

	return true;
}

bool MessageWindow::PlainText(const string &sText)
{
	pTextBuffer->append(sText.c_str());
	pStyleBuffer->append(string(sText.size(), 'A').c_str());
	return true;
}

bool MessageWindow::MessageID(MessageEnum MessageType)
{
	switch(MessageType)
	{
	case Info:
		pTextBuffer->append("    Info:   ");
		pStyleBuffer->append(string(12, 'S').c_str());
		break;
	case Warning:
		pTextBuffer->append(" Warning:   ");
		pStyleBuffer->append(string(12, 'T').c_str());
		break;
	case Error:
		pTextBuffer->append("   Error:   ");
		pStyleBuffer->append(string(12, 'U').c_str());
		break;
	case Fatal:
		pTextBuffer->append("   Fatal:   ");
		pStyleBuffer->append(string(12, 'V').c_str());
		break;
	}
	return true;
}

}	//namespace AshIDE
