//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#pragma warning (disable:4786)
#include <iostream>
#include <cstdio>
#include <fstream>
#include <new.h>
#include "../LC3Assembler/AsmUI.h"
#include "SimUI.h"

using namespace std;
using namespace LC3;

void PrintUsage();
bool ProcessArgs(int, char**);
int NewHandler(unsigned int);

int main(int argc, char* argv[])
{
	//*NOTE: GCC does not define _set_new_handler, and GCC's implementation of
	//set_new_handler(void(void)) does not work!
	//So MSVC version will print a nice error message while GCC version will crash...
#if defined _MSC_VER
	_set_new_handler(NewHandler);
#endif

	try
	{
		sProgramDir = CreateStandardPath(FileName(argv[0]).Path);
		if(!ProcessArgs(argc, argv))
		{
			PrintUsage();
			return 0;
		}
		Flags.fSimulate = true;
		
		//This will hold the symbolic form of the program.
		vector<Program *> AsmPrograms;
		//This will hold the memory image of the program
		RamVector MemoryImage;

		if(AssemblerUI(AsmPrograms, MemoryImage) && Flags.fSimulate)
			SimulatorUI(AsmPrograms, MemoryImage);

		for(unsigned int i = 0; i < AsmPrograms.size(); i++)
			delete AsmPrograms[i];
	}
	catch(const char *sMsg)
	{	//*NOTE: MSVC allows "char", but GCC requires "const"
		printf("***An unexpected problem has occured.\nPlease e-mail the following error message, along with all input command line\noptions and files to the current maintainer, and the bug will be assessed:\n***Fatal:   %s\n", sMsg);
		fflush(NULL);
		exit(-1);
	}
	catch(runtime_error e)
	{
		printf("***An unexpected problem has occured.\nPlease e-mail the following error message, along with all input command line\noptions and files to the current maintainer, and the bug will be assessed:\n***Fatal:   %s\n", e.what());
		fflush(NULL);
		exit(-1);
	}

	return 0;
}

void PrintUsage()
{
	cout << "\n\t\tLC-3 Assembler " << ASM_VER << ", Ashley Wise\n\n";
	cout << "Give assembly file names as parameters\n";
	cout << "   -3            Disable support for older LC-3 syntax\n";
	cout << "   -a            Prints AST to Filename.AST.asm\n";
	cout << "   -b            Dumps memory image to Filename.obj\n";
	cout << "   -c            Prints VHDL to stdout\n";
	cout << "   -d SymbolName Token\n";
	cout << "                 Define a symbol and its value for all input files.\n";
	cout << "   -h or -?      Prints this help\n";
	cout << "   -o            Turns on optimizations\n";
	cout << "   -s            Prints symbol table Filename.Symbols.csv\n";
	cout << "   -t            Prints tokens to Filename.Tokens.txt\n";
	cout << "   -v Filename   Prints VHDL to (optional) filename\n";
	cout << "   -w Width      Specify width of console text\n";
	cout << "\n\t\tLC-3b, Simulator " << SIM_VER << ", Ashley Wise\n\n";
	cout << "   -sim          Simulate program (on by default).\n";
	cout << "   -os           Use Ash Operating System.\n\n";
}

bool ProcessArgs(int argc, char* argv[])
{
	Flags.fOldLC3 = true;
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
			case '3':
				if(!Flags.fOldLC3)
					cout << "Warning:   -3 specified more than once.";
				Flags.fOldLC3 = false;
				break;
			case 'a':
			case 'A':
				if(Flags.fPrintAST)
					cout << "Warning:   -a specified more than once.";
				Flags.fPrintAST = true;
				break;
			case 'b':
			case 'B':
				if(Flags.fOutputImage)
					cout << "Warning:   -d specified more than once.";
				Flags.fOutputImage = true;
				break;
			case 'c':
			case 'C':
				if(Flags.fStdout)
					cout << "Warning:   -c specified more than once.";
				Flags.fStdout = true;
				break;
			case 'd':
			case 'D':
				if(i + 2 < argc)
				{
					if(argv[i+1][0] == '-' || argv[i+2][0] == '-')
					{
							cout << "Warning:   -d might be followed by another option (-*), but interpreting it as a symbol name and token ." << endl << endl;
					}
					DefineList.push_back( pair<string, string>(argv[i+1], argv[i+2]) );
					i += 2;
					break;
				}
				else
				{
						cout << "Error:   -d must be followed by a symbol name and token." << endl << endl;
						return false;
				}
				break;
			case '?':
			case 'h':
			case 'H':
				PrintUsage();
				break;
			case 'o':
			case 'O':
				if(argv[i][2] == 's' || argv[i][2] == 'S')
				{
					if(Flags.fUseOS)
						cout << "Warning:   -os specified more than once.";
					//first try to open the OS file
					//*NOTE: GCC bug doesn't recognize char[]->string conversion for "=" constructor,
					//even though tecnically Construct(blah) and Construct = blah are identical compiler operations.
					FileName sOSLocation(sProgramDir + "AshOS_LC3.asm");
					while(true)
					{
						ifstream OSFile(sOSLocation.Full.c_str());
						if(!OSFile.good())
						{
							cout << "The LC-3 Ash Operating System was not found at the expected location.\nPlease enter the new path+filename to the OS asm file (q to quit):\n";
							char Buffer[MAX_FILENAME_CHAR];
							cin >> Buffer;
							sOSLocation = CreateStandardPath(Buffer);
							if(sOSLocation.Full == "q" || sOSLocation.Full == "Q")
							{
								sOSLocation = "";
								return false;
							}
							continue;
						}
						break;
					}
					InputList.push_back(sOSLocation.Full);
					Flags.fUseOS = true;
				}
				else
				{
					if(Flags.fUseOptimizations)
						cout << "Warning:   -o specified more than once.";
					Flags.fUseOptimizations = true;
				}
				break;
			case 's':
			case 'S':
				if( (argv[i][2] == 'i' || argv[i][2] == 'I') && (argv[i][3] == 'm' || argv[i][3] == 'M') )
				{
					if(Flags.fSimulate)
						cout << "Warning:   -sim specified more than once.";
					Flags.fSimulate = true;
				}
				else
				{
					if(Flags.fPrintSymbols)
						cout << "Warning:   -s specified more than once.";
					Flags.fPrintSymbols = true;
				}
				break;
			case 't':
			case 'T':
				if(Flags.fPrintTokens)
					cout << "Warning:   -t specified more than once.";
				Flags.fPrintTokens = true;
				break;
			case 'v':
			case 'V':
				if(Flags.fOutputVHDL)
					cout << "Warning:   -v specified more than once.";
				Flags.fOutputVHDL = true;
				if(i + 1 < argc && argv[i+1][0] != '-')
				{
					sOutputFileName = argv[++i];
					break;
				}
				break;
			case 'w':
			case 'W':
				if(Flags.fConsoleWidth)
					cout << "Warning:   -w specified more than once.";
				Flags.fConsoleWidth = true;
				if(i + 1 < argc && argv[i+1][0] != '-')
				{
					i++;
					int Temp;
					Temp = atoi(argv[i]);
					if(Temp < MIN_CONSOLE_WIDTH || Temp > MAX_CONSOLE_WIDTH)
					{
						cout << "Warning:   Specified console width is out of range [" << MIN_CONSOLE_WIDTH << ":" << MAX_CONSOLE_WIDTH << "]." << endl << endl;
					}
					else
						ConsoleWidth = Temp;
				}
				else
				{
						cout << "Warning:   -w missing console width." << endl << endl;
				}
				break;
			default:
				cout << "Error:   Unrecognized switch: " << argv[i] << endl << endl;
				return false;
			}
			continue;
		}
		//Store the filename
		InputList.push_back(CreateStandardPath(argv[i]));
	}
	if(InputList.empty())
	{
		cout << "Error: No input files specified.\n\n";
		return false;
	}

	return true;
}

int NewHandler(unsigned int Size)
{
	throw "Out of memory! Try splitting up the program into smaller sections which can be built separately.";
}
