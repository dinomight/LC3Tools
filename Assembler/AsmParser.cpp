//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "AsmParser.h"
#include <cstdio>
#include "Data.h"
#include "Label.h"
#include "Symbol.h"
#include "Number.h"

using namespace std;
using namespace JMT;

namespace Assembler	{

AsmParser::AsmParser(Program &Prog, CallBackFunction) : TheProg(Prog)
{
	this->CallBack = CallBack;
	//If "Parse" is not used as the starting point, then
	//it won't warn for not having origin.
	fOrigin = true;
	fStructDef = false;
	fMaskErrors = false;
}

bool AsmParser::Parse(const list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	bool fRetVal = true;
	fOrigin = false;
	fStructDef = false;
	LabelList.clear();
	list<Token *>::iterator TokenIter = StartIter;
	fMaskErrors = false;

	while(TokenIter != EndIter)
	{
		switch((*TokenIter)->TokenType)
		{
		case TDirective:
			if(!ParseDirective(TokenIter, EndIter))
			{
				fMaskErrors = true;
				fRetVal = false;
			}
			else
				fMaskErrors = false;
			break;
		case TISA:
			if(!ParseInstruction(TokenIter, EndIter))
			{
				fMaskErrors = true;
				fRetVal = false;
			}
			else
				fMaskErrors = false;
			break;
		case TData:
			if(!ParseData(TokenIter, EndIter))
			{
				fMaskErrors = true;
				fRetVal = false;
			}
			else
				fMaskErrors = false;
			break;
		case TIdentifier:
			if(!ParseLabel(TokenIter, EndIter))
			{
				fMaskErrors = true;
				fRetVal = false;
			}
			else
				fMaskErrors = false;
			break;
		default:
			if(!fMaskErrors)
			{
				sprintf(sMessageBuffer, "Unexpected token: %.127s", (const char *)**TokenIter);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			}
			TokenIter++;
			fMaskErrors = true;
			fRetVal = false;
		}
	}

	if(fStructDef)
	{
		TokenIter = EndIter;
		TokenIter--;
		sprintf(sMessageBuffer, "Struct '%.63s' definition missing end directive.", (*TheProg.Structures.rbegin())->sStruct.c_str());
		CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
		fRetVal = false;

		if(!SaveLabelList.empty())
		{
			for(list<Label *>::iterator LabelIter = SaveLabelList.begin(); LabelIter != SaveLabelList.end(); LabelIter++)
			{
				sprintf(sMessageBuffer, "Label '%.63s' does not reference anything.", (*LabelIter)->sLabel.c_str());
				CallBack(Warning, sMessageBuffer, (*LabelIter)->LocationStack);
			}
		}
	}

	if(!LabelList.empty())
	{
		for(list<Label *>::iterator LabelIter = LabelList.begin(); LabelIter != LabelList.end(); LabelIter++)
		{
			sprintf(sMessageBuffer, "Label '%.63s' does not reference anything.", (*LabelIter)->sLabel.c_str());
			CallBack(Warning, sMessageBuffer, (*LabelIter)->LocationStack);
		}
	}

	return fRetVal;
}

bool AsmParser::ParseDirective(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	bool fRetVal = true;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is the start of the directive syntax
	list<Token *>::iterator TokenIter, OrigIter = StartIter;
	Symbol *pSymbol;
	Number *pNumber;
	Element *pElement;
	Segment *pSegment;

	switch(((DirToken *)(*StartIter))->Directive)
	{
	case DirOrigin:	//Specifies the starting address of the program
		TokenIter = StartIter++;
		if(fStructDef)
		{
			CallBack(Error, "Origin cannot be declared inside structure.", (*OrigIter)->LocationStack);
			fRetVal = false;
			break;
		}

		//Make sure origin is before segments
		if(!TheProg.Segments.empty())
		{
			CallBack(Error, "Origin must be declared before segments.", (*TokenIter)->LocationStack);
			fRetVal = false;
			break;
		}

		//Make sure only one origin
		if(!TheProg.fDynamicAddress)
		{
			#if defined _MSC_VER
				sprintf(sMessageBuffer, "Origin already declared as address 4x%I64X.", TheProg.Address >> TheProg.Addressability);
			#elif defined GPLUSPLUS
				sprintf(sMessageBuffer, "Origin already declared as address 4x%llX.", TheProg.Address >> TheProg.Addressability);
			#else
				#error "Only MSVC and GCC Compilers Supported"
			#endif
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			fRetVal = false;
			break;
		}
		else if(fOrigin)
		{
			CallBack(Error, "Origin already declared.", (*TokenIter)->LocationStack);
			CallBack(Info, "See Origin declaration.", TheProg.LocationStack);
			fRetVal = false;
			break;
		}
		fOrigin = true;

		//Get the number
		if(pNumber = ParseNumber(TokenIter, StartIter, EndIter, false, false))
		{
			pNumber->Int(64 - TheProg.Addressability, false, TheProg.Address, CallBack, " for Origin address", true);
			delete pNumber;
			TheProg.Address <<= TheProg.Addressability;
			TheProg.fDynamicAddress = false;
		}

		//Use location from ORIGIN keyword.
		TheProg.LocationStack = (*OrigIter)->LocationStack;
		break;

	case DirSegment:	//Starts a new segment
		TokenIter = StartIter++;
		if(fStructDef)
		{
			CallBack(Error, "Segment cannot be declared inside structure.", (*OrigIter)->LocationStack);
			fRetVal = false;
			break;
		}

		if(pNumber = ParseNumber(TokenIter, StartIter, EndIter, false, false))
		{	//Segment address specified
			//Use location from SEGMENT keyword
			uint64 TempInt64;
			pNumber->Int(64 - TheProg.Addressability, false, TempInt64, CallBack, " for Segment address", true);
			pSegment = new Segment( (*TokenIter)->LocationStack, TheProg.Addressability, TempInt64 << TheProg.Addressability);
			TheProg.Segments.push_back(pSegment);
			delete pNumber;
		}
		else
		{
			//Segment address not specified
			//Use location from SEGMENT keyword
			pSegment = new Segment((*OrigIter)->LocationStack, TheProg.Addressability);
			TheProg.Segments.push_back(pSegment);
		}

		if(!LabelList.empty())
		{
			for(list<Label *>::iterator LabelIter = LabelList.begin(); LabelIter != LabelList.end();)
			{
				(*LabelIter)->pSegment = pSegment;
				LabelIter = LabelList.erase(LabelIter);
			}
		}
		break;

	case DirExtern:	//Defines an external reference
		TokenIter = StartIter++;
		if(fStructDef)
		{
			CallBack(Error, "Extern cannot be declared inside structure.", (*OrigIter)->LocationStack);
			fRetVal = false;
			break;
		}

		//Get the symbol
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			CallBack(Error, "Extern directive must be followed by a symbol.", (*TokenIter)->LocationStack);
			fRetVal = false;
			break;
		}

		//Set the symbol as an extern reference
		//Use location from EXTERN keyword
		if(!TheProg.TheSymbolTable.ResolveSymbol((*OrigIter)->LocationStack, ((IDToken *)(*StartIter))->sIdentifier, CallBack, SymExtern))
			fRetVal = false;
		TokenIter = StartIter++;
		break;

	case DirStructDef:	//Defines an structure
		TokenIter = StartIter++;
		if(fStructDef)
		{
			CallBack(Error, "Nested structure definition not allowed.", (*OrigIter)->LocationStack);
			fRetVal = false;
		}

		//Get the symbol
		if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
		{
			CallBack(Error, "Extern directive must be followed by a symbol.", (*TokenIter)->LocationStack);
			fRetVal = false;
			break;
		}
		//Set the symbol as a struct definition
		//Use location from STRUCTDEF keyword
		pSymbol = TheProg.TheSymbolTable.ResolveSymbol((*OrigIter)->LocationStack, ((IDToken *)(*StartIter))->sIdentifier, CallBack, SymStruct);
		TokenIter = StartIter++;
		if(!pSymbol)
		{
			fRetVal = false;
			break;
		}

		//Use location from STRUCTDEF keyword
		pSegment = new Segment((*OrigIter)->LocationStack, TheProg.Addressability, pSymbol->sSymbol);
		TheProg.Structures.push_back(pSegment);
		pSymbol->pSegment = pSegment;

		fStructDef = true;
		//This is only a structure definition, so any previous labels can't point to it.
		//Save the label until after the definition is complete.
		SaveLabelList = LabelList;
		LabelList.clear();
		break;

	case DirEnd:	//Defines the end of the structure
		TokenIter = StartIter++;
		if(!fStructDef)
		{
			CallBack(Error, "Unmatched End directive.", (*TokenIter)->LocationStack);
			fRetVal = false;
			break;
		}
		fStructDef = false;

		//A label declared before the structure definition will point to the first thing after it.
		if(!LabelList.empty())
		{
			for(list<Label *>::iterator LabelIter = LabelList.begin(); LabelIter != LabelList.end(); LabelIter++)
			{
				sprintf(sMessageBuffer, "Label '%.63s' does not reference anything.", (*LabelIter)->sLabel.c_str());
				CallBack(Warning, sMessageBuffer, (*LabelIter)->LocationStack);
			}
		}
		LabelList = SaveLabelList;
		break;
	case DirAlign:	//Starts a new segment
		TokenIter = StartIter++;
		if(fStructDef)
		{
			CallBack(Error, "Segment cannot be declared inside structure.", (*OrigIter)->LocationStack);
			fRetVal = false;
			break;
		}

		if( !(pNumber = ParseNumber(TokenIter, StartIter, EndIter, false, false)) )
		{
			CallBack(Warning, "Align directive missing number", (*TokenIter)->LocationStack);
			fRetVal = false;
			break;
		}
		uint64 TempInt64;
		pNumber->Int(64, false, TempInt64, CallBack, " for alignment modulus", true);
		delete pNumber;

		if(fStructDef)
		{
			pSegment = *TheProg.Structures.rbegin();
		}
		else
		{
			if(!fOrigin)
				CallBack(Warning, "Program origin not specified.", (*StartIter)->LocationStack);
			fOrigin = true;
			//Make sure a segment exists to put the data in
			list<Segment *>::reverse_iterator SegmentIter = TheProg.Segments.rbegin();
			if(SegmentIter == TheProg.Segments.rend())
			{
				CallBack(Warning, "Data not in segment, creating default segment.", (*StartIter)->LocationStack);
				pSegment = new Segment((*StartIter)->LocationStack, TheProg.Addressability);
				TheProg.Segments.push_back(pSegment);
			}
			else
				pSegment = *SegmentIter;
		}
		pElement = new Align((*OrigIter)->LocationStack, TempInt64);
		pSegment->Sequence.push_back(pElement);
		break;
	}
			
	return fRetVal;
}

bool AsmParser::ParseData(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	bool fRetVal = true;
	uint64 DataSize;
	bool fSizeSpecified = false;
	vector<Number *> vData;
	Number *pNumber;
	Data *pData;
	DataEnum DataType = ((DataToken *)(*StartIter))->DataType;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is a reference to the start of the data element,
	//which is used for the element's line number
	list<Token *>::iterator TokenIter, OrigIter = StartIter;
	Segment *pSegment;

	if(DataType == STRUCT)
		return ParseStruct(StartIter, EndIter);

	if(fStructDef)
	{
		pSegment = *TheProg.Structures.rbegin();
	}
	else
	{
		if(!fOrigin)
			CallBack(Warning, "Program origin not specified.", (*StartIter)->LocationStack);
		fOrigin = true;
		//Make sure a segment exists to put the data in
		list<Segment *>::reverse_iterator SegmentIter = TheProg.Segments.rbegin();
		if(SegmentIter == TheProg.Segments.rend())
		{
			CallBack(Warning, "Data not in segment, creating default segment.", (*StartIter)->LocationStack);
			pSegment = new Segment((*StartIter)->LocationStack, TheProg.Addressability);
			TheProg.Segments.push_back(pSegment);
		}
		else
			pSegment = *SegmentIter;
	}

	TokenIter = StartIter++;
	if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpOpenBracket)
	{	//Parse a Data array
		TokenIter = StartIter++;

		//Get the array size
		if(pNumber = ParseNumber(TokenIter, StartIter, EndIter, false, false))
		{	//The size of the array is specified
			sprintf(sMessageBuffer, " for %.31s array size", sDataTypes[DataType]);
			pNumber->Int(32, false, DataSize, CallBack, sMessageBuffer, true);
			delete pNumber;
			fSizeSpecified = true;
		}

		//Look for closing bracket
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseBracket)
		{
			CallBack(Error, "Missing closing bracket in Data array declaration, or array length is not a constant.", (*TokenIter)->LocationStack);
			return false;
		}
		TokenIter = StartIter++;

		while(true)
		{
			//Get the next value (number, symbol, string, or empty data)
			if(StartIter != EndIter && (*StartIter)->TokenType == TString)
			{	//Value is a string
				string &TempString = ((StringToken *)(*StartIter))->sString;
				for(unsigned int i = 0; i < TempString.size(); i++)
					vData.push_back( new CharNumber((*StartIter)->LocationStack, TempString[i]) );
				TokenIter = StartIter++;
			}
			else if(pNumber = ParseNumber(TokenIter, StartIter, EndIter, true, true))
			{
				vData.push_back(pNumber);
			}
			else
			{	//No value
				CallBack(Error, "Data array initialization must contain value(s) (number, symbol, char, string, ?) separated by commas.", (*TokenIter)->LocationStack);
				goto CleanUp;
			}

			//Look for comma after the value
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
				break;	//Finished data list
			TokenIter = StartIter++;
		}

		if(fSizeSpecified && vData.size() > DataSize)
		{
			sprintf(sMessageBuffer, "Data (length %u) too much to fit into Data array (length %u). Truncating data length.", vData.size(), (unsigned int)DataSize);
			CallBack(Warning, sMessageBuffer, (*TokenIter)->LocationStack);
			vData.resize((unsigned int)DataSize);
		}

		//Add the data to the end of the last (most recent) segment
		pData = new Data((*OrigIter)->LocationStack, DataType, TheProg.Addressability, fSizeSpecified ? DataSize : vData.size(), vData);
		pSegment->Sequence.push_back(pData);
		if(!LabelList.empty())
		{
			for(list<Label *>::iterator LabelIter = LabelList.begin(); LabelIter != LabelList.end();)
			{
				(*LabelIter)->pElement = pData;
				LabelIter = LabelList.erase(LabelIter);
			}
		}
		return fRetVal;
	}
	//It's not an array, it's a single value
	else if( !(pNumber = ParseNumber(TokenIter, StartIter, EndIter, true, true)) )
	{
		CallBack(Error, "Data initialization must contain a value (number, symbol, char, ?).", (*TokenIter)->LocationStack);
		return false;
	}

	//Add the data to the end of the last (most recent) segment
	pData = new Data((*OrigIter)->LocationStack, DataType, TheProg.Addressability, pNumber);
	pSegment->Sequence.push_back(pData);
	if(!LabelList.empty())
	{
		for(list<Label *>::iterator LabelIter = LabelList.begin(); LabelIter != LabelList.end();)
		{
			(*LabelIter)->pElement = pData;
			LabelIter = LabelList.erase(LabelIter);
		}
	}
	return fRetVal;

CleanUp:
	for(vector<Number *>::iterator DataIter = vData.begin(); DataIter != vData.end(); DataIter++)
		delete *DataIter;

	return false;
}

bool AsmParser::ParseStruct(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	bool fRetVal = true;
	uint64 DataSize;
	DataEnum DataType = ((DataToken *)(*StartIter))->DataType;
	bool fSizeSpecified = false;
	vector<Number *> vData;
	vector< vector<Number *> > vvData;
	Number *pNumber;
	Struct *pStruct;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is a reference to the start of the data element,
	//which is used for the element's line number
	list<Token *>::iterator TokenIter, OrigIter = StartIter;
	Segment *pSegment;
	Symbol *pSymbol;

	if(fStructDef)
	{
		pSegment = *TheProg.Structures.rbegin();
	}
	else
	{
		if(!fOrigin)
			CallBack(Warning, "Program origin not specified.", (*StartIter)->LocationStack);
		fOrigin = true;
		//Make sure a segment exists to put the data in
		list<Segment *>::reverse_iterator SegmentIter = TheProg.Segments.rbegin();
		if(SegmentIter == TheProg.Segments.rend())
		{
			CallBack(Warning, "Struct not in segment, creating default segment.", (*StartIter)->LocationStack);
			pSegment = new Segment((*StartIter)->LocationStack, TheProg.Addressability);
			TheProg.Segments.push_back(pSegment);
		}
		else
			pSegment = *SegmentIter;
	}

	//Get the symbol
	TokenIter = StartIter++;
	if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
	{
		CallBack(Error, "Struct keyword must be followed by a symbol.", (*TokenIter)->LocationStack);
		return false;
	}
	pSymbol = TheProg.TheSymbolTable.ReferenceSymbol((*StartIter)->LocationStack, ((IDToken *)(*StartIter))->sIdentifier, CallBack);
	TokenIter = StartIter++;

	if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpOpenBracket)
	{	//Parse a structure array
		TokenIter = StartIter++;

		//Get the array size
		if(pNumber = ParseNumber(TokenIter, StartIter, EndIter, false, false))
		{	//The size of the array is specified
			sprintf(sMessageBuffer, " for %.31s array size", sDataTypes[DataType]);
			pNumber->Int(32, false, DataSize, CallBack, sMessageBuffer, true);
			delete pNumber;
			fSizeSpecified = true;
		}

		//Look for closing bracket
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseBracket)
		{
			CallBack(Error, "Missing closing bracket in Data array declaration, or array length is not a constant.", (*TokenIter)->LocationStack);
			return false;
		}
		TokenIter = StartIter++;

		while(true)
		{
			//Get the next structure initialization
			vData.clear();
			if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpOpenBrace)
			{	//Parse a struct initializing array
				TokenIter = StartIter++;

				while(true)
				{
					//Get the next value (number, symbol, string, or empty data)
					if(StartIter != EndIter && (*StartIter)->TokenType == TString)
					{	//Value is a string
						string &TempString = ((StringToken *)(*StartIter))->sString;
						for(unsigned int i = 0; i < TempString.size(); i++)
							vData.push_back( new CharNumber((*StartIter)->LocationStack, TempString[i]) );
						TokenIter = StartIter++;
					}
					else if(pNumber = ParseNumber(TokenIter, StartIter, EndIter, true, true))
					{
						vData.push_back(pNumber);
					}
					else
					{	//No value
						CallBack(Error, "Structure initialization in braces must contain value(s) (number, symbol, char, string, ?) separated by commas.", (*TokenIter)->LocationStack);
						goto CleanUp;
					}

					//Look for comma after the value
					if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
						break;	//Finished data list
					TokenIter = StartIter++;
				}

				//Look for closing brace
				if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseBrace)
				{
					CallBack(Error, "Missing closing brace in Data array initialization.", (*TokenIter)->LocationStack);
					goto CleanUp;
				}
				vvData.push_back(vData);
				TokenIter = StartIter++;
			}
			else if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpQuestion)
			{
				//No value specified
				vData.push_back(new VoidNumber((*StartIter)->LocationStack));
				vvData.push_back(vData);
				TokenIter = StartIter++;
			}
			else
			{	//No value
				CallBack(Error, "Structure array initialization must contain { ValueList }(s) or ?(s) separated by commas.", (*TokenIter)->LocationStack);
				goto CleanUp;
			}

			//Look for comma after the value
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
				break;	//Finished data list
			TokenIter = StartIter++;
		}

		if(fSizeSpecified && vvData.size() > DataSize)
		{
			sprintf(sMessageBuffer, "Data (length %u) too much to fit into Struct array (length %u). Truncating data length.", vvData.size(), (unsigned int)DataSize);
			CallBack(Warning, sMessageBuffer, (*TokenIter)->LocationStack);
			vvData.resize((unsigned int)DataSize);
		}

		//Add the data to the end of the last (most recent) segment
		pStruct = new Struct((*OrigIter)->LocationStack, pSymbol, fSizeSpecified ? DataSize : vvData.size(), vvData);
		pSegment->Sequence.push_back(pStruct);
		if(!LabelList.empty())
		{
			for(list<Label *>::iterator LabelIter = LabelList.begin(); LabelIter != LabelList.end();)
			{
				(*LabelIter)->pElement = pStruct;
				LabelIter = LabelList.erase(LabelIter);
			}
		}
		return fRetVal;
	}
	//It's not an array, it's a single structure
	if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpOpenBrace)
	{	//Parse a struct initializing array
		TokenIter = StartIter++;

		while(true)
		{
			//Get the next value (number, symbol, string, or empty data)
			if(StartIter != EndIter && (*StartIter)->TokenType == TString)
			{	//Value is a string
				string &TempString = ((StringToken *)(*StartIter))->sString;
				for(unsigned int i = 0; i < TempString.size(); i++)
					vData.push_back( new CharNumber((*StartIter)->LocationStack, TempString[i]) );
				TokenIter = StartIter++;
			}
			else if(pNumber = ParseNumber(TokenIter, StartIter, EndIter, true, true))
			{
				vData.push_back(pNumber);
			}
			else
			{	//No value
				CallBack(Error, "Structure initialization in braces must contain value(s) (number, symbol, char, string, ?) separated by commas.", (*TokenIter)->LocationStack);
				goto CleanUp;
			}

			//Look for comma after the value
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
				break;	//Finished data list
			TokenIter = StartIter++;
		}

		//Look for closing brace
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseBrace)
		{
			CallBack(Error, "Missing closing brace in Data array initialization.", (*TokenIter)->LocationStack);
			goto CleanUp;
		}
		TokenIter = StartIter++;
	}
	else if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpQuestion)
	{
		//No value specified
		vData.push_back(new VoidNumber((*StartIter)->LocationStack));
		TokenIter = StartIter++;
	}
	else
	{
		CallBack(Error, "Structure initialization must contain { Value-list } or ?.", (*TokenIter)->LocationStack);
		return false;
	}


	//Add the data to the end of the last (most recent) segment
	pStruct = new Struct((*OrigIter)->LocationStack, pSymbol, vData);
	pSegment->Sequence.push_back(pStruct);
	if(!LabelList.empty())
	{
		for(list<Label *>::iterator LabelIter = LabelList.begin(); LabelIter != LabelList.end();)
		{
			(*LabelIter)->pElement = pStruct;
			LabelIter = LabelList.erase(LabelIter);
		}
	}
	return fRetVal;

CleanUp:
	for(vector< vector<Number *> >::iterator vDataIter = vvData.begin(); vDataIter != vvData.end(); vDataIter++)
		for(vector<Number *>::iterator DataIter = vDataIter->begin(); DataIter != vDataIter->end(); DataIter++)
			delete *DataIter;
	for(vector<Number *>::iterator DataIter = vData.begin(); DataIter != vData.end(); DataIter++)
		delete *DataIter;

	return false;
}

Number *AsmParser::ParseNumber(list<Token *>::iterator &TokenIter, list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter, bool fSymbol, bool fVoid)
{
	Number *pNumber;

	if(StartIter != EndIter && (*StartIter)->TokenType == TInteger)
	{
		pNumber = new IntegerNumber(*((IntegerToken *)(*StartIter)));
		TokenIter = StartIter++;
		return pNumber;
	}
	else if(StartIter != EndIter && (*StartIter)->TokenType == TReal)
	{
		pNumber = new RealNumber(*((RealToken *)(*StartIter)));
		TokenIter = StartIter++;
		return pNumber;
	}
	else if(StartIter != EndIter && (*StartIter)->TokenType == TCharacter)
	{
		pNumber = new CharNumber(*((CharToken *)(*StartIter)));
		TokenIter = StartIter++;
		return pNumber;
	}
	else if(fVoid && StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpQuestion)
	{
		//No value specified
		pNumber = new VoidNumber((*StartIter)->LocationStack);
		TokenIter = StartIter++;
		return pNumber;
	}
	else if(fSymbol && StartIter != EndIter && (*StartIter)->TokenType == TIdentifier)
	{
		SymbolNumber *pNumber = new SymbolNumber( (*StartIter)->LocationStack, TheProg.TheSymbolTable.ReferenceSymbol((*StartIter)->LocationStack, ((IDToken *)(*StartIter))->sIdentifier, CallBack), TheProg.TheSymbolTable.ResolvedSymbols );
		TokenIter = StartIter++;
		bool fArray = false;

		//Check for offset modifiers
		while(true)
		{
			if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpOpenBracket)
			{	//This is an array offset
				if(fArray)
				{
					//Only 1-dimensional arrays
					//*NOTE: Should this be an error message here?
					CallBack(Error, "Multi-dimensional arrays not supported.", (*TokenIter)->LocationStack);
					delete pNumber;
					return NULL;
				}

				TokenIter = StartIter++;

				//Get the array index
				Number *pIndex = ParseNumber(TokenIter, StartIter, EndIter, true, false);
				if(!pIndex)
				{
					CallBack(Error, "Missing number in array index.", (*TokenIter)->LocationStack);
					delete pNumber;
					return NULL;
				}
				pNumber->OffsetList.push_back( SymbolNumber::OffsetInfo(true, pIndex, "") );

				//Look for closing bracket
				if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseBracket)
				{
					CallBack(Error, "Missing closing bracket in Data array declaration.", (*TokenIter)->LocationStack);
					delete pNumber;
					return NULL;
				}
				TokenIter = StartIter++;
				fArray = true;
			}
			else if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpPeriod)
			{
				TokenIter = StartIter++;

				//Get the struct member name
				if(StartIter == EndIter || (*StartIter)->TokenType != TIdentifier)
				{
					CallBack(Error, "Period must be followed by symbol for a struct member.", (*TokenIter)->LocationStack);
					delete pNumber;
					return NULL;
				}
				pNumber->OffsetList.push_back( SymbolNumber::OffsetInfo(false, 0, ((IDToken *)(*StartIter))->sIdentifier) );
				TokenIter = StartIter++;
				fArray = false;
			}
			else
				break;
		}

		//Check for attribute
		if(StartIter != EndIter && (*StartIter)->TokenType == TOperator && ((OpToken *)(*StartIter))->Operator == OpDollar)
		{
			TokenIter = StartIter++;
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TAttribute)
			{
				CallBack(Error, "Dollar sign must be followed by an attribute.", (*TokenIter)->LocationStack);
				delete pNumber;
				return NULL;
			}
			pNumber->Attribute = ((AttributeToken *)(*StartIter))->Attribute;
			pNumber->fAttribute = true;
			TokenIter = StartIter++;
		}
		return pNumber;
	}

	return NULL;
}

bool AsmParser::ParseLabel(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is the start of the label syntax
	list<Token *>::iterator TokenIter, OrigIter = StartIter;
	Segment *pSegment;

	if(fStructDef)
	{
		pSegment = *TheProg.Structures.rbegin();
	}
	else
	{
		if(!fOrigin)
			CallBack(Warning, "Program origin not specified.", (*StartIter)->LocationStack);
		fOrigin = true;
		//Make sure a segment exists to put the data in
		list<Segment *>::reverse_iterator SegmentIter = TheProg.Segments.rbegin();
		if(SegmentIter == TheProg.Segments.rend())
		{
			CallBack(Warning, "Label not in segment, creating default segment.", (*StartIter)->LocationStack);
			pSegment = new Segment((*StartIter)->LocationStack, TheProg.Addressability);
			TheProg.Segments.push_back(pSegment);
		}
		else
			pSegment = *SegmentIter;
	}

	//Get the symbol
	string sSymbol = ((IDToken *)(*StartIter))->sIdentifier;
	TokenIter = StartIter++;

	//Look for colon
	if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpColon)
	{
		sprintf(sMessageBuffer, "Misplaced symbol, or label missing colon: '%.63s'", sSymbol.c_str());
		CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
		return false;
	}
	TokenIter = StartIter++;

	Symbol *pSymbol;
	if(fStructDef)
	{
		sSymbol = ((string(pSegment->sStruct) += ".") += sSymbol);
		pSymbol = TheProg.TheSymbolTable.ResolveSymbol((*OrigIter)->LocationStack, sSymbol, CallBack, SymMember);
	}
	else
		pSymbol = TheProg.TheSymbolTable.ResolveSymbol((*OrigIter)->LocationStack, sSymbol, CallBack, SymLabel);
	if(!pSymbol)
		return false;

	//Add the label to the end of the last (most recent) segment
	Label *pLabel = new Label((*OrigIter)->LocationStack, sSymbol, pSegment);
	pSegment->Sequence.push_back(pLabel);
	//Assign the symbol to point to the label
	pSymbol->pLabel = pLabel;
	LabelList.push_back(pLabel);
	return true;
}

AsmParser::~AsmParser()
{
}

}	//namespace Assembler
