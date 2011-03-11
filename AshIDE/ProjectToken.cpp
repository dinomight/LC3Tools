//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "ProjectToken.h"
#include <cstdio>

using namespace std;
using namespace JMT;

namespace AshIDE	{

const char *const sKeywords[NUM_KEYWORDS] = {"TRUE", "FALSE", "DEFINEDEF", "FILEDEF", "FILENAME", "WINDOW", "LANGUAGE", "LANG_NONE", "LANG_LC3", "LANG_LC3B", "SOURCETYPE", "LC3_SOURCE", "LC3_HEADER", "LC3B_SOURCE", "LC3B_HEADER", "RESOURCE", "DEPENDENCY", "PRINT_TOKENS", "PRINT_AST", "PRINT_SYMBOLS", "OUTPUT_IMAGE", "OUTPUT_VHDL", "USE_OPTIMIZATIONS", "USE_OS", "OLD_LC3", "DIS_REG", "EXPANDED", "NOTEXPANDED", "EXCLUDE_FROM_BUILD", "HELP_LOCATION", "LC3_OS_LOCATION", "LC3B_OS_LOCATION"};
const char *const sOperators[NUM_OPERATORS] = {"{", "}", "="};

KeyToken::KeyToken(const LocationVector &LocationStack, KeywordEnum Key) : Token(LocationStack)
{
	Keyword = Key;
	TokenType = (TokenEnum)TKeyword;
}

Token * KeyToken::Copy() const
{
	return new KeyToken(*this);
}

KeyToken::operator const char *() const
{
	sprintf(sToken, "Keyword: %.31s", sKeywords[Keyword]);
	return sToken;
}

OpToken::OpToken(const LocationVector &LocationStack, OperatorEnum Op) : Token(LocationStack)
{
	Operator = Op;
	TokenType = (TokenEnum)TOperator;
}

Token * OpToken::Copy() const
{
	return new OpToken(*this);
}

OpToken::operator const char *() const
{
	sprintf(sToken, "Operator: '%.15s'", sOperators[Operator]);
	return sToken;
}

}	//namespace AshIDE
