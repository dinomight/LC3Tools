//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef ASMLEXER_H
#define ASMLEXER_H

#pragma warning (disable:4786)
#include <map>
#include <string>
#include <list>
#include <iostream>
#include "../JMTLib/Lexer.h"
#include "AsmToken.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		AsmLexer

		A version of Lexer with specifics for an assembly file
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class AsmLexer : public Lexer
	{
	protected:
		//This is a look-up table to see if a given string is a compiler keyword
		//It will have the keyword enum value for it if it is.
		map<string, DirectiveEnum> DirectiveTable;
		//This is a look-up table to see if a given string is an attribute keyword
		//It will have the keyword enum value for it if it is.
		map<string, AttributeEnum> AttributeTable;
		//This is a look-up table to see if a given string is a Data keyword
		//It will have the keyword enum value for it if it is.
		map<string, DataEnum> DataTable;
		//This is a look-up table to see if a given string is an operator
		//It will have the operator enum value for it if it is.
		map<string, OperatorEnum> OperatorTable;

	public:
		virtual bool LexComment(const LocationVector &, const string &, unsigned int &);
		virtual bool LookUpIdentifierChar(const LocationVector &, char, PlaceEnum);
		virtual bool LookUpOperatorChar(const LocationVector &, char, PlaceEnum);
		virtual Token *LookUpKeyword(const LocationVector &, const string &, bool fPeek = false) ;
		virtual Token *LookUpOperator(const LocationVector &, const string &, bool fPeek = false);

		/**********************************************************************\
			LookUpInstruction( [in] location stack, [in] string,
				[in] are we peeking )
			
			If the string is found in the KeywordTable, it returns the token
			for it. Otherwise it returns NULL.

			If peeking is true, then the (ISA)TokenEnum for the type of keyword
			found is returned (cast to Token *), or else TUnknown.
			A token is not created.
		\******/
		virtual Token *LookUpInstruction(const LocationVector &, const string &, bool fPeek = false) = 0;

		AsmLexer(list<Token *> &, CallBackFunction, bool);
	};
}

#endif
