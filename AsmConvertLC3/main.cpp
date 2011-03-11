//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#pragma warning (disable:4786)
#include <strstream>
#include <fstream>
#include <cstdio>
#include "AsmConvertLC3.h"
#include "../LC3Assembler/AsmUI.h"

using namespace std;
using namespace LC3;

void PrintUsage();
bool ProcessArgs(int, char**);

int main(int argc, char* argv[])
{
	cout << "\n\t\tLC-3 Assembler Converter 1.0, Ashley Wise\n\n";

	if(!ProcessArgs(argc, argv))
	{
		PrintUsage();
		return 0;
	}

	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	LocationVector LocationStack;
	bool fRetVal = true;

	for(unsigned int i = 0; i < InputList.size(); i++)
	{
		ifstream AsmFile(InputList[i].c_str());
		if(!AsmFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", InputList[i].c_str());
			AsmCallBack(Fatal, sMessageBuffer, LocationVector());
			fRetVal = false;
			continue;
		}

		string sAsm3FileName = InputList[i]+"3";
		ofstream Asm3File(sAsm3FileName.c_str());
		if(!Asm3File.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sAsm3FileName.c_str());
			AsmCallBack(Fatal, sMessageBuffer, LocationVector());
			fRetVal = false;
			continue;
		}

		LocationStack.push_back( LocationVector::value_type(i, 1) );
		cout << "Translating " << InputList[i].c_str() << endl;
		if(!AsmConvertLC3(AsmFile, Asm3File, LocationStack, AsmCallBack))
		{
			fRetVal = false;
			continue;
		}
	}
	cout << endl;

	return 0;
}

void PrintUsage()
{
	cout << "Give assembly file name as parameter\n";
	cout << "The assembly file FileName.asm will be converted into Assembler3 syntax\n";
	cout << "and output to FileName.asm3.\n\n";
}

bool ProcessArgs(int argc, char* argv[])
{
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
			default:
				cout << "Error:   Unrecognized switch: " << argv[i] << endl << endl;
				return false;
			}
			continue;
		}
		//Store the filename
		InputList.push_back(argv[i]);
	}
	if(InputList.empty())
	{
		cout << "Error: No input files specified.\n\n";
		return false;
	}

	return true;
}
