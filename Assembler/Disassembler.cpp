//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Disassembler.h"
#include <cstdio>
#include "Data.h"
#include "Label.h"
#include "Number.h"
#include <strstream>

using namespace std;
using namespace JMT;

namespace Assembler	{

Disassembler::Disassembler(Program &Prog, CallBackFunction) : TheProg(Prog)
{
	this->CallBack = CallBack;
	LabelNumber = 0;
	fRunOnce = false;
}

bool Disassembler::Disassemble(LocationVector &LocationStack, istream &InputStream, SymbolList &Symbols)
{
	bool fRetVal = true;
	unsigned int i, TempChar;
	JMT::ByteData ByteData;

	if(fRunOnce)
		throw "Disassembler called twice on the same program!";
	fRunOnce = true;

	//First read starting address.
	for(i = 0; i < sizeof(uint64); i++)
	{
		TempChar = InputStream.get();
		if(TempChar == -1)	//EOF
			break;
		//The address in the file is always little endian
#ifdef BIG_ENDIAN_BUILD
		ByteData.Bytes[sizeof(uint64) - i - 1] = TempChar;
#else
		ByteData.Bytes[i] = TempChar;
#endif
		LocationStack.rbegin()->second++;
	}
	if(i < sizeof(uint64))
	{
		CallBack(Fatal, "Unexpected end of file.", LocationStack);
		return false;
	}
	StartAddress = ByteData.UI64;

	//Then read ending address
	for(i = 0; i < sizeof(uint64); i++)
	{
		TempChar = InputStream.get();
		if(TempChar == -1)	//EOF
			break;
		//The address in the file is always little endian
#ifdef BIG_ENDIAN_BUILD
		ByteData.Bytes[sizeof(uint64) - i - 1] = TempChar;
#else
		ByteData.Bytes[i] = TempChar;
#endif
		LocationStack.rbegin()->second++;
	}
	if(i < sizeof(uint64))
	{
		CallBack(Fatal, "Unexpected end of file.", LocationStack);
		return false;
	}
	EndAddress = StartAddress + ByteData.UI64;

	//Check addresses
	if(EndAddress < StartAddress)
	{
		CallBack(Fatal, "Program end address is less than start address.", LocationStack);
		return false;
	}

	//Create program origin
	TheProg.fDynamicAddress = false;
	TheProg.Address = StartAddress;

	//Create program segment
	TheProg.Segments.push_back(new Segment(LocationStack, Addressability, 0));

	//Add the pre-existing symbols
	for(SymbolList::iterator SymbolIter = Symbols.begin(); SymbolIter != Symbols.end(); SymbolIter++)
		DisassembleSymbol(SymbolIter->first, SymbolIter->second, SymbolIter->third);

	//Try to disassemble instructions. Instructions have highest priority.
	uint64 Address = StartAddress;
	if(ToLower(TheProg.sFileName.Ext) == "obj")
	{
		if(!DisassembleInstructions(LocationStack, InputStream, Address))
			return false;
	}
	else	//"bin"
	{
		if(!DisassembleData(LocationStack, InputStream, Address))
			return false;
	}

	if(Address < EndAddress)
	{
		CallBack(Error, "Unexpected end of file.", LocationStack);
		return false;
	}

	//Create labels
	list<Element *>::iterator ElemIter = (*TheProg.Segments.begin())->Sequence.begin();
	for(LabelMap::iterator LabelIter = Labels.begin(); LabelIter != Labels.end(); LabelIter++)
	{
		//Find the element this label points to
		while(ElemIter != (*TheProg.Segments.begin())->Sequence.end() && (*ElemIter)->Address < LabelIter->first)
			ElemIter++;

		if(ElemIter == (*TheProg.Segments.begin())->Sequence.end())
		{
			ElemIter--;
			if((*ElemIter)->Address + (*ElemIter)->Size != LabelIter->first)
				throw "Disassembler: Ran out of elements while adding labels!";
			//Label points to end of program
			ElemIter++;
		}
		else if((*ElemIter)->Address != LabelIter->first)
			throw "Disassembler: Label points to inside an instruction!";

		//Add the label to the sequence
		Label *pLabel = new Label(LocationStack, LabelIter->second->sSymbol, *TheProg.Segments.begin());
		(*TheProg.Segments.begin())->Sequence.insert(ElemIter, pLabel);
		LabelIter->second->pLabel = pLabel;
		if(ElemIter != (*TheProg.Segments.begin())->Sequence.end())
		{
			if(!TheProg.TheSymbolTable.ResolveSymbol((*ElemIter)->LocationStack, LabelIter->second->sSymbol, CallBack, SymLabel))
				throw "ResolveSymbol in Disassembler failed!";
			pLabel->pElement = *ElemIter;
		}
		else
			if(!TheProg.TheSymbolTable.ResolveSymbol((*(*TheProg.Segments.begin())->Sequence.rbegin())->LocationStack, LabelIter->second->sSymbol, CallBack, SymLabel))
				throw "ResolveSymbol in Disassembler failed!";
	}

	return fRetVal;
}

bool Disassembler::DisassembleData(LocationVector &LocationStack, istream &InputStream, uint64 &Address)
{
	bool fRetVal = true;
	unsigned int TempChar;
	Data *pData;
	while(InputStream.good() && Address < EndAddress)
	{
		//Get a byte of data from the stream.
		TempChar = InputStream.get();
		if(TempChar == -1)	//EOF
			break;

		//Add the byte to the sequence
		if((*TheProg.Segments.rbegin())->Sequence.empty())
		{
			//Create a new byte element.
			pData = new Data(LocationStack, DATA1, Addressability, new IntegerNumber(LocationStack, TempChar));
			(*TheProg.Segments.rbegin())->Sequence.push_back(pData);
			pData->Address = Address;
		}
		else
		{
			//Add the byte to an existing array.
			pData = reinterpret_cast<Data *>(*(*TheProg.Segments.rbegin())->Sequence.rbegin());
			pData->vData.push_back(new IntegerNumber(LocationStack, TempChar));
			pData->Length++;
			pData->Size++;
		}

		Address++;
		LocationStack.rbegin()->second++;
	}
	return fRetVal;
}

Symbol *Disassembler::DisassembleSymbol(const LocationVector &LocationStack, uint64 Address, const string &sSymbol)
{
	if(Address < StartAddress || Address > EndAddress)
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "Target address 4x%I64X outside program range, no label created.", Address);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "Target address 4x%llX outside program range, no label created.", Address);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		//*NOTE: Do we want this warning?
		//*NOTE: Also, need to add addressability to this
//		CallBack(Warning, sMessageBuffer, LocationStack);
		return NULL;
	}

	LabelMap::iterator LabelIter = Labels.find(Address);
	if(LabelIter == Labels.end())
	{
		//This is a newly referenced address
		string sLabel;
		if(sSymbol == "")
		{
			//Create the label name
			char sTemp[16];
			sprintf(sTemp, "__label%u", LabelNumber++);
			sLabel = sTemp;
		}
		else
			sLabel = sSymbol;
		Symbol *pSymbol = TheProg.TheSymbolTable.ReferenceSymbol(LocationStack, sLabel, CallBack);
		Labels.insert(LabelMap::value_type(Address, pSymbol));
		return pSymbol;
	}
	else
	{
		//This address has been referenced before
		return LabelIter->second;
	}
}

bool Disassembler::ParseSymbolTable(const list<Token *>::iterator &startiter, const list<Token *>::iterator &EndIter, SymbolList &Symbols, unsigned char Addressability, CallBackFunction)
{
	list<Token *>::iterator TokenIter, StartIter;
	TokenIter = StartIter = startiter;

	//process one symbol at a time
	while(StartIter != EndIter)
	{
		//Get the symbol name
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			CallBack(Error, "Missing symbol name: first cell in symbol table entry.", (*TokenIter)->LocationStack);
			return false;
		}
		string sSymbol = ((IDToken *)(*StartIter))->sIdentifier;
		TokenIter = StartIter++;

		//Check for possible "." which could be for a struct member.
		if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpPeriod)
		{
			TokenIter = StartIter++;
			//Get past the member label
			if(StartIter != EndIter && (*StartIter)->TokenType == TIdentifier)
			{
				TokenIter = StartIter++;
			}
			else
			{
				CallBack(Error, "Missing member symbol name after struct symbol name and period: first cell in symbol table entry.", (*TokenIter)->LocationStack);
				return false;
			}
		}


		//Get the comma
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
		{
			CallBack(Error, "Missing comma after symbol name: first cell in symbol table entry.", (*TokenIter)->LocationStack);
			return false;
		}
		TokenIter = StartIter++;

		//Get the symbol type
		SymbolEnum SymbolType = SymVoid;
		if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TDirective || ((DirToken *)(*StartIter))->Directive != DirMacro)
		{
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TDirective || ((DirToken *)(*StartIter))->Directive != DirExtern)
			{
				if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TDirective || ((DirToken *)(*StartIter))->Directive != DirDefine)
				{
					if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TData || ((DataToken *)(*StartIter))->DataType != STRUCT)
					{
						if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier || ((IDToken *)(*StartIter))->sIdentifier != "member")
						{
							if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier || ((IDToken *)(*StartIter))->sIdentifier != "label")
							{
								CallBack(Error, "Missing symbol type after symbol name: second cell in symbol table entry.", (*TokenIter)->LocationStack);
								return false;
							}
							else
								SymbolType = SymLabel;
						}
						else
							SymbolType = SymMember;
					}
					else
						SymbolType = SymStruct;
				}
				else
					SymbolType = SymDefine;
			}
			else
				SymbolType = SymExtern;
		}
		else
			SymbolType = SymMacro;
		TokenIter = StartIter++;

		uint64 Address;
		switch(SymbolType)
		{
		case SymMacro:
		case SymDefine:
		case SymStruct:
			//There's nothing else for this entry.
			break;

		case SymMember:
			//Get the comma
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			{
				CallBack(Error, "Missing comma after symbol type: second cell in symbol table entry.", (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the number
			if(StartIter == EndIter || (*StartIter)->TokenType != TInteger)
			{
				CallBack(Error, "Missing struct member symbol offset value: third cell in symbol table entry.", (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;
			
			break;

		case SymExtern:
		case SymLabel:
			//Get the comma
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			{
				CallBack(Error, "Missing comma after symbol type: second cell in symbol table entry.", (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the number
			if(StartIter == EndIter || (*StartIter)->TokenType != TInteger)
			{
				CallBack(Error, "Missing symbol address: third cell in symbol table entry.", (*TokenIter)->LocationStack);
				return false;
			}
			Address = ((IntegerToken *)(*StartIter))->Integer;
			TokenIter = StartIter++;

			//Get the comma
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			{
				CallBack(Error, "Missing comma after symbol address: third cell in symbol table entry.", (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the number
			if(StartIter == EndIter || (*StartIter)->TokenType != TInteger)
			{
				CallBack(Error, "Missing symbol segment-relative adddress: fourth cell in symbol table entry.", (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			if(SymbolType == SymLabel)
				Symbols.push_back( make_triple((*TokenIter)->LocationStack, Address << Addressability, sSymbol) );

			break;
		}

		//depending on if they saved after viewing in excel, there could be a number of commas at the end.
		while(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpComma)
			TokenIter = StartIter++;
	}

	return true;
}

Disassembler::~Disassembler()
{
}

}	//namespace Assembler
