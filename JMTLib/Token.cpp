// Copyright 2002 Ashley Wise
// JediMasterThrash@comcast.net

#include "Token.h"
#include <cstdio>

using namespace std;

namespace JMT	{

char Token::sToken[65 + MAX(63, MAX_IDENTIFIER_CHAR)];

Token::Token(const LocationVector &locationstack)
{
	LocationStack = locationstack;
}

Token::~Token()
{
}

CharToken::CharToken(const LocationVector &LocationStack, char C) : Token(LocationStack)
{
	Character = C;
	TokenType = TCharacter;
}

Token * CharToken::Copy() const
{
	return new CharToken(*this);
}

CharToken::operator const char *() const
{
	char *sValue;
	switch(Character)
	{
	case '\a':
		sValue = "\\a";
		break;
	case '\b':
		sValue = "\\b";
		break;
	case '\f':
		sValue = "\\f";
		break;
	case '\n':
		sValue = "\\n";
		break;
	case '\r':
		sValue = "\\r";
		break;
	case '\t':
		sValue = "\\t";
		break;
	case '\v':
		sValue = "\\v";
		break;
	case '\'':
		sValue = "\\'";
		break;
	case '\"':
		sValue = "\\\"";
		break;
	case '\\':
		sValue = "\\\\";
		break;
	case '\?':
		sValue = "\\?";
		break;
	default:
		if(Character <= 0x1F || Character >= 0x7F)
			sprintf(sToken, "Character: '\\x%X'", (unsigned char)Character);
		else
			sprintf(sToken, "Character: '%c'", Character);
		return sToken;
	}
	sprintf(sToken, "Character: '%s'", sValue);
	return sToken;
}

StringToken::StringToken(const LocationVector &LocationStack, const string &sS) : Token(LocationStack)
{
	sString = sS;
	TokenType = TString;
}

Token * StringToken::Copy() const
{
	return new StringToken(*this);
}

StringToken::operator const char *() const
{
	static string strToken;
	strToken = "String: \"";
	for(unsigned int i = 0; i < sString.size(); i++)
	{
		switch(sString[i])
		{
		case '\a':
			strToken += "\\a";
			break;
		case '\b':
			strToken += "\\b";
			break;
		case '\f':
			strToken += "\\f";
			break;
		case '\n':
			strToken += "\\n";
			break;
		case '\r':
			strToken += "\\r";
			break;
		case '\t':
			strToken += "\\t";
			break;
		case '\v':
			strToken += "\\v";
			break;
		case '\'':
			strToken += "\\'";
			break;
		case '\"':
			strToken += "\\\"";
			break;
		case '\\':
			strToken += "\\\\";
			break;
		case '\?':
			strToken += "\\?";
			break;
		default:
			if(sString[i] <= 0x1F || sString[i] >= 0x7F)
			{
				sprintf(sToken, "\\x%X", (unsigned char)sString[i]);
				strToken += sToken;
			}
			else
				strToken += sString[i];
		}
	}
	strToken += "\"";
	return strToken.c_str();
}

IDToken::IDToken(const LocationVector &LocationStack, const string &sID) : Token(LocationStack)
{
	sIdentifier = sID;
	TokenType = TIdentifier;
}

Token * IDToken::Copy() const
{
	return new IDToken(*this);
}

IDToken::operator const char *() const
{
	sprintf(sToken, "Identifier: '%.63s'", sIdentifier.c_str());
	return sToken;
}

IntegerToken::IntegerToken(const LocationVector &LocationStack, uint64 Int, bool fNeg) : Token(LocationStack)
{
	TokenType = TInteger;
	Integer = Int;
	fNegative = fNeg;
}

Token * IntegerToken::Copy() const
{
	return new IntegerToken(*this);
}

IntegerToken::operator const char *() const
{
	const char *psSign = fNegative ? "-" : "";

	#if defined _MSC_VER
		sprintf(sToken, "Integer: %.1s4x%I64X (%.1s%I64u)", psSign, Integer, psSign, Integer);
	#elif defined GPLUSPLUS
		sprintf(sToken, "Integer: %.1s4x%llX (%.1s%llu)", psSign, Integer, psSign, Integer);
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif
			
	return sToken;
}

RealToken::RealToken(const LocationVector &LocationStack, double real) : Token(LocationStack)
{
	RealVal.DBL = real;
	TokenType = TReal;
}

Token * RealToken::Copy() const
{
	return new RealToken(*this);
}

RealToken::operator const char *() const
{
	char *psN = sToken;
	uint64 RealMant = RealVal.UI64 & 0x000FFFFFFFFFFFFF;
	short RealExp = (short)((RealVal.UI64 & 0x7FF0000000000000) >> 52);
	uint64 LeftMant, RightMant;
	unsigned int LeftBits;

	psN += sprintf(psN, "Real: ");

	//*NOTE: This comparison returns true if DBL is NAN
	if(RealVal.DBL < 0 && RealVal.UI64 != 0x7FFFFFFFFFFFFFFF)
		psN += sprintf(psN, "-");

	if(RealExp == 0)
	{
		LeftMant = (RealMant >> 50);
		RightMant = (RealMant << 2) & 0x000FFFFFFFFFFFFF;
		#if defined _MSC_VER
			psN += sprintf(psN, "4x%I64X.%.13I64Xg-FF", LeftMant, RightMant);
		#elif defined GPLUSPLUS
			psN += sprintf(psN, "4x%llX.%.13llXg-FF", LeftMant, RightMant);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
	}
	else if(RealExp == 0x7FF)
	{
		if(RealMant == 0)
			psN += sprintf(psN, "4x1.0gINF");
		else
			psN += sprintf(psN, "4x1.0gNAN");
	}
	else
	{
		RealExp -= 0x3FF;
		LeftBits = RealExp % 4;
		RealMant |= 0x0010000000000000;
		LeftMant = (RealMant >> (52 - LeftBits));
		RightMant = (RealMant << LeftBits) & 0x000FFFFFFFFFFFFF;
		#if defined _MSC_VER
			if(RealExp < 0)
				psN += sprintf(psN, "4x%I64X.%.13I64Xg-%X", LeftMant, RightMant, -RealExp/4);
			else
				psN += sprintf(psN, "4x%I64X.%.13I64Xg%X", LeftMant, RightMant, RealExp/4);
		#elif defined GPLUSPLUS
			if(RealExp < 0)
				psN += sprintf(psN, "4x%llX.%.13llXg-%X", LeftMant, RightMant, -RealExp/4);
			else
				psN += sprintf(psN, "4x%llX.%.13llXg%X", LeftMant, RightMant, RealExp/4);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
	}

	sprintf(psN, " (%.16e)", RealVal.DBL);
	
	return sToken;
}

}	//namespace Assembler
