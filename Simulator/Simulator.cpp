//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#pragma warning (disable:4503)
#include "Simulator.h"
#include <signal.h>
#include <strstream>
#include <cstdio>
#include <cmath>
#include <ctime>
#include "../Assembler/Data.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace Simulator	{

template<class ISA>
uint64 ArchSim<ISA>::fControlC = 0;

template<class ISA>
uint64 ArchSim<ISA>::NextSimID = 1;

//const char *const sEventTypes[ValueEvent+1] = {"NOEVENT", "READEVENT", "WRITEEVENT", "", "CHANGEEVENT", "", "", "", "VALUEEVENT"};

template<class ISA>
ArchSim<ISA>::ArchSim(CallBackFunction, SimCallBackFunction, SimCommandFunction, SimReadConsoleFunction, SimWriteConsoleFunction)
{
	this->CallBack = CallBack;
	this->SimCallBack = SimCallBack;
	this->SimCommand = SimCommand;
	this->SimReadConsole = SimReadConsole;
	this->SimWriteConsole = SimWriteConsole;

	if(!NextSimID)
		throw "Only 64 simulator instances supported!";
	SimulatorID = NextSimID;
	NextSimID <<= 1;

	pArch = NULL;
	pPrograms = NULL;
	pMemoryImage = NULL;
	fDone = false;
	fTrace = false;
	//*NOTE: Do we want runtime checks enabled or disabled by default?
	fCheck = false;

	PreInstructionCount = 5;
	PostInstructionCount = 10;
}

template<class ISA>
bool ArchSim<ISA>::NullCallBack(MessageEnum MessageType, const string &sMessage, const LocationVector &)
{
	return true;
}

template<class ISA>
void ArchSim<ISA>::ControlC(int Sig)
{
	fControlC = -1;
	//*NOTE: reassigning the signal handler appears to be a
	//one-use only event
	signal(SIGINT, ControlC);
}

template<class ISA>
bool ArchSim<ISA>::Run()
{
	if(!pArch || !pPrograms || !pMemoryImage)
		throw "Run executed on uninitialized architecture!";

	fDone = false;
	if(fBreak)
		return true;

	OldSig = signal(SIGINT, ControlC);
	if(!pArch->Run())
	{
		signal(SIGINT, OldSig);
		return false;
	}
	signal(SIGINT, OldSig);
	uint64 Address = pArch->NextInstruction();
	Element *pElement = AddressToElement(Address, false, false);
	CallStack.rbegin()->first = pElement ? InputList[pElement->LocationStack.rbegin()->first] : "NoFile";
	CallStack.rbegin()->second = pElement ? pElement->LocationStack.rbegin()->second : 0;
	CallStack.rbegin()->third = Address;

	SimCycle++;
	if(SimCycle == BreakCycle)
	{
		SimCallBack(Breakpoint, "Cycle limit.");
		fBreak = true;
	}

	if(fControlC & SimulatorID)
	{
		SimCallBack(Breakpoint, "Forced Break.");
		fControlC &= ~SimulatorID;
		fBreak = true;
	}

	if(fBreak)
	{
		fFirstBreak = true;
		//Clear the "go" breakpoints
		fInBreakpoint = false;
		fOutBreakpoint = false;
		fOverBreakpoint = false;
		BreakCycle = SimCycle;
		BreakInstruction = SimInstruction;
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::CommandRun(Architecture &Arch, vector<Program *> &Programs, RamVector &MemoryImage)
{
	fDone = false;
	fTrace = false;

	if(!Reset(Arch, Programs, MemoryImage))
		return false;

	while(!fDone)
	{
		//Either do sim command, or execute a cycle
		if(fBreak)
			//Command errors are user errors, so we don't quit when it returns false
			Command();
		else
			Run();
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::Command()
{
	if(fFirstBreak)
	{
		SimCallBack(Info, "");
		//*NOTE: This assumes the instruction word is the same as the address word
		for(uint64 i = pArch->NextInstruction() - MIN(pArch->NextInstruction(), PreInstructionCount * sizeof(typename ISA::Word)); i <= pArch->NextInstruction() + MIN( ISA::MaxAddress - pArch->NextInstruction(), PostInstructionCount * sizeof(typename ISA::Word)); i += sizeof(typename ISA::Word))
		{
			//First display the next instruction
			ostrstream strNextInstr;
			if(i == pArch->NextInstruction())
				strNextInstr << "-> ";
			else
				strNextInstr << "   ";
			if(!PrintInstruction(strNextInstr, AddressToElement(i, false, true), i))
				return false;
			strNextInstr << ends;
			SimCallBack(Info, strNextInstr.str());
		}
		fFirstBreak = false;
	}
	SimCallBack(Info, "");

	//Then get a simulator command
	string sCommand;
	if(!SimCommand(sCommand))
	{
		fDone = true;
		return false;
	}

	SimCallBack(Info, "");
	return DoCommand(sCommand);
}

template<class ISA>
bool ArchSim<ISA>::DoCommand(const string &scommand)
{
	bool fRetVal = true;
	string sCommand = scommand;

	//Mangle the simulator commands so they don't conflict with assembler or ISA keywords
	//The first word is always a simulator command, so append with sim_ if not already
	if(sCommand.size())
		sCommand = string("sim_") + sCommand;
	//In the case of the sim_help command, the next word must also be a simulator command
	if(sCommand.substr(0, 8) == "sim_help")
	{
		string sTemp = sCommand.substr(8, string::npos);
		string::size_type Loc = sTemp.find_first_not_of(" \t");
		if(Loc != string::npos)
		{
			sTemp = sTemp.substr(0, Loc) + "sim_" + sTemp.substr(Loc, string::npos);
			sCommand = sCommand.substr(0, 8) + sTemp;
		}
	}

	//Lexically scan the input
	list<Token *> TokenList;
	list<Token *>::iterator TokenIter;
	typename ISA::Lexer TheLexer(TokenList, CallBack, false);
	if(!TheLexer.LexLine(LocationVector(), sCommand))
	{
		fRetVal = false;
		goto CleanUp;
	}

	//Parse the input
	TokenIter = TokenList.begin();
	if(!ParseCommand(TokenIter, TokenList.end()))
	{
		fRetVal = false;
		goto CleanUp;
	}

	if(TokenIter != TokenList.end())
	{
		if(TokenIter == TokenList.begin())
			SimCallBack(Error, "Unrecognized sim command.");
		else
		{
			sprintf(sMessageBuffer, "Extra tokens ignored: %.127s", (const char *)**TokenIter);
			SimCallBack(Warning, sMessageBuffer);
		}
		fRetVal = false;
		goto CleanUp;
	}

CleanUp:

	//get rid of the used tokens
	for(TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
 		delete *TokenIter;

	return fRetVal;
}

template<class ISA>
bool ArchSim<ISA>::ParseCommand(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	Program TempProg(NullLocationStack, "", ISA::Addressability);
	typename ISA::Parser TheParser(TempProg, CallBack);
	list<Token *>::iterator TokenIter = StartIter;
	Number *pNumber = NULL, *pNumber2 = NULL;
	uint64 TempInt64;
	unsigned int ProgramNumber, LineNumber, Temp1, Temp2;
	bool fNumberSpecified;
	DataEnum DataType;
	EventInfo *pEvents = NULL;

	if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		//empty command, probably just hit enter
		return true;

	if(((IDToken *)(*StartIter))->sIdentifier == "sim_quit" || ((IDToken *)(*StartIter))->sIdentifier == "sim_exit")
	{
		StartIter++;

		fDone = true;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_console")
	{
		StartIter++;

		//Get the console width
		if( !(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
		{
			SimCallBack(Error, "Console command missing console width.");
			goto CleanUp;
		}
	
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for console width", true))
			goto CleanUp;

		if(!SetConsoleWidth((unsigned int)TempInt64))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_printi")
	{
		StartIter++;

		//Get the pre-instruction print count
		if( !(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
		{
			SimCallBack(Error, "Print instruction command missing pre-instruction print count.");
			goto CleanUp;
		}
	
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for pre-instruction print count", true))
			goto CleanUp;
		Temp1 = (unsigned int)TempInt64;

		//Get the post-instruction print count
		if( !(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
		{
			SimCallBack(Error, "Print instruction command missing post-instruction print count.");
			goto CleanUp;
		}
	
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for post-instruction print count", true))
			goto CleanUp;
		Temp2 = (unsigned int)TempInt64;

		if(!SetPrintI(Temp1, Temp2))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_reset")
	{
		StartIter++;

		if(!Reset(*pArch, *pPrograms, *pMemoryImage))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_traceon")
	{
		StartIter++;
		
		//Get the filename
		if(StartIter == EndIter || (*StartIter)->TokenType != TString)
		{
			SimCallBack(Error, "Trace command missing filename.");
			goto CleanUp;
		}
		string sFileName = ((StringToken *)(*StartIter))->sString;
		StartIter++;

		list<string> TraceRegSets;

		while(StartIter != EndIter && (*StartIter)->TokenType == TIdentifier)
		{
			TraceRegSets.push_back(((IDToken *)(*StartIter))->sIdentifier);
			StartIter++;
		}

		if(!TraceOn(sFileName, TraceRegSets))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_traceoff")
	{
		StartIter++;

		if(!TraceOff())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_checkon")
	{
		StartIter++;

		if(!CheckOn())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_checkoff")
	{
		StartIter++;

		if(!CheckOff())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_saves")
	{
		StartIter++;
		
		//Get the filename
		if(StartIter == EndIter || (*StartIter)->TokenType != TString)
		{
			SimCallBack(Error, "Save state command missing filename.");
			goto CleanUp;
		}
		string sFileName = ((StringToken *)(*StartIter))->sString;
		StartIter++;

		if(!SaveState(sFileName))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_loads")
	{
		StartIter++;
		
		//Get the filename
		if(StartIter == EndIter || (*StartIter)->TokenType != TString)
		{
			SimCallBack(Error, "Load state command missing filename.");
			goto CleanUp;
		}
		string sFileName = ((StringToken *)(*StartIter))->sString;
		StartIter++;

		if(!LoadState(sFileName))
		{
			SimCallBack(Error, "Simulator state undefined, forcing reset...");
			Reset(*pArch, *pPrograms, *pMemoryImage);
			goto CleanUp;
		}
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_saved")
	{
		StartIter++;
		
		if(StartIter == EndIter || (*StartIter)->TokenType != TString)
		{
			SimCallBack(Error, "Save data command missing filename.");
			goto CleanUp;
		}
		string sFileName = ((StringToken *)(*StartIter))->sString;
		StartIter++;

		//Get the data address
		if(!(pNumber = ParseValue(StartIter, EndIter, "Save data command address", ProgramNumber)))
			goto CleanUp;

		//Get the data length
		if(!(pNumber2 = ParseValue(StartIter, EndIter, "Save data command length", ProgramNumber)))
			goto CleanUp;

		//Get the optional data type
		//Check for negative sign
		bool fOpposite = false;
		if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpMinus)
		{
			fOpposite = true;
			StartIter++;
		}
		if(StartIter != EndIter && (*StartIter)->TokenType == (TokenEnum)TData && ((DataToken *)(*StartIter))->DataType != STRUCT)
		{
			DataType = ((DataToken *)(*StartIter))->DataType;
			StartIter++;
		}
		else
		{
			if(fOpposite)
				//This will cause an error to be printed
				StartIter--;

			DataType = STRUCT;
		}

		if(!SaveData(sFileName, pNumber, pNumber2, DataType, fOpposite))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_loadd")
	{
		StartIter++;
		
		if(StartIter == EndIter || (*StartIter)->TokenType != TString)
		{
			SimCallBack(Error, "Load data command missing filename.");
			goto CleanUp;
		}
		string sFileName = ((StringToken *)(*StartIter))->sString;
		StartIter++;

		//Get the data address
		if(!(pNumber = ParseValue(StartIter, EndIter, "Load data command address", ProgramNumber)))
			goto CleanUp;

		//Get the data length
		if(!(pNumber2 = ParseValue(StartIter, EndIter, "Load data command length", ProgramNumber)))
			goto CleanUp;

		//Get the optional data type
		//Check for negative sign
		bool fOpposite = false;
		if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpMinus)
		{
			fOpposite = true;
			StartIter++;
		}
		if(StartIter != EndIter && (*StartIter)->TokenType == (TokenEnum)TData && ((DataToken *)(*StartIter))->DataType != STRUCT)
		{
			DataType = ((DataToken *)(*StartIter))->DataType;
			StartIter++;
		}
		else
		{
			if(fOpposite)
				//This will cause an error to be printed
				StartIter--;

			DataType = STRUCT;
		}

		if(!LoadData(sFileName, pNumber, pNumber2, DataType, fOpposite))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_saveo")
	{
		StartIter++;
		
		if(StartIter == EndIter || (*StartIter)->TokenType != TString)
		{
			SimCallBack(Error, "Save object command missing filename.");
			goto CleanUp;
		}
		string sFileName = ((StringToken *)(*StartIter))->sString;
		StartIter++;

		//Get the data address
		if(!(pNumber = ParseValue(StartIter, EndIter, "Save object command address", ProgramNumber)))
			goto CleanUp;

		//Get the data length
		if(!(pNumber2 = ParseValue(StartIter, EndIter, "Save object command length", ProgramNumber)))
			goto CleanUp;

		if(!SaveObject(sFileName, pNumber, pNumber2))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_loado")
	{
		StartIter++;
		
		if(StartIter == EndIter || (*StartIter)->TokenType != TString)
		{
			SimCallBack(Error, "Load data command missing filename.");
			goto CleanUp;
		}
		string sFileName = ((StringToken *)(*StartIter))->sString;
		StartIter++;

		if(!LoadObject(sFileName))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_go")
	{
		StartIter++;
		
		//See if the optional run length is given
		if(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false))
		{
			if(!Go(pNumber))
				goto CleanUp;
		}
		else
		{
			if(!Go())
				goto CleanUp;
		}
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_goi")
	{
		StartIter++;

		//See if the run length is given
		if( !(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
		{
			SimCallBack(Error, "Go instruction missing instruction run length.");
			goto CleanUp;
		}

		if(!GoI(pNumber))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_goin")
	{
		StartIter++;

		if(!GoIn())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_goover")
	{
		StartIter++;

		if(!GoOver())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_goout")
	{
		StartIter++;

		if(!GoOut())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_gotol")
	{
		StartIter++;

		//Get the optional program number
		if(!ParseProgramNumber(StartIter, EndIter, fNumberSpecified, ProgramNumber))
			goto CleanUp;

		//Get the line number
		if(!(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)))
		{
			SimCallBack(Error, "Goto line command must be followed by a line number.");
			goto CleanUp;
		}
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for goto line number", true))
			goto CleanUp;
		delete pNumber;
		pNumber = NULL;
		LineNumber = (unsigned int)TempInt64;

		if(!GotoL(fNumberSpecified, ProgramNumber, LineNumber))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_gotoi")
	{
		StartIter++;

		//Get the data symbol of the breakpoint
		if(!(pNumber = ParseValue(StartIter, EndIter, "Goto instruction command symbol", ProgramNumber)))
			goto CleanUp;

		//Finalize the breakpoint
		if(!GotoI(pNumber, ProgramNumber))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_bpl")
	{
		StartIter++;

		//Get the optional program number
		if(!ParseProgramNumber(StartIter, EndIter, fNumberSpecified, ProgramNumber))
			goto CleanUp;

		//Get the line number
		if(!(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)))
		{
			SimCallBack(Error, "Breakpoint line command must be followed by a line number.");
			goto CleanUp;
		}
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for breakpoint line number", true))
			goto CleanUp;
		delete pNumber;
		pNumber = NULL;
		LineNumber = (unsigned int)TempInt64;

		//Get the event list
		EventEnum Events;
		if(StartIter != EndIter && (*StartIter)->TokenType == TIdentifier && ((IDToken *)(*StartIter))->sIdentifier == "noevent")
		{
			Events = NoEvent;
			StartIter++;
		}
		else
			Events = InstrEvent;

		//Finalize the breakpoint
		if(!BreakpointLine(fNumberSpecified, ProgramNumber, LineNumber, Events))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_bpi")
	{
		StartIter++;

		//Get the data symbol of the breakpoint
		if(!(pNumber = ParseValue(StartIter, EndIter, "Breakpoint instruction command symbol", ProgramNumber)))
			goto CleanUp;

		//Get the event list
		EventEnum Events;
		if(StartIter != EndIter && (*StartIter)->TokenType == TIdentifier && ((IDToken *)(*StartIter))->sIdentifier == "noevent")
		{
			Events = NoEvent;
			StartIter++;
		}
		else
			Events = InstrEvent;

		//Finalize the breakpoint
		if(!BreakpointInstruction(pNumber, false, ProgramNumber, 0, Events))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_bpic")
	{
		StartIter++;

		if(!BreakpointInstructionClear())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_bpd")
	{
		StartIter++;

		//Get the data symbol of the breakpoint
		if(!(pNumber = ParseValue(StartIter, EndIter, "Breakpoint data command symbol", ProgramNumber)))
			goto CleanUp;

		//Get the optional data type
		if(StartIter != EndIter && (*StartIter)->TokenType == (TokenEnum)TData && ((DataToken *)(*StartIter))->DataType != STRUCT)
		{
			DataType = ((DataToken *)(*StartIter))->DataType;
			StartIter++;
		}
		else
			DataType = STRUCT;

		//Get the event list
		if(!(pEvents = ParseEventList(StartIter, EndIter)))
			goto CleanUp;

		//Finalize the breakpoint
		if(!BreakpointData(pNumber, DataType, pEvents))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_bpdc")
	{
		StartIter++;

		if(!BreakpointDataClear())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_bpm")
	{
		StartIter++;

		//Get the name of the memory
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			SimCallBack(Error, "Breakpoint memory command must be followed by memory name, an address (number or symbol), and an event list.");
			goto CleanUp;
		}
		string sMemory = ((IDToken *)(*StartIter))->sIdentifier;
		StartIter++;

		//Check the memory name
		//This should only be in BreakpointMemory, but then ParseValue prints an error message
		//and the user never sees this message.
		if(pArch->Memories.find(sMemory) == pArch->Memories.end())
		{
			sprintf(sMessageBuffer, "\"%.63s\" is not a valid memory name.", sMemory.c_str());
			SimCallBack(Error, sMessageBuffer);
			goto CleanUp;
		}

		//Get the address of the breakpoint
		if(!(pNumber = ParseValue(StartIter, EndIter, "Breakpoint memory command address", ProgramNumber)))
			goto CleanUp;

		//Get the optional data type
		if(StartIter != EndIter && (*StartIter)->TokenType == (TokenEnum)TData && ((DataToken *)(*StartIter))->DataType != STRUCT)
		{
			DataType = ((DataToken *)(*StartIter))->DataType;
			StartIter++;
		}
		else
			DataType = STRUCT;

		//Get the event list
		if(!(pEvents = ParseEventList(StartIter, EndIter)))
			goto CleanUp;

		//Finalize the breakpoint
		if(!BreakpointMemory(sMemory, pNumber, DataType, pEvents))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_bpmc")
	{
		StartIter++;

		if(!BreakpointMemoryClear())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_bpr")
	{
		StartIter++;

		//Get the name of the register set
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			SimCallBack(Error, "Breakpoint register command must be followed by register set name, period, register name, and an event list.");
			goto CleanUp;
		}
		string sRegisterSet = ((IDToken *)(*StartIter))->sIdentifier;
		StartIter++;

		//Get the period
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator && ((OpToken *)(*StartIter))->Operator != OpPeriod)
		{
			SimCallBack(Error, "Breakpoint register command must be followed by register set name, period, register name, and an event list.");
			goto CleanUp;
		}
		StartIter++;

		//Get the name of the register
		if(StartIter == EndIter || !((*StartIter)->TokenType == TIdentifier || (*StartIter)->TokenType == (TokenEnum)TISA && ((typename ISA::ISAToken *)(*StartIter))->ISAType == ISA::TRegister))
		{
			SimCallBack(Error, "Breakpoint register command must be followed by register set name, period, register name, and an event list.");
			goto CleanUp;
		}
		string sRegister;
		if((*StartIter)->TokenType == (TokenEnum)TISA)
			sRegister = ToLower(ISA::sRegisters[((typename ISA::RegToken *)(*StartIter))->Register]);
		else
			sRegister = ((IDToken *)(*StartIter))->sIdentifier;
		StartIter++;

		//Get the event list
		if(!(pEvents = ParseEventList(StartIter, EndIter)))
			goto CleanUp;

		//Finalize the breakpoint
		if(!BreakpointRegister(sRegisterSet, sRegister, pEvents))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_bprc")
	{
		StartIter++;

		if(!BreakpointRegisterClear())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_help")
	{
		StartIter++;

		if(StartIter != EndIter && (*StartIter)->TokenType == TIdentifier)
		{
			DisplayHelp(((IDToken *)(*StartIter))->sIdentifier);
			StartIter++;
		}
		else
		{
			DisplayHelp("");
		}
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dil")
	{
		StartIter++;

		//Get the optional program number
		if(!ParseProgramNumber(StartIter, EndIter, fNumberSpecified, ProgramNumber))
			goto CleanUp;

		//Get the line number
		if(!(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)))
		{
			SimCallBack(Error, "Display instruction line command must be followed by a line number.");
			goto CleanUp;
		}
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for display instruction line number", true))
			goto CleanUp;
		delete pNumber;
		pNumber = NULL;
		LineNumber = (unsigned int)TempInt64;

		//Get optional data length
		if( !(pNumber2 = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
			pNumber2 = new IntegerNumber(LocationVector(), 1);

		//Finalize the display
		if(!DisplayInstructionLine(fNumberSpecified, ProgramNumber, LineNumber, pNumber2))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_di")
	{
		StartIter++;

		//Get the instruction symbol
		if(!(pNumber = ParseValue(StartIter, EndIter, "Display instruction command symbol", ProgramNumber)))
			goto CleanUp;

		//Get optional data length
		if( !(pNumber2 = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
			pNumber2 = new IntegerNumber(LocationVector(), 1);

		//Finalize the display
		if(!DisplayInstruction(pNumber, pNumber2))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dl")
	{
		StartIter++;

		//Get the optional program number
		if(!ParseProgramNumber(StartIter, EndIter, fNumberSpecified, ProgramNumber))
			goto CleanUp;

		//Get the line number
		if(!(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)))
		{
			SimCallBack(Error, "Display line command must be followed by a line number.");
			goto CleanUp;
		}
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for display line number", true))
			goto CleanUp;
		delete pNumber;
		pNumber = NULL;
		LineNumber = (unsigned int)TempInt64;

		//Get the optional data type
		//Check for negative sign
		bool fSigned = false;
		if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpMinus)
		{
			fSigned = true;
			StartIter++;
		}
		if(StartIter != EndIter && (*StartIter)->TokenType == (TokenEnum)TData && ((DataToken *)(*StartIter))->DataType != STRUCT)
		{
			DataType = ((DataToken *)(*StartIter))->DataType;
			StartIter++;
			if(fSigned && DataType >= REAL1)
				SimCallBack(Warning, "Signed specifier ignored on real datatypes.");
		}
		else
		{
			if(fSigned)
			{
				SimCallBack(Error, "Signed specifier missing datatype.");
				goto CleanUp;
			}

			DataType = STRUCT;
		}

		//Get optional data length
		if( !(pNumber2 = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
			pNumber2 = new IntegerNumber(LocationVector(), 1);

		//Finalize the display
		if(!DisplayLine(fNumberSpecified, ProgramNumber, LineNumber, pNumber2, DataType, fSigned))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dd")
	{
		StartIter++;

		//Get the data symbol
		if(!(pNumber = ParseValue(StartIter, EndIter, "Display data command symbol", ProgramNumber)))
			goto CleanUp;

		//Get the optional data type
		//Check for negative sign
		bool fSigned = false;
		if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpMinus)
		{
			fSigned = true;
			StartIter++;
		}
		if(StartIter != EndIter && (*StartIter)->TokenType == (TokenEnum)TData && ((DataToken *)(*StartIter))->DataType != STRUCT)
		{
			DataType = ((DataToken *)(*StartIter))->DataType;
			StartIter++;
			if(fSigned && DataType >= REAL1)
				SimCallBack(Warning, "Signed specifier ignored on real datatypes.");
		}
		else
		{
			if(fSigned)
			{
				SimCallBack(Error, "Signed specifier missing datatype.");
				goto CleanUp;
			}

			DataType = STRUCT;
		}

		//Get optional data length
		if( !(pNumber2 = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
			pNumber2 = new IntegerNumber(LocationVector(), 1);

		//Finalize the display
		if(!DisplayData(pNumber, pNumber2, DataType, fSigned))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dla")
	{
		StartIter++;

		//Get the optional program number
		if(!ParseProgramNumber(StartIter, EndIter, fNumberSpecified, ProgramNumber))
			goto CleanUp;

		//Get the line number
		if(!(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)))
		{
			SimCallBack(Error, "Display line array command must be followed by a line number.");
			goto CleanUp;
		}
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for display line array number", true))
			goto CleanUp;
		delete pNumber;
		pNumber = NULL;
		LineNumber = (unsigned int)TempInt64;

		//Get optional data length
		if( !(pNumber2 = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
			pNumber2 = new IntegerNumber(LocationVector(), DEFAULT_MEMORY_DISPLAY_LENGTH);

		//Finalize the display
		if(!DisplayLineArray(fNumberSpecified, ProgramNumber, LineNumber, pNumber2))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dda")
	{
		StartIter++;

		//Get the data symbol
		if(!(pNumber = ParseValue(StartIter, EndIter, "Display data array command symbol", ProgramNumber)))
			goto CleanUp;

		//Get optional data length
		if( !(pNumber2 = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
			pNumber2 = new IntegerNumber(LocationVector(), DEFAULT_MEMORY_DISPLAY_LENGTH);

		//Finalize the display
		if(!DisplayDataArray(pNumber, pNumber2))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dm")
	{
		StartIter++;

		//Get the name of the memory
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			if(!DisplayMemory())
				goto CleanUp;
		}
		else
		{
			string sMemory = ((IDToken *)(*StartIter))->sIdentifier;
			StartIter++;

			//Check the memory name
			//This should only be in DisplayMemory, but then ParseValue prints an error message
			//and the user never sees this message.
			if(pArch->Memories.find(sMemory) == pArch->Memories.end())
			{
				sprintf(sMessageBuffer, "\"%.63s\" is not a valid memory name.", sMemory.c_str());
				SimCallBack(Error, sMessageBuffer);
				goto CleanUp;
			}

			//Get the memory address
			if(!(pNumber = ParseValue(StartIter, EndIter, "Display memory command address", ProgramNumber)))
				goto CleanUp;

			//Get the optional data type
			//Check for negative sign
			bool fSigned = false;
			if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpMinus)
			{
				fSigned = true;
				StartIter++;
			}
			if(StartIter != EndIter && (*StartIter)->TokenType == (TokenEnum)TData && ((DataToken *)(*StartIter))->DataType != STRUCT)
			{
				DataType = ((DataToken *)(*StartIter))->DataType;
				StartIter++;
				if(fSigned && DataType >= REAL1)
					SimCallBack(Warning, "Signed specifier ignored on real datatypes.");
			}
			else
			{
				if(fSigned)
				{
					SimCallBack(Error, "Signed specifier missing datatype.");
					goto CleanUp;
				}

				DataType = STRUCT;
			}

			//Get optional memory length
			if( !(pNumber2 = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
				pNumber2 = new IntegerNumber(LocationVector(), 1);

			//Finalize the display
			if(!DisplayMemory(sMemory, pNumber, pNumber2, DataType, fSigned))
				goto CleanUp;
		}
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dma")
	{
		StartIter++;

		string sMemory = ((IDToken *)(*StartIter))->sIdentifier;
		StartIter++;

		//Check the memory name
		//This should only be in DisplayMemory, but then ParseValue prints an error message
		//and the user never sees this message.
		if(pArch->Memories.find(sMemory) == pArch->Memories.end())
		{
			sprintf(sMessageBuffer, "\"%.63s\" is not a valid memory name.", sMemory.c_str());
			SimCallBack(Error, sMessageBuffer);
			goto CleanUp;
		}

		//Get the memory address
		if(!(pNumber = ParseValue(StartIter, EndIter, "Display memory command address", ProgramNumber)))
			goto CleanUp;

		//Get optional memory length
		if( !(pNumber2 = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
			pNumber2 = new IntegerNumber(LocationVector(), DEFAULT_MEMORY_DISPLAY_LENGTH);

		//Finalize the display
		if(!DisplayMemoryArray(sMemory, pNumber, pNumber2))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_drs")
	{
		StartIter++;

		//Get the name of the register set
		//Get the name of the memory
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			if(!DisplayRegisterSet())
				goto CleanUp;
		}
		else
		{
			string sRegisterSet = ((IDToken *)(*StartIter))->sIdentifier;
			StartIter++;

			//Finalize the display
			if(!DisplayRegisterSet(sRegisterSet))
				goto CleanUp;
		}
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dr")
	{
		StartIter++;

		//Get the name of the register set
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			SimCallBack(Error, "Display register command must be followed by register set name, period, and register name.");
			goto CleanUp;
		}
		string sRegisterSet = ((IDToken *)(*StartIter))->sIdentifier;
		StartIter++;

		//Get the period
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator && ((OpToken *)(*StartIter))->Operator != OpPeriod)
		{
			SimCallBack(Error, "Display register command must be followed by register set name, period, and register name.");
			goto CleanUp;
		}
		StartIter++;

		//Get the name of the register
		if(StartIter == EndIter || !((*StartIter)->TokenType == TIdentifier || (*StartIter)->TokenType == (TokenEnum)TISA && ((typename ISA::ISAToken *)(*StartIter))->ISAType == ISA::TRegister))
		{
			SimCallBack(Error, "Display register command must be followed by register set name, period, and register name.");
			goto CleanUp;
		}
		string sRegister;
		if((*StartIter)->TokenType == (TokenEnum)TISA)
			sRegister = ToLower(ISA::sRegisters[((typename ISA::RegToken *)(*StartIter))->Register]);
		else
			sRegister = ((IDToken *)(*StartIter))->sIdentifier;
		StartIter++;

		//Finalize the display
		if(!DisplayRegister(sRegisterSet, sRegister))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dpl")
	{
		StartIter++;

		if(!DisplayPipelines())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dci")
	{
		StartIter++;

		if(!DisplayCycleInstruction())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dcs")
	{
		StartIter++;

		if(!DisplayCallStack())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dp")
	{
		StartIter++;

		if(!DisplayPrograms())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_dbp")
	{
		StartIter++;

		if(!DisplayBreakpoints())
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_wl")
	{
		StartIter++;

		//Get the optional program number
		if(!ParseProgramNumber(StartIter, EndIter, fNumberSpecified, ProgramNumber))
			goto CleanUp;

		//Get the line number
		if(!(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)))
		{
			SimCallBack(Error, "Write line command must be followed by a line number.");
			goto CleanUp;
		}
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for write line number", true))
			goto CleanUp;
		delete pNumber;
		pNumber = NULL;
		LineNumber = (unsigned int)TempInt64;

		if(!(pNumber = LineToAddress(fNumberSpecified, ProgramNumber, LineNumber, "Write line command address")))
			goto CleanUp;

		RamVector MemoryImage;
		//Get the data elements
		if(!ParseElements(StartIter, EndIter, "Write line command address", ProgramNumber, MemoryImage, pNumber))
			goto CleanUp;
		if(MemoryImage.empty())
		{
			SimCallBack(Error, "Write line command must be followed by elements.");
			goto CleanUp;
		}

		//Finalize the write
		if(!WriteData(MemoryImage))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_wd")
	{
		StartIter++;

		RamVector MemoryImage;
		//Get the data elements
		if(!ParseElements(StartIter, EndIter, "Write data command address", ProgramNumber, MemoryImage))
			goto CleanUp;
		if(MemoryImage.empty())
		{
			SimCallBack(Error, "Write data command must be followed by an address (number or symbol) and elements.");
			goto CleanUp;
		}

		//Finalize the write
		if(!WriteData(MemoryImage))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_wm")
	{
		StartIter++;

		//Get the name of the memory
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			SimCallBack(Error, "Write memory command must be followed by memory name, an address (number or symbol), and elements.");
			goto CleanUp;
		}
		string sMemory = ((IDToken *)(*StartIter))->sIdentifier;
		StartIter++;

		//Check the memory name
		//This should be in WriteMemory, but then ParseValue prints an error message
		//and the user never sees this message.
		if(pArch->Memories.find(sMemory) == pArch->Memories.end())
		{
			sprintf(sMessageBuffer, "\"%.63s\" is not a valid memory name.", sMemory.c_str());
			SimCallBack(Error, sMessageBuffer);
			goto CleanUp;
		}

		RamVector MemoryImage;
		//Get the memory elements
		if(!ParseElements(StartIter, EndIter, "Write memory command address", ProgramNumber, MemoryImage))
			goto CleanUp;
		if(MemoryImage.empty())
		{
			SimCallBack(Error, "Write memory command must be followed by memory name, an address (number or symbol), and elements.");
			goto CleanUp;
		}

		//Finalize the write
		if(!WriteMemory(sMemory, MemoryImage))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_wr")
	{
		StartIter++;

		//Get the name of the register set
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			SimCallBack(Error, "Write register command must be followed by register set name, period, register name, and a value (number or symbol).");
			goto CleanUp;
		}
		string sRegisterSet = ((IDToken *)(*StartIter))->sIdentifier;
		StartIter++;

		//Get the period
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator && ((OpToken *)(*StartIter))->Operator != OpPeriod)
		{
			SimCallBack(Error, "Write register command must be followed by register set name, period, register name, and a value (number or symbol).");
			goto CleanUp;
		}
		StartIter++;

		//Get the name of the register
		if(StartIter == EndIter || !((*StartIter)->TokenType == TIdentifier || (*StartIter)->TokenType == (TokenEnum)TISA && ((typename ISA::ISAToken *)(*StartIter))->ISAType == ISA::TRegister))
		{
			SimCallBack(Error, "Write register command must be followed by register set name, register name, and a value (number or symbol).");
			goto CleanUp;
		}
		string sRegister;
		if((*StartIter)->TokenType == (TokenEnum)TISA)
			sRegister = ToLower(ISA::sRegisters[((typename ISA::RegToken *)(*StartIter))->Register]);
		else
			sRegister = ((IDToken *)(*StartIter))->sIdentifier;
		StartIter++;

		//Get the register value
		if(!(pNumber = ParseValue(StartIter, EndIter, "Write register command value", ProgramNumber)))
			goto CleanUp;

		//Finalize the write
		if(!WriteRegister(sRegisterSet, sRegister, pNumber))
			goto CleanUp;
	}
	else if(((IDToken *)(*StartIter))->sIdentifier == "sim_int")
	{
		StartIter++;

		//Get the interrupt vector
		if(!(pNumber = ParseValue(StartIter, EndIter, "Interrupt command vector", ProgramNumber)))
			goto CleanUp;

		//Finalize the interrupt
		if(!Interrupt(pNumber))
			goto CleanUp;
	}


	return true;

CleanUp:
	if(pNumber)
		delete pNumber;
	if(pNumber2)
		delete pNumber2;
	if(pEvents)
		delete pEvents;
	return false;
}

template<class ISA>
typename ArchSim<ISA>::EventInfo *ArchSim<ISA>::ParseEventList(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	EventEnum Event = NoEvent;
	list<Number *> ValueList;
	bool fNoEvent = false;

	//Get the name of the event
	if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
	{
		SimCallBack(Error, "Breakpoint command missing event list.");
		return NULL;
	}

	//Get event types
	while(StartIter != EndIter && (*StartIter)->TokenType == TIdentifier)
	{
		string sEvent = ((IDToken *)(*StartIter))->sIdentifier;

		if(((IDToken *)(*StartIter))->sIdentifier == "noevent")
		{
			StartIter++;

			if(fNoEvent)
				SimCallBack(Warning, "NoEvent used more than once in event list.");
			if(Event)
			{
				SimCallBack(Warning, "NoEvent overrides previous events.");
				for(list<Number *>::iterator NumIter = ValueList.begin(); NumIter != ValueList.end(); NumIter++)
					delete *NumIter;
			}
			Event = NoEvent;
			fNoEvent = true;
		}
		else if(((IDToken *)(*StartIter))->sIdentifier == "readevent")
		{
			StartIter++;

			if(Event & ReadEvent)
				SimCallBack(Warning, "ReadEvent used more than once in event list.");
			if(fNoEvent)
				SimCallBack(Warning, "NoEvent overrides other events.");
			else
				Event = (EventEnum)(Event | ReadEvent);
		}
		else if(((IDToken *)(*StartIter))->sIdentifier == "writeevent")
		{
			StartIter++;

			if(Event & ReadEvent)
				SimCallBack(Warning, "WriteEvent used more than once in event list.");
			if(fNoEvent)
				SimCallBack(Warning, "NoEvent overrides other events.");
			else
				Event = (EventEnum)(Event | WriteEvent);
		}
		else if(((IDToken *)(*StartIter))->sIdentifier == "changeevent")
		{
			StartIter++;

			if(Event & ReadEvent)
				SimCallBack(Warning, "ChangeEvent used more than once in event list.");
			if(fNoEvent)
				SimCallBack(Warning, "NoEvent overrides other events.");
			else
				Event = (EventEnum)(Event | ChangeEvent);
		}
		else if(((IDToken *)(*StartIter))->sIdentifier == "valueevent")
		{
			StartIter++;

			//Get value for value event
			Number *pNumber;
			unsigned int ProgramNumber;
			if(!(pNumber = ParseValue(StartIter, EndIter, "Breakpoint value event", ProgramNumber)))
				goto CleanUp;
			if(pNumber->NumberType == NumSymbol)
			{
				//Check the resolution of the symbol and attributes
				if(!reinterpret_cast<SymbolNumber *>(pNumber)->ResolveValue(NULL, CallBack, " for breakpoint value event", true))
				{
					delete pNumber;
					goto CleanUp;
				}
			}

			if(fNoEvent)
			{
				SimCallBack(Warning, "NoEvent overrides other events.");
				delete pNumber;
			}
			else
			{
				Event = (EventEnum)(Event | ValueEvent);
				ValueList.push_back(pNumber);
			}
		}
		else
			break;
	}

	return new EventInfo(Event, ValueList);

CleanUp:
	for(list<Number *>::iterator NumIter = ValueList.begin(); NumIter != ValueList.end(); NumIter++)
		delete *NumIter;
	return NULL;
}

template<class ISA>
bool ArchSim<ISA>::ParseProgramNumber(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter, bool &fNumberSpecified, unsigned int &ProgramNumber)
{
	if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpOpenBrace)
	{
		fNumberSpecified = true;
		StartIter++;
		
		Program TempProg(NullLocationStack, "", ISA::Addressability);
		typename ISA::Parser TheParser(TempProg, CallBack);
		list<Token *>::iterator TokenIter = StartIter;
		Number *pNumber;
		uint64 TempInt64;

		//Get the program number
		if(!(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, false, false)))
		{
			SimCallBack(Error, "Open brace must be followed by program number.");
			return false;
		}
		if(!pNumber->Int(8*sizeof(unsigned int), false, TempInt64, CallBack, " for program number", true))
		{
			delete pNumber;
			return false;
		}
		delete pNumber;
		ProgramNumber = (unsigned int)TempInt64;

		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseBrace)
		{
			SimCallBack(Error, "Program number missing close brace.");
			return false;
		}
		StartIter++;
	}
	else
		fNumberSpecified = false;

	return true;
}

template<class ISA>
Number *ArchSim<ISA>::ParseValue(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter, const string &sExtra, unsigned int &ProgramNumber)
{
//	Program TempProg(NullLocationStack, "", ISA::Addressability);
//	typename ISA::Parser TheParser(TempProg, CallBack);
	list<Token *>::iterator TokenIter = StartIter;
	bool fNumberSpecified = false;
	Number *pNumber = NULL;

	//Get the optional program number
	if(!ParseProgramNumber(StartIter, EndIter, fNumberSpecified, ProgramNumber))
		return NULL;

	//Decide which program to use
	if(fNumberSpecified)
	{
		if(ProgramNumber >= pPrograms->size())
		{
			sprintf(sMessageBuffer, "%.63s program number (%u) is invalid.", sExtra.c_str(), ProgramNumber);
			SimCallBack(Error, sMessageBuffer);
			return NULL;
		}
//		TempProg.TheSymbolTable = (*pPrograms)[ProgramNumber]->TheSymbolTable;
	}
	else
	{
		ProgramNumber = 0;
		if(StartIter != EndIter && (*StartIter)->TokenType == TIdentifier)
		{
			//pick the first program which has a hit for the specified symbol
			for(vector<Program *>::iterator ProgIter = pPrograms->begin(); ProgIter != pPrograms->end(); ProgIter++)
			{
				SymbolTable::SymbolMap::iterator SymbolIter = (*ProgIter)->TheSymbolTable.ResolvedSymbols.find(((IDToken *)(*StartIter))->sIdentifier);
				if(SymbolIter != (*ProgIter)->TheSymbolTable.ResolvedSymbols.end() && SymbolIter->second->SymbolType == SymLabel)
				{
					ProgramNumber = (*ProgIter)->LocationStack[0].first;
					if(pPrograms->size() > 1)
					{
						sprintf(sMessageBuffer, "Chose program \"%.255s\" to resolve symbolic value.", (*pPrograms)[ProgramNumber]->sFileName.Full.c_str());
						SimCallBack(Info, sMessageBuffer);
					}
//					TempProg.TheSymbolTable = (*ProgIter)->TheSymbolTable;
					break;
				}
			}
			//If no program chosen, Check() will give error
		}
		//else, it's irrelavent
	}

	typename ISA::Parser TheParser(*(*pPrograms)[ProgramNumber], CallBack);

	//Get the value
	if(!(pNumber = TheParser.ParseNumber(TokenIter, StartIter, EndIter, true, false)))
	{
		sprintf(sMessageBuffer, "%.63s missing value (number or symbol).", sExtra.c_str());
		SimCallBack(Error, sMessageBuffer);
		goto CleanUp;
	}
	//If the value is a symbol, make sure it is resolved.
	if(pNumber->NumberType == NumSymbol)
	{
//		if(!TempProg.TheSymbolTable.Check(CallBack))
		if(!(*pPrograms)[ProgramNumber]->TheSymbolTable.Check(CallBack))
			goto CleanUp;
	}

//	TempProg.TheSymbolTable.ResolvedSymbols.clear();
//	TempProg.TheSymbolTable.ExternSymbols.clear();
	return pNumber;

CleanUp:
//	TempProg.TheSymbolTable.ResolvedSymbols.clear();
//	TempProg.TheSymbolTable.ExternSymbols.clear();
	(*pPrograms)[ProgramNumber]->TheSymbolTable.UnresolvedSymbols.clear();
	if(pNumber)
		delete pNumber;
	return NULL;
}

template<class ISA>
bool ArchSim<ISA>::ParseElements(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter, const string &sExtra, unsigned int &ProgramNumber, RamVector &MemoryImage, Number *pNumber)
{
	bool fRetVal = true;
	Program TempProg(NullLocationStack, "", ISA::Addressability);
	typename ISA::Parser TheParser(TempProg, CallBack);
	uint64 TempInt64;

	MemoryImage.clear();

	//Get the address to place the elements
	if(!pNumber && !(pNumber = ParseValue(StartIter, EndIter, sExtra, ProgramNumber)))
		return false;
	if(!pNumber->Int(64 - ISA::Addressability, false, TempInt64, CallBack, " for element address", true, ISA::Addressability, false, true, 0))
		goto CleanUp;
	//Copy the symbol table to resolve symbols
	TempProg.TheSymbolTable = (*pPrograms)[ProgramNumber]->TheSymbolTable;
	//Create the default segment
	TempProg.Segments.push_back(new Segment(LocationVector(), ISA::Addressability));

	//Get the elements
	while(StartIter != EndIter)
	{
		if((*StartIter)->TokenType == (TokenEnum)TISA)
		{
			if(!TheParser.ParseInstruction(StartIter, EndIter))
				goto CleanUp;
		}
		else if((*StartIter)->TokenType == (TokenEnum)TData)
		{
			if(!TheParser.ParseData(StartIter, EndIter))
				goto CleanUp;
		}
		else
			break;
	}

	//See if there are any unresolved symbols
	if(!TempProg.TheSymbolTable.Check(CallBack))
		goto CleanUp;

	//Resolve the address of the new element
	TempInt64 <<= ISA::Addressability;
	if(!TempProg.ResolveAddresses(TempInt64, CallBack))
		goto CleanUp;

	//build the memory image of the new element
	if(!TempProg.GenerateImage(MemoryImage, ISA::fLittleEndian, CallBack))
		goto CleanUp;

	TempProg.TheSymbolTable.ResolvedSymbols.clear();
	TempProg.TheSymbolTable.ExternSymbols.clear();
	delete pNumber;
	return true;

CleanUp:
	TempProg.TheSymbolTable.ResolvedSymbols.clear();
	TempProg.TheSymbolTable.ExternSymbols.clear();
	return false;
}

template<class ISA>
Number *ArchSim<ISA>::LineToAddress(bool fNumberSpecified, unsigned int &ProgramNumber, unsigned int &LineNumber, const string &sExtra)
{
	if(fNumberSpecified)
	{
		if(ProgramNumber >= pPrograms->size())
		{
			sprintf(sMessageBuffer, "%.63s program number (%u) is invalid.", sExtra.c_str(), ProgramNumber);
			SimCallBack(Error, sMessageBuffer);
			return NULL;
		}
	}
	else
	{
		ProgramNumber = 0;
		if(pPrograms->size() > 1)
		{
			sprintf(sMessageBuffer, "Chose program \"%.255s\" for line number.", (*pPrograms)[ProgramNumber]->sFileName.Full.c_str());
			SimCallBack(Info, sMessageBuffer);
		}
	}

	Program &TheProg = *(*pPrograms)[ProgramNumber];
	Number *pNumber;

	//Go through the segments to find the segment which contains the requested line number
	list<Segment *>::iterator SegmentIter = TheProg.Segments.begin();
	if(SegmentIter != TheProg.Segments.end())
	{
		list<Segment *>::iterator SegmentIter2 = TheProg.Segments.begin();
		SegmentIter2++;
		for(; SegmentIter2 != TheProg.Segments.end(); SegmentIter++, SegmentIter2++)
		{
			if((*SegmentIter2)->LocationStack[0].second > LineNumber)
				break;
		}
	}

	//The element with the requested line number could either be in this segment, or the first element in the next segment
	Element *pElement;
	for(; SegmentIter != TheProg.Segments.end(); SegmentIter++)
	{
		for(list<Element *>::iterator SequenceIter = (*SegmentIter)->Sequence.begin(); SequenceIter != (*SegmentIter)->Sequence.end(); SequenceIter++)
		{
			//Only allow an element located in this file
			if((*SequenceIter)->LocationStack[0].second >= LineNumber && (*SequenceIter)->LocationStack.size() == 1)
			{
				pElement = *SequenceIter;
				if(pElement->LocationStack[0].second > LineNumber)
				{
					sprintf(sMessageBuffer, "Line %u does not contain an element, moved to line %u.", LineNumber, pElement->LocationStack[0].second);
					SimCallBack(Warning, sMessageBuffer);
					LineNumber = pElement->LocationStack[0].second;
				}
				//If the element is a label, find out what it points to
				if(pElement->ElementType == LabelElement)
				{
					if(((Label*)pElement)->pElement)
						//The line number referring to the label now refers to the next instruction/data
						pElement = ((Label*)pElement)->pElement;
				}
				if(pElement->ElementType != InstructionElement)
				{
					sprintf(sMessageBuffer, "Line %u element is not an instruction.", pElement->LocationStack[0].second);
					SimCallBack(Warning, sMessageBuffer);
				}
				pNumber = new IntegerNumber(LocationVector(), pElement->Address);
				return pNumber;
			}
		}
	}

	sprintf(sMessageBuffer, "No element on or after line %u.", LineNumber);
	SimCallBack(Error, sMessageBuffer);
	return NULL;
}

template<class ISA>
Element *ArchSim<ISA>::AddressToElement(uint64 Address, bool fInside, bool fLabel)
{
	//*NOTE: This algorithm does perform a hierarchical search, similar to a B-tree.
	//However, since each segment could contain 200+ elements, this search has a high constant.
	//A more efficient method may be looked into in the future.

	//Go through the programs to find the program which contains the requested address
	vector<Program *>::iterator ProgramIter;
	for(ProgramIter = pPrograms->begin(); ProgramIter != pPrograms->end(); ProgramIter++)
	{
		if((*ProgramIter)->Address <= Address && (*ProgramIter)->Address + (*ProgramIter)->Size > Address)
			break;
	}
	if(ProgramIter == pPrograms->end())
		return NULL;

	//Go through the segments to find the segment which contains the requested address
	list<Segment *>::iterator SegmentIter;
	for(SegmentIter = (*ProgramIter)->Segments.begin(); SegmentIter != (*ProgramIter)->Segments.end(); SegmentIter++)
	{
		if((*SegmentIter)->Address <= Address && (*SegmentIter)->Address + (*SegmentIter)->Size > Address)
			break;
	}
	if(SegmentIter == (*ProgramIter)->Segments.end())
		return NULL;

	//Go through the elements to find the element which contains the requested address
	//This search may take us into a structure instance, which is implemented as a segment.
	//If so, we will update the ElementIter and EndIter to refer to the new "structure" segment.
	list<Element *>::iterator ElementIter, EndIter = (*SegmentIter)->Sequence.end();
	for(ElementIter = (*SegmentIter)->Sequence.begin(); ElementIter != EndIter;)
	{
		Element *pElement = *ElementIter;
		if(pElement->ElementType == LabelElement)
		{
			string sLabel = ((Label *)pElement)->sLabel;
		}
		if((*ElementIter)->Address == Address || fInside && (*ElementIter)->Address < Address && (*ElementIter)->Address + (*ElementIter)->Size > Address)
		{
			//Ok, now we have the element that contains this address. Depending on what the element is
			//could effect the next move
			if((*ElementIter)->ElementType == StructElement)
			{
				//Go through the segments (array of stucture instances) to find the segment
				//(structure instance) which contains the requested address
				vector<Segment *>::iterator SegmentIter;
				for(SegmentIter = (reinterpret_cast<Struct *>(*ElementIter))->Segments.begin(); SegmentIter != (reinterpret_cast<Struct *>(*ElementIter))->Segments.end(); SegmentIter++)
				{
					if((*SegmentIter)->Address <= Address && (*SegmentIter)->Address + (*SegmentIter)->Size > Address)
						break;
				}
				if(SegmentIter == (reinterpret_cast<Struct *>(*ElementIter))->Segments.end())
					return NULL;

				ElementIter = ElementIter = (*SegmentIter)->Sequence.begin();
				EndIter = (*SegmentIter)->Sequence.end();
				continue;
			}
			else if((*ElementIter)->ElementType == LabelElement && (!fLabel || !reinterpret_cast<Label *>(*ElementIter)->pElement || reinterpret_cast<Label *>(*ElementIter)->pElement->ElementType == StructElement))
			{
				//Don't allow a label that points to a segment or structure

				//*NOTE: The case where a label points to a segment, and the segment specified an offset so there is space
				//between the label and the segment will cause the label to have a different address than the segment.
				//In this case, an address pointing to the segment start will not find the label.
				//I'm not sure if there's any case where this matters.
				ElementIter++;
				continue;
			}
			else	//Data or instruction element, or labels allowed
				break;
		}
		ElementIter++;
	}
	if(ElementIter == (*SegmentIter)->Sequence.end())
		return NULL;

	//*NOTE: Should we do something about if the address is not the start of the element?
	//At least for data arrays, it's possible it could be an index into an array, which could be checked.

	return *ElementIter;
}

template<class ISA>
bool ArchSim<ISA>::PrintInstruction(ostream &strOutput, Element *pElement, uint64 Address, bool fPrintFile, bool fPrintLabelElement, bool fPrintValue)
{
	if(pElement)
	{
		if(pElement->ElementType == LabelElement)
		{
			if(reinterpret_cast<Label *>(pElement)->pSegment)
				Address = reinterpret_cast<Label *>(pElement)->pSegment->Address;
			else if(reinterpret_cast<Label *>(pElement)->pElement)
				Address = reinterpret_cast<Label *>(pElement)->pElement->Address;
		}
		char *sMB = sMessageBuffer;
		#if defined _MSC_VER
			sMB += sprintf(sMB, "[4x%.*I64X]:", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability);
		#elif defined GPLUSPLUS
			sMB += sprintf(sMB, "[4x%.*llX]:", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		if(fPrintFile)
			strOutput << InputList[pElement->LocationStack.rbegin()->first].c_str() << "(" << pElement->LocationStack.rbegin()->second << "): ";
		strOutput << sMessageBuffer;
		if(fPrintValue)
		{
			RamVector vData;
			pElement->GetImage(vData, ISA::fLittleEndian, CallBack);
			if(vData.size())
			{
				strOutput << " (4x";
				if(ISA::fLittleEndian)
					for(RamVector::reverse_iterator RamIter = vData.rbegin(); RamIter != vData.rend(); RamIter++)
					{
						sprintf(sMessageBuffer, "%.2X", RamIter->second);
						strOutput << sMessageBuffer;
					}
				else
					for(RamVector::iterator RamIter = vData.begin(); RamIter != vData.end(); RamIter++)
					{
						sprintf(sMessageBuffer, "%.2X", RamIter->second);
						strOutput << sMessageBuffer;
					}
				strOutput << "):";
			}
		}
		if(pElement->ElementType == LabelElement)
		{
			strOutput << " " << reinterpret_cast<Label *>(pElement)->sLabel.c_str() << ":";
			if(fPrintLabelElement)
				strOutput << "\t" << (const char *)*reinterpret_cast<Label *>(pElement)->pElement;
		}
		else
			strOutput << "\t" << (const char *)*pElement;
	}
	else
	{
		char *sMB = sMessageBuffer;
		#if defined _MSC_VER
			sMB += sprintf(sMB, "[4x%.*I64X]:", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability);
		#elif defined GPLUSPLUS
			sMB += sprintf(sMB, "[4x%.*llX]:", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		if(fPrintFile)
			strOutput << "NoFile(0): ";
		strOutput << sMessageBuffer;

		//Get the instruction binary
		RamVector vData;
		//*NOTE: This is assuming the instruction word is the same as the address word.
		if(!pArch->DataRead(vData, Address, sizeof(typename ISA::Word)))
			return false;
		union wByteData_t
		{
			typename ISA::Word WordData;
			char Bytes[sizeof(typename ISA::Word)];
		} wByteData;
		if(fPrintValue && vData.size())
		{
			strOutput << " (4x";
			if(ISA::fLittleEndian)
				for(RamVector::reverse_iterator RamIter = vData.rbegin(); RamIter != vData.rend(); RamIter++)
				{
					sprintf(sMessageBuffer, "%.2X", RamIter->second);
					strOutput << sMessageBuffer;
				}
			else
				for(RamVector::iterator RamIter = vData.begin(); RamIter != vData.end(); RamIter++)
				{
					sprintf(sMessageBuffer, "%.2X", RamIter->second);
					strOutput << sMessageBuffer;
				}
			strOutput << "):";
		}

		for(int i = 0; i < sizeof(typename ISA::Word); i++)
		{
			if(ISA::fLittleEndian)
#ifdef BIG_ENDIAN_BUILD
				wByteData.Bytes[sizeof(uint64) - i - 1] = vData[i].second;
#else
				wByteData.Bytes[i] = vData[i].second;
#endif
			else
#ifdef BIG_ENDIAN_BUILD
				wByteData.Bytes[i] = vData[i].second;
#else
				wByteData.Bytes[sizeof(uint64) - i - 1] = vData[i].second;
#endif
		}
		//*NOTE: Bug in GCC won't recognize "ISA::Disassembler::DisassemblerInstruction" function call.
		typename ISA::Disassembler *pDisassembler = NULL;
		pElement = pDisassembler->DisassembleInstruction(NULL, LocationVector(), Address, wByteData.WordData);

		strOutput << "\t" << (const char *)*pElement;
		delete pElement;
	}
	return true;
}

template<class ISA>
bool ArchSim<ISA>::SetConsoleWidth(unsigned int Width)
{
	if(Width < MIN_CONSOLE_WIDTH || Width > MAX_CONSOLE_WIDTH)
	{
		sprintf(sMessageBuffer, "Specified console width is out of range [%u:%u].", MIN_CONSOLE_WIDTH, MAX_CONSOLE_WIDTH);
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	ConsoleWidth = Width;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::SetPrintI(unsigned int precount, unsigned int postcount)
{
	if(precount > MAX_DISPLAY_LENGTH || postcount > MAX_DISPLAY_LENGTH)
	{
		sprintf(sMessageBuffer, "Specified pre- and/or post-instruction count is out of range [0:%u].", MAX_DISPLAY_LENGTH);
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	PreInstructionCount = precount;
	PostInstructionCount = postcount;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::Reset(Architecture &Arch, vector<Program *> &Programs, RamVector &MemoryImage)
{
	SimCallBack(Info, "Initializing architecture...");
	SimCallBack(Info, "");

	pArch = &Arch;
	pPrograms = &Programs;
	pMemoryImage = &MemoryImage;
	
	if(!pArch->Reset(MemoryImage))
		return false;

	fBreak = true;
	SimCycle = 0;
	BreakCycle = 0;
	SimInstruction = 0;
	BreakInstruction = 0;
	fInBreakpoint = false;
	fOutBreakpoint = false;
	fOverBreakpoint = false;
	fFirstBreak = true;
	CallStack.clear();
	uint64 Address = pArch->NextInstruction();
	Element *pElement, *pLElement = AddressToElement(Address, false, true);
	if(pLElement && pLElement->ElementType == LabelElement)
		pElement = reinterpret_cast<Label *>(pLElement)->pElement;
	else
		pElement = pLElement;
	CallStack.push_back( CallStackInfo(
		pElement ? InputList[pElement->LocationStack.rbegin()->first] : "NoFile",
		pElement ? pElement->LocationStack.rbegin()->second : 0,
		Address,
		(pLElement && pLElement->ElementType == LabelElement ? reinterpret_cast<Label *>(pLElement)->sLabel : "")) );

	return true;
}

template<class ISA>
bool ArchSim<ISA>::TraceOn(string sFileName, list<string> &TraceRegSets)
{
	if(fTrace)
	{
		TraceOff();
//		SimCallBack(Error, "Trace is already on. Stop the current trace before starting another.");
//		return false;
	}

	for(list<string>::iterator TraceRegIter = TraceRegSets.begin(); TraceRegIter!= TraceRegSets.end(); TraceRegIter++)
	{
		//verify this is a valid register set name
		if(pArch->RegisterSets.find(*TraceRegIter) == pArch->RegisterSets.end())
		{
			sprintf(sMessageBuffer, "\"%.63s\" is not a valid register set name.", TraceRegIter->c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}
	TraceRegisterSets = TraceRegSets;

	TraceFile.clear();
	TraceFile.open(sFileName.c_str());

	if(!TraceFile.good())
	{
		TraceFile.close();
		TraceRegisterSets.clear();
		sprintf(sMessageBuffer, "Error opening file %.255s.", sFileName.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	fTrace = true;

	//Start tracing the current instruction, which will otherwise be missed
	Element *pElement = AddressToElement(pArch->NextInstruction(), false, true);
	#if defined _MSC_VER
		sprintf(sMessageBuffer, "%I64u;\t%I64u;\t", SimCycle, SimInstruction);
	#elif defined GPLUSPLUS
		sprintf(sMessageBuffer, "%llu;\t%llu;\t", SimCycle, SimInstruction);
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif
	TraceFile << sMessageBuffer;
	if(!PrintInstruction(TraceFile, pElement, pArch->NextInstruction()))
		return false;
	for(list<string>::iterator RegSetIter = TraceRegisterSets.begin(); RegSetIter != TraceRegisterSets.end(); RegSetIter++)
		TraceFile << ";\t" << (const char *)pArch->RegisterSets.find(*RegSetIter)->second;
	TraceFile << endl;

	return true;
}

template<class ISA>
bool ArchSim<ISA>::TraceOff()
{
	if(!fTrace)
	{
//		SimCallBack(Error, "Trace is already off.");
//		return false;
	}

	TraceFile.close();
	TraceRegisterSets.clear();
	fTrace = false;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::CheckOn()
{
	if(fCheck)
	{
//		SimCallBack(Error, "Check is already on.");
//		return false;
	}
	fCheck = true;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::CheckOff()
{
	if(!fCheck)
	{
//		SimCallBack(Error, "Check is already off.");
//		return false;
	}
	fCheck = false;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::SaveState(string sFileName)
{
	//save the memories
	for(Architecture::MemoryMap::iterator MemIter = pArch->Memories.begin(); MemIter != pArch->Memories.end(); MemIter++)
	{
		string sTempFileName = sFileName + ".Mem." + MemIter->second.sName + ".bin";
		ofstream OutputFile(sTempFileName.c_str(), ios::out | ios::binary);
		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sTempFileName.c_str());
			SimCallBack(Fatal, sMessageBuffer);
			return false;
		}
		OutputFile << MemIter->second;
		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Error writing to %.255s", sTempFileName.c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}

	//save the registers
	for(Architecture::RegisterSetMap::iterator RegSetIter = pArch->RegisterSets.begin(); RegSetIter != pArch->RegisterSets.end(); RegSetIter++)
	{
		string sTempFileName = sFileName + ".RegSet." + RegSetIter->second.sName + ".bin";
		ofstream OutputFile(sTempFileName.c_str(), ios::out | ios::binary);
		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sTempFileName.c_str());
			SimCallBack(Fatal, sMessageBuffer);
			return false;
		}
		OutputFile << RegSetIter->second;
		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Error writing to %.255s", sTempFileName.c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}

	//Save the architecture
	{
		string sTempFileName = sFileName + ".Arch.bin";
		ofstream OutputFile(sTempFileName.c_str(), ios::out | ios::binary);
		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sTempFileName.c_str());
			SimCallBack(Fatal, sMessageBuffer);
			return false;
		}
		*pArch >> OutputFile;
		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Error writing to %.255s", sTempFileName.c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}

	//Save the simulator
	{
		string sTempFileName = sFileName + ".Sim.bin";
		ofstream OutputFile(sTempFileName.c_str(), ios::out | ios::binary);
		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sTempFileName.c_str());
			SimCallBack(Fatal, sMessageBuffer);
			return false;
		}

		unsigned int i;
		JMT::ByteData ByteData;

		//Save the cycle count
		ByteData.UI64 = SimCycle;
		for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
			OutputFile.put(ByteData.Bytes[sizeof(uint64) - i - 1]);
#else
			OutputFile.put(ByteData.Bytes[i]);
#endif

		//Save the instruction count
		ByteData.UI64 = SimInstruction;
		for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
			OutputFile.put(ByteData.Bytes[sizeof(uint64) - i - 1]);
#else
			OutputFile.put(ByteData.Bytes[i]);
#endif

		//Save the call stack
		//First save the length
		ByteData.UI64 = CallStack.size();
		for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
			OutputFile.put(ByteData.Bytes[sizeof(uint64) - i - 1]);
#else
			OutputFile.put(ByteData.Bytes[i]);
#endif
		//Then write each entry of the call stack
		for(CallStackList::iterator CSIter = CallStack.begin(); CSIter != CallStack.end(); CSIter++)
		{
			CallStackInfo &SCInfo = *CSIter;

			//First is the filename
			ByteData.UI64 = SCInfo.first.size();
			for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
				OutputFile.put(ByteData.Bytes[sizeof(uint64) - i - 1]);
#else
				OutputFile.put(ByteData.Bytes[i]);
#endif
			for(i = 0; i < SCInfo.first.size(); i++)
				OutputFile.put(SCInfo.first[i]);

			//Second is the line number
			ByteData.UI64 = SCInfo.second;
			for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
				OutputFile.put(ByteData.Bytes[sizeof(uint64) - i - 1]);
#else
				OutputFile.put(ByteData.Bytes[i]);
#endif

			//third is the address
			ByteData.UI64 = SCInfo.third;
			for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
				OutputFile.put(ByteData.Bytes[sizeof(uint64) - i - 1]);
#else
				OutputFile.put(ByteData.Bytes[i]);
#endif

			//Fourth is the label
			ByteData.UI64 = SCInfo.fourth.size();
			for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
				OutputFile.put(ByteData.Bytes[sizeof(uint64) - i - 1]);
#else
				OutputFile.put(ByteData.Bytes[i]);
#endif
			for(i = 0; i < SCInfo.fourth.size(); i++)
				OutputFile.put(SCInfo.fourth[i]);
		}

		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Error writing to %.255s", sTempFileName.c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}

	//Output a file using the bare filename so that the user will have a file to select when re-loading.
	{
		string sTempFileName = sFileName + ".Sim.bin";
		ofstream OutputFile(sTempFileName.c_str());
		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sTempFileName.c_str());
			SimCallBack(Fatal, sMessageBuffer);
			return false;
		}

		//Dump the time to the file for something to do.
		time_t Time = time(NULL);
		OutputFile << ctime(&Time) << endl;

		if(!OutputFile.good())
		{
			sprintf(sMessageBuffer, "Error writing to %.255s", sTempFileName.c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}
	
	return true;
}

template<class ISA>
bool ArchSim<ISA>::LoadState(string sFileName)
{
	//Load the memories
	for(Architecture::MemoryMap::iterator MemIter = pArch->Memories.begin(); MemIter != pArch->Memories.end(); MemIter++)
	{
		string sTempFileName = sFileName + ".Mem." + MemIter->second.sName + ".bin";
		ifstream InputFile(sTempFileName.c_str(), ios::in | ios::binary);
		if(!InputFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sTempFileName.c_str());
			SimCallBack(Fatal, sMessageBuffer);
			return false;
		}
		InputFile >> MemIter->second;
		if(!InputFile.good() && !InputFile.eof())
		{
			sprintf(sMessageBuffer, "Error reading from %.255s", sTempFileName.c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}

	//Load the registers
	for(Architecture::RegisterSetMap::iterator RegSetIter = pArch->RegisterSets.begin(); RegSetIter != pArch->RegisterSets.end(); RegSetIter++)
	{
		string sTempFileName = sFileName + ".RegSet." + RegSetIter->second.sName + ".bin";
		ifstream InputFile(sTempFileName.c_str(), ios::in | ios::binary);
		if(!InputFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sTempFileName.c_str());
			SimCallBack(Fatal, sMessageBuffer);
			return false;
		}
		InputFile >> RegSetIter->second;
		if(!InputFile.good())
		{
			sprintf(sMessageBuffer, "Error reading from %.255s", sTempFileName.c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}

	//Load the architecture
	{
		string sTempFileName = sFileName + ".Arch.bin";
		ifstream InputFile(sTempFileName.c_str(), ios::in | ios::binary);
		if(!InputFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sTempFileName.c_str());
			SimCallBack(Fatal, sMessageBuffer);
			return false;
		}
		*pArch << InputFile;
		if(!InputFile.good())
		{
			sprintf(sMessageBuffer, "Error reading from %.255s", sTempFileName.c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}

	//Load the simulator
	{
		string sTempFileName = sFileName + ".Sim.bin";
		ifstream InputFile(sTempFileName.c_str(), ios::in | ios::binary);
		if(!InputFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", sTempFileName.c_str());
			SimCallBack(Fatal, sMessageBuffer);
			return false;
		}

		unsigned int i;
		JMT::ByteData ByteData;

		//Load the cycle count
		for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = InputFile.get();
#else
			ByteData.Bytes[i] = InputFile.get();
#endif
		SimCycle = ByteData.UI64;

		//Load the instruction count
		for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = InputFile.get();
#else
			ByteData.Bytes[i] = InputFile.get();
#endif
		SimInstruction = ByteData.UI64;

		//Load the call stack
		CallStack.clear();
		//First load the length
		for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = InputFile.get();
#else
			ByteData.Bytes[i] = InputFile.get();
#endif
		unsigned int CSSize = (unsigned int)ByteData.UI64;

		//Then read each entry of the call stack
		for(; CSSize > 0; CSSize--)
		{
			CallStackInfo SCInfo;

			//First is the filename
			for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
				ByteData.Bytes[sizeof(uint64) - i - 1] = InputFile.get();
#else
				ByteData.Bytes[i] = InputFile.get();
#endif
			SCInfo.first.resize((unsigned int)ByteData.UI64);
			for(i = 0; i < SCInfo.first.size(); i++)
				SCInfo.first[i] = InputFile.get();

			//Second is the line number
			for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
				ByteData.Bytes[sizeof(uint64) - i - 1] = InputFile.get();
#else
				ByteData.Bytes[i] = InputFile.get();
#endif
			SCInfo.second = (unsigned int)ByteData.UI64;

			//third is the address
			for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
				ByteData.Bytes[sizeof(uint64) - i - 1] = InputFile.get();
#else
				ByteData.Bytes[i] = InputFile.get();
#endif
			SCInfo.third = ByteData.UI64;

			//Fourth is the label
			for(i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
				ByteData.Bytes[sizeof(uint64) - i - 1] = InputFile.get();
#else
				ByteData.Bytes[i] = InputFile.get();
#endif
			SCInfo.fourth.resize((unsigned int)ByteData.UI64);
			for(i = 0; i < SCInfo.fourth.size(); i++)
				SCInfo.fourth[i] = InputFile.get();
			CallStack.push_back(SCInfo);
		}

		if(!InputFile.good())
		{
			sprintf(sMessageBuffer, "Error reading from %.255s", sTempFileName.c_str());
			SimCallBack(Error, sMessageBuffer);
			return false;
		}
	}

	fBreak = true;
	fFirstBreak = true;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::SaveData(string sFileName, Number *pAddress, Number *pLength, DataEnum DataType, bool fOpposite)
{
	if(!pAddress || !pLength)
		throw "NULL parameter to SaveData!";

	//Check the address
	uint64 Address;
	if(!pAddress->Int(MIN(8*sizeof(typename ISA::Word), 64 - ISA::Addressability), false, Address, CallBack, " for save data address", true, ISA::Addressability, false, true, 0))
		return false;
	Address <<= ISA::Addressability;

	//check the datatype and length
	uint64 Length;
	if(!pLength->Int(8*sizeof(typename ISA::Word), false, Length, CallBack, " for save data length", true))
		return false;

	unsigned int DataBytes = vDataBytes[DataType];
	if(DataType == STRUCT)
		DataBytes = sizeof(typename ISA::Word);

	if(Address + Length * DataBytes > ISA::MaxAddress || Address + Length * DataBytes < Address)
	{
		SimCallBack(Error, "Specified range includes addresses beyond the addressible data range.");
		return false;
	}

	//Save the data
	ofstream OutputFile(sFileName.c_str(), ios::out | ios::binary);
	if(!OutputFile.good())
	{
		sprintf(sMessageBuffer, "Unable to open file %.255s", sFileName.c_str());
		SimCallBack(Fatal, sMessageBuffer);
		return false;
	}
	for(uint64 i = 0; i < Length; i++)
	{
		RamVector vRam;
		pArch->DataRead(vRam, Address, DataBytes);
		for(unsigned int j = 0; j < DataBytes; j++)
		{
			if(fOpposite)
				OutputFile.put(vRam[DataBytes - j - 1].second);
			else
				OutputFile.put(vRam[j].second);
		}
		Address += DataBytes;
	}
	if(!OutputFile.good())
	{
		sprintf(sMessageBuffer, "Error writing to %.255s", sFileName.c_str());
		SimCallBack(Error, sMessageBuffer);
	}

	delete pAddress;
	delete pLength;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::LoadData(string sFileName, Number *pAddress, Number *pLength, DataEnum DataType, bool fOpposite)
{
	if(!pAddress || !pLength)
		throw "NULL parameter to LoadData!";

	//Check the address
	uint64 Address;
	if(!pAddress->Int(MIN(8*sizeof(typename ISA::Word), 64 - ISA::Addressability), false, Address, CallBack, " for load data address", true, ISA::Addressability, false, true, 0))
		return false;
	Address <<= ISA::Addressability;

	//check the datatype and length
	uint64 Length;
	if(!pLength->Int(8*sizeof(typename ISA::Word), false, Length, CallBack, " for load data length", true))
		return false;

	unsigned int DataBytes = vDataBytes[DataType];
	if(DataType == STRUCT)
		DataBytes = sizeof(typename ISA::Word);

	if(Address + Length * DataBytes > ISA::MaxAddress || Address + Length * DataBytes < Address)
	{
		SimCallBack(Error, "Specified range includes addresses beyond the addressible data range.");
		return false;
	}

	//Read the data
	ifstream InputFile(sFileName.c_str(), ios::in | ios::binary);
	if(!InputFile.good())
	{
		sprintf(sMessageBuffer, "Unable to open file %.255s", sFileName.c_str());
		SimCallBack(Fatal, sMessageBuffer);
		return false;
	}
	for(uint64 i = 0; i < Length; i++)
	{
		RamVector vRam;
		vRam.resize(DataBytes);
		for(unsigned int j = 0; j < DataBytes; j++)
		{
			if(fOpposite)
				vRam[DataBytes - j - 1] = RamVector::value_type(Address + DataBytes - j - 1, InputFile.get());
			else
				vRam[j] = RamVector::value_type(Address + j, InputFile.get());
		}
		if(InputFile.eof())
		{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "EOF encountered after %I64u data elements (%I64u bytes) read from %.255s", i+1, (i+1)*DataBytes, sFileName.c_str());
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "EOF encountered after %llu data elements (%llu bytes) read from %.255s", i+1, (i+1)*DataBytes, sFileName.c_str());
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
			SimCallBack(Warning, sMessageBuffer);
			break;
		}
		pArch->DataWrite(vRam);
		Address += DataBytes;
	}
	if(InputFile.bad() || InputFile.fail() && !InputFile.eof())
	{
		sprintf(sMessageBuffer, "Error reading from %.255s", sFileName.c_str());
		SimCallBack(Error, sMessageBuffer);
	}

	delete pAddress;
	delete pLength;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::SaveObject(string sFileName, Number *pAddress, Number *pLength)
{
	if(!pAddress || !pLength)
		throw "NULL parameter to SaveObject!";

	//Check the address
	uint64 Address;
	if(!pAddress->Int(MIN(8*sizeof(typename ISA::Word), 64 - ISA::Addressability), false, Address, CallBack, " for save object address", true, ISA::Addressability, false, true, 0))
		return false;
	Address <<= ISA::Addressability;

	//check the datatype and length
	uint64 Length;
	if(!pLength->Int(8*sizeof(typename ISA::Word), false, Length, CallBack, " for save object length", true))
		return false;

	if(Address + Length > ISA::MaxAddress || Address + Length < Address)
	{
		SimCallBack(Error, "Specified range includes addresses beyond the addressible data range.");
		return false;
	}

	//Save the data
	ofstream OutputFile(sFileName.c_str(), ios::out | ios::binary);
	if(!OutputFile.good())
	{
		sprintf(sMessageBuffer, "Unable to open file %.255s", sFileName.c_str());
		SimCallBack(Fatal, sMessageBuffer);
		return false;
	}
	//OBJ file header is always little-endian
	uint64 i;
	for(i = 0; i < sizeof(uint64); i++)
		OutputFile.put( (unsigned char)(Address >> 8 * i) );
	for(i = 0; i < sizeof(uint64); i++)
		OutputFile.put( (unsigned char)(Length  >> 8 * i) );
	for(i = 0; i < Length; i++)
	{
		RamVector vRam;
		pArch->DataRead(vRam, Address, 1);
		OutputFile.put(vRam[0].second);
		Address++;
	}
	if(!OutputFile.good())
	{
		sprintf(sMessageBuffer, "Error writing to %.255s", sFileName.c_str());
		SimCallBack(Error, sMessageBuffer);
	}

	delete pAddress;
	delete pLength;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::LoadObject(string sFileName)
{
	unsigned int TempChar;
	JMT::ByteData ByteData;
	uint64 i, Address, Length;

	//Open the file
	ifstream InputFile(sFileName.c_str(), ios::in | ios::binary);
	if(!InputFile.good())
	{
		sprintf(sMessageBuffer, "Unable to open file %.255s", sFileName.c_str());
		SimCallBack(Fatal, sMessageBuffer);
		return false;
	}

	//First read starting address.
	for(i = 0; i < sizeof(uint64); i++)
	{
		TempChar = InputFile.get();
		if(TempChar == -1)	//EOF
			break;
		//The address in the file is always little endian
#ifdef BIG_ENDIAN_BUILD
		ByteData.Bytes[sizeof(uint64) - i - 1] = TempChar;
#else
		ByteData.Bytes[i] = TempChar;
#endif
	}
	if(i < sizeof(uint64))
	{
		SimCallBack(Fatal, "Unexpected end of file.");
		return false;
	}
	Address = ByteData.UI64;

	//Then read ending address
	for(i = 0; i < sizeof(uint64); i++)
	{
		TempChar = InputFile.get();
		if(TempChar == -1)	//EOF
			break;
		//The address in the file is always little endian
#ifdef BIG_ENDIAN_BUILD
		ByteData.Bytes[sizeof(uint64) - i - 1] = TempChar;
#else
		ByteData.Bytes[i] = TempChar;
#endif
	}
	if(i < sizeof(uint64))
	{
		SimCallBack(Fatal, "Unexpected end of file.");
		return false;
	}
	Length = ByteData.UI64;

	if(Address + Length > ISA::MaxAddress || Address + Length < Address)
	{
		SimCallBack(Error, "Specified range includes addresses beyond the addressible data range.");
		return false;
	}

	for(i = 0; i < Length; i++)
	{
		RamVector vRam;
		vRam.resize(1);
				vRam[0] = RamVector::value_type(Address, InputFile.get());
		if(InputFile.eof())
		{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "EOF encountered after %I64u bytes read from %.255s", i+1, sFileName.c_str());
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "EOF encountered after %llu bytes read from %.255s", i+1, sFileName.c_str());
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
			SimCallBack(Warning, sMessageBuffer);
			break;
		}
		pArch->DataWrite(vRam);
		Address++;
	}
	if(InputFile.bad() || InputFile.fail() && !InputFile.eof())
	{
		sprintf(sMessageBuffer, "Error reading from %.255s", sFileName.c_str());
		SimCallBack(Error, sMessageBuffer);
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::Go()
{
	fBreak = false;

	return true;
}

template<class ISA>
bool ArchSim<ISA>::Go(Number *pNumber)
{
	if(!pNumber)
		throw "NULL parameter to Go!";

	uint64 TempInt64;
	if(!pNumber->Int(64, false, TempInt64, CallBack, "", true))
		return false;
	BreakCycle = SimCycle + TempInt64;
	if(TempInt64 == 0)
		fFirstBreak = true;
	else
		fBreak = false;

	delete pNumber;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::GoI(Number *pNumber)
{
	if(!pNumber)
		throw "NULL parameter to GoI!";

	uint64 TempInt64;
	if(!pNumber->Int(64, false, TempInt64, CallBack, "", true))
		return false;
	BreakInstruction = SimInstruction + TempInt64;
	if(TempInt64 == 0)
		fFirstBreak = true;
	else
		fBreak = false;

	delete pNumber;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::GoIn()
{
	fInBreakpoint = true;
	DepthCount = -1;
	fBreak = false;

	return true;
}

template<class ISA>
bool ArchSim<ISA>::GoOver()
{
	fOverBreakpoint = true;
	DepthCount = 0;
	fBreak = false;

	return true;
}

template<class ISA>
bool ArchSim<ISA>::GoOut()
{
	fOutBreakpoint = true;
	DepthCount = 1;
	fBreak = false;

	return true;
}

template<class ISA>
bool ArchSim<ISA>::GotoL(bool fNumberSpecified, unsigned int ProgramNumber, unsigned int LineNumber)
{
	//Convert the program and line numbers into an address
	Number *pNumber;
	if( !(pNumber = LineToAddress(fNumberSpecified, ProgramNumber, LineNumber, "Goto line command")) )
		return false;

	//See if there is a label pointing to this address
	Element *pElement = AddressToElement(((IntegerNumber *)(pNumber))->Value, false, true);
	if(pElement && pElement->ElementType == LabelElement)
	{
		//If so, use a symbolnumber rather than an integer number
		delete pNumber;
		pNumber = new SymbolNumber(LocationVector(), (*pPrograms)[ProgramNumber]->TheSymbolTable.ReferenceSymbol(LocationVector(), ((Label *)pElement)->sLabel, CallBack), (*pPrograms)[ProgramNumber]->TheSymbolTable.ResolvedSymbols);
	}
	else
	{
		((IntegerNumber *)(pNumber))->Value >>= ISA::Addressability;
	}

	//Finalize the breakpoint
	if(!BreakpointInstruction(pNumber, true, ProgramNumber, LineNumber, GotoEvent))
	{
		delete pNumber;
		return false;
	}

	fBreak = false;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::GotoI(Number *pNumber, unsigned int ProgramNumber)
{
	if(!pNumber)
		throw "NULL parameter to GotoI!";

	//Finalize the breakpoint
	if(!BreakpointInstruction(pNumber, false, ProgramNumber, 0, GotoEvent))
		return false;
	
	fBreak = false;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::BreakpointLine(bool fNumberSpecified, unsigned int ProgramNumber, unsigned int LineNumber, EventEnum Events)
{
	//Convert the program and line numbers into an address
	Number *pNumber;
	if( !(pNumber = LineToAddress(fNumberSpecified, ProgramNumber, LineNumber, "Breakpoint line command")) )
		return false;

	//See if there is a label pointing to this address
	Element *pElement = AddressToElement(((IntegerNumber *)(pNumber))->Value, false, true);
	if(pElement && pElement->ElementType == LabelElement)
	{
		//If so, use a symbolnumber rather than an integer number
		delete pNumber;
		pNumber = new SymbolNumber(LocationVector(), (*pPrograms)[ProgramNumber]->TheSymbolTable.ReferenceSymbol(LocationVector(), ((Label *)pElement)->sLabel, CallBack), (*pPrograms)[ProgramNumber]->TheSymbolTable.ResolvedSymbols);
	}
	else
	{
		((IntegerNumber *)(pNumber))->Value >>= ISA::Addressability;
	}

	//Finalize the breakpoint
	if(!BreakpointInstruction(pNumber, true, ProgramNumber, LineNumber, Events))
	{
		delete pNumber;
		return false;
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::BreakpointInstruction(Number *pNumber, bool fLineInfo, unsigned int ProgramNumber, unsigned int LineNumber, EventEnum Events)
{
	if(!pNumber)
		throw "NULL parameter to BreakpointInstruction!";

	//Check the event types
	if(Events & ~(InstrEvent | GotoEvent))
		throw "Only NoEvent can be specified for instruction breakpoints!";

	//Get the address
	uint64 Address;
	if(!pNumber->Int(MIN(8*sizeof(typename ISA::Word), 64 - ISA::Addressability), false, Address, CallBack, " for instruction breakpoint address", true, ISA::Addressability, false, true, 0))
		return false;
	Address <<= ISA::Addressability;

	//Try to get the program and line number information
	if(!fLineInfo)
	{
		Element *pElement = NULL;
		if(pNumber->NumberType == NumSymbol)
			//First try to get it based off the symbols used
			reinterpret_cast<SymbolNumber *>(pNumber)->ResolveValue(&pElement, NullCallBack, " for data breakpoint address");
		if(!pElement)
			//Second try to get it based on the address
			pElement = AddressToElement(Address);
		if(pElement)
		{
			fLineInfo = true;
			ProgramNumber = pElement->LocationStack.rbegin()->first;
			LineNumber = pElement->LocationStack.rbegin()->second;
		}
	}
	
	//See if there are breakpoints for this address
	pair<STDTYPENAME InstrMap::iterator, bool> InstrInsert = InstrBreakpoints.insert(STDTYPENAME InstrMap::value_type(Address, InstrInfo(pNumber, LineInfo(fLineInfo, ProgramNumber, LineNumber), Events)));
	if(InstrInsert.second == false)
	{
		//Update the number
		if(InstrInsert.first->second.first->NumberType == NumSymbol && pNumber->NumberType != NumSymbol)
			//Keep the old number if it's a symbol and this one isn't.
			delete pNumber;
		else
		{
			delete InstrInsert.first->second.first;
			InstrInsert.first->second.first = pNumber;
		}

		//Update the line info
		if(!InstrInsert.first->second.second.first || fLineInfo)
		{
			InstrInsert.first->second.second.first = fLineInfo;
			InstrInsert.first->second.second.second = ProgramNumber;
			InstrInsert.first->second.second.third = LineNumber;
		}

		if(InstrInsert.first->second.third != InstrEvent || Events != GotoEvent)
			//Don't overwrite a permanent instruction breakpoint.
			InstrInsert.first->second.third = Events;
	}

	//See if we should remove the breakpoint
	if(Events == NoEvent)
	{
		delete InstrInsert.first->second.first;
		InstrBreakpoints.erase(InstrInsert.first);
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::BreakpointInstructionClear()
{
	for(typename InstrMap::iterator InstrIter = InstrBreakpoints.begin(); InstrIter != InstrBreakpoints.end(); InstrIter++)
		delete InstrIter->second.first;
	InstrBreakpoints.clear();
	
	return true;
}

template<class ISA>
bool ArchSim<ISA>::BreakpointData(Number *pNumber, DataEnum DataType, EventInfo *pEvents)
{
	if(!pNumber || !pEvents)
		throw "NULL parameter to BreakpointData!";

	bool fLineInfo = false;
	unsigned int ProgramNumber, LineNumber;

	//Get the address
	uint64 Address;
	if(!pNumber->Int(MIN(8*sizeof(typename ISA::Word), 64 - ISA::Addressability), false, Address, CallBack, " for data breakpoint address", true, ISA::Addressability, false, true, 0))
		return false;
	Address <<= ISA::Addressability;

	//Try to get the program and line number information
	//Also Get the type of the data
	{
		Element *pElement = NULL;
		if(pNumber->NumberType == NumSymbol)
			//First try to get it based off the symbols used
			reinterpret_cast<SymbolNumber *>(pNumber)->ResolveValue(&pElement, NullCallBack, " for data breakpoint address");
		if(!pElement)
			//Second try to get it based on the address
			pElement = AddressToElement(Address);
		if(pElement)
		{
			fLineInfo = true;
			ProgramNumber = pElement->LocationStack.rbegin()->first;
			LineNumber = pElement->LocationStack.rbegin()->second;
			//Only get datatype info for STRUCT (auto-select)
			if(DataType == STRUCT && pElement->ElementType == DataElement)
			{
				Data *pData = reinterpret_cast<Data *>(pElement);
				if((Address - pData->Address)%vDataBytes[pData->DataType] == 0)
					//The address is aligned at the start of a data element
					DataType = pData->DataType;
			}
		}
	}

	//Check the address
	if(Address + vDataBytes[DataType] - 1 > ISA::MaxAddress)
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%.31s at address (4x%.*I64X) would cross ISA's maximum address (4x%.*I64X)", sDataTypes[DataType], sizeof(typename ISA::Word)*2, Address >> ISA::Addressability, sizeof(typename ISA::Word)*2, ISA::MaxAddress >> ISA::Addressability);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%.31s at address (4x%.*llX) would cross ISA's maximum address (4x%.*llX)", sDataTypes[DataType], sizeof(typename ISA::Word)*2, Address >> ISA::Addressability, sizeof(typename ISA::Word)*2, ISA::MaxAddress >> ISA::Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Check the valueevents
	EventEnum Events = pEvents->Events;
	if(Events & ValueEvent)
	{
		for(list<Number *>::iterator ValueIter = pEvents->ValueList.begin(); ValueIter != pEvents->ValueList.end(); ValueIter++)
		{
			if(DataType != STRUCT && DataType >= REAL1)
			{
				unsigned char Sign;
				unsigned short Exponent;
				uint64 Mantissa;
				(*ValueIter)->Float(vExponentBits[DataType], vMantissaBits[DataType], Sign, Exponent, Mantissa, CallBack, " for data breakpoint value event", false, ISA::Addressability);
			}
			else
			{
				uint64 TempInt64;
				(*ValueIter)->Int(8*vDataBytes[DataType], false, TempInt64, CallBack, " for data breakpoint value event", false, ISA::Addressability);
			}
		}
	}

	//See if there are breakpoints for this address
	pair<STDTYPENAME DataMap::iterator, bool> DataInsert = DataBreakpoints.insert(STDTYPENAME DataMap::value_type(Address, DataInfo(pNumber, DataType, LineInfo(fLineInfo, ProgramNumber, LineNumber), pEvents)));
	if(DataInsert.second == false)
	{
		//Update the number
		if(DataInsert.first->second.first->NumberType == NumSymbol && pNumber->NumberType != NumSymbol)
			//Keep the old number if it's a symbol and this one isn't.
			delete pNumber;
		else
		{
			delete DataInsert.first->second.first;
			DataInsert.first->second.first = pNumber;
		}

		//Update the line info
		if(!DataInsert.first->second.third.first || fLineInfo)
		{
			DataInsert.first->second.third.first = fLineInfo;
			DataInsert.first->second.third.second = ProgramNumber;
			DataInsert.first->second.third.third = LineNumber;
		}

		if(DataType != STRUCT)
			DataInsert.first->second.second = DataType;
		*DataInsert.first->second.fourth += *pEvents;
		delete pEvents;
	}

	//See if we should remove the breakpoint
	if(Events == NoEvent)
	{
		delete DataInsert.first->second.first;
		delete DataInsert.first->second.fourth;
		DataBreakpoints.erase(DataInsert.first);
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::BreakpointDataClear()
{
	for(typename DataMap::iterator DataIter = DataBreakpoints.begin(); DataIter != DataBreakpoints.end(); DataIter++)
	{
		delete DataIter->second.first;
		delete DataIter->second.fourth;
	}
	DataBreakpoints.clear();

	return true;
}

template<class ISA>
bool ArchSim<ISA>::BreakpointMemory(const string &sMemory, Number *pNumber, DataEnum DataType, EventInfo *pEvents)
{
	if(!pNumber || !pEvents)
		throw "NULL parameter to BreakpointMemory!";

	//Check the memory name
	if(pArch->Memories.find(sMemory) == pArch->Memories.end())
	{
		sprintf(sMessageBuffer, "\"%.63s\" is not a valid memory name.", sMemory.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Get the address
	uint64 Address;
	if(!pNumber->Int(64, false, Address, CallBack, " for memory breakpoint address", true, 0, false, true, 0))
		return false;

	//Check the memory address range
	Architecture::MemoryMap::iterator MemIter = pArch->Memories.find(sMemory);

	if(Address < MemIter->second.Array.Begin())
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%.63s[4x%I64X] is outside of the memory's address range.", sMemory.c_str(), Address);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%.63s[4x%llX] is outside of the memory's address range.", sMemory.c_str(), Address);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Error, sMessageBuffer);
		return false;
	}
	if(Address + vDataBytes[DataType] - 1 > MemIter->second.Array.End())
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%.63s[4x%I64X] is outside of the memory's address range.", sMemory.c_str(), Address + vDataBytes[DataType] - 1);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%.63s[4x%llX] is outside of the memory's address range.", sMemory.c_str(), Address + vDataBytes[DataType] - 1);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Check the valueevents
	EventEnum Events = pEvents->Events;
	if(Events & ValueEvent)
	{
		for(list<Number *>::iterator ValueIter = pEvents->ValueList.begin(); ValueIter != pEvents->ValueList.end(); ValueIter++)
		{
			if(DataType != STRUCT && DataType >= REAL1)
			{
				unsigned char Sign;
				unsigned short Exponent;
				uint64 Mantissa;
				(*ValueIter)->Float(vExponentBits[DataType], vMantissaBits[DataType], Sign, Exponent, Mantissa, CallBack, " for memory breakpoint value event", false, ISA::Addressability);
			}
			else
			{
				uint64 TempInt64;
				(*ValueIter)->Int(8*vDataBytes[DataType], false, TempInt64, CallBack, " for memory breakpoint value event", false, ISA::Addressability);
			}
		}
	}

	//See if there are breakpoints for this memory
	pair<STDTYPENAME MemoryMap::iterator, bool> MemoryInsert = MemoryBreakpoints.insert(STDTYPENAME  MemoryMap::value_type(sMemory, MemoryAddrMap()));

	//See if there are breakpoints for this address
	pair<STDTYPENAME MemoryAddrMap::iterator, bool> AddrInsert = MemoryInsert.first->second.insert(STDTYPENAME  MemoryAddrMap::value_type(Address, MemoryInfo(DataType, pEvents)));
	if(AddrInsert.second == false)
	{
		if(DataType != STRUCT)
			AddrInsert.first->second.first = DataType;
		*AddrInsert.first->second.second += *pEvents;
		delete pEvents;
	}

	//See if we should remove the breakpoint
	if(Events == NoEvent)
	{
		delete AddrInsert.first->second.second;
		MemoryInsert.first->second.erase(AddrInsert.first);
		if(MemoryInsert.first->second.empty())
			MemoryBreakpoints.erase(MemoryInsert.first);
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::BreakpointMemoryClear()
{
	for(typename MemoryMap::iterator MemIter = MemoryBreakpoints.begin(); MemIter != MemoryBreakpoints.end(); MemIter++)
	{
		for(typename MemoryAddrMap::iterator MemAddrIter = MemIter->second.begin(); MemAddrIter != MemIter->second.end(); MemAddrIter++)
			delete MemAddrIter->second.second;
	}
	MemoryBreakpoints.clear();

	return true;
}

template<class ISA>
bool ArchSim<ISA>::BreakpointRegister(const string &sRegisterSet, const string &sRegister, EventInfo *pEvents)
{
	if(!pEvents)
		throw "NULL parameter to BreakpointRegister!";

	//Check the register set name
	Architecture::RegisterSetMap::iterator RegSetIter = pArch->RegisterSets.find(sRegisterSet);
	if(RegSetIter == pArch->RegisterSets.end())
	{
		sprintf(sMessageBuffer, "\"%.63s\" is not a valid register set name.", sRegisterSet.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Check the register name
	if(RegSetIter->second.Registers.find(sRegister) == RegSetIter->second.Registers.end())
	{
		sprintf(sMessageBuffer, "\"%.63s.%.63s\" is not a valid register name.", sRegisterSet.c_str(), sRegister.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Check the valueevents
	EventEnum Events = pEvents->Events;
	if(Events & ValueEvent)
	{
		Register &TheReg = pArch->RegisterSets.find(sRegisterSet)->second.Registers.find(sRegister)->second;
		for(list<Number *>::iterator ValueIter = pEvents->ValueList.begin(); ValueIter != pEvents->ValueList.end(); ValueIter++)
		{
			if(TheReg.fFloat)
			{
				unsigned char Sign;
				unsigned short Exponent;
				uint64 Mantissa;
				(*ValueIter)->Float(TheReg.ExponentBits, TheReg.MantissaBits, Sign, Exponent, Mantissa, CallBack, " for register breakpoint value event", false, ISA::Addressability);
			}
			else
			{
				uint64 TempInt64;
				(*ValueIter)->Int(TheReg.Bits, false, TempInt64, CallBack, " for register breakpoint value event", false, ISA::Addressability);
			}
		}
	}

	//See if there are breakpoints for this registerset
	pair<STDTYPENAME RegisterSetMap::iterator, bool> RegSetInsert = RegisterBreakpoints.insert(STDTYPENAME  RegisterSetMap::value_type(sRegisterSet, RegisterMap()));

	//See if there are breakpoints for this register
	pair<STDTYPENAME RegisterMap::iterator, bool> RegInsert = RegSetInsert.first->second.insert(STDTYPENAME  RegisterMap::value_type(sRegister, pEvents));
	if(RegInsert.second == false)
	{
		*RegInsert.first->second += *pEvents;
		delete pEvents;
	}

	//See if we should remove the breakpoint
	if(Events == NoEvent)
	{
		delete RegInsert.first->second;
		RegSetInsert.first->second.erase(RegInsert.first);
		if(RegSetInsert.first->second.empty())
			RegisterBreakpoints.erase(RegSetInsert.first);
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::BreakpointRegisterClear()
{
	for(typename RegisterSetMap::iterator RegSetIter = RegisterBreakpoints.begin(); RegSetIter != RegisterBreakpoints.end(); RegSetIter++)
	{
		for(typename RegisterMap::iterator RegIter = RegSetIter->second.begin(); RegIter != RegSetIter->second.end(); RegIter++)
			delete RegIter->second;
	}
	RegisterBreakpoints.clear();

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayHelp(string sCommand)
{
	SimCallBack(Info, "Commands are case insensitive, [] = optional, | = or, () = precedence");
	SimCallBack(Info, "");
	if(sCommand == "sim_help")
	{
		SimCallBack(Info, "Syntax: HELP");
		SimCallBack(Info, "    Displays a list of all the commands.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: HELP (sim command)");
		SimCallBack(Info, "    Displays info about the specified sim command.");
	}
	else if(sCommand == "sim_reset")
	{
		SimCallBack(Info, "Syntax: RESET");
		SimCallBack(Info, "    Reinitializes the architecture (clears registers and memory). Loads the original program image into the logical memory.");
	}
	else if(sCommand == "sim_quit" || sCommand == "sim_exit")
	{
		SimCallBack(Info, "Syntax: QUIT");
		SimCallBack(Info, "Syntax: EXIT");
		SimCallBack(Info, "    Either command quits simulation and exits the simulator.");
	}
	else if(sCommand == "sim_console")
	{
		SimCallBack(Info, "Syntax: CONSOLE number");
		SimCallBack(Info, "    Sets the console width to the specified number. Assembler and simulator messages are smart-wrapped at the console width. The default is 80 characters.");
	}
	else if(sCommand == "sim_printi")
	{
		SimCallBack(Info, "Syntax: PRINTI number number");
		SimCallBack(Info, "    Sets the pre-instruction count and post-instruction count to the specified number. When the current instruction is printed, the specified number of instructions before and after it are also printed. The default is 1 before and 3 after.");
	}
	else if(sCommand == "sim_saves" || sCommand == "sim_loads")
	{
		SimCallBack(Info, "Syntax: SAVES \"filename\"");
		SimCallBack(Info, "    Saves the state of the simulation. All of the memories will be saved to FileName.Mem.Name.bin, all of the register sets will be saved to FileName.RegSet.Name.bin, any architecture state which is not in registers is saved to FileName.Arch.bin, and any required simulator data is saved to FileName.Sim.bin. The filename must be enclosed in quotes, and does not need an extension.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: LOADS \"filename\"");
		SimCallBack(Info, "    Loads the state of the simulation from a previous save.");
	}
	else if(sCommand == "sim_saved" || sCommand == "sim_loadd")
	{
		SimCallBack(Info, "Syntax: SAVED \"filename\" address length [[-] (datatype)]");
		SimCallBack(Info, "    Saves a logical data image beginning at Address spanning Length number of data elements to the specified binary file. If a datatype is specified (the assembler keywords DATA1-8 and REAL1-8), it interprets the data as elements of that datatype, otherwise ISA data. If the minus sign is provided, it indicates to save the data in the opposite endian-ness as the ISA, otherwise the same endian-ness as the ISA. The filename must be enclosed in quotes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: LOADD \"filename\" address length [[-] (datatype)]");
		SimCallBack(Info, "    Loads a logical data image beginning at Address spanning Length number of data elements from the specified binary file. If a datatype is specified (the assembler keywords DATA1-8 and REAL1-8), it interprets the data as elements of that datatype, otherwise ISA data. If the minus sign is provided, it indicates to load the data in the opposite endian-ness as the ISA, otherwise the same endian-ness as the ISA. The filename must be enclosed in quotes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_saveo" || sCommand == "sim_loado")
	{
		SimCallBack(Info, "Syntax: SAVEO \"filename\" address length");
		SimCallBack(Info, "    Saves a program object file beginning at Address spanning Length number of data elements to the specified binary file. The filename must be enclosed in quotes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: LOADO \"filename\"");
		SimCallBack(Info, "    Loads a program object from the specified binary file. The filename must be enclosed in quotes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_trace" || sCommand == "sim_traceon" || sCommand == "sim_traceoff")
	{
		SimCallBack(Info, "Syntax: TRACEON \"FileName\" [(list of register set names)]");
		SimCallBack(Info, "    Turns on a trace of the dynamically executing instructions. If a list of register set names is provided (each name separated by whitespace only), then the contents of those register sets will be dumped to file along with the instruction trace. The filename must be enclosed in quotes. Whenever possible, the originating program filename and line number for the executing instruction will be recorded in the trace file.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: TRACEOFF");
		SimCallBack(Info, "    Turns off the trace and closes the file.");
	}
	else if(sCommand == "sim_check" || sCommand == "sim_checkon" || sCommand == "sim_checkoff")
	{
		SimCallBack(Info, "Syntax: CHECKON");
		SimCallBack(Info, "    Enables certain runtime checking of the program execution which may detect possible program or architectural errors. Check violations will create warning messages and will break into the simulator command. Typing \"go\" will resume simulation.");
		SimCallBack(Info, "    Example violations: PC addresses which do not correspond to programmed instruction elements, infinite subroutine recursion, and unmatched subroutine returns. Runtime checking is enabled by default.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: CHECKOFF");
		SimCallBack(Info, "    Disables runtime checking. Architectural processor exceptions are not disabled. Runtime checking could be a considerable overhead in a large program.");
	}
	else if(sCommand == "sim_go" || sCommand == "sim_goi" || sCommand == "sim_goin" || sCommand == "sim_goover" || sCommand == "sim_goout")
	{
		SimCallBack(Info, "Syntax: GO");
		SimCallBack(Info, "    Starts or resumes simulation.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: GO (cycle count)");
		SimCallBack(Info, "    Simulates the specified number of cycles and then breaks, unless another breakpoint intervenes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: GOI (instruction count)");
		SimCallBack(Info, "    Executes the specified number of instructions and then breaks, unless another breakpoint intervenes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: GOIN");
		SimCallBack(Info, "    Simulates until the next subroutine call, unless another breakpoint intervenes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: GOOVER");
		SimCallBack(Info, "    Simulates the next subroutine call (which may or may not be the next instruction), and then breaks after the subroutine returns, unless another breakpoint intervenes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: GOOUT");
		SimCallBack(Info, "    Simulates until the currently executing subroutine returns, unless another breakpoint intervenes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    A number parameter can be of any integral or real syntax as defined in the Assembler documentation.");
	}
	else if(sCommand == "sim_goto" || sCommand == "sim_gotol" || sCommand == "sim_gotoi")
	{
		SimCallBack(Info, "Syntax: GOTOL (line number)");
		SimCallBack(Info, "    Simulates until the instruction at or after the specified code line number is reached. The line number refers to the assembly file this program was generated from. The simulator will still break at this instruction, even if another breakpoint intervened, unless the breakpoint is disabled using the BPL or BPI command.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: GOTOI (instruction label or address)");
		SimCallBack(Info, "    Simulates until the instruction at the specified label or address is reached. The simulator will still break at this instruction, even if another breakpoint intervened, unless the breakpoint is disabled using the BPL or BPI command.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_bpl" || sCommand == "sim_bpi" || sCommand == "sim_bpic")
	{
		SimCallBack(Info, "Syntax: BPL (line number)");
		SimCallBack(Info, "    Sets a breakpoint for the instruction at or after the given code line number.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: BPI (instruction label or address)");
		SimCallBack(Info, "    Sets a breakpoint for the instruction at the specified label or address.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: BPIC");
		SimCallBack(Info, "    Clears all instruction, line, and goto breakpoints.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_bpd" || sCommand == "sim_bpdc")
	{
		SimCallBack(Info, "Syntax: BPD (data label or address) [(datatype)] (event list)");
		SimCallBack(Info, "    Sets a breakpoint for the specified logical data location. See the \"EVENTLIST\" help for info on the other parameters.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: BPDC");
		SimCallBack(Info, "    Clears all data breakpoints.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_bpm" || sCommand == "sim_bpmc")
	{
		SimCallBack(Info, "Syntax: BPM (memory name) (memory address) [(datatype)] (event list)");
		SimCallBack(Info, "    Sets a breakpoint for the specified memory location. See the \"EVENTLIST\" help for info on the other parameters. See the \"DM\" command to get the memory names.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: BPMC");
		SimCallBack(Info, "    Clears all memory breakpoints.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_bpr" || sCommand == "sim_bprc")
	{
		SimCallBack(Info, "Syntax: BPR (register set name) . (register name) (event list)");
		SimCallBack(Info, "    Sets a breakpoint for the specified register. See the \"EVENTLIST\" help for info on the other parameters. See the \"DRS\" command to get the register names.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: BPRC");
		SimCallBack(Info, "    Clears all register breakpoints.");
	}
	else if(sCommand == "sim_eventlist")
	{
		SimCallBack(Info, "Syntax: [(datatype)] (list of events)");
		SimCallBack(Info, "Event: (NOEVENT | READEVENT | WRITEEVENT | CHANGEEVENT | ValueEvent)");
		SimCallBack(Info, "ValueEvent: VALUEEVENT Value");
		SimCallBack(Info, "    Specifies what type of events this breakpoint is for. For data value events, the simulator will use the program to determine the datatype of the breakpoint location, or the datatype can be specified (the assembler keywords DATA1-8 and REAL1-8). If the datatype cannot be determined or the datatype is not specified for a memory value event, it will be interpreted as byte data. \"BPR\" does not use the datatype. \"BPI\" and \"BPL\" do not use the eventlist because they only specify an instruction execution event. Multiple events can be specified, and VALUEEVENT can be specified multple times.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    The value can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a symbolic value. If not, the simulator will guess which program to use to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_dil" || sCommand == "sim_di" || sCommand == "sim_dl" || sCommand == "sim_dd" || sCommand == "sim_dla" || sCommand == "sim_dda")
	{
		SimCallBack(Info, "Syntax: DIL (line number) [(display length)]");
		SimCallBack(Info, "Syntax: DI (instruction label or address) [(display length)]");
		SimCallBack(Info, "    Displays the specified number of instructions at the specified code line number or logical data location. Even if the data was not declared as an instruction, it will be disassembled and displayed as an instruction. The default display length is one instruction.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: DL (line number) [[-] (datatype)] [(display length)]");
		SimCallBack(Info, "Syntax: DD (data label or address) [[-] (datatype)] [(display length)]");
		SimCallBack(Info, "    Displays the specified number of data elements at the specified code line number or logical data location. The simulator will use the program to determine the datatype, or the datatype can be specified (the assembler keywords DATA1-8 and REAL1-8). Integral data is interpreted as unsigned unless the '-' operator is specified. The default display length is one data element");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: DLA (line number) [(display length)]");
		SimCallBack(Info, "Syntax: DDA (data label or address) [(display length)]");
		SimCallBack(Info, "    Displays an array of byte data and corresponding characters at the specified code line number or logical data location. The default display length is 128 bytes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_dm" || sCommand == "sim_dma")
	{
		SimCallBack(Info, "Syntax: DM");
		SimCallBack(Info, "    Displays the names of all the memories.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: DM (memory name) (memory address) [[-] (datatype)]  [(display length)]");
		SimCallBack(Info, "    Displays the specified number of data elements at the specified memory location. If the datatype is specified (the assembler keywords DATA1-8 and REAL1-8), then the memory is interpreted as that datatype. Integral data is interpreted as unsigned unless the '-' operator is specified. If the datatype is not specified, then it is interpreted as ISA data. The default display length is one data element.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: DMA (memory name) (data label or address) [(display length)]");
		SimCallBack(Info, "    Displays an array of byte data and corresponding characters at the specified memory location. The default display length is 128 bytes.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_drs" || sCommand == "sim_dr")
	{
		SimCallBack(Info, "Syntax: DRS");
		SimCallBack(Info, "    Displays the names of all the register sets and the names of all the registers within each set.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: DRS (register set name)");
		SimCallBack(Info, "    Displays the contents of all the registers in the specified register set.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Syntax: DR (register set name) . (register name)");
		SimCallBack(Info, "    Displays the contents of the specified register.");
	}
	else if(sCommand == "sim_dpl")
	{
		SimCallBack(Info, "Syntax: DPL");
		SimCallBack(Info, "    Displays the names of all the pipelines and the names of all the pipeline stages within each pipeline.");
	}
	else if(sCommand == "sim_dc")
	{
		SimCallBack(Info, "Syntax: DCI");
		SimCallBack(Info, "    Displays the current cycle and instruction counts.");
	}
	else if(sCommand == "sim_dcs")
	{
		SimCallBack(Info, "Syntax: DCS");
		SimCallBack(Info, "    Displays the contents of the call stack. Whenever possible, the originating file and line number of the subroutine call is listed.");
	}
	else if(sCommand == "sim_dp")
	{
		SimCallBack(Info, "Syntax: DP");
		SimCallBack(Info, "    Displays all of the program filenames and their corresponding program numbers for all the program files which were linked together to create this program image.");
	}
	else if(sCommand == "sim_dbp")
	{
		SimCallBack(Info, "Syntax: DBP");
		SimCallBack(Info, "    Displays all of the currently active breakpoints.");
	}
	else if(sCommand == "sim_wl")
	{
		SimCallBack(Info, "Syntax: WL (line number) (element list)");
		SimCallBack(Info, "    Write the memory image from the assembled elements into the logical data location at or after the specified code line number. Data already at that location will be overwritten. Changes due to this write are lost on reset and do not affect the original program. The element list can consist of any number of data, struct, and instruction elements as defined in the Assembler documentation. Symbols can be used within the element list.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    A number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number. Program numbers should not appear inside the element list; instead a program number specified before the line number will be used to resolve any symbols used in the element list.");
	}
	else if(sCommand == "sim_wd")
	{
		SimCallBack(Info, "Syntax: WD (data label or address) (element list)");
		SimCallBack(Info, "    Write the memory image from the assembled elements into the specified logical data location. Data already at that location will be overwritten. Changes due to this write are lost on reset and do not affect the original program. The element list can consist of any number of data, struct, and instruction elements as defined in the Assembler documentation. Symbols can be used within the element list.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number. Program numbers should not appear inside the element list; instead a program number specified before the address will be used to resolve any symbols used in the element list.");
	}
	else if(sCommand == "sim_wm")
	{
		SimCallBack(Info, "Syntax: WM (memory name) (data label or address) (element list)");
		SimCallBack(Info, "    Write the memory image from the assembled elements into the specified memory location. The element list can consist of any number of data, struct, and instruction elements as defined in the Assembler documentation. Symbols can be used within the element list. See the \"DM\" command to get the memory names.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    An address can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a line number or symbolic address. If not, the simulator will guess which program to use for the line number or to resolve the symbol. See the \"DP\" command to get the program number. Program numbers should not appear inside the element list; instead a program number specified before the address will be used to resolve any symbols used in the element list.");
	}
	else if(sCommand == "sim_wr")
	{
		SimCallBack(Info, "Syntax: WR (register set name) . (register name) value");
		SimCallBack(Info, "    Write the value into the specified register. See the \"DRS\" command to get the register names.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    The value can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a symbolic value. If not, the simulator will guess which program to use to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else if(sCommand == "sim_int")
	{
		SimCallBack(Info, "Syntax: INT (interrupt label or vector number)");
		SimCallBack(Info, "    Sends an external interrupt to the processor. You can use a label into the interrupt vector table or an interrupt number.");
		SimCallBack(Info, "");
		SimCallBack(Info, "Notes:");
		SimCallBack(Info, "    The value can be specified using a number or symbol (including structure member access, array indexing, and attributes as defined in the Assembler documentation). The number parameter can be of any integral or real syntax as defined in the Assembler documentation. If more than one program file was linked together, then a program number (a number enclosed in braces) should preceed a symbolic value. If not, the simulator will guess which program to use to resolve the symbol. See the \"DP\" command to get the program number.");
	}
	else
	{
		SimCallBack(Info, "HELP, RESET, QUIT, EXIT, CONSOLE, PRINTI");
		SimCallBack(Info, "SAVES, LOADS, SAVED, LOADD, SAVEO, LOADO");
		SimCallBack(Info, "TRACEON, TRACEOFF, CHECKON, CHECKOFF");
		SimCallBack(Info, "GO, GO #, GOI, GOIN, GOOVER, GOOUT, GOTOL, GOTOI");
		SimCallBack(Info, "BPL, BPI, BPD, BPM, BPR (Breakpoint: Line, Instruction, Data, Memory, Register)");
		SimCallBack(Info, "    BPIC, BPDC, BPMC, BPRC (Breakpoint Clear)");
		SimCallBack(Info, "    EVENTLIST (Syntax used for breakpoint commands)");
		SimCallBack(Info, "DIL, DI, DL, DD, DLA, DDA, DM, DMA, DRS, DR, DPL");
		SimCallBack(Info, "    (Display: Instruction, Data, Memory, Register Set, Register, Pipeline)");
		SimCallBack(Info, "DCI, DCS, DP, DBP (Display: Cycle, Call Stack, Programs, Breakpoints)");
		SimCallBack(Info, "WL, WD, WM, WR (Write: Line, Data, memory, Register)");
		SimCallBack(Info, "INT (Processor Interrupt)");
		SimCallBack(Info, "");
		SimCallBack(Info, "Type \"HELP\" followed by a command for detailed info (e.g. \"sim> help bpl\")");
	}
	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayInstructionLine(bool fNumberSpecified, unsigned int ProgramNumber, unsigned int LineNumber, Number *pLength)
{
	//Convert the program and line numbers into an address
	Number *pNumber;
	if( !(pNumber = LineToAddress(fNumberSpecified, ProgramNumber, LineNumber, "Display instruction line command")) )
		return false;
	((IntegerNumber *)(pNumber))->Value >>= ISA::Addressability;

	//Finalize the display
	if(!DisplayInstruction(pNumber, pLength))
	{
		delete pNumber;
		return false;
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayInstruction(Number *pNumber, Number *pLength)
{
	if(!pNumber || !pLength)
		throw "NULL parameter to DisplayInstruction!";

	//Get the address
	uint64 Address;
	if(!pNumber->Int(MIN(8*sizeof(typename ISA::Word), 64 - ISA::Addressability), false, Address, CallBack, " for instruction display address", true, ISA::Addressability, false, true, 0))
		return false;
	Address <<= ISA::Addressability;

	//Get the number of instructions to display
	uint64 Length;
	if(!pLength->Int(MAX_DISPLAY_LENGTH_LG2, false, Length, CallBack, " for instruction display length", false))
		return false;
	if(Address + Length * sizeof(typename ISA::Word) > ISA::MaxAddress || Address + Length * sizeof(typename ISA::Word) < Address)
	{
		Length = ISA::MaxAddress - Address;
	}

	//Get the data
	RamVector vData;
	if(!pArch->DataRead(vData, Address, Length * sizeof(typename ISA::Word)))
		return false;

	for(uint64 i = 0; i < Length; i++)
	{
		ostrstream strInstr;
		if(!PrintInstruction(strInstr, NULL, Address + i * sizeof(typename ISA::Word), false))
			return false;
		strInstr << ends;
		SimCallBack(Info, strInstr.str());
	}

	delete pNumber;
	delete pLength;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayLine(bool fNumberSpecified, unsigned int ProgramNumber, unsigned int LineNumber, Number *pLength, DataEnum DataType, bool fSigned)
{
	//Convert the program and line numbers into an address
	Number *pNumber;
	if( !(pNumber = LineToAddress(fNumberSpecified, ProgramNumber, LineNumber, "Display line command")) )
		return false;
	((IntegerNumber *)(pNumber))->Value >>= ISA::Addressability;

	//Finalize the display
	if(!DisplayData(pNumber, pLength, DataType, fSigned))
	{
		delete pNumber;
		return false;
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayData(Number *pAddress, Number *pLength, DataEnum DataType, bool fSigned)
{
	if(!pAddress || !pLength)
		throw "NULL parameter to DisplayData!";

	//Get the address
	uint64 Address;
	if(!pAddress->Int(MIN(8*sizeof(typename ISA::Word), 64 - ISA::Addressability), false, Address, CallBack, " for data display address", true, ISA::Addressability, false, true, 0))
		return false;
	Address <<= ISA::Addressability;

	//Get the number of data elements to display
	uint64 Length;
	if(!pLength->Int(MAX_DISPLAY_LENGTH_LG2, false, Length, CallBack, " for data display length", false))
		return false;

	bool fDynDataType = false;
	if(DataType == STRUCT)
		//The datatype is determined at each iteration
		fDynDataType = true;

	//Print each element
	for(uint64 i = 0; i < Length; i++)
	{
		//Get the type of the data
		bool fInstrType = false;
		if(fDynDataType)
		{
			Element *pElement = NULL;
			if(i == 0 && pAddress->NumberType == NumSymbol)
				//First try to get it based off the symbols used
				reinterpret_cast<SymbolNumber *>(pAddress)->ResolveValue(&pElement, NullCallBack, " for data display address");
			if(!pElement)
				//Second try to get it based on the address
				pElement = AddressToElement(Address);
			if(pElement && pElement->ElementType == DataElement)
			{
				Data *pData = reinterpret_cast<Data *>(pElement);
				if((Address - pData->Address)%vDataBytes[pData->DataType] == 0)
					//The address is aligned at the start of a data element
					DataType = pData->DataType;
			}
			else if(pElement && pElement->ElementType == InstructionElement)
			{
				//*NOTE: This is assuming the instruction word is the same as the address word.
				unsigned short Temp = (unsigned short)(log((double)sizeof(typename ISA::Word))/log(2.));
				DataType = (DataEnum)Temp;
				fInstrType = true;
			}
			else
				//cannot determine, choose the ISA's addressability
				DataType = (DataEnum)ISA::Addressability;
		}

		//Get the data
		RamVector vData;
		if(!pArch->DataRead(vData, Address, vDataBytes[DataType]))
			return false;

		if(fInstrType)
		{
			ostrstream strInstr;
			if(!PrintInstruction(strInstr, NULL, Address, false))
				return false;
			strInstr << ends;
			SimCallBack(Info, strInstr.str());
		}
		else if(DataType >= REAL1)
		{
			RealNumber Real(NullLocationStack, vExponentBits[DataType], vMantissaBits[DataType], vData, ISA::fLittleEndian);
			#if defined _MSC_VER
				sprintf(sMessageBuffer, "[4x%.*I64X]:\t%.63s (%.16e)", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability, (const char *)Real, Real.Value.DBL);
			#elif defined GPLUSPLUS
				sprintf(sMessageBuffer, "[4x%.*llX]:\t%.63s (%.16e)", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability, (const char *)Real, Real.Value.DBL);
			#else
				#error "Only MSVC and GCC Compilers Supported"
			#endif
			SimCallBack(Info, sMessageBuffer);
		}
		else
		{
			IntegerNumber Int(NullLocationStack, vData, ISA::fLittleEndian);
			uint64 Mask = (uint64)1 << (8*vDataBytes[DataType] - 1);
			unsigned int DisplayLength = vDataBytes[DataType]*2;
			if(fSigned && (Int.Value & Mask))
			{
				Mask = (vDataBytes[DataType] == sizeof(uint64)) ? -1 : (Mask << 1) - 1;
				#if defined _MSC_VER
					sprintf(sMessageBuffer, "[4x%.*I64X]:\t-4x%.*I64X (-%I64u)", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability, DisplayLength, -Int.Value & Mask, -Int.Value & Mask);
				#elif defined GPLUSPLUS
					sprintf(sMessageBuffer, "[4x%.*llX]:\t-4x%.*llX (-%llu)", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability, DisplayLength, -Int.Value & Mask, -Int.Value & Mask);
				#else
					#error "Only MSVC and GCC Compilers Supported"
				#endif
			}
			else
			{
				#if defined _MSC_VER
					sprintf(sMessageBuffer, "[4x%.*I64X]:\t4x%.*I64X (%I64u)", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability, DisplayLength, Int.Value, Int.Value);
				#elif defined GPLUSPLUS
					sprintf(sMessageBuffer, "[4x%.*llX]:\t4x%.*llX (%llu)", sizeof(typename ISA::Word)*2, Address >> ISA::Addressability, DisplayLength, Int.Value, Int.Value);
				#else
					#error "Only MSVC and GCC Compilers Supported"
				#endif
			}
			SimCallBack(Info, sMessageBuffer);
		}

		if(Address + vDataBytes[DataType] > ISA::MaxAddress || Address + vDataBytes[DataType] < Address)
			break;
		else
			Address += vDataBytes[DataType];
	}

	delete pAddress;
	delete pLength;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayLineArray(bool fNumberSpecified, unsigned int ProgramNumber, unsigned int LineNumber, Number *pLength)
{
	//Convert the program and line numbers into an address
	Number *pNumber;
	if( !(pNumber = LineToAddress(fNumberSpecified, ProgramNumber, LineNumber, "Display line array command")) )
		return false;
	((IntegerNumber *)(pNumber))->Value >>= ISA::Addressability;

	//Finalize the display
	if(!DisplayDataArray(pNumber, pLength))
	{
		delete pNumber;
		return false;
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayDataArray(Number *pAddress, Number *pLength)
{
	if(!pAddress || !pLength)
		throw "NULL parameter to DisplayDataArray!";

	//Get the address
	uint64 Address;
	if(!pAddress->Int(MIN(8*sizeof(typename ISA::Word), 64 - ISA::Addressability), false, Address, CallBack, " for data display array address", true, ISA::Addressability, false, true, 0))
		return false;
	Address <<= ISA::Addressability;

	//Get the length
	uint64 Length;
	if(!pLength->Int(MAX_DISPLAY_ARRAY_LENGTH_LG2, false, Length, CallBack, " for data display array length", true))
		return false;

	//Get the memory data
	RamVector vData;
	if(!pArch->DataRead(vData, Address, Length))
		return false;

	//Setup the character lengths of numbers in the display
	int AddrLength = ((ISA::MaxAddress >> ISA::Addressability) & 0xFFFF000000000000) ? 16 :
					(
						((ISA::MaxAddress >> ISA::Addressability) & 0xFFFFFFFF00000000) ? 12 :
						(
							((ISA::MaxAddress >> ISA::Addressability) & 0xFFFFFFFFFFFF0000) ? 8 : 4
						)
					);
	char sAddress[32], sDataBytes[256], sDataChar[128];
	static const unsigned int vDisplayTable[16] = {2, 4, 8, 8, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32};
	unsigned int ByteLength = vDisplayTable[((ConsoleWidth - (AddrLength + 5)) / 4) >> 2];//(AddrLength > 8) ? 8 : 16;

	//Iterate through the memory bytes
	for(uint64 i = Address; i < Address + Length;)
	{
		string sMemoryBytes;
		#if defined _MSC_VER
			sprintf(sAddress, "4x%.*I64X", AddrLength, i >> ISA::Addressability);
		#elif defined GPLUSPLUS
			sprintf(sAddress, "4x%.*llX", AddrLength, i >> ISA::Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		char *psDB = sDataBytes, *psDC = sDataChar;
		psDB += sprintf(psDB, ":");
		psDC += sprintf(psDC, ": ");
		for(unsigned int j = 0; j < ByteLength; j++, i++)
		{
			if(i < Address + Length && i >= Address)
			{
				unsigned char Byte = vData[(unsigned int)(i - Address)].second;
				psDB += sprintf(psDB, " %.2X", Byte);
				if(Byte <= 0x1F || Byte >= 0x7F)
					psDC += sprintf(psDC, ".");
				else
					psDC += sprintf(psDC, "%c", Byte);
			}
			else
			{
				psDB += sprintf(psDB, "   ");
				psDC += sprintf(psDC, " ");
			}
		}
		(((sMemoryBytes += sAddress) += sDataBytes) += sDataChar);
		SimCallBack(Info, sMemoryBytes);

		if(i + 1 > ISA::MaxAddress || i + 1 < i)
			break;
	}

	delete pAddress;
	delete pLength;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayMemory()
{
	for(Architecture::MemoryMap::iterator MemIter = pArch->Memories.begin(); MemIter != pArch->Memories.end(); MemIter++)
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%.63s[4x%I64X-4x%I64X]", MemIter->second.sName.c_str(), MemIter->second.Array.Begin(), MemIter->second.Array.End());
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%.63s[4x%llX-4x%llX]", MemIter->second.sName.c_str(), MemIter->second.Array.Begin(), MemIter->second.Array.End());
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Info, sMessageBuffer);
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayMemory(const string &sMemory, Number *pAddress, Number *pLength, DataEnum DataType, bool fSigned)
{
	if(!pAddress || !pLength)
		throw "NULL parameter to DisplayMemory!";

	//Check the memory name
	if(pArch->Memories.find(sMemory) == pArch->Memories.end())
	{
		sprintf(sMessageBuffer, "\"%.63s\" is not a valid memory name.", sMemory.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}
	Architecture::MemoryMap::iterator MemIter = pArch->Memories.find(sMemory);

	//Get the address
	uint64 Address;
	if(!pAddress->Int(64, false, Address, CallBack, " for memory display array address", true, 0, false, true, 0))
		return false;
	if(Address < MemIter->second.Array.Begin() || Address > MemIter->second.Array.End())
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "Memory address 4x%I64x is outside of memory range %.63s[4x%I64X-4x%I64X]", Address, MemIter->second.sName.c_str(), MemIter->second.Array.Begin(), MemIter->second.Array.End());
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "Memory address 4x%llx is outside of memory range %.63s[4x%llX-4x%llX]", Address, MemIter->second.sName.c_str(), MemIter->second.Array.Begin(), MemIter->second.Array.End());
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Get the length
	uint64 Length;
	if(!pLength->Int(MAX_DISPLAY_LENGTH_LG2, false, Length, CallBack, " for memory display array length", true))
		return false;

	if(DataType == STRUCT)
		//The default is ISA addressability
		DataType = (DataEnum)ISA::Addressability;

	//Setup the character lengths of numbers in the display
	int AddrLength = (MemIter->second.Array.End() & 0xFFFF000000000000) ? 16 :
					(
						(MemIter->second.Array.End() & 0xFFFFFFFF00000000) ? 12 :
						(
							(MemIter->second.Array.End() & 0xFFFFFFFFFFFF0000) ? 8 : 4
						)
					);

	//Print each memory element
	for(uint64 i = 0; i < Length; i++)
	{
		//Get the memory data
		RamVector vData;
		if(!MemIter->second.Read(vData, Address, vDataBytes[DataType], SimCallBack))
			return false;

		if(DataType >= REAL1)
		{
			RealNumber Real(NullLocationStack, vExponentBits[DataType], vMantissaBits[DataType], vData, ISA::fLittleEndian);
			#if defined _MSC_VER
				sprintf(sMessageBuffer, "%.63s[4x%.*I64X]:\t%.63s (%.16e)", MemIter->second.sName.c_str(), AddrLength, Address, (const char *)Real, Real.Value.DBL);
			#elif defined GPLUSPLUS
				sprintf(sMessageBuffer, "%.63s[4x%.*llX]:\t%.63s (%.16e)", MemIter->second.sName.c_str(), AddrLength, Address, (const char *)Real, Real.Value.DBL);
			#else
				#error "Only MSVC and GCC Compilers Supported"
			#endif
			SimCallBack(Info, sMessageBuffer);
		}
		else
		{
			IntegerNumber Int(NullLocationStack, vData, ISA::fLittleEndian);
			uint64 Mask = (uint64)1 << (8*vDataBytes[DataType] - 1);
			unsigned int DisplayLength = vDataBytes[DataType]*2;
			if(fSigned && (Int.Value & Mask))
			{
				Mask = (vDataBytes[DataType] == sizeof(uint64)) ? -1 : (Mask << 1) - 1;
				#if defined _MSC_VER
					sprintf(sMessageBuffer, "%.63s[4x%.*I64X]:\t-4x%.*I64X (-%I64u)", MemIter->second.sName.c_str(), AddrLength, Address, DisplayLength, -Int.Value & Mask, -Int.Value & Mask);
				#elif defined GPLUSPLUS
					sprintf(sMessageBuffer, "%.63s[4x%.*llX]:\t-4x%.*llX (-%llu)", MemIter->second.sName.c_str(), AddrLength, Address, DisplayLength, -Int.Value & Mask, -Int.Value & Mask);
				#else
					#error "Only MSVC and GCC Compilers Supported"
				#endif
			}
			else
			{
				#if defined _MSC_VER
					sprintf(sMessageBuffer, "%.63s[4x%.*I64X]:\t4x%.*I64X (%I64u)", MemIter->second.sName.c_str(), AddrLength, Address, DisplayLength, Int.Value, Int.Value);
				#elif defined GPLUSPLUS
					sprintf(sMessageBuffer, "%.63s[4x%.*llX]:\t4x%.*llX (%llu)", MemIter->second.sName.c_str(), AddrLength, Address, DisplayLength, Int.Value, Int.Value);
				#else
					#error "Only MSVC and GCC Compilers Supported"
				#endif
			}
			SimCallBack(Info, sMessageBuffer);
		}

		if(Address + vDataBytes[DataType] > MemIter->second.Array.End() || Address + vDataBytes[DataType] < Address)
			break;
		else
			Address += vDataBytes[DataType];
	}

	delete pAddress;
	delete pLength;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayMemoryArray(const string &sMemory, Number *pAddress, Number *pLength)
{
	if(!pAddress || !pLength)
		throw "NULL parameter to DisplayMemoryArray!";

	//Check the memory name
	if(pArch->Memories.find(sMemory) == pArch->Memories.end())
	{
		sprintf(sMessageBuffer, "\"%.63s\" is not a valid memory name.", sMemory.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}
	Architecture::MemoryMap::iterator MemIter = pArch->Memories.find(sMemory);

	//Get the address
	uint64 Address;
	if(!pAddress->Int(64, false, Address, CallBack, " for memory display address", true, 0, false, true, 0))
		return false;
	if(Address < MemIter->second.Array.Begin() || Address > MemIter->second.Array.End())
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "Memory address 4x%I64x is outside of memory range %.63s[4x%I64X-4x%I64X]", Address, MemIter->second.sName.c_str(), MemIter->second.Array.Begin(), MemIter->second.Array.End());
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "Memory address 4x%llx is outside of memory range %.63s[4x%llX-4x%llX]", Address, MemIter->second.sName.c_str(), MemIter->second.Array.Begin(), MemIter->second.Array.End());
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Get the length
	uint64 Length;
	if(!pLength->Int(MAX_DISPLAY_ARRAY_LENGTH_LG2, false, Length, CallBack, " for memory display length", true))
		return false;

	//Get the memory data
	RamVector vData;
	if(!MemIter->second.Read(vData, Address, Length, SimCallBack))
		return false;

	//Setup the character lengths of numbers in the display
/*
 00.
 00 00..
 00 00 00 00....
 00 00 00 00 00 00 00 00........
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00................
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00................................
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00................................................................
*/
	int AddrLength = (MemIter->second.Array.End() & 0xFFFF000000000000) ? 16 :
					(
						(MemIter->second.Array.End() & 0xFFFFFFFF00000000) ? 12 :
						(
							(MemIter->second.Array.End() & 0xFFFFFFFFFFFF0000) ? 8 : 4
						)
					);
	char sAddress[32], sDataBytes[256], sDataChar[128];
	static const unsigned int vDisplayTable[16] = {2, 4, 8, 8, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32};
	int ByteLength = vDisplayTable[((ConsoleWidth - (AddrLength + 5)) / 4) >> 2];//(AddrLength > 8) ? 8 : 16;

	//Iterate through the memory bytes
	for(uint64 i = Address; i < Address + Length;)
	{
		string sMemoryBytes;
		#if defined _MSC_VER
			sprintf(sAddress, "4x%.*I64X", AddrLength, i);
		#elif defined GPLUSPLUS
			sprintf(sAddress, "4x%.*llX", AddrLength, i);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		char *psDB = sDataBytes, *psDC = sDataChar;
		psDB += sprintf(psDB, ":");
		psDC += sprintf(psDC, ": ");
		for(int j = 0; j < ByteLength; j++, i++)
		{
			if(i < Address + Length && i >= Address)
			{
				unsigned char Byte = vData[(unsigned int)(i - Address)].second;
				psDB += sprintf(psDB, " %.2X", Byte);
				if(Byte <= 0x1F || Byte >= 0x7F)
					psDC += sprintf(psDC, ".");
				else
					psDC += sprintf(psDC, "%c", Byte);
			}
			else
			{
				psDB += sprintf(psDB, "   ");
				psDC += sprintf(psDC, " ");
			}
		}
		(((sMemoryBytes += sAddress) += sDataBytes) += sDataChar);
		SimCallBack(Info, sMemoryBytes);

		if(i + 1 > MemIter->second.Array.End() || i + 1 < i)
			break;
	}

	delete pAddress;
	delete pLength;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayRegisterSet()
{
	for(Architecture::RegisterSetMap::iterator RegSetIter = pArch->RegisterSets.begin(); RegSetIter != pArch->RegisterSets.end(); RegSetIter++)
	{
		string sRegisters = RegSetIter->second.sName;
		sRegisters += ": ";
		for(RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.begin(); RegIter != RegSetIter->second.Registers.end(); RegIter++)
		{
			if(RegIter != RegSetIter->second.Registers.begin())
				sRegisters += ", ";
			sRegisters += RegIter->second.sName;
		}
		SimCallBack(Info, sRegisters.c_str());
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayRegisterSet(const string &sRegisterSet)
{
	//Check the register set name
	Architecture::RegisterSetMap::iterator RegSetIter = pArch->RegisterSets.find(sRegisterSet);
	if(RegSetIter == pArch->RegisterSets.end())
	{
		sprintf(sMessageBuffer, "\"%.63s\" is not a valid register set name.", sRegisterSet.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	for(RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.begin(); RegIter != RegSetIter->second.Registers.end(); RegIter++)
	{
		sprintf(sMessageBuffer, "%.63s:\t%.63s", RegIter->second.sName.c_str(), (const char *)RegIter->second);
		SimCallBack(Info, sMessageBuffer);
	}
//	SimCallBack(Info, (const char *)RegSetIter->second);

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayRegister(const string &sRegisterSet, const string &sRegister)
{
	//Check the register set name
	Architecture::RegisterSetMap::iterator RegSetIter = pArch->RegisterSets.find(sRegisterSet);
	if(RegSetIter == pArch->RegisterSets.end())
	{
		sprintf(sMessageBuffer, "\"%.63s\" is not a valid register set name.", sRegisterSet.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Check the register name
	RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.find(sRegister);
	if(RegIter == RegSetIter->second.Registers.end())
	{
		sprintf(sMessageBuffer, "\"%.63s.%.63s\" is not a valid register name.", sRegisterSet.c_str(), sRegister.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

//	sprintf(sMessageBuffer, "%.63s.%.63s:\t%.63s", sRegisterSet.c_str(), sRegister.c_str(), (const char *)RegIter->second);
	SimCallBack(Info, (const char *)RegIter->second);

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayPipelines()
{
	for(Architecture::PipelineVector::iterator PipeIter = pArch->Pipelines.begin(); PipeIter != pArch->Pipelines.end(); PipeIter++)
	{
		string sPipelines = PipeIter->sName;
		sPipelines += ": ";
		for(Pipeline::PipelineStageVector::iterator PipeStageIter = PipeIter->PipelineStages.begin(); PipeStageIter != PipeIter->PipelineStages.end(); PipeStageIter++)
		{
			if(PipeStageIter != PipeIter->PipelineStages.begin())
				sPipelines += "->";
			sPipelines += PipeStageIter->sName;
		}
		SimCallBack(Info, sPipelines.c_str());
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayCycleInstruction()
{
	#if defined _MSC_VER
		sprintf(sMessageBuffer, "Cycle: %I64u", SimCycle);
	#elif defined GPLUSPLUS
		sprintf(sMessageBuffer, "Cycle: %llu", SimCycle);
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif
	SimCallBack(Info, sMessageBuffer);

	#if defined _MSC_VER
		sprintf(sMessageBuffer, "Instruction: %I64u", SimInstruction);
	#elif defined GPLUSPLUS
		sprintf(sMessageBuffer, "Instruciton: %llu", SimInstruction);
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif
	SimCallBack(Info, sMessageBuffer);

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayCallStack()
{
	for(CallStackList::reverse_iterator CallIter = CallStack.rbegin(); CallIter != CallStack.rend(); CallIter++)
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%.255s(%u): [4x%.*I64X]: %.63s", CallIter->first.c_str(), CallIter->second, sizeof(typename ISA::Word)*2, CallIter->third >> ISA::Addressability, CallIter->fourth.c_str());
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%.255s(%u): [4x%.*llX]: %.63s", CallIter->first.c_str(), CallIter->second, sizeof(typename ISA::Word)*2, CallIter->third >> ISA::Addressability, CallIter->fourth.c_str());
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Info, sMessageBuffer);
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayPrograms()
{
	for(unsigned int i = 0; i < pPrograms->size(); i++)
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "Program(%u): \"%.255s\", Address 4x%.*I64X, Size 4x%I64X", i, (*pPrograms)[i]->sFileName.Full.c_str(), sizeof(typename ISA::Word)*2, (*pPrograms)[i]->Address >> ISA::Addressability, (*pPrograms)[i]->Size >> ISA::Addressability);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "Program(%u): \"%.255s\", Address 4x%.*llX, Size 4x%llX", i, (*pPrograms)[i]->sFileName.Full.c_str(), sizeof(typename ISA::Word)*2, (*pPrograms)[i]->Address >> ISA::Addressability, (*pPrograms)[i]->Size >> ISA::Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Info, sMessageBuffer);
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DisplayBreakpoints()
{
	bool fSomething = false;

	if(!InstrBreakpoints.empty())
		SimCallBack(Info, "Instruction Breakpoints:");
	for(typename InstrMap::iterator InstrIter = InstrBreakpoints.begin(); InstrIter != InstrBreakpoints.end(); InstrIter++)
	{
		ostrstream strBreakpoints;
		if(InstrIter->second.second.first)
			strBreakpoints << InputList[InstrIter->second.second.second].c_str() << "(" << InstrIter->second.second.third << "):\t";
		strBreakpoints << (const char *)*InstrIter->second.first;
		if(InstrIter->second.third == GotoEvent)
			strBreakpoints << " GOTO";
		strBreakpoints << ends;
		SimCallBack(Info, strBreakpoints.str());
		fSomething = true;
	}

	if(!DataBreakpoints.empty())
	{
		if(fSomething)
			SimCallBack(Info, "");
		SimCallBack(Info, "Data Breakpoints:");
	}
	for(typename DataMap::iterator DataIter = DataBreakpoints.begin(); DataIter != DataBreakpoints.end(); DataIter++)
	{
		ostrstream strBreakpoints;
		if(DataIter->second.third.first)
			strBreakpoints << InputList[DataIter->second.third.second].c_str() << "(" << DataIter->second.third.third << "):\t";
		strBreakpoints << (const char *)*DataIter->second.first << (const char *)*DataIter->second.fourth << ends;
		SimCallBack(Info, strBreakpoints.str());
		fSomething = true;
	}

	if(!MemoryBreakpoints.empty())
	{
		if(fSomething)
			SimCallBack(Info, "");
		SimCallBack(Info, "Memory Breakpoints:");
	}
	for(typename MemoryMap::iterator MemIter = MemoryBreakpoints.begin(); MemIter != MemoryBreakpoints.end(); MemIter++)
	{
		for(typename MemoryAddrMap::iterator MemAddrIter = MemIter->second.begin(); MemAddrIter != MemIter->second.end(); MemAddrIter++)
		{
			ostrstream strBreakpoints;
			#if defined _MSC_VER
				sprintf(sMessageBuffer, "[4x%I64X]", MemAddrIter->first);
			#elif defined GPLUSPLUS
				sprintf(sMessageBuffer, "[4x%llX]", MemAddrIter->first);
			#else
				#error "Only MSVC and GCC Compilers Supported"
			#endif
			strBreakpoints << MemIter->first.c_str() << sMessageBuffer << (const char *)*MemAddrIter->second.second << ends;
			SimCallBack(Info, strBreakpoints.str());
		}
		fSomething = true;
	}

	if(!RegisterBreakpoints.empty())
	{
		if(fSomething)
			SimCallBack(Info, "");
		SimCallBack(Info, "Register Breakpoints:");
	}
	for(typename RegisterSetMap::iterator RegSetIter = RegisterBreakpoints.begin(); RegSetIter != RegisterBreakpoints.end(); RegSetIter++)
	{
		for(typename RegisterMap::iterator RegIter = RegSetIter->second.begin(); RegIter != RegSetIter->second.end(); RegIter++)
		{
			ostrstream strBreakpoints;
			strBreakpoints << RegSetIter->first.c_str() << "." << RegIter->first.c_str() << (const char *)*RegIter->second << ends;
			SimCallBack(Info, strBreakpoints.str());
		}
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::WriteData(const RamVector &DataImage)
{
	//The physical location(s) of logical data is determined by the architecture
	return pArch->DataWrite(DataImage);
}

template<class ISA>
bool ArchSim<ISA>::WriteMemory(const string &sMemory, const RamVector &DataImage)
{
	//Check the memory name
	if(pArch->Memories.find(sMemory) == pArch->Memories.end())
	{
		sprintf(sMessageBuffer, "\"%.63s\" is not a valid memory name.", sMemory.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Assign the memory data
	return pArch->Memories.find(sMemory)->second.Write(DataImage, SimCallBack);
}

template<class ISA>
bool ArchSim<ISA>::WriteRegister(const string &sRegisterSet, const string &sRegister, Number *pNumber)
{
	if(!pNumber)
		throw "NULL parameter to WriteRegister!";

	Architecture::RegisterSetMap::iterator RegSetIter = pArch->RegisterSets.find(sRegisterSet);
	if(RegSetIter == pArch->RegisterSets.end())
	{
		sprintf(sMessageBuffer, "\"%.63s\" is not a valid register set name.", sRegisterSet.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Check the register name
	RegisterSet::RegisterMap::iterator RegIter = RegSetIter->second.Registers.find(sRegister);
	if(RegIter == RegSetIter->second.Registers.end())
	{
		sprintf(sMessageBuffer, "\"%.63s.%.63s\" is not a valid register name.", sRegisterSet.c_str(), sRegister.c_str());
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Assign the register data
	uint64 Value;
	if(RegIter->second.fFloat)
	{
		unsigned char Sign;
		unsigned short Exponent;
		uint64 Mantissa;
		if(!pNumber->Float(RegIter->second.ExponentBits, RegIter->second.MantissaBits, Sign, Exponent, Mantissa, CallBack, " for register write value", false, ISA::Addressability))
			return false;
		Value |= ((uint64)Sign << (RegIter->second.ExponentBits + RegIter->second.MantissaBits)) | ((uint64)Exponent << RegIter->second.MantissaBits) | Mantissa;
	}
	else
	{
		if(!pNumber->Int(RegIter->second.Bits, false, Value, CallBack, " for register write value", true, ISA::Addressability))
			return false;
	}
	RegIter->second = Value;

	delete pNumber;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::Interrupt(Number *pNumber)
{
	if(!pNumber)
		throw "NULL parameter to Interrupt!";

	//Get the address
	uint64 Vector;
	if(!pNumber->Int(MIN(8*sizeof(typename ISA::Word), 64 - sizeof(typename ISA::Word)), false, Vector, CallBack, " for interrupt vector", true, sizeof(typename ISA::Word), false, true, 0))
		return false;
	//We don't shift the vector over by the ISA::Word size because this value is supposed to be a "vector" number, and not an actual address

	if(!pArch->Interrupt(Vector))
		return false;

	delete pNumber;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::SubInEvent(uint64 CallAddress, uint64 SubAddress)
{
	CallAddress <<= ISA::Addressability;
	SubAddress <<= ISA::Addressability;

	//Update breakpoints
	if(fInBreakpoint || fOutBreakpoint || fOverBreakpoint)
	{
		DepthCount++;
		if(DepthCount == 0 && fInBreakpoint)
		{
			SimCallBack(Breakpoint, "Step into subroutine.");
			fBreak = true;
		}
	}

	//Update callstack
	Element *pCallElement = AddressToElement(CallAddress, false, true);
	Element *pSubElement = AddressToElement(SubAddress, false, true);
	CallStack.push_back( CallStackInfo(
		pSubElement ? InputList[pSubElement->LocationStack.rbegin()->first] : "NoFile",
		pSubElement ? pSubElement->LocationStack.rbegin()->second : 0,
		SubAddress,
		(pSubElement && pSubElement->ElementType == LabelElement ? reinterpret_cast<Label *>(pSubElement)->sLabel : "")) );

	if(fCheck && CallStack.size() >= MAX_CALLSTACK_DEPTH)
	{
		SimCallBack(Check, "Call-stack has reached maximum recommended depth. Possible inifinite recursion. Simulator may abort if it runs out of dynamic memory. Type \"go\" to continue. Type \"checkoff\" to disable this warning.");
		fBreak = true;
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::SubOutEvent()
{
	if(fInBreakpoint || fOutBreakpoint || fOverBreakpoint)
	{
		DepthCount--;
		if(DepthCount == 0)
		{
			if(fOutBreakpoint)
				SimCallBack(Breakpoint, "Step out of subroutine.");
			if(fOverBreakpoint)
				SimCallBack(Breakpoint, "Step over subroutine.");
			fBreak = true;
		}
		//SubOut means we didn't find a subroutine to step into or over.
		if(DepthCount == -2)
			fInBreakpoint = false;
		if(DepthCount == -1)
			fOverBreakpoint = false;
	}

	if(CallStack.size() == 1)
	{
		if(fCheck)
		{
			SimCallBack(Check, "Function return encountered when the call-stack is empty. Type \"go\" to continue. Type \"checkoff\" to disable this warning.");
			fBreak = true;
		}
	}
	else
		CallStack.pop_back();

	return true;
}

template<class ISA>
bool ArchSim<ISA>::InstructionEvent(uint64 Address)
{
	Element *pElement;

	SimInstruction++;
	if(SimInstruction == BreakInstruction)
	{
		SimCallBack(Breakpoint, "Instruction limit.");
		fBreak = true;
	}

	if(fTrace || fCheck)
		pElement = AddressToElement(Address, false, true);
	if(fCheck)
	{
		if( !pElement || !(pElement->ElementType == InstructionElement || pElement->ElementType == LabelElement && reinterpret_cast<Label *>(pElement)->pElement->ElementType == InstructionElement) )
		{
			SimCallBack(Check, "The next instruction is not an instruction declared in the input program. Type \"go\" to continue. Type \"checkoff\" to disable this warning.");
			fBreak = true;
		}
	}
	if(fTrace)
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%I64u;\t%I64u;\t", SimCycle+1, SimInstruction);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%llu;\t%llu;\t", SimCycle+1, SimInstruction);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		TraceFile << sMessageBuffer;
		if(!PrintInstruction(TraceFile, pElement, Address))
			return false;
		for(list<string>::iterator RegSetIter = TraceRegisterSets.begin(); RegSetIter != TraceRegisterSets.end(); RegSetIter++)
			TraceFile << ";\t" << (const char *)pArch->RegisterSets.find(*RegSetIter)->second;
		TraceFile << endl;
		if(!TraceFile.good())
		{
			SimCallBack(Error, "Error writing to trace file.");
			fBreak = true;
		}
	}

	//See if there are breakpoints for this address
	typename InstrMap::iterator InstrIter = InstrBreakpoints.find(Address);
	if(InstrIter == InstrBreakpoints.end())
		return false;

	ostrstream strBreakpoints;
	strBreakpoints << "Instruction: ";
	strBreakpoints << (const char *)*InstrIter->second.first << ":";
	if(InstrIter->second.second.first)
		strBreakpoints << " " << InputList[InstrIter->second.second.second].c_str() << "(" << InstrIter->second.second.third << ")";
	if(InstrIter->second.third == GotoEvent)
		strBreakpoints << " GOTO";
	strBreakpoints << ends;
	SimCallBack(Breakpoint, strBreakpoints.str());
	fBreak = true;

	//If this was a temp event (goto), remove the breakpoint
	if(InstrIter->second.third == GotoEvent)
	{
		delete InstrIter->second.first;
		InstrBreakpoints.erase(InstrIter);
	}

	return true;
}

template<class ISA>
bool ArchSim<ISA>::DataEvent(uint64 Address, EventEnum Events)
{
	//See if there are breakpoints for this address
	typename DataMap::iterator DataIter = DataBreakpoints.find(Address);
	if(DataIter == DataBreakpoints.end())
		return false;
	DataEnum DataType = DataIter->second.second;
	EventInfo *pEvents = DataIter->second.fourth;

	//See if there is a breakpoint for this particular event
	if(!(pEvents->Events & Events))
		return false;
	//If it's a value breakpoint, check the value
	bool fValueBreak = false;
	list<Number *>::iterator ValueIter;
	if(pEvents->Events & Events & ValueEvent)
	{
		//Get the data for comparison
		RamVector vData;
		pArch->DataRead(vData, Address, vDataBytes[DataType]);
		RealNumber Real(NullLocationStack, vExponentBits[DataType], vMantissaBits[DataType], vData, ISA::fLittleEndian);
		IntegerNumber Int(NullLocationStack, vData, ISA::fLittleEndian);

		for(ValueIter = pEvents->ValueList.begin(); ValueIter != pEvents->ValueList.end(); ValueIter++)
		{
			//DataType == STRUCT This address does not point directly to a program element,
			//so we assume the value type is a byte
			if(DataType != STRUCT && DataType >= REAL1)
			{
				unsigned char Sign1, Sign2;
				unsigned short Exponent1, Exponent2;
				uint64 Mantissa1, Mantissa2;
				(*ValueIter)->Float(vExponentBits[DataType], vMantissaBits[DataType], Sign1, Exponent1, Mantissa1, NullCallBack, " for data breakpoint value event", false, ISA::Addressability);
				Real.Float(vExponentBits[DataType], vMantissaBits[DataType], Sign2, Exponent2, Mantissa2, NullCallBack);
				if(Sign1 == Sign2 && Exponent1 == Exponent2 && Mantissa1 == Mantissa2)
					break;
			}
			else
			{
				uint64 TempInt641, TempInt642;
				(*ValueIter)->Int(8*vDataBytes[DataType], false, TempInt641, NullCallBack, " for data breakpoint value event", false, ISA::Addressability);
				Int.Int(8*vDataBytes[DataType], false, TempInt642, NullCallBack);
				if(TempInt641 == TempInt642)
					break;
			}
		}
		if(ValueIter == pEvents->ValueList.end())
		{
			if(!(pEvents->Events & Events & ~ValueEvent))
				return false;
		}
		else
			fValueBreak = true;
	}

	string sBreakpoint = "Data: ";
	(sBreakpoint += *DataIter->second.first) += ":";
	if(pEvents->Events & Events & ReadEvent)
		(sBreakpoint += " ") += sEventTypes[ReadEvent];
	if(pEvents->Events & Events & WriteEvent)
		(sBreakpoint += " ") += sEventTypes[WriteEvent];
	if(pEvents->Events & Events & ChangeEvent)
		(sBreakpoint += " ") += sEventTypes[ChangeEvent];
	if(fValueBreak)
		(((sBreakpoint += " ") += sEventTypes[ValueEvent]) += " ") += (const char *)**ValueIter;
	SimCallBack(Breakpoint, sBreakpoint);

	fBreak = true;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::MemoryEvent(const Memory &TheMem, uint64 Address, EventEnum Events)
{
	//See if there are breakpoints for this memory
	typename MemoryMap::iterator MemIter = MemoryBreakpoints.find(TheMem.sName);
	if(MemIter == MemoryBreakpoints.end())
		return false;
	//See if there are breakpoints for this address in this memory
	typename MemoryAddrMap::iterator AddressIter = MemIter->second.find(Address);
	if(AddressIter == MemIter->second.end())
		return false;
	DataEnum DataType = AddressIter->second.first;
	EventInfo *pEvents = AddressIter->second.second;

	//See if there is a breakpoint for this particular event
	if(!(pEvents->Events & Events))
		return false;
	//If it's a value breakpoint, check the value
	bool fValueBreak = false;
	list<Number *>::iterator ValueIter;
	if(pEvents->Events & Events & ValueEvent)
	{
		//Get the data for comparison
		RamVector vData;
		TheMem.Read(vData, Address, vDataBytes[DataType], SimCallBack);
		RealNumber Real(NullLocationStack, vExponentBits[DataType], vMantissaBits[DataType], vData, ISA::fLittleEndian);
		IntegerNumber Int(NullLocationStack, vData, ISA::fLittleEndian);

		for(ValueIter = pEvents->ValueList.begin(); ValueIter != pEvents->ValueList.end(); ValueIter++)
		{
			//DataType == STRUCT This address does not point directly to a program element,
			//so we assume the value type is a byte
			if(DataType != STRUCT && DataType >= REAL1)
			{
				unsigned char Sign1, Sign2;
				unsigned short Exponent1, Exponent2;
				uint64 Mantissa1, Mantissa2;
				(*ValueIter)->Float(vExponentBits[DataType], vMantissaBits[DataType], Sign1, Exponent1, Mantissa1, NullCallBack, " for memory breakpoint value event", false, ISA::Addressability);
				Real.Float(vExponentBits[DataType], vMantissaBits[DataType], Sign2, Exponent2, Mantissa2, NullCallBack);
				if(Sign1 == Sign2 && Exponent1 == Exponent2 && Mantissa1 == Mantissa2)
					break;
			}
			else
			{
				uint64 TempInt641, TempInt642;
				(*ValueIter)->Int(8*vDataBytes[DataType], false, TempInt641, NullCallBack, " for memory breakpoint value event", false, ISA::Addressability);
				Int.Int(8*vDataBytes[DataType], false, TempInt642, NullCallBack);
				if(TempInt641 == TempInt642)
					break;
			}
		}
		if(ValueIter == pEvents->ValueList.end())
		{
			if(!(pEvents->Events & Events & ~ValueEvent))
				return false;
		}
		else
			fValueBreak = true;
	}

	string sBreakpoint;
	sBreakpoint += TheMem.sName;
	#if defined _MSC_VER
		sprintf(sMessageBuffer, "[4x%I64X]", Address);
	#elif defined GPLUSPLUS
		sprintf(sMessageBuffer, "[4x%llX]", Address);
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif
	sBreakpoint += sMessageBuffer;
	if(pEvents->Events & Events & ReadEvent)
		(sBreakpoint += " ") += sEventTypes[ReadEvent];
	if(pEvents->Events & Events & WriteEvent)
		(sBreakpoint += " ") += sEventTypes[WriteEvent];
	if(pEvents->Events & Events & ChangeEvent)
		(sBreakpoint += " ") += sEventTypes[ChangeEvent];
	if(fValueBreak)
		(((sBreakpoint += " ") += sEventTypes[ValueEvent]) += " ") += (const char *)**ValueIter;
	SimCallBack(Breakpoint, sBreakpoint);

	fBreak = true;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::RegisterEvent(const string &sRegisterSet, const Register &TheReg, EventEnum Events)
{
	//See if there are breakpoints for this register set
	typename RegisterSetMap::iterator RegSetIter = RegisterBreakpoints.find(sRegisterSet);
	if(RegSetIter == RegisterBreakpoints.end())
		return false;
	//See if there are breakpoints for this register in this register set
	typename RegisterMap::iterator RegIter = RegSetIter->second.find(TheReg.sName);
	if(RegIter == RegSetIter->second.end())
		return false;
	EventInfo *pEvents = RegIter->second;

	//See if there is a breakpoint for this particular event
	if(!(pEvents->Events & Events))
		return false;
	//If it's a value breakpoint, check the value
	bool fValueBreak = false;
	list<Number *>::iterator ValueIter;
	if(pEvents->Events & Events & ValueEvent)
	{
		for(ValueIter = pEvents->ValueList.begin(); ValueIter != pEvents->ValueList.end(); ValueIter++)
		{
			if(TheReg.fFloat)
			{
				unsigned char Sign1, Sign2;
				unsigned short Exponent1, Exponent2;
				uint64 Mantissa1, Mantissa2;
				(*ValueIter)->Float(TheReg.ExponentBits, TheReg.MantissaBits, Sign1, Exponent1, Mantissa1, NullCallBack, " for register breakpoint value event", false, ISA::Addressability);
				RealNumber Real(NullLocationStack, TheReg.ExponentBits, TheReg.MantissaBits, TheReg);
				Real.Float(TheReg.ExponentBits, TheReg.MantissaBits, Sign2, Exponent2, Mantissa2, NullCallBack);
				if(Sign1 == Sign2 && Exponent1 == Exponent2 && Mantissa1 == Mantissa2)
					break;
			}
			else
			{
				uint64 TempInt641, TempInt642;
				(*ValueIter)->Int(TheReg.Bits, false, TempInt641, NullCallBack, " for register breakpoint value event", false, ISA::Addressability);
				IntegerNumber Int(NullLocationStack, TheReg, false);
				Int.Int(TheReg.Bits, false, TempInt642, NullCallBack);
				if(TempInt641 == TempInt642)
					break;
			}
		}
		if(ValueIter == pEvents->ValueList.end())
		{
			if(!(pEvents->Events & Events & ~ValueEvent))
				return false;
		}
		else
			fValueBreak = true;
	}

	string sBreakpoint;
	((sBreakpoint += sRegisterSet) += ".") += TheReg.sName;
	if(pEvents->Events & Events & ReadEvent)
		(sBreakpoint += " ") += sEventTypes[ReadEvent];
	if(pEvents->Events & Events & WriteEvent)
		(sBreakpoint += " ") += sEventTypes[WriteEvent];
	if(pEvents->Events & Events & ChangeEvent)
		(sBreakpoint += " ") += sEventTypes[ChangeEvent];
	if(fValueBreak)
		(((sBreakpoint += " ") += sEventTypes[ValueEvent]) += " ") += (const char *)**ValueIter;
	SimCallBack(Breakpoint, sBreakpoint);

	fBreak = true;
	return true;
}

template<class ISA>
bool ArchSim<ISA>::ReadConsole(string &sBuffer, unsigned int CharsToRead, unsigned int &CharsRead)
{
	return SimReadConsole(sBuffer, CharsToRead, CharsRead);
}

template<class ISA>
bool ArchSim<ISA>::WriteConsole(const string &sBuffer, unsigned int CharsToWrite, unsigned int &CharsWritten)
{
	return SimWriteConsole(sBuffer, CharsToWrite, CharsWritten);
}

template<class ISA>
bool ArchSim<ISA>::Exception()
{
	fBreak = true;
	return true;
}

template<class ISA>
ArchSim<ISA>::~ArchSim()
{
	for(typename InstrMap::iterator InstrIter = InstrBreakpoints.begin(); InstrIter != InstrBreakpoints.end(); InstrIter++)
	{
		delete InstrIter->second.first;
	}

	for(typename DataMap::iterator DataIter = DataBreakpoints.begin(); DataIter != DataBreakpoints.end(); DataIter++)
	{
		delete DataIter->second.first;
		delete DataIter->second.fourth;
	}

	for(typename MemoryMap::iterator MemIter = MemoryBreakpoints.begin(); MemIter != MemoryBreakpoints.end(); MemIter++)
	{
		for(typename MemoryAddrMap::iterator MemAddrIter = MemIter->second.begin(); MemAddrIter != MemIter->second.end(); MemAddrIter++)
			delete MemAddrIter->second.second;
	}

	for(typename RegisterSetMap::iterator RegSetIter = RegisterBreakpoints.begin(); RegSetIter != RegisterBreakpoints.end(); RegSetIter++)
	{
		for(typename RegisterMap::iterator RegIter = RegSetIter->second.begin(); RegIter != RegSetIter->second.end(); RegIter++)
			delete RegIter->second;
	}

	signal(SIGINT, OldSig);
}

}	//namespace Simulator
