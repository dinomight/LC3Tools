//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "AsmConvertLC3.h"
#include <list>
#include <cstdio>
#include "../LC3Assembler/LC3ISA.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace LC3
{

bool fOldLC3 = false;

bool AsmConvertLC3(istream &Input, ostream &Output, LocationVector LocationStack, CallBackFunction)
{
	//Temporary string storage
	char sTempBuff[MAX_LINE+2];
	//Buffer for formatting error messages
	char sMessageBuffer[128];
	bool fRetVal = true;
	//Forcing this to start out as false will make it so that an identifier at the start of a line will only be converted
	//to a "label:" if a ".orig" was found in the file. Otherwise the identifier will be left alone.
	//For the moment, I've decided to leave this as true, which will make it so that all identifiers
	//at the starts of lines will be assumed to be labels. This is just to avoid some possibly confusing
	//error messages if the ".orig" is ommitted.
	fOldLC3 = false;

	while(!Input.eof())
	{
		sTempBuff[MAX_LINE+1] = 1;
		string sOutput;

		//get a line from the assembly file
		Input.get(sTempBuff, MAX_LINE+1);
		//*NOTE: MSVC's STL version of get() sets the failbit if it gets no characters.
		if(!sTempBuff[0])
			Input.clear(Input.rdstate() & ~ios::failbit);
		Input.ignore((((unsigned int)-1) >> 1), '\n');

		if(Input.bad() || Output.bad())
		{
			CallBack(Fatal, "Error reading file.", LocationStack);
			fOldLC3 = false;
			return false;
		}
		
		//Check to see if the line was too long
		if(!sTempBuff[MAX_LINE+1])
		{
			sTempBuff[MAX_LINE] = 0;
			sprintf(sMessageBuffer, "Line exceeds %u characters. Excess ignored.", MAX_LINE);
			CallBack(Warning, sMessageBuffer, LocationStack);
		}

		//Lexically scan the line
		if(!AsmConvertLC3Line(sTempBuff, sOutput, LocationStack, CallBack, fOldLC3))
			fRetVal = false;

		Output << sOutput << endl;

		LocationStack.rbegin()->second++;
	}

	fOldLC3 = false;
	return fRetVal;
}

bool AsmConvertLC3Line(const string &sinput, string &sOutput, const LocationVector &LocationStack, CallBackFunction, bool fOldLabel)
{
	string sInput = ToLower(sinput);
	//Remove extraneous newlines and carraige returns from bad unix conversions.
	sInput = sInput.substr(0, sInput.find("\x0D"));
	sInput = sInput.substr(0, sInput.find("\x0A"));
	if(sInput == "")
		//base case ends recursion
		return true;

	//Buffer for formatting error messages
	//char sMessageBuffer[128];
	char TempC;
	unsigned int i;
	string::size_type TempLoc1, TempLoc2, TempLoc3;
	bool fEscape, fInQuote, fInChar;

	//Locations in the string of things we need to change
	//are sorted in order of first to last using a map
	map<unsigned int, ChangeEnum> ChangeMap;
	map<unsigned int, ChangeEnum>::iterator ChangeIter;

	ChangeMap.insert(make_pair(sInput.find("#"), Pound));
	ChangeMap.insert(make_pair(sInput.find("x"), X));
	ChangeMap.insert(make_pair(sInput.find("b"), B));
	ChangeMap.insert(make_pair(sInput.find(".orig"), Orig));
	ChangeMap.insert(make_pair(sInput.find(".external"), External));
	ChangeMap.insert(make_pair(sInput.find(".fill"), Fill));
	ChangeMap.insert(make_pair(sInput.find(".blkw"), Blkw));
	ChangeMap.insert(make_pair(sInput.find(".stringz"), Stringz));
	ChangeMap.insert(make_pair(sInput.find(";"), Comment));
	ChangeMap.insert(make_pair(sInput.find(".end"), End));
	ChangeMap.insert(make_pair(sInput.find("getc"), TrapGetc));
	ChangeMap.insert(make_pair(sInput.find("out"), TrapOut));
	//*NOTE: Must check for longer putsp before puts
	ChangeMap.insert(make_pair(sInput.find("putsp"), TrapPutsp));
	ChangeMap.insert(make_pair(sInput.find("puts"), TrapPuts));
	ChangeMap.insert(make_pair(sInput.find("in"), TrapIn));
	ChangeMap.insert(make_pair(sInput.find("halt"), TrapHalt));
	ChangeMap.insert(make_pair(sInput.size(), EndOfLine));
	
	//Everything up to the first candidate for translation is copied to output.
	if(fOldLabel)
	{
		//If this is for sure only LC3 syntax, then the first identifier on a line could be a label
		//We need to add the colon ":" to this label.
		//Grab the first identifier in this string
		TempLoc1 = sInput.find_first_not_of(" \t");
		TempLoc2 = sInput.find_first_of("abcdefghijklmnopqrstuvwxyz_");
		if(TempLoc1 == TempLoc2 && TempLoc1 != string::npos && TempLoc1 != ChangeMap.begin()->first)
		{
			//There wasn't anything before the identifier
			TempLoc2 = MIN(sInput.size(), sInput.find_first_not_of("abcdefghijklmnopqrstuvwxyz_0123456789", TempLoc1));
			if(TempLoc2 != string::npos)
			{
				//Look for the next character after the identifier
				//If it is one of these special characters or operators, then this identifier is likely not a label
				TempLoc3 = sInput.find_first_not_of(" \t", TempLoc2);
				if(TempLoc3 == string::npos || ( sInput[TempLoc3] != '"' && sInput[TempLoc3] != '\'' && sInput[TempLoc3] != '[' && sInput[TempLoc3] != ',' && sInput[TempLoc3] != ':' && sInput[TempLoc3] != '?' && sInput[TempLoc3] != '(' && sInput[TempLoc3] != '{' && sInput[TempLoc3] != '$') )
				{
					//This is a valid identifier
					//check to make sure it's not a keyword
					string sID = sInput.substr(TempLoc1, TempLoc2 - TempLoc1);
					list<Token *> TokenList;
					LC3ISA::Lexer TheLexer(TokenList, CallBack);
					TheLexer.LexLine(LocationStack, sID);
					if(!TokenList.empty() && (*TokenList.begin())->TokenType == TIdentifier)
					{
						//get rid of the used tokens
						for(list<Token *>::iterator TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
 							delete *TokenIter;
						sOutput += sInput.substr(0, TempLoc2);
						sOutput += ":";
						return AsmConvertLC3Line(sInput.substr(TempLoc2, string::npos), sOutput, LocationStack, CallBack);
					}
					//get rid of the used tokens
					for(list<Token *>::iterator TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
 						delete *TokenIter;
				}
			}
		}
	}
	sOutput += sInput.substr(0, ChangeMap.begin()->first);

	//Determine if we are in a quote or character constant. If we are, then we don't translate
	fInQuote = false;
	fInChar = false;
	fEscape = false;
	for(i = 0; i < sOutput.size(); i++)
	{
		TempC = sOutput[i];
		switch(TempC)
		{
		case '"':
			if(fEscape)
				fEscape = false;
			else if(fInQuote)
				fInQuote = false;
			else
				fInQuote = true;
			break;
		case '\\':
			if(fEscape)
				fEscape = false;
			else if(fInQuote || fInChar)
				fEscape = true;
			break;
		case '\'':
			if(fEscape)
				fEscape = false;
			else if(fInChar)
				fInChar = false;
			else
				fInChar = true;
			break;
		default:
			fEscape = false;
		}
	}
	if(fInQuote || fInChar)
	{
		//We're in a quote or char, so don't translate this
		//Also, we need to throw everything up until the end of the current quote into the output so we don't
		//end up converting it to lowercase in the recursive call.
		bool fDone = false;
		for(i = ChangeMap.begin()->first; i < sInput.size() && !fDone; i++)
		{
			TempC = sInput[i];
			switch(TempC)
			{
			case '"':
				if(fEscape)
					fEscape = false;
				else if(fInQuote)
					fDone = true;
				else
					fInQuote = true;
				break;
			case '\\':
				if(fEscape)
					fEscape = false;
				else if(fInQuote || fInChar)
					fEscape = true;
				break;
			case '\'':
				if(fEscape)
					fEscape = false;
				else if(fInChar)
					fDone = true;
				else
					fInChar = true;
				break;
			default:
				fEscape = false;
			}
		}
		sOutput += sInput.substr(ChangeMap.begin()->first, i - ChangeMap.begin()->first);
		return AsmConvertLC3Line(sInput.substr(i, string::npos), sOutput, LocationStack, CallBack);
	}

	//Process the first candidate for translation
	switch(ChangeMap.begin()->second)
	{
	case Pound:
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+1, string::npos), sOutput, LocationStack, CallBack);
	case X:
		if(sOutput.size() > 0)
		{
			//Check to see if this 'x' is in the middle of an identifier (or Asm3 number), rather than the start of a number
			TempC = sOutput[sOutput.size()-1];
			if(TempC >= 'a' && TempC <= 'z' || TempC >= '0' && TempC <= '9' || TempC == '_')
			{
				//It's part of an identifier
				sOutput += "x";
				return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+1, string::npos), sOutput, LocationStack, CallBack);
			}
		}
		TempC = sInput[ChangeMap.begin()->first+1];
		if(TempC == '-')
		{
			//It's a negative hex number
			sOutput += "-4x";
			return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+2, string::npos), sOutput, LocationStack, CallBack);
		}
		if(TempC >= '0' && TempC <= '9' || TempC >= 'a' && TempC <= 'f')
		{
			//It's a hex digit
			sOutput += "4x";
			return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+1, string::npos), sOutput, LocationStack, CallBack);
		}
		else
		{
			//It's part of an identifier
			sOutput += "x";
			return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+1, string::npos), sOutput, LocationStack, CallBack);
		}
	case B:
		if(sOutput.size() > 0)
		{
			//Check to see if this 'b' is in the middle of an identifier (or Asm3 number), rather than the start of a number
			TempC = sOutput[sOutput.size()-1];
			if(TempC >= 'a' && TempC <= 'z' || TempC >= '0' && TempC <= '9' || TempC == '_')
			{
				//It's part of an identifier
				sOutput += "b";
				return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+1, string::npos), sOutput, LocationStack, CallBack);
			}
		}
		TempC = sInput[ChangeMap.begin()->first+1];
		if(TempC == '-')
		{
			//It's a negative binary number
			sOutput += "-1x";
			return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+2, string::npos), sOutput, LocationStack, CallBack);
		}
		if(TempC >= '0' && TempC <= '1')
		{
			//It's a hex digit
			sOutput += "1x";
			return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+1, string::npos), sOutput, LocationStack, CallBack);
		}
		else
		{
			//It's part of an identifier
			sOutput += "b";
			return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+1, string::npos), sOutput, LocationStack, CallBack);
		}
	case Orig:
		//a ".orig" is our "for sure LC3 syntax" sign.
		fOldLC3 = true;
		sOutput += "origin";
		TempLoc1 = sInput.find_first_not_of(" \t", ChangeMap.begin()->first+5);
		if(TempLoc1 == string::npos)
		{
			sOutput += "\tsegment";
			return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+5, string::npos), sOutput, LocationStack, CallBack);
		}
		TempLoc2 = TempLoc1+1;
		while(true)
		{
			if(TempLoc2 == sInput.size())
				break;
			TempC = sInput[TempLoc2];
			if( !(TempC == '-' || TempC >= '0' && TempC <= '9' || TempC >= 'a' && TempC <= 'f') )
				break;
			TempLoc2++;
		}
		//Translate the number
		sOutput += sInput.substr(ChangeMap.begin()->first+5, TempLoc1-(ChangeMap.begin()->first+5));
		AsmConvertLC3Line(sInput.substr(TempLoc1, TempLoc2-TempLoc1), sOutput, LocationStack, CallBack);
		sOutput += "\tsegment";
		return AsmConvertLC3Line(sInput.substr(MIN(TempLoc2, sInput.size()), string::npos), sOutput, LocationStack, CallBack);
	case External:
		sOutput += "extern";
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+9, string::npos), sOutput, LocationStack, CallBack);
	case Fill:
		sOutput += "data2";
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+5, string::npos), sOutput, LocationStack, CallBack);
	case Blkw:
		//This one's a bit tricky; we need to put brackets around the number which follows the .blkw
		//And we also have to translate this number
		sOutput += "data2[";
		TempLoc1 = sInput.find_first_not_of(" \t", ChangeMap.begin()->first+5);
		if(TempLoc1 == string::npos)
		{
			sOutput += "]";
			return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+5, string::npos), sOutput, LocationStack, CallBack);
		}
		TempLoc2 = TempLoc1+1;
		while(true)
		{
			if(TempLoc2 == sInput.size())
				break;
			TempC = sInput[TempLoc2];
			if( !(TempC == '-' || TempC >= '0' && TempC <= '9' || TempC >= 'a' && TempC <= 'f') )
				break;
			TempLoc2++;
		}
		//Translate the number
		AsmConvertLC3Line(sInput.substr(TempLoc1, TempLoc2-TempLoc1), sOutput, LocationStack, CallBack);
		sOutput += "]";
		sOutput += sInput.substr(ChangeMap.begin()->first+5, TempLoc1-(ChangeMap.begin()->first+5));
		sOutput += "?";
		return AsmConvertLC3Line(sInput.substr(MIN(TempLoc2, sInput.size()), string::npos), sOutput, LocationStack, CallBack);
	case Stringz:
		//This one's a bit tricky too; we need to add the null terminator to the end of the string.
		sOutput += "data2[]";
		TempLoc1 = sInput.find_first_of("\"", ChangeMap.begin()->first+8);
		if(TempLoc1 == string::npos)
		{
			return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+8, string::npos), sOutput, LocationStack, CallBack);
		}
		//loop to find closing quote taking into account a possible escaped quote
		TempLoc2 = TempLoc1+1;
		fEscape = false;
		while(true)
		{
			if(TempLoc2 == sInput.size())
				break;
			TempC = sInput[TempLoc2];
			if(fEscape)
			{
				fEscape = false;
			}
			else
			{
				if(TempC == '\\')
					fEscape = true;
				else if(TempC == '"')
					break;
			}
			TempLoc2++;
		}
		sOutput += sInput.substr(ChangeMap.begin()->first+8, TempLoc2-(ChangeMap.begin()->first+8)+1);
		sOutput += ",0";
		return AsmConvertLC3Line(sInput.substr(MIN(TempLoc2+1, sInput.size()), string::npos), sOutput, LocationStack, CallBack);
	case End:
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+4, string::npos), sOutput, LocationStack, CallBack);
	case TrapGetc:
		sOutput += (fOldLC3 && ChangeMap.begin()->first+4 != sInput.find_first_of("abcdefghijklmnopqrstuvwxyz_0123456789", ChangeMap.begin()->first+4))
			? "trap 4x20" : "getc";
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+4, string::npos), sOutput, LocationStack, CallBack);
	case TrapOut:
		sOutput += (fOldLC3 && ChangeMap.begin()->first+3 != sInput.find_first_of("abcdefghijklmnopqrstuvwxyz_0123456789", ChangeMap.begin()->first+3))
			? "trap 4x21" : "out";
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+3, string::npos), sOutput, LocationStack, CallBack);
	case TrapPuts:
		sOutput += (fOldLC3 && ChangeMap.begin()->first+4 != sInput.find_first_of("abcdefghijklmnopqrstuvwxyz_0123456789", ChangeMap.begin()->first+4))
			? "trap 4x22" : "puts";
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+4, string::npos), sOutput, LocationStack, CallBack);
	case TrapIn:
		sOutput += (fOldLC3 && ChangeMap.begin()->first+2 != sInput.find_first_of("abcdefghijklmnopqrstuvwxyz_0123456789", ChangeMap.begin()->first+2))
			? "trap 4x23" : "in";
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+2, string::npos), sOutput, LocationStack, CallBack);
	case TrapPutsp:
		sOutput += (fOldLC3 && ChangeMap.begin()->first+5 != sInput.find_first_of("abcdefghijklmnopqrstuvwxyz_0123456789", ChangeMap.begin()->first+5))
			? "trap 4x24" : "putsp";
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+5, string::npos), sOutput, LocationStack, CallBack);
	case TrapHalt:
		sOutput += (fOldLC3 && ChangeMap.begin()->first+4 != sInput.find_first_of("abcdefghijklmnopqrstuvwxyz_0123456789", ChangeMap.begin()->first+4))
			? "trap 4x25" : "halt";
		return AsmConvertLC3Line(sInput.substr(ChangeMap.begin()->first+4, string::npos), sOutput, LocationStack, CallBack);
	case Comment:
		sOutput += sInput.substr(ChangeMap.begin()->first, string::npos);
		return true;
	case EndOfLine:
		return true;
	default:
		throw "Default case in AsmConvertLC3Line!";
	}
	return true;
}

}	//namespace LC3

