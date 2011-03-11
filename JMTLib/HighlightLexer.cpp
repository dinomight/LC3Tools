//	Copyright 2003 Ashley Wise
// JediMasterThrash@comcast.net

#include "HighlightLexer.h"
#include <cmath>
#include <cstdio>

using namespace std;

namespace JMT	{

HighlightLexer::HighlightLexer(Lexer *plexer, bool fcase)
{
	fCase = fcase;
	pLexer = plexer;
}

bool HighlightLexer::Lex(const string &sinput, string &sStyle)
{
	//Don't allow an Identifier or Number to be lexed unless they are
	//preceeded by whitespace or a valid operator.
	bool fWhiteSpace = true;
	TokenEnum Lexicon;
	string sInput;

	if(fCase)
		sInput = sinput;
	else	//case insensitive
		//*NOTE: There is a bug in the GCC 3.0 implementation of string that results in the null terminator
		//not being added to the end of sInput when it is constructed from the return value of ToLower which
		//does included the null terminator.
		sInput = ToLower(sinput);

	//Initialize the type string to match the input string
	sStyle.resize(sInput.size());

	for(unsigned int i = 0; i < sInput.size();)
	{
		unsigned int Begin = i;
		if(pLexer->LexComment(LocationVector(), sInput, i))
		{
			while(Begin < i)
				sStyle[Begin++] = (char)TComment + 'A';
			fWhiteSpace = true;
			continue;
		}

		char CurChar = sInput[i];
		if(fWhiteSpace && pLexer->LookUpIdentifierChar(LocationVector(), CurChar, Lexer::Beginning))
		{
			Lexicon = LexIdentifier(sInput, i);
			while(Begin < i)
				sStyle[Begin++] = (char)Lexicon + 'A';
			fWhiteSpace = false;
			continue;
		}

		if(fWhiteSpace && (CurChar >= '0' && CurChar <= '9') || CurChar == '-' || CurChar == '+' || CurChar == '.' )
		{
			Lexicon = LexNumber(sInput, i);
			while(Begin < i)
				sStyle[Begin++] = (char)Lexicon + 'A';
			fWhiteSpace = true;

			if(Lexicon == TInteger || Lexicon == TReal)
				fWhiteSpace = false;
			//Otherwise, LexNumber may have lexed an Operator instead
			continue;
		}

		if(pLexer->LookUpOperatorChar(LocationVector(), CurChar, Lexer::Beginning))
		{
			Lexicon = LexOperator(sInput, i);
			while(Begin < i)
				sStyle[Begin++] = (char)Lexicon + 'A';
			fWhiteSpace = true;
			continue;
		}

		switch(CurChar)
		{
		case ' ':
		case '\t':	//whitespace
		case '\r':	//carraige return
		case '\n':	//newline
			fWhiteSpace = true;
			sStyle[i++] = (char)TUnknown + 'A';
			break;
		case '"':
			Lexicon = LexString(sInput, i);
			while(Begin < i)
				sStyle[Begin++] = (char)Lexicon + 'A';
			if(Lexicon == TString)
				//LexString actually parsed a string
				fWhiteSpace = true;
			else
				fWhiteSpace = false;
			break;
		case '\'':
			Lexicon = LexCharacter(sInput, i);
			while(Begin < i)
				sStyle[Begin++] = (char)Lexicon + 'A';
			if(Lexicon == TCharacter)
				//LexCharacter actually parsed a character
				fWhiteSpace = true;
			else
				fWhiteSpace = false;
			break;
		default:
			sStyle[i++] = (char)TUnknown + 'A';
			fWhiteSpace = true;
			continue;
		}
	}

	//Do something about newline?

	return true;
}

TokenEnum HighlightLexer::LexIdentifier(const string &sInput, unsigned int &i)
{
	unsigned int End;
	TokenEnum Lexicon;

	for(End = i+1; End < sInput.size(); End++)
	{
		if( !pLexer->LookUpIdentifierChar(LocationVector(), sInput[End], Lexer::Middle) )
			break;
	}
	while(End > i)
	{
		if(pLexer->LookUpIdentifierChar(LocationVector(), sInput[End-1], Lexer::Ending))
			break;
		End--;
	}

	//See if the identifier is a compiler keyword
	Lexicon = (TokenEnum)(int)pLexer->LookUpKeyword(LocationVector(), sInput.substr(i, End - i), true);
	if(Lexicon == TUnknown)
	{
		//It's a user-defined identifier
		Lexicon = TIdentifier;
	}

	i = End;
	return Lexicon;
}

TokenEnum HighlightLexer::LexOperator(const string &sInput, unsigned int &i)
{
	//Get the string of operators
	unsigned int End;
	TokenEnum Lexicon;

	for(End = i+1; End < sInput.size(); End++)
	{
		if( !pLexer->LookUpOperatorChar(LocationVector(), sInput[End], Lexer::Middle))
			break;
	}

	while(End > i)
	{
		if(pLexer->LookUpOperatorChar(LocationVector(), sInput[End-1], Lexer::Ending))
			break;
		End--;
	}

	//Attempt to match the longest string of operators first. Then iteratively
	//try matching shorter strings. For instance, this will match ++ beore +
	while( End > i && (Lexicon = (TokenEnum)(int)pLexer->LookUpOperator(LocationVector(), sInput.substr(i, End-i), true)) == TUnknown)
		End--;
	if(Lexicon == TUnknown)
		i++;
	else
		i = End;
	return Lexicon;
}

TokenEnum HighlightLexer::LexNumber(const string &sInput, unsigned int &i)
{
	bool fRetVal = true;

	//Check for sign
	if(sInput[i] == '+' || sInput[i] == '-')
		i++;

	//Get the base of the number
	BaseEnum Base;
	if(!LexBase(sInput, i, Base))
	{
		//The leading + or - might have been part of an operator.
		if(pLexer->LookUpOperatorChar(LocationVector(), sInput[--i], Lexer::Beginning))
			return LexOperator(sInput, i);
		else 
		{
			i++;
			return TUnknown;
		}
	}

	//Get the integral part of the number
	bool fReal = false;
	bool fNumber = LexInteger(sInput, i, Base);

	//Get the fraction part of the number
	if(sInput[i] == '.')
	{
		i++;
		if(!LexInteger(sInput, i, Base) && !fNumber)
		{	//The leading '.' might have been part of an operator
			i--;
			if(Base == Dec)
				return LexOperator(sInput, i);
		}	
		else
			fReal = true;
	}

	//Get the exponent part of the number
	if(sInput[i] == 'e' || sInput[i] == 'E' || sInput[i] == 'g' || sInput[i] == 'G')
	{
		i++;
		if(sInput[i] == '+' || sInput[i] == '-')
			i++;
		if(!LexInteger(sInput, i, Base))
		{
			if(	i+2 < sInput.size()
				&&
				(sInput[i] == 'i' || sInput[i] == 'I')
				&&
				(sInput[i+1] == 'n' || sInput[i+1] == 'N')
				&&
				(sInput[i+2] == 'f' || sInput[i+2] == 'F') )
			{
				i += 3;
			}
			else if(	i+2 < sInput.size()
				&&
				(sInput[i] == 'n' || sInput[i] == 'N')
				&&
				(sInput[i+1] == 'a' || sInput[i+1] == 'A')
				&&
				(sInput[i+2] == 'n' || sInput[i+2] == 'N') )
			{
				i += 3;
			}
		}
		fReal = true;
	}

	return fReal ? TReal : TInteger;
}

bool HighlightLexer::LexBase(const string &sInput, unsigned int &i, BaseEnum &Base)
{
	if(i >= sInput.size())
		return false;

	switch(sInput[i])
	{
	case '.':
	case '0':
	case '2':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		Base = Dec;
		break;
	case '1':
		if(i+1 < sInput.size() && sInput[i+1] == 'x' || sInput[i+1] == 'X')
		{
			Base = Bin;
			i += 2;
		}
		else
			Base = Dec;
		break;
	case '3':
		if(i+1 < sInput.size() && sInput[i+1] == 'x' || sInput[i+1] == 'X')
		{
			Base = Oct;
			i += 2;
		}
		else
			Base = Dec;
		break;
	case '4':
		if(i+1 < sInput.size() && sInput[i+1] == 'x' || sInput[i+1] == 'X')
		{
			Base = Hex;
			i += 2;
		}
		else
			Base = Dec;
		break;
	default:
		return false;
	}
	
	return true;
}

bool HighlightLexer::LexInteger(const string &sInput, unsigned int &i, BaseEnum Base)
{
	unsigned int Start = i;

	switch(Base)
	{
	case Bin:
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '1')
			i++;
		break;
	case Oct:
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '7')
			i++;
		break;
	case Dec:
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '9')
			i++;
		break;
	case Hex:
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '9')
			i++;
		while(i < sInput.size())
		{
			unsigned char TempC = sInput[i];
			if(TempC >= '0' && TempC <= '9' || TempC >= 'a' && TempC <= 'f' || TempC >= 'A' && TempC <= 'F')
				i++;
			else
				break;
		}
		break;
	}

	if(i == Start)
		return false;
	return true;
}

TokenEnum HighlightLexer::LexString(const string &sInput, unsigned int &i)
{
	bool fRetVal = true;
	i++;

	while(i < sInput.size())
	{
		switch(sInput[i])
		{
		case '\"':	//end of string
			i++;
			return TString;
		case '\\':
			LexCharacterConstant(sInput, i);
			break;
		default:
			i++;
		}
	}

	return TString;
}

TokenEnum HighlightLexer::LexCharacter(const string &sInput, unsigned int &i)
{
	if(i+1 >= sInput.size())
	{
		i++;
		return TCharacter;
	}

	if(sInput[++i] == '\\')
		LexCharacterConstant(sInput, i);
	else
		i++;

	if(i < sInput.size() && sInput[i] == '\'')
		i++;

	return TCharacter;
}

bool HighlightLexer::LexCharacterConstant(const string &sInput, unsigned int &i)
{
	if(i+1 >= sInput.size())
	{
		i++;
		return false;
	}

	switch(sInput[++i])
	{
	case 'a':
	case 'b':
	case 'f':
	case 'n':
	case 'r':
	case 't':
	case 'v':
	case '\'':
	case '\"':
	case '\\':
	case '?':
		i++;
		return true;
	}

	unsigned int j = 0;

	if(sInput[i] >= '0' && sInput[i] <= '7')
	{
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '7')
		{
			i++;
			if(++j == 3)
				break;
		}
		return true;
	}

	if(sInput[i] == 'x')
	{
		i++;
		while(i < sInput.size())
		{	//Hex escape sequence
			unsigned char TempC = sInput[i];
			if(TempC >= '0' && TempC <= '9' || TempC >= 'a' && TempC <= 'f' || TempC >= 'A' && TempC <= 'F')
				i++;
			else
				break;
			if(++j == 2)
				break;
		}
		return true;
	}

	return true;
}

HighlightLexer::~HighlightLexer()
{
}

}	//namespace JMT
