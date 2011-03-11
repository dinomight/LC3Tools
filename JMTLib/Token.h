// Copyright 2002 Ashley Wise
// JediMasterThrash@comcast.net

#ifndef TOKEN_H
#define TOKEN_H

#pragma warning (disable:4786)
#include <string>
#include "JMTLib.h"

using namespace std;
using namespace JMT;

namespace JMT
{
//*** LEXER/TOKEN Stuff ***//

	//The maximum number of characters allowed in a line, excluding the newline
	const unsigned int MAX_LINE = 2046;
	//The largetst number of characters a filename can have
	const unsigned int MAX_FILENAME_CHAR = 255;
	//The largest number of characters an identifier can have
	const unsigned int MAX_IDENTIFIER_CHAR = 63;
	//The largest number of characters a keyword (opcode, directive, etc) can have
	const unsigned int MAX_KEYWORD_CHAR = 31;
	//The largest number of characters an operator can have
	const unsigned int MAX_OPERATOR_CHAR = 15;

	//These are all the first level tokens.
	//Other token enumerations such as keywords and operators must be defined by the Token header of the
	//inherited Lexer class which creates them.
	//The enumeration for the inherited tokens should begin with value NUM_TOKEN_TYPES
	//The inherited token types can then be downcast to TokenEnum without conflicts.
	const unsigned int NUM_TOKEN_TYPES = 9;
	enum TokenEnum {TUnknown = 0, TBad, TComment, TCharConst, TCharacter, TString, TInteger, TReal, TIdentifier};

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		XToken

		An instance of this class is a symbolic representation of the tokens
		in an assembly program.
		
		X is the kind of token, and could be a Keyword, Operator, Number,
		String, or Symbol. The TokenType will identify what kind of Token
		each instance is. It can then be appropriately cast and accessed.

		Specific keyword and operator tokens are defined by the implementation.

		In order to know where the token originated from, each token stores
		a location stack. The last entry in the stack is the
		<input ID, line number> that the token came from. Input ID is
		the index into the InputList which holds filenames of input assembly
		files. Line number is the line within that file.
		All previous entries in the stack recursive locations each file was
		included from in other assembly files.

		Usage of the operator const char *() must be done serially, not
		multi-threaded.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/

	class Token
	{
	protected:
		//Buffer for printing tokens
		static char sToken[65 + MAX(63, MAX_IDENTIFIER_CHAR)];

	public:
		//What kind of token this is
		TokenEnum TokenType;
		//The input ID and line number this token was found on
		LocationVector LocationStack;

		//Line number is parameter
		Token(const LocationVector &);
		//Make a copy of the token
		virtual Token * Copy() const = 0;
		//Print the token
		virtual operator const char *() const = 0;
		virtual ~Token();
	};

	//The token type for identifiers, which become symbols
	class IDToken : public Token
	{
	public:
		string sIdentifier;
		IDToken(const LocationVector &, const string &);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};

	//The token type for character constants
	class CharToken : public Token
	{
	public:
		char Character;
		CharToken(const LocationVector &, char);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};

	//The token type for string constants
	class StringToken : public Token
	{
	public:
		string sString;
		StringToken(const LocationVector &, const string &);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};

	//The token type for integers
	class IntegerToken : public Token
	{
	public:
		uint64 Integer;
		bool fNegative;
		IntegerToken(const LocationVector &, uint64, bool fNeg = false);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};

	//The token type for real numbers
	class RealToken : public Token
	{
	public:
		Real RealVal;
		RealToken(const LocationVector &, double);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};
}

#endif
