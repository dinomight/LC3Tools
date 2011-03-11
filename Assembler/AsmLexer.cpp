//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "AsmLexer.h"

using namespace std;
using namespace JMT;

namespace Assembler	{

AsmLexer::AsmLexer(list<Token *> &tokenlist, CallBackFunction, bool fcase) : Lexer(tokenlist, CallBack, fcase)
{
	unsigned int i;
	for(i = 0; i < NUM_DIRECTIVES; i++)
		DirectiveTable.insert(map<string, DirectiveEnum>::value_type( (fCase ? sDirectives[i] : ToLower(sDirectives[i])), (DirectiveEnum)i));
	for(i = 0; i < NUM_ATTRIBUTES; i++)
		AttributeTable.insert(map<string, AttributeEnum>::value_type( (fCase ? sAttributes[i] : ToLower(sAttributes[i])), (AttributeEnum)i));
	for(i = 0; i < NUM_DATATYPES; i++)
		DataTable.insert(map<string, DataEnum>::value_type( (fCase ? sDataTypes[i] : ToLower(sDataTypes[i])), (DataEnum)i));
	for(i = 0; i < NUM_OPERATORS; i++)
		OperatorTable.insert(map<string, OperatorEnum>::value_type(sOperators[i], (OperatorEnum)i));
}

bool AsmLexer::LexComment(const LocationVector &LocationStack, const string &sInput, unsigned int &i)
{
	if(sInput[i] == ';')
	{
		while(i < sInput.size() && sInput[i] != '\n' && sInput[i] != '\r')
			i++;
		return true;
	}
	return false;
}

bool AsmLexer::LookUpIdentifierChar(const LocationVector &, char c, PlaceEnum Place)
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
bool AsmLexer::LookUpOperatorChar(const LocationVector &, char c, PlaceEnum Place)
{
	switch(Place)
	{
	case Beginning:
	case Middle:
	case Ending:
		if(	c == '[' || c == ']' || c == '.' || c == ',' || c == ':' || c == '?' || c == '(' || c == ')' || c == '{' ||
			c == '}' || c == '$' || c == '-' )
			return true;
		return false;
	}
	return false;
}

Token *AsmLexer::LookUpKeyword(const LocationVector &LocationStack, const string &sKeyword, bool fPeek)
{
	//See if the keyword is a directive
	map<string, DirectiveEnum>::iterator DirIter = DirectiveTable.find(sKeyword);
	if(DirIter != DirectiveTable.end())
	{
		if(fPeek)
			//If peeking, return a TokenEnum
			return (Token *)TDirective;
		return new DirToken(LocationStack, DirIter->second);
	}

	//See if the keyword is an attribute
	map<string, AttributeEnum>::iterator AttrIter = AttributeTable.find(sKeyword);
	if(AttrIter != AttributeTable.end())
	{
		if(fPeek)
			//If peeking, return a TokenEnum
			return (Token *)TAttribute;
		return new AttributeToken(LocationStack, AttrIter->second);
	}

	//See if the keyword is a data keyword
	map<string, DataEnum>::iterator DataIter = DataTable.find(sKeyword);
	if(DataIter != DataTable.end())
	{
		if(fPeek)
			//If peeking, return a TokenEnum
			return (Token *)TData;
		return new DataToken(LocationStack, DataIter->second);
	}

	//See if the identifier is an ISA keyword
	Token *pNewToken;
	if(pNewToken = LookUpInstruction(LocationStack, sKeyword, fPeek))
		return pNewToken;

	if(fPeek)
		//If peeking, return a TokenEnum
		return (Token *)TUnknown;
	return NULL;
}

Token *AsmLexer::LookUpOperator(const LocationVector &LocationStack, const string &sOperator, bool fPeek)
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

}	//namespace Assembler
