//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef ISA_H
#define ISA_H

#pragma warning (disable:4786)
#include <map>
#include <list>
#include <vector>
#include <string>
#include "../Assembler/AsmLexer.h"
#include "../Assembler/AsmParser.h"
#include "../Assembler/Disassembler.h"
#include "../Assembler/Element.h"
#include "../Assembler/AsmToken.h"
#include "../Assembler/Number.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace My
{
	//*NOTE: Some bug in MSVC refuses to believe that something declared
	//in a class as "static const int BLAH" is actually const.
	//Trying to use it in static const char *const sOpcodes[BLAH]
	//gives a compile error.
	const unsigned int NUM_OPCODES = 0;
	const unsigned int NUM_REGISTERS = 0;

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		MyISA

		The following is a sample ISA. All ISAs must adhere to this interface.
		An ISA can offer more, but not less.

		Basically, it contains basic information about the word size and
		endian-ness, a Token class factory, and an Element class factory.

		The ISA will also have to create its own inherited Token and Element
		classes to hold the instruction information.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class MyISA
	{
	public:

	//*** ISA Information ***

		//This is the datatype for the ISA's word size.
		//An ISA with 16-bit words/addressibility would use unsigned short.
		typedef unsigned short Word;
		//Addressability. Specifies the number of bits in a byte address
		//that aren't addressable in this ISA.
		//An ISA with byte addressability would use value "0"
		//An ISA with word addressability would use value "1"
		//An ISA with dword addressability would use value "2"
		static const unsigned char Addressability;
		//The maximum byte address
		static const uint64 MaxAddress;
		//true if this ISA is little endian, false if it's big endian
		static const bool fLittleEndian;

	//*** ISA Lexing ***

		//This is an enumeration of all keywords for this ISA
		enum ISAEnum {TOpcode = NUM_ASMTOKEN_TYPES, TRegister /*, ...*/ };
		//This is an enumeration of all subtypes of keyword TInstruction
		enum OpcodeEnum { /*...*/ };
		//This holds strings for each instruction subtype
		static const char *const sOpcodes[NUM_OPCODES];
		//This is an enumeration of all subtypes of keyword TRegister
		enum RegisterEnum { /*...*/ };
		//This holds strings for each register subtype
		static const char *const sRegisters[NUM_REGISTERS];

		/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
			XToken

			An instance of this class is a symbolic representation of the tokens
			for the ISA.

			The base ISAToken tells if it is an instruction or register token.
		\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
		class ISAToken : public Token
		{
		protected:
			ISAToken();
		public:
			ISAEnum ISAType;
			virtual operator const char *() const;
		};

		/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
			ISA::Lexer

			Inherit from the main lexing class to provide the token factory.
		\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
		class Lexer : public AsmLexer
		{
		protected:
			//These are the look-up tables to see if a given string is a keyword
			//in this ISA. It will have the keyword enum value for it if it is.
			map<string, OpcodeEnum> InstrKeywordTable;
			map<string, RegisterEnum> RegKeywordTable;

		public:
			//The token class factory.
			virtual Token *LookUpInstruction(const LocationVector &, const string &, bool fPeek = false);

			Lexer(list<Token *> &, CallBackFunction, bool fcase = false);
		};

	//*** ISA Parsing ***

		/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
			ISA::Parser

			Inherit from the main parsing class to provide the instruction
			factory.
		\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
		class Parser : public Assembler::Parser
		{
		protected:
		public:
			//The instruction element class factory.
			virtual bool ParseInstruction(list<Token *>::iterator &, const list<Token *>::iterator &);

			Parser(Program &, CallBackFunction);
		};

	//*** ISA Disassembling ***

		/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
			ISA::Disassembler

			Inherit from the main disassembling class to provide the instruction
			disassembling.
		\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
		class Disassembler : public Assembler::Disassembler
		{
		protected:
			//The instruction disassembler
			virtual bool DisassembleInstructions(LocationVector &, istream &, uint64 &);

		public:
			//The ISA must set the Addressability of the Assembler::Disassembler
			Disassembler(Program &, CallBackFunction);
			/******************************************************************\
				DisassembleInstruction( [in] Disassembler instance,
					[in] location vector, [in] instruction address,
					[in] instruction data)

				Disassembles a single instruction. If the disassembler instance
				is provided, it will create labels (symbol numbers). Otherwise
				no labels or symbols are created. If the instruction is invalid,
				word data is created instead.
			\******/
			static Element *DisassembleInstruction(Disassembler *, LocationVector &, uint64, Word);
		};

	//*** ISA Instructions ***

		/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
			InstructionX

			An instance of this class is a symbolic representation of an
			instruction element in an assembly program.

			This element is something that will become bytes in memory, such as
			an instruction or data.

			X is the type of instruction.

			Specific instructions will inherit from Instruction.
			If you want to support optimizations, you'll have to implement
			the virtual functions in the Instruction class.
		\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
		class Instruction : public Element
		{
		public:
			//The type of instruction this is
			OpcodeEnum Opcode;
			//parameter is location stack to send to Element constructor
			Instruction(const LocationVector &);
			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			/******************************************************************\
				Stuff for optimization algorithms
				GetSources/Destinations will reflect NZP use so that branches
				will be dependent on the previous ALU op. Returns the number
				of sources/destinations.
				IsBranch returns true if the instruction may update the PC
				IsMemory rerurns true if the instruction may read or
				write from memory.
			\******/
			virtual unsigned int GetSources(list<RegisterEnum> &) const;
			virtual unsigned int GetDestinations(list<RegisterEnum> &) const;
			virtual bool IsBranch() const;
			virtual bool IsMemory() const;
			
			virtual ~Instruction();
		};
	};
}

#endif
