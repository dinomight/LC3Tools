//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "ProjectLexer.h"

using namespace std;
using namespace JMT;

namespace AshIDE	{

ProjectLexer::ProjectLexer(list<Token *> &tokenlist, CallBackFunction, bool fcase) : Lexer(tokenlist, CallBack, fcase)
{
	unsigned int i;
	for(i = 0; i < NUM_KEYWORDS; i++)
		KeywordTable.insert(map<string, KeywordEnum>::value_type( (fCase ? sKeywords[i] : ToLower(sKeywords[i])), (KeywordEnum)i));
	for(i = 0; i < NUM_OPERATORS; i++)
		OperatorTable.insert(map<string, OperatorEnum>::value_type(sOperators[i], (OperatorEnum)i));
}

bool ProjectLexer::LexComment(const LocationVector &LocationStack, const string &sInput, unsigned int &i)
{
	if(sInput[i] == '#')
	{
		while(i < sInput.size() && sInput[i] != '\n' && sInput[i] != '\r')
			i++;
		return true;
	}
	return false;
}

bool ProjectLexer::LookUpIdentifierChar(const LocationVector &, char c, PlaceEnum Place)
{
	switch(Place)
	{
	case Beginning:
		if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
			return true;
		return false;
	case Middle:
	case Ending:
		if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
			return true;
		return false;
	}
	return false;
}
bool ProjectLexer::LookUpOperatorChar(const LocationVector &, char c, PlaceEnum Place)
{
	switch(Place)
	{
	case Beginning:
	case Middle:
	case Ending:
		if( c == '{' || c == '}' || c == '=' )
			return true;
		return false;
	}
	return false;
}

Token *ProjectLexer::LookUpKeyword(const LocationVector &LocationStack, const string &sKeyword, bool fPeek)
{
	//See if the keyword is a keyword
	map<string, KeywordEnum>::iterator KeyIter = KeywordTable.find(sKeyword);
	if(KeyIter != KeywordTable.end())
	{
		if(fPeek)
			//If peeking, return a TokenEnum
			return (Token *)TKeyword;
		return new KeyToken(LocationStack, KeyIter->second);
	}

	if(fPeek)
		//If peeking, return a TokenEnum
		return (Token *)TUnknown;
	return NULL;
}

Token *ProjectLexer::LookUpOperator(const LocationVector &LocationStack, const string &sOperator, bool fPeek)
{
	map<string, OperatorEnum>::iterator OpIter = OperatorTable.find(sOperator);
	if(OpIter == OperatorTable.end())
	{
		if(fPeek)
			//If peeking, return a TokenEnum
			return (Token *)TUnknown;
		return NULL;
	}

	if(fPeek)
		//If peeking, return a TokenEnum
		return (Token *)TOperator;
	return new OpToken(LocationStack, OpIter->second);
}

}	//namespace AshIDE
