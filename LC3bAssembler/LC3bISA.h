//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef LC3BISA_H
#define LC3BISA_H

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

namespace LC3b
{
	//*NOTE: Some bug in MSVC refuses to believe that something declared
	//in a class as "static const int BLAH" is actually const.
	//Trying to use it in static const char *const sOpcodes[BLAH]
	//gives a compile error.
	const unsigned int NUM_OPCODES = 28;
	const unsigned int NUM_REGISTERS = 9;

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		LC3b

		The following defines the LC-3b ISA.

		Basically, it contains basic information about the word size and
		endian-ness, a Token class factory, and an Element class factory.
		It also contains classes for each type of instruction that is supported,
		and a class for its token.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class LC3bISA
	{
	public:

	//*** ISA Information ***

		//16-bit words.
		typedef unsigned short Word;
		//Addressability. Specifies the number of bits in a byte address
		//that aren't addressable in this ISA.
		static const unsigned char Addressability;
		//The maximum address
		static const uint64 MaxAddress;
		//little endian
		static const bool fLittleEndian;
		//Instruction formats
#include "LC3bISA.def"

	//*** ISA Lexing ***

		//This is an enumeration of all keywords for this ISA
		enum ISAEnum {TOpcode = NUM_ASMTOKEN_TYPES, TRegister};
		//This is an enumeration of all subtypes of keyword TInstruction
		enum OpcodeEnum {ADD = 0, AND, BR, BRp, BRz, BRzp, BRn, BRnp, BRnz, BRnzp, JSR, JSRR, TRAP, JMP, LEA, LDB, LDI, LDR, STB, STI, STR, LSHF, RSHFL, RSHFA, NOT, RET, RTI, NOP};
		//This holds opcode values for each instruction subtype
		static const Word vOpcodes [NUM_OPCODES];
		//This holds strings for each instruction subtype
		static const char *const sOpcodes[NUM_OPCODES];
		//This is an enumeration of all subtypes of keyword TRegister
		enum RegisterEnum {R0 = 0, R1, R2, R3, R4, R5, R6, R7, PSR};
		//This holds strings for each register subtype
		static const char *const sRegisters[NUM_REGISTERS];

		/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
			XToken

			An instance of this class is a symbolic representation of the tokens
			for the ISA.

			The base ISAToken tells if it is an isntruction or register token.
		\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/

		class ISAToken : public Token
		{
		protected:
			ISAToken(const LocationVector &);
		public:
			ISAEnum ISAType;
		};

		class OpcodeToken : public ISAToken
		{
		public:
			OpcodeEnum Opcode;
			OpcodeToken(const LocationVector &, OpcodeEnum);
			virtual Token * Copy() const;
			virtual operator const char *() const;
		};

		class RegToken : public ISAToken
		{
		public:
			RegisterEnum Register;
			RegToken(const LocationVector &, RegisterEnum);
			virtual Token * Copy() const;
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
		class Parser : public Assembler::AsmParser
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
			static Element *DisassembleInstruction(Disassembler *, const LocationVector &, uint64, Word);
		};

	//*** ISA Instructions ***

		/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
			InstructionX

			An instance of this class is a symbolic representation of an
			instruction element in an assembly program.

			This element is something that will become bytes in memory, such as
			an instruction or data.

			X is the type of instruction.
		\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/

		class Instruction : public Element
		{
		public:
			//The type of instruction this is
			OpcodeEnum Opcode;
			//parameter is location stack to send to Element constructor
			Instruction(const LocationVector &);
			/******************************************************************\
				Stuff for optimization algorithms
				GetSources/Destinations will reflect NZP use so that branches
				will be dependent on the previous ALU op. Returns the number
				of sources/destinations.
				IsBranch returns true if the instruction may update the PC
				IsMemory rerurns true if the instruction may read or
				write from memory.
			\******/
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual bool IsBranch() const;
			virtual bool IsMemory() const;
		};

		class AddInstr : public Instruction
		{
		public:
			RegisterEnum DR, SR1, SR2;
			bool fUseImm;
			Number *pSImm5;

			/******************************************************************\
				AddInstr( [in] location stack, [in] DR, [in] SR1, [in] SR2 )
				AddInstr( [in] location stack, [in] DR, [in] SR1, [in] Imm5 )

				Creates an add instruction. The final parameter could be a
				register, an immediate value, or a Symbol value.
			\******/
			AddInstr(const LocationVector &, RegisterEnum, RegisterEnum, RegisterEnum);
			AddInstr(const LocationVector &, RegisterEnum, RegisterEnum, Number *);
			virtual ~AddInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual operator const char *() const;
		};

		class AndInstr : public Instruction
		{
		public:
			RegisterEnum DR, SR1, SR2;
			bool fUseImm;
			Number *pSImm5;

			/******************************************************************\
				AndInstr( [in] location stack, [in] DR, [in] SR1, [in] SR2 )
				AndInstr( [in] location stack, [in] DR, [in] SR1, [in] Imm5 )

				Creates an and instruction. The final parameter could be a
				register, an immediate value, or a Symbol.
			\******/
			AndInstr(const LocationVector &, RegisterEnum, RegisterEnum, RegisterEnum);
			AndInstr(const LocationVector &, RegisterEnum, RegisterEnum, Number *);
			virtual ~AndInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual operator const char *() const;
		};

		class BrInstr : public Instruction
		{
		public:
			Number *pSOffset9;

			/******************************************************************\
				BrInstr( [in] location stack, [in] branch type, [in] offset9 )
		
				Creates a branch instruction. The final paraemter could be a
				numerical offset or a Symbol.
			\******/
			BrInstr(const LocationVector &, OpcodeEnum, Number *);
			virtual ~BrInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual bool IsBranch() const;
			virtual operator const char *() const;
		};

		class JsrInstr : public Instruction
		{
		public:
			Number *pSOffset11;

			/******************************************************************\
				JsrInstr( [in] location stack, [in] Offset11 )

				Creates a jsr instruction. The final parameter could be
				a numerical offset or a Symbol.
			\******/
			JsrInstr(const LocationVector &, Number *);
			virtual ~JsrInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual bool IsBranch() const;
			virtual operator const char *() const;
		};

		class JsrrInstr : public Instruction
		{
		public:
			RegisterEnum BaseR;

			/******************************************************************\
				JsrrInstr( [in] location stack, [in] BaseR )

				Creates a jsrr instruction.
			\******/
			JsrrInstr(const LocationVector &, RegisterEnum);
			virtual ~JsrrInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual bool IsBranch() const;
			virtual operator const char *() const;
		};

		class TrapInstr : public Instruction
		{
		public:
			Number *pTrapVect8;

			/******************************************************************\
				TrapInstr( [in] location stack, [in] Trapvect8 )

				Creates a trap instruction. The final parameter could be
				a numerical offset or a Symbol.
			\******/
			TrapInstr(const LocationVector &, Number *);
			virtual ~TrapInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual bool IsBranch() const;
			virtual bool IsMemory() const;
			virtual operator const char *() const;
		};

		class JmpInstr : public Instruction
		{
		public:
			RegisterEnum BaseR;

			/******************************************************************\
				JmpInstr( [in] location stack, [in] register )

				Creates a jump instruction.
			\******/
			JmpInstr(const LocationVector &, RegisterEnum);
			virtual ~JmpInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual bool IsBranch() const;
			virtual operator const char *() const;
		};

		class LeaInstr : public Instruction
		{
		public:
			RegisterEnum DR;
			Number *pSOffset9;

			/******************************************************************\
				LeaInstr( [in] location stack, [in] register, [in] Offset9 )

				Creates a lea instruction. The final parameter could be
				a numerical offset or a Symbol.
			\******/
			LeaInstr(const LocationVector &, RegisterEnum, Number *);
			virtual ~LeaInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual operator const char *() const;
		};

		class LdrInstr : public Instruction
		{
		public:
			RegisterEnum DR, BaseR;
			Number *pSOffset6;

			/******************************************************************\
				LdrInstr( [in] location stack, [in] Load type, [in] DR, [in] BaseR,
					[in] SOffset6 )

				Creates a load instruction. The final value could be a numerical
				index or a Symbol.
			\******/
			LdrInstr(const LocationVector &, OpcodeEnum, RegisterEnum, RegisterEnum, Number *);
			virtual ~LdrInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual bool IsMemory() const;
			virtual operator const char *() const;
		};

		class StrInstr : public Instruction
		{
		public:
			RegisterEnum SR, BaseR;
			Number *pSOffset6;

			/******************************************************************\
				StrInstr( [in] location stack, [in] store type, [in] SR, [in] BaseR,
					[in] SOffset6 )

				Creates a store instruction. The final value could be a
				numerical index or a Symbol.
			\******/
			StrInstr(const LocationVector &, OpcodeEnum, RegisterEnum, RegisterEnum, Number *);
			virtual ~StrInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual bool IsMemory() const;
			virtual operator const char *() const;
		};

		class ShfInstr : public Instruction
		{
		public:
			RegisterEnum DR, SR;
			Number *pImm4;

			/******************************************************************\
				ShfInstr( [in] location stack, [in] shift type, [in] DR, [in] SR,
					[in] Imm4 )

				Creates a shift instruction. The final value could be a
				numerical index or a Symbol.
			\******/
			ShfInstr(const LocationVector &, OpcodeEnum, RegisterEnum, RegisterEnum, Number *);
			virtual ~ShfInstr();

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual operator const char *() const;
		};

		class NotInstr : public Instruction
		{
		public:
			RegisterEnum DR, SR;

			/******************************************************************\
				NotInstr( [in] location stack, [in] DR, [in] SR )

				Creates a not instruction.
			\******/
			NotInstr(const LocationVector &, RegisterEnum, RegisterEnum);

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual operator const char *() const;
		};

		class RetInstr : public Instruction
		{
		public:
			/******************************************************************\
				RetInstr( [in] location stack )

				Creates a ret instruction.
			\******/
			RetInstr(const LocationVector &);

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual bool IsBranch() const;
			virtual operator const char *() const;
		};

		class RtiInstr : public Instruction
		{
		public:
			/******************************************************************\
				RtiInstr( [in] location stack )

				Creates a rti instruction.
			\******/
			RtiInstr(const LocationVector &);

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual unsigned int GetSources(vector<RegisterEnum> &) const;
			virtual unsigned int GetDestinations(vector<RegisterEnum> &) const;
			virtual bool IsBranch() const;
			virtual bool IsMemory() const;
			virtual operator const char *() const;
		};

		class NopInstr : public Instruction
		{
		public:
			/******************************************************************\
				NopInstr( [in] location stack )

				Creates a nop instruction.
			\******/
			NopInstr(const LocationVector &);

			virtual Element *Copy() const;
			virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
			virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
			virtual operator const char *() const;
		};

	};
}

#endif
