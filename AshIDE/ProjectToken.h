//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef PROJECTTOKEN_H
#define PROJECTTOKEN_H

#pragma warning (disable:4786)
#include <string>
#include "../JMTLib/Token.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;

namespace AshIDE
{
	//These are all the first level tokens.
	//*NOTE: GCC gives annoying errors for comparing tokenenums to asmtokenenums, so I've casted in the code
	const unsigned int NUM_PROJECTTOKEN_TYPES = (NUM_TOKEN_TYPES+6);
	enum ProjectTokenEnum {TOperator = NUM_TOKEN_TYPES, TKeyword};
	//These are the enumerations and strings for the keywords
	const unsigned int NUM_KEYWORDS = 32;
	enum KeywordEnum {KeyTrue = 0, KeyFalse, KeyDefineDef, KeyFileDef, KeyFileName, KeyWindow, KeyLanguage, KeyLangNone, KeyLangLC3, KeyLangLC3b, KeySourceType, KeyLC3Source, KeyLC3Header, KeyLC3bSource, KeyLC3bHeader, KeyResources, KeyDependencies, KeyTokens, KeyAST, KeySymbols, KeyImage, KeyVHDL, KeyOptimizations, KeyUseOS, KeyOldLC3, KeyDisReg, KeyExpanded, KeyNotExpanded, KeyExcludeFromBuild, KeyHelpLocation, KeyLC3OSLocation, KeyLC3bOSLocation};
	const KeywordEnum FirstLang = KeyLangNone, LastLang = KeyLangLC3b, FirstSource = KeyLC3Source, LastSource = KeyDependencies, FirstSetting = KeyTokens, FirstFileSetting = KeyExcludeFromBuild;
	extern const char *const sKeywords[NUM_KEYWORDS];
	//These are the enumerations and strings for the operators
	const unsigned int NUM_OPERATORS = 3;
	enum OperatorEnum {OpOpenBrace = 0, OpCloseBrace, OpEquals};
	extern const char *const sOperators[NUM_OPERATORS];

	//The token type for compiler keywords
	class KeyToken : public Token
	{
	public:
		KeywordEnum Keyword;
		KeyToken(const LocationVector &, KeywordEnum);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};

	//The token type for operators
	class OpToken : public Token
	{
	public:
		OperatorEnum Operator;
		OpToken(const LocationVector &, OperatorEnum);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};
}

#endif
