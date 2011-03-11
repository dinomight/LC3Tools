// Copyright 2002 Ashley Wise
// JediMasterThrash@comcast.net

#include "Lexer.h"
#include <cmath>
#include <cstdio>

using namespace std;

namespace JMT	{

Lexer::Lexer(list<Token *> &tokenlist, CallBackFunction, bool fcase) : TokenList(tokenlist)
{
	fCase = fcase;
	this->CallBack = CallBack;
}

bool Lexer::Lex(LocationVector &LocationStack, istream &Input)
{
	//Temporary string storage
	char sTempBuff[MAX_LINE+2];
	bool fRetVal = true;

	while(!Input.eof())
	{
		sTempBuff[MAX_LINE+1] = 1;
		//get a line from the input file
		Input.get(sTempBuff, MAX_LINE+1);
		//*NOTE: MSVC's STL version of get() sets the failbit if it gets no characters.
		if(!sTempBuff[0])
			Input.clear(Input.rdstate() & ~ios::failbit);
		Input.ignore((((unsigned int)-1) >> 1), '\n');

		if(Input.bad())
		{
			CallBack(Fatal, "Error reading file.", LocationStack);
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
		if(!LexLine(LocationStack, sTempBuff))
			fRetVal = false;

		if(LocationStack != LocationVector())
			LocationStack.rbegin()->second++;
	}

	return fRetVal;
}

bool Lexer::LexLine(const LocationVector &LocationStack, const string &sinput)
{
	bool fRetVal = true;
	//Don't allow an Identifier or Number to be lexed unless they are
	//preceeded by whitespace or a valid operator.
	bool fWhiteSpace = true;
	Token *pNewToken;
	string sInput;
	fMaskErrors = false;

	if(fCase)
		sInput = sinput;
	else	//case insensitive
		//*NOTE: There is a bug in the GCC 3.0 implementation of string that results in the null terminator
		//not being added to the end of sInput when it is constructed from the return value of ToLower which
		//does included the null terminator.
		sInput = ToLower(sinput);

	for(unsigned int i = 0; i < sInput.size();)
	{
		if(LexComment(LocationStack, sInput, i))
		{
			fWhiteSpace = true;
			continue;
		}

		char CurChar = sInput[i];
		if(fWhiteSpace && LookUpIdentifierChar(LocationStack, CurChar, Beginning))
		{
			if(!LexIdentifier(LocationStack, sInput, i))
			{
				fMaskErrors = true;
				fRetVal = false;
			}
			else
				fMaskErrors = false;
			fWhiteSpace = false;
			continue;
		}

		if(fWhiteSpace && (CurChar >= '0' && CurChar <= '9') || CurChar == '-' || CurChar == '+' || CurChar == '.' )
		{
			if(!LexNumber(LocationStack, sInput, i))
			{
				fMaskErrors = true;
				fRetVal = false;
			}
			else
				fMaskErrors = false;
			fWhiteSpace = true;

			if(!fMaskErrors && ((*TokenList.rbegin())->TokenType == TInteger || (*TokenList.rbegin())->TokenType == TReal))
				fWhiteSpace = false;
			//Otherwise, LexNumber may have lexed an Operator instead
			continue;
		}

		if(LookUpOperatorChar(LocationStack, CurChar, Beginning))
		{
			if(!LexOperator(LocationStack, sInput, i))
			{
				fMaskErrors = true;
				fRetVal = false;
			}
			else
			{
				fMaskErrors = false;
			}
			fWhiteSpace = true;
			continue;
		}

		switch(CurChar)
		{
		case ' ':
		case '\t':	//whitespace
		case '\r':	//carraige return
			fMaskErrors = false;
			fWhiteSpace = true;
			i++;
			break;
		case '"':
			if(!LexString(LocationStack, sInput, i)) 
			{
				fMaskErrors = true;
				fRetVal = false;
				fWhiteSpace = false;
			}
			else
			{
				fMaskErrors = false;
				fWhiteSpace = true;
			}
			break;
		case '\'':
			if(!LexCharacter(LocationStack, sInput, i))
			{
				fMaskErrors = true;
				fRetVal = false;
				fWhiteSpace = false;
			}
			else
			{
				fMaskErrors = false;
				fWhiteSpace = true;
			}
			break;
		default:
			if(!fMaskErrors)
			{
				sprintf(sMessageBuffer, "Unexpected character: '%c' (4x%02X)", sInput[i], (unsigned int)sInput[i]);
				CallBack(Error, sMessageBuffer, LocationStack);
			}
			fMaskErrors = true;
			fRetVal = false;
			fWhiteSpace = true;
			i++;
		}
	}

	//See if a newline is an operator (ie, valid delimiter or something)
	if(pNewToken = LookUpOperator(LocationStack, "\n"))
		TokenList.push_back(pNewToken);

	return fRetVal;
}

bool Lexer::LexIdentifier(const LocationVector &LocationStack, const string &sInput, unsigned int &i)
{
	unsigned int End;
	Token *pNewToken;

	for(End = i+1; End < sInput.size(); End++)
	{
		if( !LookUpIdentifierChar(LocationStack, sInput[End], Middle) )
			break;
	}
	while(End > i)
	{
		if(LookUpIdentifierChar(LocationStack, sInput[End-1], Ending))
			break;
		End--;
	}

	//See if the identifier is a compiler keyword
	if( !(pNewToken = LookUpKeyword(LocationStack, sInput.substr(i, End - i))) )
		//It's a user-defined identifier
		pNewToken = new IDToken(LocationStack, sInput.substr(i, End - i));

	TokenList.push_back(pNewToken);

	i = End;
	return true;
}

bool Lexer::LexOperator(const LocationVector &LocationStack, const string &sInput, unsigned int &i)
{
	//Get the string of operators
	unsigned int End;
	Token *pNewToken;

	for(End = i+1; End < sInput.size(); End++)
	{
		if( !LookUpOperatorChar(LocationStack, sInput[End], Middle))
			break;
	}

	while(End > i)
	{
		if(LookUpOperatorChar(LocationStack, sInput[End-1], Ending))
			break;
		End--;
	}

	//Attempt to match the longest string of operators first. Then iteratively
	//try matching shorter strings. For instance, this will match ++ beore +
	while( End > i && !(pNewToken = LookUpOperator(LocationStack, sInput.substr(i, End-i))) )
		End--;
	if(!pNewToken)
	{
		if(!fMaskErrors)
		{
			sprintf(sMessageBuffer, "Invalid operator: '%c' (4x%02X)", sInput[i], (unsigned int)sInput[i]);
			CallBack(Error, sMessageBuffer, LocationStack);
		}
		i++;
		return false;
	}
	else
	{
		TokenList.push_back(pNewToken);
		i = End;
		return true;
	}
}

bool Lexer::LexNumber(const LocationVector &LocationStack, const string &sInput, unsigned int &i)
{
	bool fRetVal = true;

	//*NOTE: Since there are no expressions with binary + or - that will be assembled,
	//we can assume they are always the unary version.
	//If not, then + and - should be operators.
	//What would this mean? This means that opcode parameters would have to be
	//expressions instead of atoms (symbols or numbers).
	bool fNegative = false;
	switch(sInput[i])
	{
	case '+':
		i++;
		break;
	case '-':
		i++;
		fNegative = true;
		break;
	}

	//Get the base of the number
	BaseEnum Base;
	if(!LexBase(sInput, i, Base))
	{
		//The leading + or - might have been part of an operator.
		if(LookUpOperatorChar(LocationStack, sInput[--i], Beginning))
			return LexOperator(LocationStack, sInput, i);
		else 
		{
			if(!fMaskErrors)
			{
				sprintf(sMessageBuffer, "Unexpected character: '%c' (4x%02X)", sInput[i], (unsigned int)sInput[i]);
				CallBack(Error, sMessageBuffer, LocationStack);
			}
			i++;
			return false;
		}
	}

	//Get the integral part of the number
	bool fLostPrecision = false, fReal = false;
	uint64 Number, LowerNumber;
	unsigned int NumBits, LeftShifts, RightShifts = 0;
	bool fNumber = LexInteger(LocationStack, sInput, i, Base, Number, LowerNumber, NumBits, LeftShifts, fLostPrecision);

	//Get the fraction part of the number
	if(i < sInput.size() && sInput[i] == '.')
	{
		i++;
		if(!LexFraction(LocationStack, sInput, i, Base, Number, NumBits, RightShifts, fLostPrecision) && !fNumber)
		{	//The leading '.' might have been part of an operator
			i--;
			if(Base != Dec)
			{
				CallBack(Error, "Numerical base identifier missing number.", LocationStack);
				fRetVal = false;
			}
			else
				return LexOperator(LocationStack, sInput, i);
		}	
		else
			fReal = true;
	}

	//Get the exponent part of the number
	bool fLostExpPrecision = false, fExpNegative = false, fInfinity = false, fNAN = false;
	uint64 Exponent = 0;
	unsigned int NumExpBits = 0, ExpShifts = 0;
	if(i < sInput.size() && (sInput[i] == 'e' || sInput[i] == 'E' || sInput[i] == 'g' || sInput[i] == 'G'))
	{
		i++;
		if(i >= sInput.size())
		{
			CallBack(Error, "Missing exponent in real number after 'e' or 'g'.", LocationStack);
			return false;
		}
		switch(sInput[i])
		{
		case '+':
			i++;
			break;
		case '-':
			i++;
			fExpNegative = true;
			break;
		} 
		if(i >= sInput.size())
		{
			CallBack(Error, "Missing exponent in real number after 'e' or 'g'.", LocationStack);
			return false;
		}
		if(!LexInteger(LocationStack, sInput, i, Base, Exponent, LowerNumber, NumExpBits, ExpShifts, fLostExpPrecision))
		{
			if(	i+2 < sInput.size()
				&&
				(sInput[i] == 'i' || sInput[i] == 'I')
				&&
				(sInput[i+1] == 'n' || sInput[i+1] == 'N')
				&&
				(sInput[i+2] == 'f' || sInput[i+2] == 'F') )
			{
				fInfinity = true;
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
				fNAN = true;
				i += 3;
			}
			else
			{
				CallBack(Error, "Invalid exponent in real number after 'e' or 'g'.", LocationStack);
				fRetVal = false;
			}
		}
		fReal = true;
	}

	if(fReal)	//It's a real number
	{
		Real RealNum;
		double Radix, RealExp;

		if(fInfinity)
		{
			if(fExpNegative)
				RealNum.DBL = 0.;
			else if(fNegative)
				RealNum.UI64 = 0xFFF0000000000000;
			else
				RealNum.UI64 = 0x7FF0000000000000;
		}
		else if(fNAN)
		{
				RealNum.UI64 = 0x7FFFFFFFFFFFFFFF;
		}
		else
		{
			//Check Precision
			if(fLostPrecision)
				CallBack(Warning, "Real number fraction's precision truncated to 53 bits.", LocationStack);

			//Check exponent.
			if(fLostExpPrecision)
				CallBack(Warning, "Real number exponent's precision truncated to 53 bits.", LocationStack);

			switch(Base)
			{
			case Bin:
				Radix = 2;
				break;
			case Oct:
				Radix = 8;
				break;
			case Dec:
				Radix = 10;
				break;
			case Hex:
				Radix = 16;
				break;
			}

			//Calculate the exponent
			RealExp = UINT64_TO_DOUBLE(Exponent) * pow(Radix, (double)ExpShifts);
			if(fExpNegative)
				RealExp = -RealExp;
			RealExp += LeftShifts;
			RealExp -= RightShifts;

			//Calculate the real value
			RealNum.DBL = UINT64_TO_DOUBLE(Number) * pow(Radix, RealExp);

			//Check result
			if(RealNum.UI64 == 0x7FF0000000000000 || RealNum.UI64 == 0x7FF0000000000000)
				CallBack(Warning, "Magnitude of real number too large, treated as infinity.", LocationStack);
			if(RealNum.UI64 == 0 && Number)
				CallBack(Warning, "Magnitude of real number too small, treated as zero.", LocationStack);
			
			if(fNegative)
				RealNum.DBL = -RealNum.DBL;
		}

		TokenList.push_back(new RealToken(LocationStack, RealNum.DBL));
	}
	else	//It's an Integer
	{
		if(LeftShifts > 0)
			CallBack(Warning, "Integer truncated to lower 64 bits.", LocationStack);

		TokenList.push_back(new IntegerToken(LocationStack, LowerNumber, fNegative));
	}

	return fRetVal;
}

bool Lexer::LexBase(const string &sInput, unsigned int &i, BaseEnum &Base)
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
		if(i+1 < sInput.size() && (sInput[i+1] == 'x' || sInput[i+1] == 'X'))
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

bool Lexer::LexInteger(const LocationVector &LocationStack, const string &sInput, unsigned int &i, BaseEnum Base, uint64 &UpperInteger, uint64 &LowerInteger, unsigned int &NumBits, unsigned int &NumShifts, bool &fLostPrecision)
{
	uint64 UpperNumber = 0, LowerNumber = 0, PrevNumber;
	unsigned int Bits = 0, Shifts = 0, Start = i;
	bool fTooMuch = false;

	switch(Base)
	{
	case Bin:
		//Get rid of trailing 0's
		while(i < sInput.size() && sInput[i] == '0')
			i++;
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '1')
		{
			LowerNumber = (LowerNumber << 1) + (sInput[i] - '0');
			if(Bits < 63)
				UpperNumber = (UpperNumber << 1) + (sInput[i] - '0');
			else
			{
				//Stop updating UpperNumber if filled up the bits already
				if(sInput[i] != '0')
					fLostPrecision = true;
				Shifts++;
			}
			Bits++;
			i++;
		}
		break;
	case Oct:
		//Get rid of trailing 0's
		while(i < sInput.size() && sInput[i] == '0')
			i++;
		//The first digit could contribute just 1 bits
		if(i < sInput.size() && sInput[i] == '1')
			Bits = (unsigned int)-2;
		else if(i < sInput.size() && sInput[i] >= '2' && sInput[i] <= '3')
			Bits = (unsigned int)-1;
		//Finish up the rest of the digits
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '7')
		{
			LowerNumber = (LowerNumber << 3) + (sInput[i] - '0');
			if(Bits + 3 <= 64)
				UpperNumber = (UpperNumber << 3) + (sInput[i] - '0');
			//*NOTE: Should we bother making some way to keep the extra top bits from a digit?
/*			else if(Bits <= 62 && sInput[i] >= '2' && sInput[i] <= '3')
				UpperNumber = (UpperNumber << 2) + (sInput[i] - '0');
			else if(Bits <= 63 && sInput[i] == '1')
				UpperNumber = (UpperNumber << 1) + (sInput[i] - '0');
*/			else
			{
				//Stop updating UpperNumber if filled up the bits already
				if(sInput[i] != '0')
					fLostPrecision = true;
				Shifts++;
			}
			Bits += 3;
			i++;
		}
		break;
	case Dec:
		//Get rid of trailing 0's
		while(i < sInput.size() && sInput[i] == '0')
			i++;
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '9')
		{
			LowerNumber = LowerNumber * 10 + (sInput[i] - '0');
			if(!fTooMuch)
			{	//Stop updating Number if filled up the bits already
				PrevNumber = UpperNumber;
				UpperNumber = UpperNumber * 10 + (sInput[i] - '0');
				if(PrevNumber != UpperNumber / 10)
				{
					fTooMuch = true;
					UpperNumber = PrevNumber;
					fLostPrecision = true;
					Shifts++;
				}
			}
			else
				//Stop updating Number if filled up the bits already
				Shifts++;
			i++;
		}
		break;
	case Hex:
		//Get rid of trailing 0's
		while(i < sInput.size() && sInput[i] == '0')
			i++;
		while(i < sInput.size())
		{
			unsigned char TempC = sInput[i];
			if(TempC >= '0' && TempC <= '9')
				LowerNumber = (LowerNumber << 4) + (TempC - '0');
			else if(TempC >= 'a' && TempC <= 'f')
				LowerNumber = (LowerNumber << 4) + 10 + (TempC - 'a');
			else if(TempC >= 'A' && TempC <= 'F')
				LowerNumber = (LowerNumber << 4) + 10 + (TempC - 'A');
			else
				break;
			if(Bits <= 60)
			{
				if(TempC >= '0' && TempC <= '9')
					UpperNumber = (UpperNumber << 4) + (TempC - '0');
				else if(TempC >= 'a' && TempC <= 'f')
					UpperNumber = (UpperNumber << 4) + 10 + (TempC - 'a');
				else if(TempC >= 'A' && TempC <= 'F')
					UpperNumber = (UpperNumber << 4) + 10 + (TempC - 'A');
			}
			else
			{
				//Stop updating UpperNumber if filled up the bits already
				if(TempC != '0')
					fLostPrecision = true;
				Shifts++;
			}
			Bits += 4;
			i++;
		}
		break;
	}

	NumBits = Bits;
	NumShifts = Shifts;
	UpperInteger = UpperNumber;
	LowerInteger = LowerNumber;

	if(i == Start)
		return false;
	return true;
}

bool Lexer::LexFraction(const LocationVector &LocationStack, const string &sInput, unsigned int &i, BaseEnum Base, uint64 &Integer, unsigned int &NumBits, unsigned int &NumShifts, bool &fLostPrecision)
{
	uint64 Number = Integer, PrevNumber;
	unsigned int Bits = 0, Shifts = 0, Start = i;
	bool fTooMuch = false;

	switch(Base)
	{
	case Bin:
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '1')
		{
			if(Bits + NumBits < 63)
			{
				Number = (Number << 1) + (sInput[i] - '0');
				Shifts++;
			}
			else if(sInput[i] != '0')
				fLostPrecision = true;
			Bits++;
			i++;
		}
		break;
	case Oct:
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '7')
		{
			if(Bits + NumBits + 3 <= 64)
			{
				if(Bits + NumBits == 0)
				{	//A leading 1 only counts for 1 bit
					if(sInput[i] == '1')
						Bits = (unsigned int)-2;
					else if(sInput[i] >= '2' && sInput[i] <= '3')
						Bits = (unsigned int)-1;
				}
				Number = (Number << 3) + (sInput[i] - '0');
				Shifts++;
			}
			else if(sInput[i] != '0')
				fLostPrecision = true;
			Bits += 3;
			i++;
		}
		break;
	case Dec:
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '9')
		{
			if(!fTooMuch)
			{
				PrevNumber = Number;
				Number = Number * 10 + (sInput[i] - '0');
				if(PrevNumber != Number / 10)
				{
					fTooMuch = true;
					Number = PrevNumber;
					fLostPrecision = true;
				}
				else
					Shifts++;
			}
			i++;
		}
		break;
	case Hex:
		while(i < sInput.size())
		{
			unsigned char TempC = sInput[i];
			if(Bits <= 60)
			{
				if(TempC >= '0' && TempC <= '9')
					Number = (Number << 4) + (TempC - '0');
				else if(TempC >= 'a' && TempC <= 'f')
					Number = (Number << 4) + 10 + (TempC - 'a');
				else if(TempC >= 'A' && TempC <= 'F')
					Number = (Number << 4) + 10 + (TempC - 'A');
				else
					break;
				Bits += 4;
				Shifts++;
				i++;
			}
			else
			{
				if(	TempC >= '0' && TempC <= '9'
					||
					TempC >= 'a' && TempC <= 'f'
					||
					TempC >= 'A' && TempC <= 'F')
				{
					Bits += 4;
					i++;
				}
				else
					break;
				if(TempC != '0')
					fLostPrecision = true;
			}
		}
		break;
	}

	NumBits += Bits;
	NumShifts = Shifts;
	Integer = Number;

	if(i == Start)
		return false;
	return true;
}

bool Lexer::LexString(const LocationVector &LocationStack, const string &sInput, unsigned int &i)
{
	string NewString;
	unsigned char CharConst;
	bool fRetVal = true;
	i++;

	while(i < sInput.size())
	{
		switch(sInput[i])
		{
		case '\"':	//end of string
			i++;
			TokenList.push_back(new StringToken(LocationStack, NewString));
			return fRetVal;
		case '\\':
			if(!LexCharacterConstant(LocationStack, sInput, i, CharConst))
				fRetVal = false;
			NewString += CharConst;
			break;
		default:
			NewString += sInput[i];
			i++;
		}
	}

	CallBack(Error, "No matching double-quote.", LocationStack);
	return false;
}

bool Lexer::LexCharacter(const LocationVector &LocationStack, const string &sInput, unsigned int &i)
{
	if(i+1 >= sInput.size())
	{
		i++;
		CallBack(Error, "Invalid character constant.", LocationStack);
		return false;
	}

	unsigned char CharConst;

	switch(sInput[++i])
	{
	case 0:
		CallBack(Error, "No matching single-quote.", LocationStack);
		return false;
	case '\'':	//null character
		i++;
		TokenList.push_back( new CharToken(LocationStack, 0) );
		return true;
	case '\\':
		if(i+1 >= sInput.size())
		{
			i++;
			CallBack(Error, "No matching single-quote.", LocationStack);
			return false;
		}
		if(!LexCharacterConstant(LocationStack, sInput, i, CharConst))
			return false;
		TokenList.push_back( new CharToken(LocationStack, CharConst) );
		break;
	default:
		TokenList.push_back( new CharToken(LocationStack, (unsigned char)sInput[i]) );
		i++;
	}

	if(i >= sInput.size() && sInput[i] != '\'')
	{
		CallBack(Error, "No matching single-quote, or invalid escape sequence.", LocationStack);
		return false;
	}
	i++;
	return true;
}

bool Lexer::LexCharacterConstant(const LocationVector &LocationStack, const string &sInput, unsigned int &i, unsigned char &CharConst)
{
	if(i+1 >= sInput.size())
	{
		i++;
		CallBack(Error, "Invalid character escape sequence.", LocationStack);
		return false;
	}

	switch(sInput[++i])
	{
	case 'a':
		i++;
		CharConst = '\a';
		return true;
	case 'b':
		i++;
		CharConst = '\b';
		return true;
	case 'f':
		i++;
		CharConst = '\f';
		return true;
	case 'n':
		i++;
		CharConst = '\n';
		return true;
	case 'r':
		i++;
		CharConst = '\r';
		return true;
	case 't':
		i++;
		CharConst = '\t';
		return true;
	case 'v':
		i++;
		CharConst = '\v';
		return true;
	case '\'':
		i++;
		CharConst = '\'';
		return true;
	case '\"':
		i++;
		CharConst = '\"';
		return true;
	case '\\':
		i++;
		CharConst = '\\';
		return true;
	case '?':
		i++;
		CharConst = '\?';
		return true;
	}

	unsigned int Value = 0, j = 0;

	if(sInput[i] >= '0' && sInput[i] <= '7')
	{
		while(i < sInput.size() && sInput[i] >= '0' && sInput[i] <= '7')
		{	//Octal escape sequence
			Value = (Value << 3) + (unsigned char)sInput[i++] - '0';
			if(++j == 3)
				break;
		}
		if(Value > 0xff)
		{
			sprintf(sMessageBuffer, "Octal escape sequence too large: '\\%o'", Value);
			CallBack(Warning, sMessageBuffer, LocationStack);
		}
		CharConst = Value;
		return true;
	}

	if(sInput[i] == 'x')
	{
		i++;
		while(i < sInput.size())
		{	//Hex escape sequence
			unsigned char TempC = sInput[i];
			if(TempC >= '0' && TempC <= '9')
				Value = (Value << 4) + (TempC - '0');
			else if(TempC >= 'a' && TempC <= 'f')
				Value = (Value << 4) + 10 + (TempC - 'a');
			else if(TempC >= 'A' && TempC <= 'F')
				Value = (Value << 4) + 10 + (TempC - 'A');
			else
				break;
			i++;
			if(++j == 2)
				break;
		}
		CharConst = Value;
		return true;
	}

	sprintf(sMessageBuffer, "Invalid escape sequence: '\\%c'", sInput[i]);
	CallBack(Warning, sMessageBuffer, LocationStack);
	CharConst = sInput[i];
	i++;
	return true;
}

Lexer::~Lexer()
{
}

}	//namespace JMT
