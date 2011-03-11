//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "AsmToken.h"
#include <cstdio>

using namespace std;
using namespace JMT;

namespace Assembler	{

const char *const sDirectives[NUM_DIRECTIVES] = {"ORIGIN", "SEGMENT", "INCLUDE", "EXTERN", "STRUCTDEF", "DEFINE", "IFDEF", "IFNDEF", "ELSE", "END", "MACRO", "ALIGN"};
const char *const sAttributes[NUM_ATTRIBUTES] = {"ABS", "SEGREL", "BASEREL", "SIZE", "LEN", "SEG"};
const char *const sDataTypes[NUM_DATATYPES] = {"DATA1", "DATA2", "DATA4", "DATA8", "REAL1", "REAL2", "REAL4", "REAL8", "STRUCT"};
const char *const sOperators[NUM_OPERATORS] = {"[", "]", ".", ",", ":", "?", "(", ")", "{", "}", "$", "-"};

DirToken::DirToken(const LocationVector &LocationStack, DirectiveEnum Dir) : Token(LocationStack)
{
	Directive = Dir;
	TokenType = (TokenEnum)TDirective;
}

Token * DirToken::Copy() const
{
	return new DirToken(*this);
}

DirToken::operator const char *() const
{
	sprintf(sToken, "Directive: %.31s", sDirectives[Directive]);
	return sToken;
}

AttributeToken::AttributeToken(const LocationVector &LocationStack, AttributeEnum Attr) : Token(LocationStack)
{
	Attribute = Attr;
	TokenType = (TokenEnum)TAttribute;
}

Token * AttributeToken::Copy() const
{
	return new AttributeToken(*this);
}

AttributeToken::operator const char *() const
{
	sprintf(sToken, "Attribute: %.31s", sAttributes[Attribute]);
	return sToken;
}

DataToken::DataToken(const LocationVector &LocationStack, DataEnum DType) : Token(LocationStack)
{
	DataType = DType;
	TokenType = (TokenEnum)TData;
}

Token * DataToken::Copy() const
{
	return new DataToken(*this);
}

DataToken::operator const char *() const
{
	sprintf(sToken, "Data Keyword: %.31s", sDataTypes[DataType]);
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

ExpandToken::ExpandToken(const LocationVector &LocationStack) : Token(LocationStack)
{
	TokenType = (TokenEnum)TExpand;
}


Token * ExpandToken::Copy() const
{
	return new ExpandToken(*this);
}

ExpandToken::operator const char *() const
{
	sprintf(sToken, "End of expansion:");
	return sToken;
}

}	//namespace Assembler
