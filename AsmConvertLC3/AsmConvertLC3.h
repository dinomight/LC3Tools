//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef ASMCONVERTLC3_H
#define ASMCONVERTLC3_H

#pragma warning (disable:4786)
#include <map>
#include <string>
#include <list>
#include <iostream>
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace LC3
{
	//Enumeration of things we need to change
	enum ChangeEnum { Pound,	//decimal number
		X,	//hexadecimal number
		B,	//binary number
		Orig,	//origin directive
		External,	//external directive
		Fill,	//fill directive
		Blkw,	//block of words directive
		Stringz,	//string directive
		End,	//end directive
		TrapGetc,
		TrapOut,
		TrapPuts,
		TrapIn,
		TrapPutsp,
		TrapHalt,
		Comment,	//comment
		EndOfLine	//end of line
	};

	extern bool fOldLC3;

	/**********************************************************************\
		AsmConvertLC3Line( [in] input file stream, [out] output file stream,
			[in] location stack, [out] message callback)
		
		It will parse the input string and convert it from old LC-3 syntax to
		new Assembler3 syntax, and output the new line to the output sting.

		If there is a problem it will send a message to the CallBack
		function and return false.
	\******/
	bool AsmConvertLC3(istream &, ostream &, LocationVector, CallBackFunction);

	/**********************************************************************\
		AsmConvertLC3Line( [in] input line, [out] output line,
			[in] location stack, [out] message callback,
			[in] true if check for old label syntax)
		
		It will parse the input string and convert it from old LC-3 syntax
		to new Assembler3 syntax, and output the new line to the output
		sting.

		The "check for old label syntax" flag allows it to do some extra
		converting that it can't normally do incase there is Asm3 syntax.

		If there is a problem it will send a message to the CallBack
		function and return false.
	\******/
	bool AsmConvertLC3Line(const string &, string &, const LocationVector &, CallBackFunction, bool fOldLabel = false);
}

#endif
