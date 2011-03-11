//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#pragma warning (disable:4786)
#include <iostream>
#include <vector>
#include "Program.h"
#include "Base.h"

using namespace std;
using namespace JMT;

/*----------------------------------------------------------------------------*\
	Assembler

	This is a library of functions and classes for compiling an assembly
	program.

	Compile will return a symbolic representation of the input program.
	The assembler can then do several more things to the program,
	such as optimizing and reordering. The symbolic program can also
	be serialized into binary words or VHDL vectors.

	The assembler is built for a generic assembly language format, which
	can be supported by almost any ISA. The assembler commands, format,
	MACROS, and operatives remain the same for any ISA. It is only the
	instructions and keywords which vary.

	To create an ISA, all you have to do is fill in the functions and data in
	the ISA template. The assembler functions can then be templatized off of
	this specific ISA.

	*NOTE: Should RTTI be turned on?

	*NOTE: MSVC6 has a template bug where different template
	instantiations of a plain function without a template parameter will end
	up using the same instantiation (i.e, foo<int>() and foo<double>() will
	both think they are foo<double>()). To avoid this, I've placed the
	functinos as static members of a class. Class functions don't have this
	issue.
\*----------------------------------------------------------------------------*/
namespace Assembler
{
	template<class ISA>
	class Assemble
	{
	public:
		/**********************************************************************\
			Compile( [in] input stream, [in-out] symbolic program,
				[in] error callback function )

			Lexically scans the input file and converts it into a
			sequence of tokens.

			Expands macros in the token stream.
			
			Parses the token streams into Elements.
			Creates an abstract syntax tree in the form of a symbolic Program
			Returns the Program.

			See documentation on the assembly language syntax.
			
			A callback function is given as the parameter:
				bool MessageCallBack( MessageType, message string, include stack )
			This function will be called everytime the compiler encounters
			a warning or error. The lexer/parser will continue to lex/parse the
			remainder of the input if the callback returns true.

			Returns true if it completed compilation successfully, else false.
		\******/
		static bool Compile(istream &, Program &, CallBackFunction);

		/**********************************************************************\
			Disassemble( [in] input stream, [in-out] symbolic program,
				[in] error callback function )

			Takes a binary memory image file. The first 8-bytes are
			little-endian 64-bit starting address of the memory image. The
			second 8-bytes are little-endian 64-bit size of the memory image.
			
			The rest of the file is sequential bytes from the memory image. The
			ISA determines the endianness of data within these bytes.
			
			The memory image is disassembled into a program. All bytes that
			could represent valid isntructions are created as instruction
			elements. All other bytes are created as byte data.

			Labels are created for targets of LEA, JSR, and BR instructions.
			Only one segment is created.

			A callback function is given as the parameter:
				bool MessageCallBack( MessageType, message string, include stack )
			This function will be called everytime the compiler encounters
			a warning or error. The lexer/parser will continue to lex/parse the
			remainder of the input if the callback returns true.

			Returns true if it completed compilation successfully, else false.
		\******/
		static bool Disassemble(istream &, Program &, CallBackFunction);

		/**********************************************************************\
			Link( [in-out] symbolic program, [out] memory image,
				[in] error callback function )

			Resolves symbols and addresses.
			Obtains the binary representation of each element in the program.
			Links all the segments' binary representations into a binary image.
			Links all the programs' binary images into a memory image.

			Returns true if it completed linking, else false.
		\******/
		static bool Link(vector<Program *> &, RamVector &, CallBackFunction);

		/**********************************************************************\
			VHDLBuild( [in-out] output streamn, [in] memory image,
				[in] error callback function )

			Outputs VHDL ram vectors for the image.
		\******/
		static bool VHDLWrite(ostream &, const RamVector &, CallBackFunction);

		/**********************************************************************\
			Exercises for students could involve implementing the following
			functions:

			Additional functionality may need to be added to the InstructionBase
			class to support these:

			bool SemanticAnalyzer()
			bool Optimize();
			bool LoopUnrolling();
			bool ReOrder();
			bool Bundle();

			An example framework is provided in this Optimize Function.

			Optimizations may not be reliable if any PC-relative address offset
			is specified as a hardcoded number rather than a symbol/label.
		\******/
		static bool Optimize(Program &);
	};
}

#include "Assembler.cpp"

#endif
