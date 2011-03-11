//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef PROJECTLEXER_H
#define PROJECTLEXER_H

#pragma warning (disable:4786)
#include <map>
#include <string>
#include <list>
#include "../JMTLib/Lexer.h"
#include "ProjectToken.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;

namespace AshIDE
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		ProjectLexer

		A version of Lexer with specifics for a project file
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class ProjectLexer : public Lexer
	{
	protected:
		//This is a look-up table to see if a given string is a keyword
		//It will have the keyword enum value for it if it is.
		map<string, KeywordEnum> KeywordTable;
		//This is a look-up table to see if a given string is an operator
		//It will have the operator enum value for it if it is.
		map<string, OperatorEnum> OperatorTable;

	public:
		virtual bool LexComment(const LocationVector &, const string &, unsigned int &);
		virtual bool LookUpIdentifierChar(const LocationVector &, char, PlaceEnum);
		virtual bool LookUpOperatorChar(const LocationVector &, char, PlaceEnum);
		virtual Token *LookUpKeyword(const LocationVector &, const string &, bool fPeek = false) ;
		virtual Token *LookUpOperator(const LocationVector &, const string &, bool fPeek = false);

		ProjectLexer(list<Token *> &, CallBackFunction, bool);
	};
}

#endif
