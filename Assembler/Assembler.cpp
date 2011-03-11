//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Assembler.h"
#include <ctype.h>
#include <cstdio>
#include <string>
#include <list>
#include "Disassembler.h"
#include "Expander.h"
#include "AsmParser.h"
#include "AsmLexer.h"
#include "AsmToken.h"

using namespace std;
using namespace JMT;

namespace Assembler	{

template<class ISA>
bool Assemble<ISA>::Compile(istream &AsmFile, Program &AsmProg, CallBackFunction)
{
	bool fRetVal = true;
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];

	sprintf(sMessageBuffer, "%.255s", AsmProg.sFileName.Full.c_str());
	CallBack(Info, sMessageBuffer, LocationVector());

	//Lexically scan the input
//	CallBack(Info, "Lexing...", AsmProg.LocationStack);
	list<Token *> TokenList;
	list<Token *>::iterator TokenIter;
	typename ISA::Lexer TheLexer(TokenList, CallBack, false);
	LocationVector LocationStack = AsmProg.LocationStack;

	//Convert command-line defines into tokens at the top of the file
	LocationStack.rbegin()->second = 0;
	for(list< pair<string, string> >::iterator DefineIter = DefineList.begin(); DefineIter != DefineList.end(); DefineIter++)
	{
		string sDefine = "DEFINE ";
		sDefine += DefineIter->first;
		sDefine += " ";
		if(DefineIter->second.find_first_of(" \t\\") != string::npos)
			((sDefine += "\"") += DefineIter->second) += "\"";
		else
			sDefine += DefineIter->second;
		if(!TheLexer.LexLine(LocationStack, sDefine))
			fRetVal = false;
	}

	LocationStack.rbegin()->second = 1;
	if(!TheLexer.Lex(LocationStack, AsmFile))
		fRetVal = false;

	//Expand macros in the TokenList 
//	CallBack(Info, "Expanding macros...", AsmProg.LocationStack);
	Expander<ISA> TheExpander(AsmProg, TokenList, CallBack);
	if(!TheExpander.Expand())
		fRetVal = false;

	//print the tokens
	if(Flags.fPrintTokens)
	{
		string sTokenFileName;
		sTokenFileName = AsmProg.sFileName.Path + AsmProg.sFileName.Bare + ".Tokens.txt";
//		sprintf(sMessageBuffer, "Printing tokens to %.255s...", sTokenFileName.c_str());
//		CallBack(Info, sMessageBuffer, AsmProg.LocationStack);
		ofstream TokenFile(CreateFileNameRelative(sWorkingDir, sTokenFileName).c_str());
		if(!TokenFile.good())
		{
			sprintf(sMessageBuffer, "Unable to create file %.255s", CreateFileNameRelative(sWorkingDir, sTokenFileName).c_str());
			CallBack(Warning, sMessageBuffer, AsmProg.LocationStack);
		}
		else
		{
			for(TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
			{
				for(LocationVector::iterator LVIter = (*TokenIter)->LocationStack.begin(); LVIter != (*TokenIter)->LocationStack.end(); LVIter++)
					TokenFile << InputList[LVIter->first].c_str() << "(" << LVIter->second << "): ";
				TokenFile << (const char *)**TokenIter << endl;
			}
		}
	}

	//Parse the TokenList 
//	CallBack(Info, "Parsing...", AsmProg.LocationStack);
	typename ISA::Parser TheParser(AsmProg, CallBack);
	if(!TheParser.Parse(TokenList.begin(), TokenList.end()))
		fRetVal = false;

	//get rid of the used tokens
	for(TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
 		delete *TokenIter;

	if(Flags.fPrintAST)
	{
		string sASTFileName;
		sASTFileName = AsmProg.sFileName.Path + AsmProg.sFileName.Bare + ".AST.asm";
//		sprintf(sMessageBuffer, "Printing AST to %.255s...", sASTFileName.c_str());
//		CallBack(Info, sMessageBuffer, AsmProg.LocationStack);
		ofstream ASTFile(CreateFileNameRelative(sWorkingDir, sASTFileName).c_str());
		if(!ASTFile.good())
		{
			sprintf(sMessageBuffer, "Unable to create file %.255s", CreateFileNameRelative(sWorkingDir, sASTFileName).c_str());
			CallBack(Warning, sMessageBuffer, AsmProg.LocationStack);
		}
		else
			ASTFile << AsmProg;
	}

	if(!fRetVal)
		//Only run the next steps if successful so far
		return false;

//	CallBack(Info, "Checking...", AsmProg.LocationStack);
	//Check structure definitions. Calculate structure sizes. Check for recursion.
	if(!AsmProg.ResolveStructDefs(CallBack))
		fRetVal = false;
	//See if there are any unresolved symbols
	if(!AsmProg.TheSymbolTable.Check(CallBack))
		fRetVal = false;

	if(!fRetVal)
		//Only run the next steps if successful so far
		return false;

	//Run the optimizer
	if(Flags.fUseOptimizations)
	{
//		CallBack(Info, "Optimizing...", AsmProg.LocationStack);
		if(!Optimize(AsmProg))
				fRetVal = false;
	}

	return fRetVal;
}

template<class ISA>
bool Assemble<ISA>::Disassemble(istream &AsmFile, Program &AsmProg, CallBackFunction)
{
	bool fRetVal = true;
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	typename Disassembler::SymbolList Symbols;

	//Lets see if there's a symbol table for this object file
	string sSymbolFile = AsmProg.sFileName.Path + AsmProg.sFileName.Bare + ".Symbols.csv";
	ifstream SymbolFile(CreateFileNameRelative(sWorkingDir, sSymbolFile).c_str());
	if(SymbolFile.good())
	{
		sprintf(sMessageBuffer, "%.255s", sSymbolFile.c_str());
		CallBack(Info, sMessageBuffer, LocationVector());

		//Load the symbol table into the program.
		//Get the ID for the new input file.
		unsigned int IncludeID;
		for(IncludeID = 0; IncludeID < InputList.size(); IncludeID++)
			if(InputList[IncludeID] == sSymbolFile)
				//The file is already in the global input list
				break;
		if(IncludeID == InputList.size())
			//The file is not yet in the global input list
			InputList.push_back(sSymbolFile);

		//Lexically scan the input
		list<Token *> TokenList;
		typename ISA::Lexer TheLexer(TokenList, CallBack, false);
		LocationVector LocationStack;
		LocationStack.push_back(make_pair(IncludeID, 1));
		if(!TheLexer.Lex(LocationStack, SymbolFile))
			fRetVal = false;
		else
			//Parse the tokens
			Disassembler::ParseSymbolTable(TokenList.begin(), TokenList.end(), Symbols, ISA::Addressability, CallBack);

		//get rid of the used tokens
		for(list<Token *>::iterator TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
 			delete *TokenIter;
	}

	//Convert the programs filename so that the extension is part of the base.
	//This way, if the symbol file, object file, ast file, etc are created they
	//won't overwrite possible existing files.
	(AsmProg.sFileName.Bare += ".") += AsmProg.sFileName.Ext;

	sprintf(sMessageBuffer, "%.255s", AsmProg.sFileName.Full.c_str());
	CallBack(Info, sMessageBuffer, LocationVector());

	//Disassemble the image
//	CallBack(Info, "Disassembling...", AsmProg.LocationStack);
	typename ISA::Disassembler TheDisassembler(AsmProg, CallBack);

	LocationVector LocationStack = AsmProg.LocationStack;
	if(!TheDisassembler.Disassemble(LocationStack, AsmFile, Symbols))
		fRetVal = false;

	if(Flags.fPrintAST)
	{
		string sASTFileName;
		sASTFileName = AsmProg.sFileName.Path + AsmProg.sFileName.Bare + ".AST.asm";
//		sprintf(sMessageBuffer, "Printing AST to %.255s...", sASTFileName.c_str());
//		CallBack(Info, sMessageBuffer, AsmProg.LocationStack);
		ofstream ASTFile(CreateFileNameRelative(sWorkingDir, sASTFileName).c_str());
		if(!ASTFile.good())
		{
			sprintf(sMessageBuffer, "Unable to create file %.255s", CreateFileNameRelative(sWorkingDir, sASTFileName).c_str());
			CallBack(Warning, sMessageBuffer, AsmProg.LocationStack);
		}
		else
			ASTFile << AsmProg;
	}

	return fRetVal;
}

template<class ISA>
bool Assemble<ISA>::Link(vector<Program *> &AsmProgs, RamVector &MemoryImage, CallBackFunction)
{
	char sMessageBuffer[65 + MAX(63, MAX_FILENAME_CHAR)];
	bool fRetVal = true;
	unsigned int i, j;

	//Resolve externs
	//*NOTE: This is O(lg*n^3). Each table * each other table * each extern symbol * tree search
	//Can it be made more efficient? The constants should be small, so it shouldn't take up much real time.
	for(i = 0; i < AsmProgs.size(); i++)
	{
		if(AsmProgs[i]->TheSymbolTable.ExternSymbols.empty())
			continue;
		AsmProgs[i]->TheSymbolTable.ClearExtern(CallBack);

//		CallBack(Info, "Resolving external references...", AsmProgs[i]->LocationStack);

		//See if the extern symbols in this program are defined in any of the other programs
		for(j = 0; j < AsmProgs.size(); j++)
		{
			if(i == j)
				continue;
			if(!AsmProgs[i]->TheSymbolTable.ResolveExternTable(AsmProgs[j]->TheSymbolTable, CallBack))
				fRetVal = false;
		}
		
		if(!AsmProgs[i]->TheSymbolTable.CheckExtern(CallBack))
			fRetVal = false;
	}

	if(!fRetVal)
		//We can't resolve addresses if there are unresolved symbols.
		return false;

	//Reorder the programs so that they are built in order of increasing origin address
	typedef pair<unsigned int, uint64> OrderInfo;
	vector<OrderInfo> Order;
	vector<OrderInfo>::iterator OrderIter;
	for(i = 0; i < AsmProgs.size(); i++)
	{
		if(AsmProgs[i]->fDynamicAddress)
			Order.push_back( OrderInfo(i, -1) );
		else
		{
			for(OrderIter = Order.begin(); OrderIter != Order.end(); OrderIter++)
				if(AsmProgs[i]->Address < OrderIter->second)
					break;
			Order.insert( OrderIter, OrderInfo(i, AsmProgs[i]->Address) );
		}
	}

	//Resolve addresses
	uint64 CurrentAddress = 0;
	for(j = 0; j < Order.size(); j++)
	{
		i = Order[j].first;
//		if(AsmProgs[i]->Structures.empty())
//			CallBack(Info, "Resolving addresses...", AsmProgs[i]->LocationStack);
//		else
//			CallBack(Info, "Resolving addresses and structures...", AsmProgs[i]->LocationStack);

		if(!AsmProgs[i]->fDynamicAddress)
		{
			//The program specified its own origin. Make sure it's valid.
			uint64 OriginAddress = AsmProgs[i]->Address;
			if(OriginAddress < CurrentAddress)
			{
				#if defined _MSC_VER
				sprintf(sMessageBuffer, "Origin address (4x%I64X) is before next unallocated location (4x%I64X).", OriginAddress >> ISA::Addressability, CurrentAddress >> ISA::Addressability);
				#elif defined GPLUSPLUS
					sprintf(sMessageBuffer, "Origin address (4x%llX) is before next unallocated location (4x%llX).", OriginAddress >> ISA::Addressability, CurrentAddress >> ISA::Addressability);
				#else
					#error "Only MSVC and GCC Compilers Supported"
				#endif
				CallBack(Error, sMessageBuffer, AsmProgs[i]->LocationStack);
				fRetVal = false;
			}
			CurrentAddress = OriginAddress;
		}

		if(!AsmProgs[i]->ResolveAddresses(CurrentAddress, CallBack))
			fRetVal = false;
	}

	if(!fRetVal)
		//We can't build if the addresses are wrong.
		return false;

	//Build the program images
	for(j = 0; j < Order.size(); j++)
	{
		i = Order[j].first;
		bool fLocalRetVal = true;

//		CallBack(Info, "Building...", AsmProgs[i]->LocationStack);
		RamVector ProgramImage;
		if(!AsmProgs[i]->GenerateImage(ProgramImage, ISA::fLittleEndian, CallBack))
			fLocalRetVal = false;
		if(ProgramImage.empty())
			CallBack(Warning, "Memory image is empty.", AsmProgs[i]->LocationStack);

		if(Flags.fPrintSymbols)
		{
			string sSymbolFileName;
			sSymbolFileName = AsmProgs[i]->sFileName.Path + AsmProgs[i]->sFileName.Bare + ".Symbols.csv";
//			sprintf(sMessageBuffer, "Printing symbols to %.255s...", sSymbolFileName.c_str());
//			CallBack(Info, sMessageBuffer, AsmProgs[i]->LocationStack);
			ofstream SymbolFile(CreateFileNameRelative(sWorkingDir, sSymbolFileName).c_str());
			if(!SymbolFile.good())
			{
				sprintf(sMessageBuffer, "Unable to create file %.255s", CreateFileNameRelative(sWorkingDir, sSymbolFileName).c_str());
				CallBack(Warning, sMessageBuffer, AsmProgs[i]->LocationStack);
			}
			else
				SymbolFile << AsmProgs[i]->TheSymbolTable;
		}
		if(fLocalRetVal && Flags.fOutputImage)
		{
			string sDumpFileName;
			sDumpFileName = AsmProgs[i]->sFileName.Path + AsmProgs[i]->sFileName.Bare + ".obj";
//			sprintf(sMessageBuffer, "Dumping image to %.255s...", sDumpFileName.c_str());
//			CallBack(Info, sMessageBuffer, AsmProgs[i]->LocationStack);
			ofstream DumpFile(CreateFileNameRelative(sWorkingDir, sDumpFileName).c_str(), ios::out | ios::binary);
			if(!DumpFile.good())
			{
				sprintf(sMessageBuffer, "Unable to create file %.255s", CreateFileNameRelative(sWorkingDir, sDumpFileName).c_str());
				CallBack(Warning, sMessageBuffer, AsmProgs[i]->LocationStack);
			}
			else
			{
				uint64 Address = AsmProgs[i]->Address, Size = AsmProgs[i]->Size;
				uint64 EndAddress = Address + Size;
				
				//OBJ file header is always little-endian
				unsigned int i;
				for(i = 0; i < sizeof(uint64); i++)
					DumpFile.put( (unsigned char)(Address >> 8 * i) );
				for(i = 0; i < sizeof(uint64); i++)
					DumpFile.put( (unsigned char)(Size  >> 8 * i) );
/*				DumpFile.put( (unsigned char)((Address & 0xFF)) );
				DumpFile.put( (unsigned char)((Address & 0xFF00) >> 8) );
				DumpFile.put( (unsigned char)((Address & 0xFF0000) >> 16) );
				DumpFile.put( (unsigned char)((Address & 0xFF000000) >> 24) );
				DumpFile.put( (unsigned char)((Address & 0xFF00000000) >> 32) );
				DumpFile.put( (unsigned char)((Address & 0xFF0000000000) >> 40) );
				DumpFile.put( (unsigned char)((Address & 0xFF000000000000) >> 48) );
				DumpFile.put( (unsigned char)((Address & 0xFF00000000000000) >> 56) );
				DumpFile.put( (unsigned char)((Size & 0xFF)) );
				DumpFile.put( (unsigned char)((Size & 0xFF00) >> 8) );
				DumpFile.put( (unsigned char)((Size & 0xFF0000) >> 16) );
				DumpFile.put( (unsigned char)((Size & 0xFF000000) >> 24) );
				DumpFile.put( (unsigned char)((Size & 0xFF00000000) >> 32) );
				DumpFile.put( (unsigned char)((Size & 0xFF0000000000) >> 40) );
				DumpFile.put( (unsigned char)((Size & 0xFF000000000000) >> 48) );
				DumpFile.put( (unsigned char)((Size & 0xFF00000000000000) >> 56) );
*/				for(RamVector::iterator ProgramIter = ProgramImage.begin(); ProgramIter != ProgramImage.end(); ProgramIter++)
				{
					for(; Address < ProgramIter->first; Address++)
						//This creates an invalid instruction, so a disassembler will always treat it as data.
						//This is also a reference to one of my favorite japanese animations ^_^
						DumpFile.put((unsigned char)UNINITIALIZED_VALUE);
					DumpFile.put(ProgramIter->second);
					Address++;
				}
				for(; Address < EndAddress; Address++)
					DumpFile.put((unsigned char)UNINITIALIZED_VALUE);
			}
		}

		MemoryImage.insert(MemoryImage.end(), ProgramImage.begin(), ProgramImage.end());
		if(!fLocalRetVal)
			fRetVal = false;
	}

	if(MemoryImage.rbegin() != MemoryImage.rend() && MemoryImage.rbegin()->first > ISA::MaxAddress)
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "Program image (4x%I64X+1 Bytes) too large to fit in memory (4x%I64X+1 Bytes).", MemoryImage.rbegin()->first, ISA::MaxAddress);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "Program image (4x%llX+1 Bytes) too large to fit in memory (4x%llX+1 Bytes).", MemoryImage.rbegin()->first, ISA::MaxAddress);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		CallBack(Fatal, sMessageBuffer, LocationVector());

		fRetVal = false;
	}

	return fRetVal;
}

template<class ISA>
bool Assemble<ISA>::VHDLWrite(ostream &OutputStream, const RamVector &MemoryImage, CallBackFunction)
{
	char sBuffer[128];

	for(RamVector::const_iterator MemoryIter = MemoryImage.begin(); MemoryIter != MemoryImage.end(); MemoryIter++)
	{
		#if defined _MSC_VER
			sprintf(sBuffer, "\tmem(%I64u) := To_stdlogicvector(X\"%02X\");\n", MemoryIter->first, MemoryIter->second);
		#elif defined GPLUSPLUS
			sprintf(sBuffer, "\tmem(%llu) := To_stdlogicvector(X\"%02X\");\n", MemoryIter->first, MemoryIter->second);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		OutputStream << sBuffer;
	}

	return true;
}

template<class ISA>
bool Assemble<ISA>::Optimize(Program &AsmProg)
{
	list<Segment *>::iterator SegmentIter;
	list<Element *>::iterator SequenceIter;
	vector<STDTYPENAME ISA::RegisterEnum> Sources;
	vector<STDTYPENAME ISA::RegisterEnum> Destinations;
	typename ISA::Instruction *pCurrentInstruction;
	bool fRetVal = true;
	unsigned int i;

	//Go over each segment
	for(SegmentIter = AsmProg.Segments.begin(); SegmentIter != AsmProg.Segments.end(); SegmentIter++)
	{
		//This is the sequence of elements for this segment
		(*SegmentIter)->Sequence;

		//Go over each element in the segment
		for(SequenceIter = (*SegmentIter)->Sequence.begin(); SequenceIter != (*SegmentIter)->Sequence.end(); SequenceIter++)
		{
			//See what the current Element is
			if( (*SequenceIter)->ElementType == InstructionElement )
			{
				pCurrentInstruction = reinterpret_cast<typename ISA::Instruction *>(*SequenceIter);

				//This is the type of instruction
				pCurrentInstruction->Opcode;

				//These are its sources
				pCurrentInstruction->GetSources(Sources);
				//loop over each source
				for(i = 0; i < Sources.size(); i++)
					Sources[i];

				//These are its destinations
				pCurrentInstruction->GetDestinations(Destinations);
				//loop over each destination
				for(i = 0; i < Destinations.size(); i++)
					Destinations[i];

				//Quick check to see if it's a branch or not
				pCurrentInstruction->IsBranch();

				//Quick check to see if it accesses memory
				pCurrentInstruction->IsMemory();
			}
			else if( (*SequenceIter)->ElementType == LabelElement )
			{
				//You can just move a label around and everything that points to
				//it will automatically be updated. This also lets you see entry
				//points for branches.
				//When you move a label, you need to update the following:
				//Label.pSeg = the segment the label resides in.
				//	(This only matters for the Seg attribute)
				//Label.pElement OR .pSegment = what the label points to.
				//A label should not point to another label, it should point to
				//the next physical element (data, instr, etc) or segment.
				//	(This only matters for the Size and Len attributes)
				
				//If the program always uses symbols and attributes when
				//referencing addresses, then the program will still work.
				//However, if the program had any hardcoded addresses, the
				//program may behave unpredictably.
			}
			//Else? You can also rearrange data for better cache use.
			//If you change the size of a structure, you will need to run
			//ResolveStructDefs() again so that other structures which use
			//this struct will also have their sizes updated.
			//If you change the size of an array, make sure the Length and Size
			//variables are updated.
		}
	}

	return fRetVal;
}

}	//namespace Assembler
