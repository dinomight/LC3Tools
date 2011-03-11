//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "AsmUI.h"
#include <fstream>
#include <strstream>
#include <cstdio>
#include "../Assembler/Assembler.h"
#include "../LC3bAssembler/LC3bISA.h"

using namespace std;
using namespace JMT;
using namespace Assembler;
using namespace LC3b;

namespace LC3b
{

static unsigned int Warnings = 0, Errors = 0;

bool AssemblerUI(vector<Program *> &AsmPrograms, RamVector &MemoryImage)
{
	bool fRetVal = true;
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	unsigned int i;
	ofstream VHDLFile;

	AsmCallBack(Info, "", LocationVector());
	sprintf(sMessageBuffer, "\t\tLC-3b Assembler %.15s, Ashley Wise", ASM_VER);
	AsmCallBack(Info, sMessageBuffer, LocationVector());
	AsmCallBack(Info, "", LocationVector());

#ifdef BIG_ENDIAN_BUILD
	if(EndianCheck())
	{
		AsmCallBack(Fatal, "Platform is little endian; assembler built for big endian! Aborting...", LocationVector());
		return false;
	}
#else
	if(!EndianCheck())
	{
		AsmCallBack(Fatal, "Platform is big endian; assembler built for little endian! Aborting...", LocationVector());
		return false;
	}
#endif

	//Initialize program list
	for(i = 0; i < InputList.size(); i++)
	{
		LocationVector LocationStack;
		LocationStack.push_back( LocationVector::value_type(i, 0) );
		AsmPrograms.push_back(new Program(LocationStack, InputList[i], LC3bISA::Addressability));
	}
	
	//*** Perform Compilation ***

	//Lex, parse, and optimize each input asm file
	AsmCallBack(Info, "Compiling...", LocationVector());
	for(i = 0; i < AsmPrograms.size(); i++)
	{
		//Open the assembly file
		if(ToLower(AsmPrograms[i]->sFileName.Ext) == "obj" || ToLower(AsmPrograms[i]->sFileName.Ext) == "bin")
		{
			ifstream AsmFile(InputList[i].c_str(), ios::in | ios::binary);
			if(!AsmFile.good())
			{
				sprintf(sMessageBuffer, "Unable to open file %.255s", InputList[i].c_str());
				AsmCallBack(Fatal, sMessageBuffer, LocationVector());
				fRetVal = false;
				continue;
			}
			if(!Assemble<LC3bISA>::Disassemble(AsmFile, *AsmPrograms[i], AsmCallBack))
				fRetVal = false;
		}
		else
		{
			ifstream AsmFile(InputList[i].c_str());
			if(!AsmFile.good())
			{
				sprintf(sMessageBuffer, "Unable to open file %.255s", InputList[i].c_str());
				AsmCallBack(Fatal, sMessageBuffer, LocationVector());
				fRetVal = false;
				continue;
			}
			if(!Assemble<LC3bISA>::Compile(AsmFile, *AsmPrograms[i], AsmCallBack))
				fRetVal = false;
		}
	}	
	if(!fRetVal)
		goto CleanUp;

	//*** Perform Linking ***

	//Convert the symbolic assembly program into a memory image
	AsmCallBack(Info, "Linking...", LocationVector());
	fRetVal = Assemble<LC3bISA>::Link(AsmPrograms, MemoryImage, AsmCallBack);
	if(!fRetVal)
		goto CleanUp;

	if(Flags.fOutputVHDL)
	{
		//Create output file name. If none is specified, the filename will be the same as
		//the first input file except the extention is replaced with ".vhd"
		if(sOutputFileName == "")
			sOutputFileName = AsmPrograms[0]->sFileName.Path + AsmPrograms[0]->sFileName.Bare + ".vhd";
		//Open the output file.
		VHDLFile.open(sOutputFileName.c_str());
		if(!VHDLFile.good())
		{
			sprintf(sMessageBuffer, "Unable to create file %.255s...", sOutputFileName.c_str());
			AsmCallBack(Fatal, sMessageBuffer, LocationVector());
			goto CleanUp;
		}
		//write the memory image into a VHDL Ram vectors file
		sprintf(sMessageBuffer, "Writing VHDL to %.255s...", sOutputFileName.c_str());
		AsmCallBack(Info, sMessageBuffer, LocationVector());
		Assemble<LC3bISA>::VHDLWrite(VHDLFile, MemoryImage, AsmCallBack);
	}
	else
		sOutputFileName = AsmPrograms[0]->sFileName.Path + AsmPrograms[0]->sFileName.Bare;
	//Print VHDL to stdout
	if(Flags.fStdout)
	{
		cout << endl;
		Assemble<LC3bISA>::VHDLWrite(cout, MemoryImage, AsmCallBack);
		cout << endl;
	}

CleanUp:
	AsmCallBack(Info, "", LocationVector());
	sprintf(sMessageBuffer, "%.255s:   %u Error%.1s, %u Warning%.1s", sOutputFileName.c_str(), Errors, (Errors != 1 ? "s" : ""), Warnings, (Warnings != 1 ? "s" : ""));
	AsmCallBack(Info, sMessageBuffer, LocationVector());
	AsmCallBack(Info, "", LocationVector());

	return fRetVal;
}

bool AsmCallBack(MessageEnum MessageType, const string &sMessage, const LocationVector &LocationStack)
{
	unsigned int i, j, Length, PreLength, OrigPreLength, CutOff, Temp, StackLength = 0, StackNumber = LocationStack.size()-1;
	ostrstream sMsg;
	const char *Buffer;
	char DBuffer[MAX_CONSOLE_WIDTH+1];
	char Space[MAX_CONSOLE_WIDTH/2+1];
	memset(Space, ' ', ConsoleWidth/2);
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
			sMsg << InputList[LocationStack[i].first].c_str() << "(" << LocationStack[i].second <<"):    Info:   From included file:\n";
		}
		LastLocation = LocationStack;
		StackLength = sMsg.pcount();
	}

	switch(MessageType)
	{
	case Info:
		if(!LocationStack.empty())
			sMsg << InputList[LocationStack[StackNumber].first].c_str() << "(" << LocationStack[StackNumber].second << "):    Info:   ";
		break;
	case Warning:
		Warnings++;
		if(LocationStack.empty())
			throw "Empty location stack for Warning!";
		sMsg << InputList[LocationStack[StackNumber].first].c_str() << "(" << LocationStack[StackNumber].second << "): Warning:   ";
		break;
	case Error:
		Errors++;
		if(LocationStack.empty())
			throw "Empty location stack for Error!";
		sMsg << InputList[LocationStack[StackNumber].first].c_str() << "(" << LocationStack[StackNumber].second << "):   Error:   ";
		break;
	case Fatal:
		Errors++;
		if(!LocationStack.empty())
			sMsg << InputList[LocationStack[StackNumber].first].c_str() << "(" << LocationStack[StackNumber].second << "):   ";
		sMsg << "Fatal:   ";
		break;
	}

	//Print the pre-message
	if(LocationStack.empty())
		OrigPreLength = PreLength = 0;
	else
		OrigPreLength = PreLength = sMsg.pcount() - StackLength;

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
				OrigPreLength = PreLength = MIN(8, ConsoleWidth/2) + PreLength;
				Space[0] = ' ';
				Space[MIN(8, ConsoleWidth/2)] = 0;
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

}	//namespace LC3b
