//	Copyright 2003 Ashley Wise
// JediMasterThrash@comcast.net

#ifndef HIGHLIGHTLEXER_H
#define HIGHLIGHTLEXER_H

#pragma warning (disable:4786)
#include <map>
#include <string>
#include <list>
#include <iostream>
#include "Lexer.h"

using namespace std;
using namespace JMT;

namespace JMT
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		HighlightLexer

		An instance of this class is used to determine the type of lexicons in a
		string for use in syntax highlighting of the text in that string.

		Identifies:
		Keywords, operators, numbers, strings, identifiers, comments

		You must define another class which inherits from HighlightLexer to
		define the differnet keywords, operators, and comment styles.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class HighlightLexer
	{
	protected:
		//true if case sensitive, false if case insensitive
		bool fCase;
		//LookUp* functions from the Lexer are used for finding keywords
		//and operators
		Lexer *pLexer;

	public:
		/**********************************************************************\
			HighlightLexer( [in] Lexer, [in] true if case sensitive  )

			Constructor
			LookUp* functions from the Lexer are used for finding keywords
			and operators			
		\******/
		HighlightLexer(Lexer *, bool);

		/**********************************************************************\
			Lex( [in] input string, [out] type string )
			
			It will find the type of all the lexicons in the input string.
			It will place the value of the lexicon type (TokenEnum)
			into the Type string for each character in the input string.
		\******/
		virtual bool Lex(const string &, string &);

		/**********************************************************************\
			LexIdentifier( [in] input line, [in-out] current character )
			
			It will lex the input line for an identifier. It also checks the
			identifier to see if it is a keyword.
			Returns the type (identifier or keyword).
		\******/
		virtual TokenEnum LexIdentifier(const string &, unsigned int &);

		/**********************************************************************\
			LexOperator( [in] input line, [in-out] current character )
			
			It will lex the input line for an operator.
			If it finds an operator, it returns Operator, else TUnknown
		\******/
		virtual TokenEnum LexOperator(const string &, unsigned int &);

		/**********************************************************************\
			LexNumber( [in] input line, [in-out] current character,
				[out] type of number )
			
			It will lex the input line for a number. Returns the type of number
			(Float or Integer).
		\******/
		virtual TokenEnum LexNumber(const string &, unsigned int &);

		/**********************************************************************\
			LexBase( [in] input line, [in-out] current character,
				[out] base )
			
			It will lex the input line for a base identifier and return
			it. Returns true if sucessful.
		\******/
		virtual bool LexBase(const string &, unsigned int &, BaseEnum &);

		/**********************************************************************\
			LexInteger( [in] input line, [in-out] current character, [in] Base,
				[out] type of number )
			
			It will lex the input line for an integral value of the given base.
			Returns true if sucessful.
		\******/
		virtual bool LexInteger(const string &, unsigned int &, BaseEnum);

		/**********************************************************************\
			LexString( [in] input line, [in-out] current character )
			
			It will lex the input line for a string. Returns true if sucessful.
		\******/
		virtual TokenEnum LexString(const string &, unsigned int &);

		/**********************************************************************\
			LexCharacter( [in] input line, [in-out] current character )
			
			It will lex the input line for a character.
			Returns true if sucessful.
		\******/
		virtual TokenEnum LexCharacter(const string &, unsigned int &);

		/**********************************************************************\
			LexCharacterConstant( [in] input line, [in-out] current character )
			
			It will lex the input line for a character escape sequence.
			Returns true if sucessful.
		\******/
		virtual bool LexCharacterConstant(const string &, unsigned int &);

		virtual ~HighlightLexer();
	};
}

#endif
