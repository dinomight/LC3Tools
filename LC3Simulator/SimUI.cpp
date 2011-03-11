//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#pragma warning (disable:4503)
#include "SimUI.h"
#include <strstream>
#include <cstdio>
#include "../LC3Simulator/LC3Arch.h"
#include "../LC3Assembler/LC3ISA.h"
#include "../Simulator/Simulator.h"
#include "../AsmConvertLC3/AsmConvertLC3.h"

using namespace std;
using namespace JMT;
using namespace Simulator;

namespace LC3
{

static unsigned int Warnings = 0, Errors = 0;
static bool fSignalWrite = true, fLeftOvers = false;

bool SimulatorUI(vector<Program *> &AsmPrograms, RamVector &MemoryImage)
{
	ArchSim<LC3ISA> TheSim(SimMessageCallBack, SimCallBack, SimCommand, SimReadConsole, SimWriteConsole);
	LC3Arch TheArch(TheSim);

	char sMessageBuffer[64];
	SimCallBack(Info, "");
	sprintf(sMessageBuffer, "\t\tLC-3 Simulator %.15s, Ashley Wise", SIM_VER);
	SimCallBack(Info, sMessageBuffer);
	SimCallBack(Info, "");

#ifdef BIG_ENDIAN_BUILD
	if(EndianCheck())
	{
		SimCallBack(Fatal, "Platform is little endian; assembler built for big endian! Aborting...");
		return false;
	}
#else
	if(!EndianCheck())
	{
		SimCallBack(Fatal, "Platform is big endian; assembler built for little endian! Aborting...");
		return false;
	}
#endif

	return TheSim.CommandRun(TheArch, AsmPrograms, MemoryImage);
}

bool SimCallBack(MessageEnum MessageType, const string &sMessage)
{
	unsigned int i, j, Length, PreLength, OrigPreLength, CutOff, Temp;
	ostrstream sMsg;
	const char *Buffer;
	char DBuffer[MAX_CONSOLE_WIDTH+1];
	char Space[MAX_CONSOLE_WIDTH/2+1];
	memset(Space, ' ', ConsoleWidth/2);

	if(!fSignalWrite)
	{
		fSignalWrite = true;
		cout << "\n\n";
	}

	switch(MessageType)
	{
	case Info:
		break;
	case Warning:
		Warnings++;
		sMsg << "   Warning:   ";
		break;
	case Error:
		Errors++;
		sMsg << "     Error:   ";
		break;
	case Fatal:
		Errors++;
		sMsg << "     Fatal:   ";
		break;
	case Exception:
		Errors++;
		sMsg << " Exception:   ";
		break;
	case Breakpoint:
		sMsg << "Breakpoint:   ";
		break;
	}

	//Print the pre-message
	OrigPreLength = PreLength = sMsg.pcount();

	//If the pre-message is longer than half the width of the console,
	//add a newline and spaces so that it looks like the pre-message
	//ended at half-width.
	if(PreLength > ConsoleWidth/2)
	{
		sMsg << "\n";
		Space[ConsoleWidth/2] = 0;
		PreLength = ConsoleWidth/2;
		sMsg << Space;
	}
	else
		Space[PreLength] = 0;

	Length = sMessage.size();
	Buffer = sMessage.c_str();
	if(Length >= 4 && Buffer[0] == ' ' && Buffer[1] == ' ' && Buffer[2] == ' ' && Buffer[3] == ' ')
		OrigPreLength = 4;

	//Check for tabs, which increases the effective length of the string
	for(i = 0; Buffer[i]; i++)
		if(Buffer[i] == '\t')
			PreLength += CONSOLE_TAB_SIZE - 1;

	//Print lines of the message
	while(true)
	{
		//Determine where to partition this message so that it fits in the console
		//The -1 is there so that there is room for the newline character.
		Temp = CutOff = MIN(Length, ConsoleWidth-PreLength-1);
		if(Temp != Length)
		{	//Adjust the cutoff so that it doesn't break words
			while(Buffer[Temp] != ' ' && Buffer[Temp] != '\n' && Buffer[Temp] != '\t' && Buffer[Temp] != '\r' && Temp > 0)
				Temp--;
			if(Temp > 0)
				CutOff = Temp;
		}
		for(i = 0, j = 0; i < CutOff; i++)
		{
			if(Buffer[i] != '\n')
			{
				if(Buffer[i] == '\t')
					PreLength -= CONSOLE_TAB_SIZE - 1;
				DBuffer[j++] = Buffer[i];
			}
		}
		DBuffer[j] = 0;
		while(CutOff != Length && (Buffer[CutOff] == ' ' || Buffer[CutOff] == '\n' || Buffer[CutOff] == '\t' || Buffer[CutOff] == '\r'))
			CutOff++;
		Buffer = &Buffer[CutOff];
		Length -= CutOff;
		sMsg << DBuffer << '\n';

		//While there is still message left, print spaces to line it up
		if(Length > 0)
		{
			if(OrigPreLength == 0)
			{
				OrigPreLength = PreLength = MIN(14, ConsoleWidth/2) + PreLength;
				Space[0] = ' ';
				Space[MIN(14, ConsoleWidth/2)] = 0;
			}
			sMsg << Space;
		}
		else
			break;
	}

	sMsg << ends;
	cout << sMsg.str();

	cout.flush();

	return true;
}

bool SimMessageCallBack(MessageEnum MessageType, const string &sMessage, const LocationVector &)
{
	return SimCallBack(MessageType, sMessage);
}

//*NOTE: HORRIBLE bug in GCC 3.x Linux causes a call to cin.seekg() to irreversably corrupt the input stream!
//So I've changed all the cins to use C-style stdin.
bool SimCommand(string &sCommand)
{
	if(fLeftOvers)
	{
		fseek(stdin, 0, SEEK_END);
		fLeftOvers = false;
	}

	cout << "sim> ";
	fflush(NULL);

	//Temporary string storage
	char sTempBuff[MAX_LINE+2];
	char sMessageBuffer[64];
	bool fRetVal = true;

	sTempBuff[MAX_LINE+1] = 1;
	//get a line from the assembly file
	fgets(sTempBuff, MAX_LINE+1, stdin);
	//*NOTE: MSVC's STL version of get() sets the failbit if it gets no characters.
	if(!sTempBuff[0])
		clearerr(stdin);
	fseek(stdin, 0, SEEK_END);

	if(ferror(stdin))
	{
		SimCallBack(Fatal, "Error reading command.");
		clearerr(stdin);
		return false;
	}
	if(feof(stdin))
		clearerr(stdin);
	
	//Check to see if the line was too long
	if(!sTempBuff[MAX_LINE+1])
	{
		sTempBuff[MAX_LINE] = 0;
		sprintf(sMessageBuffer, "Line exceeds %u characters. Excess ignored.", MAX_LINE);
		SimCallBack(Warning, sMessageBuffer);
	}

	//Return the input
	string sTemp = sTempBuff;
	sTemp = sTemp.substr(0, sTemp.find_first_of("\x0A\x0D\x04\xFF"));

	//Check to see if we should run the translator on it
	if(Flags.fOldLC3)
	{
		sCommand = "";
		if(!AsmConvertLC3Line(sTemp, sCommand, LocationVector(), SimMessageCallBack))
			return false;
	}

	return true;
}

bool SimReadConsole(string &sBuffer, unsigned int CharsToRead, unsigned int &CharsRead)
{
	char sMessageBuffer[64];
	sprintf(sMessageBuffer, "<Simulating program requests %u character%.1s>", CharsToRead, (CharsToRead != 1 ? "s" : ""));
	SimCallBack(Info, sMessageBuffer);

	fLeftOvers = false;
	CharsRead = 0;
	sBuffer = "";
	fflush(NULL);
	if(CharsToRead == 0)
		return true;

	while(true)
	{
		if(CharsRead == CharsToRead)
		{
			if(sBuffer[CharsRead-1] != '\n')
			{
				//*NOTE: Windows console keeps the CTRL-D(4) and following characters, including the required \n(A)
				//UNIX console returns from get() as soon as CTRL-D(4) is sent.
				//The CTRL-D and all following characters are no longer available to get/peek.
#if !defined UNIX_BUILD
				if(cin.peek() == 4)
				{	//End-Of-Input given after last requested character
					fseek(stdin, 0, SEEK_END);
				}
				else
#else
				cout << endl;
#endif
					fLeftOvers = true;
			}
			return true;
		}

		int TempC = fgetc(stdin);

		//*NOTE: CTRL-D on Unix is treated like a CTRL-Z on Windows.
		//-1 is sent if CTRL-D is the only input. CTRL-Z on unix suspends the program instead of treating it as EOF
#if !defined UNIX_BUILD
		if(TempC == 4)	//End-Of-Input CTRL-D
		{
			fseek(stdin, 0, SEEK_END);
			return false;
		}
#endif
		if(TempC == -1)	//CTRL-C or CTRL-Z
		{
			clearerr(stdin);
			return false;
		}

		sBuffer += (char)TempC;
		CharsRead++;
	}
}

bool SimWriteConsole(const string &sBuffer, unsigned int CharsToWrite, unsigned int &CharsWritten)
{
	if(fSignalWrite)
	{
		char sMessageBuffer[64];
		sprintf(sMessageBuffer, "<Simulating program outputting characters>");
		SimCallBack(Info, sMessageBuffer);
	}
	fSignalWrite = false;

	CharsWritten = 0;
	for(unsigned int i = 0; i < sBuffer.size(); i++)
	{
		if(i == CharsToWrite)
			break;
		cout.put(sBuffer[i]);
		cout.clear();
		CharsWritten++;
	}
	return true;
}

}	//namespace LC3

/*
Old versions of SimCommand and SimReadConsole which worked with half-decent C++ runtimes.
HOWEVER bloody GCC 3.x on redhat linux is the most horribly broken compiler I've ever seen.

bool LC3bTools::SimCommand(string &sCommand)
{
	if(fLeftOvers)
	{
		cin.seekg(0, ios::end);
		fLeftOvers = false;
	}
	cout << "sim> ";
	fflush(NULL);

	//Temporary string storage
	char sTempBuff[MAX_LINE+2];
	char sMessageBuffer[64];
	bool fRetVal = true;

	sTempBuff[MAX_LINE+1] = 1;
	//get a line from the assembly file
	cin >> sTempBuff;
	//*NOTE: MSVC's STL version of get() sets the failbit if it gets no characters.
	if(!sTempBuff[0])
		cin.clear(cin.rdstate() & ~ios::failbit);
	cin.seekg(0, ios::end);

	if(cin.bad())
	{
		SimCallBack(Fatal, "Error reading command.");
		cin.clear();
		return false;
	}
	if(cin.eof())
		cin.clear();
	
	//Check to see if the line was too long
	if(!sTempBuff[MAX_LINE+1])
	{
		sTempBuff[MAX_LINE] = 0;
		sprintf(sMessageBuffer, "Line exceeds %u characters. Excess ignored.", MAX_LINE);
		SimCallBack(Warning, sMessageBuffer);
	}

	//Return the input
	sCommand = sTempBuff;

	return true;
}

bool LC3bTools::SimReadConsole(string &sBuffer, unsigned int CharsToRead, unsigned int &CharsRead)
{
	char sMessageBuffer[64];
	sprintf(sMessageBuffer, "<Simulating program requests %u character%.1s>", CharsToRead, (CharsToRead > 1 ? "s" : ""));
	SimCallBack(Info, sMessageBuffer);

	fLeftOvers = false;
	CharsRead = 0;
	sBuffer = "";
	cout << flush;
	while(true)
	{
		if(CharsRead == CharsToRead)
		{
			if(sBuffer[CharsRead-1] != '\n')
			{
				//*NOTE: Windows console keeps the CTRL-D(4) and following characters, including the required \n(A)
				//UNIX console returns from get() as soon as CTRL-D(4) is sent.
				//The CTRL-D and all following characters are no longer available to get/peek.
#if !defined UNIX_BUILD
				if(cin.peek() == 4)
				{	//End-Of-Input given after last requested character
					cin.seekg(0, ios::end);
				}
				else
#else
				cout << endl;
#endif
					fLeftOvers = true;
			}
			return true;
		}

		int TempC = cin.get();

		//*NOTE: CTRL-D on Unix is treated like a CTRL-Z on Windows.
		//-1 is sent if CTRL-D is the only input. CTRL-Z on unix suspends the program instead of treating it as EOF
#if !defined UNIX_BUILD
		if(TempC == 4)	//End-Of-Input CTRL-D
		{
			cin.seekg(0, ios::end);
			return false;
		}
#endif
		if(TempC == -1)	//CTRL-C or CTRL-Z
		{
			cin.clear();
			return false;
		}

		sBuffer += (char)TempC;
		CharsRead++;
	}
}
*/
