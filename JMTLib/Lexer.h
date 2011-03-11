// Copyright 2002 Ashley Wise
// JediMasterThrash@comcast.net

#ifndef LEXER_H
#define LEXER_H

#pragma warning (disable:4786)
#include <map>
#include <string>
#include <list>
#include <iostream>
#include "Token.h"
#include "JMTLib.h"

using namespace std;

namespace JMT
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Lexer

		An instance of this class lexically analyzes an input file.

		Keywords, operators, numbers, strings, and identifiers are converted
		into appropriate tokens.

		Comments are ignored.

		New tokens are added to the end of the Token List parameter.

		Multiple lexer instances can be run simultaneously (multi-threaded).
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Lexer
	{
	protected:
		//Buffer for formatting error messages
		char sMessageBuffer[130 + 2*MAX_IDENTIFIER_CHAR];
		//true if case sensitive, false if case insensitive
		bool fCase;
		//The current input source
		list<Token *> &TokenList;
		//true if shouldn't print message for subsequent errors.
		//This is set if future errors are likely just caused by an error that
		//was already printed.
		bool fMaskErrors;

		//Error callback function
		bool (*CallBack)(MessageEnum, const string &, const LocationVector &);

	public:
		//Location of a charater within a string
		enum PlaceEnum {Beginning, Middle, Ending};

		/**********************************************************************\
			Lexer( [in-out] token list, [in] error callback function,
				[in] case sensitive )

			Lexed tokens will be appended to the token list
			If case is false, the lexer is case insensitive. If true, it is case
			sensitive.
			Error messages will be sent to the callback.
		\******/
		Lexer(list<Token *> &, CallBackFunction, bool);

		/**********************************************************************\
			bool Lex( [in-out] location stack, [in] input stream )
			
			It will lex the input stream and add to the token list. The input
			stream is assumed to begin at the given input file and line number
			of the last entry in the location stack. The line number of the
			last entry will be updated to the next line after the last token
			lexed.
			If there is a syntactical error, it will print an error message and
			return false. It will attempt to finish lexing even after
			an error.
		\******/
		virtual bool Lex(LocationVector &, istream &);

		/**********************************************************************\
			LexLine( [in] location stack, [in] input line)
			
			It will lex the input line and add to the token list.
			If there is a syntactical error, it will print an error message and
			return false. It will attempt to finish lexing a line even after
			an error.
		\******/
		virtual bool LexLine(const LocationVector &, const string &);

		/**********************************************************************\
			LexComment( [in] location stack, [in] input line,
				[in-out] current character )
			
			It will lex the input line for a comment, and ignore it.
			Comment is the first function called, so by keeping state, it can
			handle multi-line comments.
			If it finds a comment, it returns true, else false
		\******/
		virtual bool LexComment(const LocationVector &, const string &, unsigned int &) = 0;

		/**********************************************************************\
			LexIdentifier( [in] location stack, [in] input line,
				[in-out] current character )
			
			It will lex the input line for an identifier and add
			the token to the list. Returns true if sucessful.
		\******/
		virtual bool LexIdentifier(const LocationVector &, const string &, unsigned int &);

		/**********************************************************************\
			LexOperator( [in] location stack, [in] input line,
				[in-out] current character )
			
			It will lex the input line for an operator and add
			the token to the list. Returns true if sucessful.
		\******/
		virtual bool LexOperator(const LocationVector &, const string &, unsigned int &);

		/**********************************************************************\
			LexNumber( [in] location stack, [in] input line,
				[in-out] current character )
			
			It will lex the input line for a number and add
			the token to the list. Returns true if sucessful.
		\******/
		virtual bool LexNumber(const LocationVector &, const string &, unsigned int &);

		/**********************************************************************\
			LexBase( [in] input line, [in-out] current character,
				[out] base )
			
			It will lex the input line for a base identifier and return
			it.
		\******/
		virtual bool LexBase(const string &, unsigned int &, BaseEnum &);

		/**********************************************************************\
			LexInteger( [in] location stack, [in] input line,
				[in-out] current character, [in] Base, [out] UpperInteger,
				[out] LowerInteger, [out] number of bits (if not decimal),
				[out] number of shifts, [in-out] true if precision lost)
			
			It will lex the input line for an integral value of the given base
			and return it.
			The number of significant bits read is returned. This number
			may be more than was stored in Integer. If the number was too large,
			UpperInteger will hold the most significant portion, and the number
			of extra digits is returned in Shifts. LowerInteger holds the least
			significant portion. The final bool is true if precision was lost in
			the UpperInteger.
		\******/
		virtual bool LexInteger(const LocationVector &, const string &, unsigned int &, BaseEnum, uint64 &, uint64 &, unsigned int &, unsigned int &, bool &);

		/**********************************************************************\
			LexFraction( [in] location stack, [in] input line,
				[in-out] current character, [in] Base, [in-out] Integer,
				[in-out] number of bits (if not decimal),
				[out] number of shifts, [in-out] true if precision lost )
			
			It will lex the input line for the fraction of an integral value of
			the given base and shift it into Integer.
			The number of significant bits already read for Integer (from the
			left side of the decimal) is supplied, and is updated with the total
			number of significant bits read including the fraction (if not
			decimal). This number
			may be more than was stored in Integer. If the number was too large,
			Integer will hold the most significant portion.
		\******/
		virtual bool LexFraction(const LocationVector &, const string &, unsigned int &, BaseEnum, uint64 &, unsigned int &, unsigned int &, bool &);

		/**********************************************************************\
			LexString( [in] location stack, [in] input line,
				[in-out] current character )
			
			It will lex the input line for a string and add
			the token to the list. Returns true if sucessful.
		\******/
		virtual bool LexString(const LocationVector &, const string &, unsigned int &);

		/**********************************************************************\
			LexCharacter( [in] location stack, [in] input line,
				[in-out] current character )
			
			It will lex the input line for a character, convert it to
			a number, the token to the list. Returns true if sucessful.
		\******/
		virtual bool LexCharacter(const LocationVector &, const string &, unsigned int &);

		/**********************************************************************\
			LexCharacterConstant( [in] location stack, [in] input line,
				[in-out] current character, [out] character constant )
			
			It will lex the input line for a character escape sequence and
			convert it to a character. Returns the character.
		\******/
		virtual bool LexCharacterConstant(const LocationVector &, const string &, unsigned int &, unsigned char &);

		/**********************************************************************\
			LookUpIdentifierChar( [in] location stack, [in] character,
				[in] place )
			
			If the character is a valid identifier character.for the given
			place (Beginning, Middle, or Ending of the string), it returns true,
			otherwise false.
		\******/
		virtual bool LookUpIdentifierChar(const LocationVector &, char, PlaceEnum) = 0;

		/**********************************************************************\
			LookUpOperatorChar( [in] location stack, [in] character,
				[in] place )
			
			If the character is a valid operator character.for the given
			place (Beginning, Middle, or Ending of the string), it returns true,
			otherwise false.
		\******/
		virtual bool LookUpOperatorChar(const LocationVector &, char, PlaceEnum) = 0;

		/**********************************************************************\
			LookUpKeyword( [in] location stack, [in] string,
				[in] are we peeking )
			
			If the string is found in the KeywordTable, it returns the token
			for it. Otherwise it returns NULL.

			If peeking is true, then the TokenEnum for the type of keyword
			found is returned (cast to Token *), or else TUnknown.
			A token is not created.
		\******/
		virtual Token *LookUpKeyword(const LocationVector &, const string &, bool fPeek = false) = 0;

		/**********************************************************************\
			LookUpOperator( [in] location stack, [in] line number, [in] string,
				[in] are we peeking )
			
			If the string is found in the OperatorTable, it returns the token
			for it. Otherwise it returns NULL.

			If peeking is true, then the TokenEnum for the type of keyword
			found is returned (cast to Token *), or else TUnknown.
			A token is not created.
		\******/
		virtual Token *LookUpOperator(const LocationVector &, const string &, bool fPeek = false) = 0;

		virtual ~Lexer();
	};
}

#endif
