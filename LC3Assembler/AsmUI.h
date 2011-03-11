//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef ASMUI_H
#define ASMUI_H

#pragma warning (disable:4786)
#include <vector>
#include <string>
#include "../Assembler/Program.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace LC3
{
	/**************************************************************************\
		AssemblerUI( [in-out] the program to simulate,
			[in] memory image for the progra )
		
		Assembles the list of input files (which should be set up by main).

		Returns true if it completed assembling successfully, else false.
	\******/
	bool AssemblerUI(vector<Program *> &, RamVector &);

	/**************************************************************************\
		AsmCallBack( [in] message type, [in] message, [in] location stack )
		
		Prints the message. Prefixes the message with the location of the
		message (code line within file).

		If the location stack is empty and the message is Info, no prefix is
		printed. If the location stack is empty and the message is Fatal, only
		the prefix "Fatal" is printed.
	\******/
	bool AsmCallBack(MessageEnum, const string &, const LocationVector &LocationStack);
}

#endif
