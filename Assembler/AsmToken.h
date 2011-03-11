//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef ASMTOKEN_H
#define ASMTOKEN_H

#pragma warning (disable:4786)
#include <string>
#include "../JMTLib/Token.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	//These are all the first level tokens.
	//*NOTE: GCC gives annoying errors for comparing tokenenums to asmtokenenums, so I've casted in the code
	const unsigned int NUM_ASMTOKEN_TYPES = (NUM_TOKEN_TYPES+6);
	enum AsmTokenEnum {TOperator = NUM_TOKEN_TYPES, TDirective, TAttribute, TExpand, TData, TISA};
	//These are the enumerations and strings for the directives
	const unsigned int NUM_DIRECTIVES = 12;
	enum DirectiveEnum {DirOrigin = 0, DirSegment, DirInclude, DirExtern, DirStructDef, DirDefine, DirIfDef, DirIfNDef, DirElse, DirEnd, DirMacro, DirAlign};
	extern const char *const sDirectives[NUM_DIRECTIVES];
	//These are the enumerations and strings for the attributes
	const unsigned int NUM_ATTRIBUTES = 6;
	enum AttributeEnum {AttrAbs = 0, AttrSegRel, AttrBaseRel, AttrSize, AttrLen, AttrSeg};
	extern const char *const sAttributes[NUM_ATTRIBUTES];
	//This is an enumeration of all keywords for Data
	const unsigned int NUM_DATATYPES = 9;
	enum DataEnum {DATA1 = 0, DATA2, DATA4, DATA8, REAL1, REAL2, REAL4, REAL8, STRUCT};
	extern const char *const sDataTypes[NUM_DATATYPES];
	//These are the enumerations and strings for the operators
	const unsigned int NUM_OPERATORS = 12;
	enum OperatorEnum {OpOpenBracket = 0, OpCloseBracket, OpPeriod, OpComma, OpColon, OpQuestion, OpOpenParen, OpCloseParen, OpOpenBrace, OpCloseBrace, OpDollar, OpMinus};
	extern const char *const sOperators[NUM_OPERATORS];

	//The token type for compiler directives (keywords)
	class DirToken : public Token
	{
	public:
		DirectiveEnum Directive;
		DirToken(const LocationVector &, DirectiveEnum);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};

	//The token type for symbol attributes (keywords)
	class AttributeToken : public Token
	{
	public:
		AttributeEnum Attribute;
		AttributeToken(const LocationVector &, AttributeEnum);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};

	//The token type for data keywords
	class DataToken : public Token
	{
	public:
		DataEnum DataType;
		DataToken(const LocationVector &, DataEnum);
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

	//This token is used as a place holder for the end of a define or macro
	//expansion. If the same macro is encountered again before we reach the
	//end, then we know there has been a (possibly infinite) recusion of macro
	//expansion.
	class ExpandToken : public Token
	{
	public:
		ExpandToken(const LocationVector &);
		virtual Token * Copy() const;
		virtual operator const char *() const;
	};

}

#endif
