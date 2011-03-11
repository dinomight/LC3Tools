//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#pragma warning (disable:4786)
#include <list>
#include "Program.h"
#include "Symbol.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Disassembler

		An instance of this class parses an binary memory image into
		an abstract syntrax tree.

		A binary file's location stack references bytes of data rather than
		line numbers.

		Multiple Disassembler instances can be run simultaneously (multi-threaded).
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Disassembler
	{
	protected:
		//Buffer for formatting error messages
		char sMessageBuffer[128 + MAX(2+2*MAX_IDENTIFIER_CHAR, 1+MAX_FILENAME_CHAR)];
		//The program we are parsing
		Program &TheProg;
		//Addressability. Specifies the number of bits in a byte address
		//that aren't addressable in this ISA.
		unsigned char Addressability;
		//Error callback function
		bool (*CallBack)(MessageEnum, const string &, const LocationVector &);
		uint64 StartAddress, EndAddress;
		//Stores the labels as they are created.
		typedef map<uint64, Symbol *> LabelMap;
		LabelMap Labels;
		unsigned int LabelNumber;
		//Each Disassembler instance can only be run once.
		bool fRunOnce;

		/**********************************************************************\
			DisassembleInstruction( [in-out] location stack, [in] input stream,
				[in-out] address )
			
			This will see if the current position in the memory image
			starts an instruction. if it does, it creates an instruction.
			Otherwise, it creates byte data.

			Updates the parameter address and location's byte number to
			point to the byte after the last byte processed.
		\******/
		virtual bool DisassembleInstructions(LocationVector &, istream &, uint64 &) = 0;

		/**********************************************************************\
			DisassembleData( [in] location stack, [in] input stream,
				[in-out] address )

			Creates a byte data array element for the memory image.

			Updates the parameter address and location's byte number to
			point to the byte after the last byte processed.
		\******/
		virtual bool DisassembleData(LocationVector &, istream &, uint64 &);

		/**********************************************************************\
			DisassembleSymbol( [in] location stack, [in] address,
				[in] symbol name )
			
			Sees if the address already has a label. If so, returns pointer to
			existing label. Otherwise, creates a new label.

			If the symbol name is not provided (""), then it auto-generates
			a label name.

			If the label points before the start or after the end,
			then it returns NULL, and no label is created.
		\******/
		virtual Symbol *DisassembleSymbol(const LocationVector &, uint64, const string &sSymbol = "");

	public:
		typedef list< triple<LocationVector, uint64, string> > SymbolList;

		/**********************************************************************\
			Disassembler( [in-out] symbolic program, [in] error callback function )

			Attaches the program to this disassembler.
		\******/
		Disassembler(Program &, CallBackFunction);

		/**********************************************************************\
			Disassemble( [in-out] location stack, [in] input stream,
				[in] list of pre-defined symbols)
			
			This will go through the input binary data stream and create
			isntruction and data elements from it. The byte number of the last
			entry in the location stack will be updated to the last byte read.

			You should not call Disassembler more than once. Each memory image
			must be a complete program.
		\******/
		virtual bool Disassemble(LocationVector &, istream &, SymbolList &);


		/**********************************************************************\
			ParseSymbolTable( [in-out] start token, [in] end token,
				[out] list of symbols, [in] addressability,
				[in] message callback function )

			Loads a saved symbol table into the label map.
			Creates a list of location, name, address for each symbol.
		\******/
		static bool ParseSymbolTable(const list<Token *>::iterator &, const list<Token *>::iterator &, SymbolList &, unsigned char, CallBackFunction);

		/**********************************************************************\
			~Disassembler()
		\******/
		virtual ~Disassembler();
	};
}

#endif
