//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef SIMUI_H
#define SIMUI_H

#pragma warning (disable:4786)
#include <vector>
#include <string>
#include "../Assembler/Program.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;
using namespace Assembler;
using namespace Simulator;

namespace LC3b
{
	/**************************************************************************\
		SimulatorUI( [in-out] the program to simulate,
			[in-out] memory image for the program )
		
		Simulates the program.

		Returns true if it completed simulating successfully, else false.
	\******/
	bool SimulatorUI(vector<Program *> &, RamVector &);

	/**************************************************************************\
		SimCallBack( [in] message type, [in] message )
		
		Prints the message.
	\******/
	bool SimCallBack(MessageEnum, const string &);

	/**************************************************************************\
		SimMessageCallBack( [in] message type, [in] message,
			[in] location stack )
		
		Prints the message. Ignores the location vector, calls SimCallBack.
	\******/
	bool SimMessageCallBack(MessageEnum, const string &, const LocationVector &);

	/**************************************************************************\
		SimCommand( [out] simulator command )
		
		Gets a single simulator command from the user and returns it.
	\******/
	bool SimCommand(string &);

	/**************************************************************************\
		ReadConsole( [out] input buffer, [in] characters to read,
			[out] characters read)

		Reads the number of characters from the console buffer, places it
		in the input buffer, returns the number of characters actually read.
		Removes the input characters from the console buffer.
		CTRL-D is used to signal the end of input. "Enter" is considered
		a valid character to read. All characters entered after a CTRL-D
		are ignored. In Windows, the enter is still necessary after the
		CTRL-D to cause getchar to return. In Unix, CTRL-D alone causes
		getchar to return.

		CTRL-C can be used to give an EOF or Error to the read console.
		"Enter" does not have to be hit after CTRL-C, as getchar returns
		after a CTRL-C. CTRL-C also causes a "pause" breakpoint in the
		simulator. Characters entered inbetween the last "enter" and 
		the CTRL-C are lost.
		
		CTRL-Z gives an EOF on Windows, the same as CTRL-C, except that
		"Enter" must still be hit, and the characters before CTRL-Z are
		not lost. Everything after CTRL-Z is ignored.
		In Unix, CTRL-Z causes the process to suspend, so it should not
		be used.

		//*NOTE: This function should use getch. The win32 implementation
		of getch works perfectly. However, the UNIX implementation of
		getch uses curses and has absolutely horrible behavoir (which btw
		does not agree with the documentation). So in order to have the
		same behavoir across all platforms, the ANSI getchar is used,
		even though this loses the ability to input characters without
		having to hit enter.
	\******/
	bool SimReadConsole(string &, unsigned int, unsigned int &);

	/**************************************************************************\
		WriteConsole( [in] output buffer, [in] characters to write,
			[out] characters written)

		Writes the number of characters from the output buffer, places it
		in the console buffer, returns the number of characters actually
		written.
	\******/
	bool SimWriteConsole(const string &, unsigned int, unsigned int &);
}

#endif
