//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "LC3bISA.h"
#include <cstdio>
#include <strstream>
#include "../Assembler/Data.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace LC3b	{

const unsigned char LC3bISA::Addressability = 0;
const uint64 LC3bISA::MaxAddress = 0xFFFF;
const bool LC3bISA::fLittleEndian = true;

const LC3bISA::Word LC3bISA::vOpcodes[NUM_OPCODES] = {0x1,   0x5,   0x0,  0x0,   0x0,   0x0,    0x0,   0x0,    0x0,    0x0,     0x4,   0x4,    0xF,    0xC,   0xE,   0x2,   0xA,   0x6,   0x3,   0xB,   0x7,   0xD,    0xD,     0xD,     0x9,   0xC,   0x8,   0x0};
const char *const LC3bISA::sOpcodes[NUM_OPCODES] =  {"ADD", "AND", "BR", "BRp", "BRz", "BRzp", "BRn", "BRnp", "BRnz", "BRnzp", "JSR", "JSRR", "TRAP", "JMP", "LEA", "LDb", "LDI", "LDR", "STb", "STI", "STR", "LSHF", "RSHFL", "RSHFA", "NOT", "RET", "RTI", "NOP"};
const char *const LC3bISA::sRegisters[NUM_REGISTERS] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "nzp"};



LC3bISA::ISAToken::ISAToken(const LocationVector &LocationStack) : Token(LocationStack)
{
	TokenType = (TokenEnum)TISA;
}

LC3bISA::OpcodeToken::OpcodeToken(const LocationVector &LocationStack, OpcodeEnum opcode) : ISAToken(LocationStack)
{
	ISAType = TOpcode;
	Opcode = opcode;
}

Token * LC3bISA::OpcodeToken::Copy() const
{
	return new OpcodeToken(*this);
}

LC3bISA::OpcodeToken::operator const char *() const
{
	sprintf(sToken, "LC3b ISA Opcode Keyword: %.31s", sOpcodes[Opcode]);
	return sToken;
}

LC3bISA::RegToken::RegToken(const LocationVector &LocationStack, RegisterEnum Reg) : ISAToken(LocationStack)
{
	ISAType = TRegister;
	Register = Reg;
}

Token * LC3bISA::RegToken::Copy() const
{
	return new RegToken(*this);
}

LC3bISA::RegToken::operator const char *() const
{
	sprintf(sToken, "LC3b ISA Register Keyword: %.15s", sRegisters[Register]);
	return sToken;
}

LC3bISA::Lexer::Lexer(list<Token *> &tokenlist, CallBackFunction, bool fcase) : AsmLexer(tokenlist, CallBack, fcase)
{
	unsigned int i;
	for(i = 0; i < NUM_OPCODES; i++)
		InstrKeywordTable.insert(map<string, OpcodeEnum>::value_type( (fCase ? sOpcodes[i] : ToLower(sOpcodes[i])) , (OpcodeEnum)i));
	for(i = 0; i < NUM_REGISTERS; i++)
		RegKeywordTable.insert(map<string, RegisterEnum>::value_type( (fCase ? sRegisters[i] : ToLower(sRegisters[i])), (RegisterEnum)i));
}

Token *LC3bISA::Lexer::LookUpInstruction(const LocationVector &LocationStack, const string &sKeyword, bool fPeek)
{
	//See if the keyword is an instruction
	map<string, OpcodeEnum>::iterator InstrKeyIter = InstrKeywordTable.find(sKeyword);
	if(InstrKeyIter != InstrKeywordTable.end())
	{
		if(fPeek)
			//If peeking, return a TokenEnum
			return (Token *)TOpcode;
		return new OpcodeToken(LocationStack, InstrKeyIter->second);
	}
	//See if the keyword is a register
	map<string, RegisterEnum>::iterator RegKeyIter = RegKeywordTable.find(sKeyword);
	if(RegKeyIter != RegKeywordTable.end())
	{
		if(RegKeyIter->second >= PSR)
		{
			//flags aren't valid registers
			if(fPeek)
				//If peeking, return a TokenEnum
				return (Token *)TUnknown;
			return NULL;
		}
		if(fPeek)
			//If peeking, return a TokenEnum
			return (Token *)TRegister;
		return new RegToken(LocationStack, RegKeyIter->second);
	}

	if(fPeek)
		//If peeking, return a TokenEnum
		return (Token *)TUnknown;
	return NULL;
}

LC3bISA::Parser::Parser(Program &Prog, CallBackFunction) : Assembler::AsmParser(Prog, CallBack)
{
}

bool LC3bISA::Parser::ParseInstruction(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	bool fRetVal = true;
	Instruction *pNewInstr;
	char sMessageBuffer[256];
	OpcodeEnum Instr;
	RegisterEnum DR, SR1, SR2;
	Number *pNumber;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is a reference to the start of the data element,
	//which is used for the element's line number
	list<Token *>::iterator TokenIter, OrigIter = StartIter;
	Segment *pSegment;

	if(fStructDef)
	{
		pSegment = *TheProg.Structures.rbegin();
	}
	else
	{
		if(!fOrigin)
			CallBack(Warning, "Program origin not specified.", (*StartIter)->LocationStack);
		fOrigin = true;
		//Make sure a segment exists to put the data in
		list<Segment *>::reverse_iterator SegmentIter = TheProg.Segments.rbegin();
		if(SegmentIter == TheProg.Segments.rend())
		{
			CallBack(Warning, "Instruction not in segment, creating default segment.", (*StartIter)->LocationStack);
			pSegment = new Segment((*StartIter)->LocationStack, LC3bISA::Addressability);
			TheProg.Segments.push_back(pSegment);
		}
		else
			pSegment = *SegmentIter;
	}

	switch(((ISAToken *)(*StartIter))->ISAType)
	{
	case TOpcode:
		switch(Instr = ((OpcodeToken *)(*StartIter))->Opcode)
		{
		//All "Num" can also be "Symbol"
		//Reg, Reg, Num or Reg
		case ADD:
		case AND:
			TokenIter = StartIter++;

			//Get the first register
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TISA || ((ISAToken *)(*StartIter))->ISAType != TRegister)
			{
				sprintf(sMessageBuffer, "%.31s instruction requires register as first parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			DR = ((RegToken *)(*StartIter))->Register;
			TokenIter = StartIter++;

			//Look for comma
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			{
				sprintf(sMessageBuffer, "Missing first comma in %.31s instruction argument list.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the second register
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TISA || ((ISAToken *)(*StartIter))->ISAType != TRegister)
			{
				sprintf(sMessageBuffer, "%.31s instruction requires register as second parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			SR1 = ((RegToken *)(*StartIter))->Register;
			TokenIter = StartIter++;

			//Look for comma
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			{
				sprintf(sMessageBuffer, "Missing second comma in %.31s instruction argument list.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the third parameter (register or number or symbol)
			if(StartIter != EndIter && (*StartIter)->TokenType == (TokenEnum)TISA && ((ISAToken *)(*StartIter))->ISAType == TRegister)
			{	//It's a register
				SR2 = ((RegToken *)(*StartIter))->Register;
				if(Instr == ADD)
					pNewInstr = new AddInstr((*OrigIter)->LocationStack, DR, SR1, SR2);
				else
					pNewInstr = new AndInstr((*OrigIter)->LocationStack, DR, SR1, SR2);
				TokenIter = StartIter++;
				break;
			}
			else if( !(pNumber = ParseNumber(TokenIter, StartIter, EndIter, true, false)) )
			{
				sprintf(sMessageBuffer, "%.31s instruction requires register, number, or symbol as third parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}

			if(Instr == ADD)
				pNewInstr = new AddInstr((*OrigIter)->LocationStack, DR, SR1, pNumber);
			else
				pNewInstr = new AndInstr((*OrigIter)->LocationStack, DR, SR1, pNumber);
			break;
		//Num
		case BR:
		case BRp:
		case BRz:
		case BRzp:
		case BRn:
		case BRnp:
		case BRnz:
		case BRnzp:
		case JSR:
		case TRAP:
			TokenIter = StartIter++;

			//Get the parameter (number or symbol)
			if( !(pNumber = ParseNumber(TokenIter, StartIter, EndIter, true, false)) )
			{
				sprintf(sMessageBuffer, "%.31s instruction requires a number or symbol as parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			if(pNumber->NumberType != NumSymbol && (pNumber->NumberType != NumInteger || ((IntegerNumber *)(pNumber))->Value != 0))
			{
				sprintf(sMessageBuffer, "%.31s instruction target address is not symbolic.", sOpcodes[Instr]);
				CallBack(Warning, sMessageBuffer, (*TokenIter)->LocationStack);
			}
			if(Instr >= BR && Instr <= BRnzp)
				pNewInstr = new BrInstr((*OrigIter)->LocationStack, Instr, pNumber);
			else if(Instr == JSR)
				pNewInstr = new JsrInstr((*OrigIter)->LocationStack, pNumber);
			else if(Instr == TRAP)
				pNewInstr = new TrapInstr((*OrigIter)->LocationStack, pNumber);
			break;
		//Reg
		case JSRR:
		case JMP:
			TokenIter = StartIter++;

			//Get the register
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TISA || ((ISAToken *)(*StartIter))->ISAType != TRegister)
			{
				sprintf(sMessageBuffer, "%.31s instruction requires a register as parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			SR1 = ((RegToken *)(*StartIter))->Register;
			TokenIter = StartIter++;

			if(Instr == JSRR)
				pNewInstr = new JsrrInstr((*OrigIter)->LocationStack, SR1);
			else
				pNewInstr = new JmpInstr((*OrigIter)->LocationStack, SR1);
			break;
			break;
		//Reg, Num
		case LEA:
			TokenIter = StartIter++;

			//Get the first register
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TISA || ((ISAToken *)(*StartIter))->ISAType != TRegister)
			{
				sprintf(sMessageBuffer, "%.31s instruction requires register as first parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			DR = ((RegToken *)(*StartIter))->Register;
			TokenIter = StartIter++;

			//Look for comma
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			{
				sprintf(sMessageBuffer, "Missing comma in %.31s instruction argument list.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the second parameter (number or symbol)
			if( !(pNumber = ParseNumber(TokenIter, StartIter, EndIter, true, false)) )
			{
				sprintf(sMessageBuffer, "%.31s instruction requires number or symbol as second parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			if(pNumber->NumberType != NumSymbol && (pNumber->NumberType != NumInteger || ((IntegerNumber *)(pNumber))->Value != 0))
			{
				sprintf(sMessageBuffer, "%.31s instruction target address is not symbolic.", sOpcodes[Instr]);
				CallBack(Warning, sMessageBuffer, (*TokenIter)->LocationStack);
			}

			pNewInstr = new LeaInstr((*OrigIter)->LocationStack, DR, pNumber);
			break;
		//Reg, Reg, Num
		case LDB:
		case LDI:
		case LDR:
		case STB:
		case STI:
		case STR:
		case LSHF:
		case RSHFL:
		case RSHFA:
			TokenIter = StartIter++;

			//Get the first register
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TISA || ((ISAToken *)(*StartIter))->ISAType != TRegister)
			{
				sprintf(sMessageBuffer, "%.31s instruction requires register as first parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			DR = ((RegToken *)(*StartIter))->Register;
			TokenIter = StartIter++;

			//Look for comma
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			{
				sprintf(sMessageBuffer, "Missing first comma in %.31s instruction argument list.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the second register
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TISA || ((ISAToken *)(*StartIter))->ISAType != TRegister)
			{
				sprintf(sMessageBuffer, "%.31s instruction requires register as second parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			SR1 = ((RegToken *)(*StartIter))->Register;
			TokenIter = StartIter++;

			//Look for comma
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			{
				sprintf(sMessageBuffer, "Missing second comma in %.31s instruction argument list.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the third parameter (number or symbol)
			if( !(pNumber = ParseNumber(TokenIter, StartIter, EndIter, true, false)) )
			{
				sprintf(sMessageBuffer, "%.31s instruction requires number or symbol as third parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			if( (Instr < LSHF || Instr > RSHFA) && pNumber->NumberType != NumSymbol && (pNumber->NumberType != NumInteger || ((IntegerNumber *)(pNumber))->Value != 0))
			{
				sprintf(sMessageBuffer, "%.31s instruction target address is not symbolic.", sOpcodes[Instr]);
				CallBack(Warning, sMessageBuffer, (*TokenIter)->LocationStack);
			}

			if(Instr >= LDB && Instr <= LDR)
				pNewInstr = new LdrInstr((*OrigIter)->LocationStack, Instr, DR, SR1, pNumber);
			else if(Instr >= LSHF && Instr <= RSHFA)
				pNewInstr = new ShfInstr((*OrigIter)->LocationStack, Instr, DR, SR1, pNumber);
			else if(Instr >= STB && Instr <= STR)
				pNewInstr = new StrInstr((*OrigIter)->LocationStack, Instr, DR, SR1, pNumber);
			break;
		//Reg, Reg
		case NOT:
			TokenIter = StartIter++;

			//Get the first register
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TISA || ((ISAToken *)(*StartIter))->ISAType != TRegister)
			{
				sprintf(sMessageBuffer, "%.31s instruction requires register as first parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			DR = ((RegToken *)(*StartIter))->Register;
			TokenIter = StartIter++;

			//Look for comma
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			{
				sprintf(sMessageBuffer, "Missing comma in %.31s instruction argument list.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the second register
			if(StartIter == EndIter || (*StartIter)->TokenType != (TokenEnum)TISA || ((ISAToken *)(*StartIter))->ISAType != TRegister)
			{
				sprintf(sMessageBuffer, "%.31s instruction requires register as second parameter.", sOpcodes[Instr]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			SR1 = ((RegToken *)(*StartIter))->Register;
			TokenIter = StartIter++;

			pNewInstr = new NotInstr((*OrigIter)->LocationStack, DR, SR1);
			break;
		//none
		case RET:
		case RTI:
		case NOP:
			TokenIter = StartIter++;

			if(Instr == RET)
				pNewInstr = new RetInstr((*OrigIter)->LocationStack);
			else if(Instr == RTI)
				pNewInstr = new RtiInstr((*OrigIter)->LocationStack);
			else	//NOP
				pNewInstr = new NopInstr((*OrigIter)->LocationStack);
			break;
		}
		break;
	case TRegister:
		if(!fMaskErrors)
		{
			sprintf(sMessageBuffer, "Unexpected token: %.127s", (const char *)**StartIter);
			CallBack(Error, sMessageBuffer, (*StartIter)->LocationStack);
		}
		TokenIter = StartIter++;
		return false;
	}

	//Add the instruction to the end of the last (most recent) segment
	pSegment->Sequence.push_back(pNewInstr);
	if(!LabelList.empty())
	{
		for(list<Label *>::iterator LabelIter = LabelList.begin(); LabelIter != LabelList.end();)
		{
			(*LabelIter)->pElement = pNewInstr;
			LabelIter = LabelList.erase(LabelIter);
		}
	}
	return fRetVal;
}

LC3bISA::Disassembler::Disassembler(Program &Prog, CallBackFunction) : Assembler::Disassembler(Prog, CallBack)
{
	Addressability = Addressability;
}

bool LC3bISA::Disassembler::DisassembleInstructions(LocationVector &LocationStack, istream &InputStream, uint64 &Address)
{
	bool fRetVal = true;
	unsigned int i, j, TempChar;
	Element *pElement;
	union wByteData_t
	{
		Word WordData;
		char Bytes[sizeof(Word)];
	} wByteData;

	while(InputStream.good() && Address < EndAddress)
	{
		//Get an instruction from the stream.
		for(i = 0; i < sizeof(Word) && Address+i < EndAddress; i++)
		{
			TempChar = InputStream.get();
			if(TempChar == -1)	//EOF
				break;
			if(fLittleEndian)
#ifdef BIG_ENDIAN_BUILD
				wByteData.Bytes[sizeof(Word) - i - 1] = TempChar;
#else
				wByteData.Bytes[i] = TempChar;
#endif
			else	//Big Endian
#ifdef BIG_ENDIAN_BUILD
				wByteData.Bytes[i] = TempChar;
#else
				wByteData.Bytes[sizeof(Word) - i - 1] = TempChar;
#endif
		}

		if(i < sizeof(Word))
		{
			//There wasn't enough data for an instruction
			for(j = 0; j < i; j++)
			{
				if(fLittleEndian)
#ifdef BIG_ENDIAN_BUILD
					pElement = new Data(LocationStack, DATA1, Addressability, new IntegerNumber(LocationStack, wByteData.Bytes[sizeof(Word) - j - 1]));
#else
					pElement = new Data(LocationStack, DATA1, Addressability, new IntegerNumber(LocationStack, wByteData.Bytes[j]));
#endif
				else	//Big Endian
#ifdef BIG_ENDIAN_BUILD
					pElement = new Data(LocationStack, DATA1, Addressability, new IntegerNumber(LocationStack, wByteData.Bytes[j]));
#else
					pElement = new Data(LocationStack, DATA1, Addressability, new IntegerNumber(LocationStack, wByteData.Bytes[sizeof(Word) - j - 1]));
#endif
				(*TheProg.Segments.rbegin())->Sequence.push_back(pElement);
				Address++;
				LocationStack.rbegin()->second++;
			}
			break;
		}

		//build instruction
		pElement = DisassembleInstruction(this, LocationStack, Address, wByteData.WordData);
		(*TheProg.Segments.rbegin())->Sequence.push_back(pElement);
		Address += sizeof(Word);
		LocationStack.rbegin()->second += sizeof(Word);
	}

	return fRetVal;
}

Element *LC3bISA::Disassembler::DisassembleInstruction(Disassembler *pThis, const LocationVector &LocationStack, uint64 Address, Word InstrData)
{
	//build instruction
	Symbol *pSymbol;
	Element *pElement;
	LC3bInstruction Instr;
	OpcodeEnum Opcode;
	uint64 TempAddr;
	Instr.Binary = InstrData;

	switch(Instr.Base.Opcode)
	{
	case 0x1:	//vOpcodes[ADD]:
		if(Instr.Add.UseImm)
		{
			if(Instr.AddImm.Imm5 & 0x10)
				pElement = new AddInstr(LocationStack, (RegisterEnum)Instr.Add.DR, (RegisterEnum)Instr.Add.SR1, new IntegerNumber(LocationStack, -((uint64)Instr.AddImm.Imm5 | 0xFFFFFFFFFFFFFFE0), true));
			else
				pElement = new AddInstr(LocationStack, (RegisterEnum)Instr.Add.DR, (RegisterEnum)Instr.Add.SR1, new IntegerNumber(LocationStack, Instr.AddImm.Imm5, false));
		}
		else
		{
			if(Instr.Add.Zeros)
				goto Default;
			pElement = new AddInstr(LocationStack, (RegisterEnum)Instr.Add.DR, (RegisterEnum)Instr.Add.SR1, (RegisterEnum)Instr.Add.SR2);
		}
		break;
	case 0x5:	//vOpcodes[AND]:
		if(Instr.And.UseImm)
		{
			if(Instr.AndImm.Imm5 & 0x10)
				pElement = new AndInstr(LocationStack, (RegisterEnum)Instr.And.DR, (RegisterEnum)Instr.And.SR1, new IntegerNumber(LocationStack, -((uint64)Instr.AndImm.Imm5 | 0xFFFFFFFFFFFFFFE0), true));
			else
				pElement = new AndInstr(LocationStack, (RegisterEnum)Instr.And.DR, (RegisterEnum)Instr.And.SR1, new IntegerNumber(LocationStack, Instr.AndImm.Imm5, false));
		}
		else
		{
			if(Instr.And.Zeros)
				goto Default;
			pElement = new AndInstr(LocationStack, (RegisterEnum)Instr.And.DR, (RegisterEnum)Instr.And.SR1, (RegisterEnum)Instr.And.SR2);
		}
		break;
	case 0x0:	//vOpcodes[BR]:
		if(Instr.Br.N)
		{
			if(Instr.Br.Z)
			{
				if(Instr.Br.P)
					Opcode = BRnzp;
				else
					Opcode = BRnz;
			}
			else
			{
				if(Instr.Br.P)
					Opcode = BRnp;
				else
					Opcode = BRn;
			}
		}
		else
		{
			if(Instr.Br.Z)
			{
				if(Instr.Br.P)
					Opcode = BRzp;
				else
					Opcode = BRz;
			}
			else
			{
				if(Instr.Br.P)
					Opcode = BRp;
				else
				{
					//This is a NOP
					pElement = new NopInstr(LocationStack);
					break;
				}
			}
		}
		if(Instr.Br.Offset9 & 0x100)
			TempAddr = Address + 2 + ((uint64)(Instr.Br.Offset9 << 1) | 0xFFFFFFFFFFFFFC00);
		else
			TempAddr = Address + 2 + (uint64)(Instr.Br.Offset9 << 1);
		if( pThis && (pSymbol = pThis->DisassembleSymbol(LocationStack, TempAddr)) )
			pElement = new BrInstr(LocationStack, Opcode, new SymbolNumber(LocationStack, pSymbol, pThis->TheProg.TheSymbolTable.ResolvedSymbols));
		else
		{
			if(Instr.Br.Offset9 & 0x100)
				pElement = new BrInstr(LocationStack, Opcode, new IntegerNumber(LocationStack, -((uint64)Instr.Br.Offset9 | 0xFFFFFFFFFFFFFE00), true));
			else
				pElement = new BrInstr(LocationStack, Opcode, new IntegerNumber(LocationStack, Instr.Br.Offset9, false));
		}
		break;
	case 0x4:	//vOpcodes[JSR]:
		if(Instr.Jsr.JsrType)
		{	//JSR
		
			if(Instr.Jsr.Offset11 & 0x400)
				TempAddr = Address + 2 + ((uint64)(Instr.Jsr.Offset11 << 1) | 0xFFFFFFFFFFFFF000);
			else
				TempAddr = Address + 2 + (uint64)(Instr.Jsr.Offset11 << 1);
			if( pThis && (pSymbol = pThis->DisassembleSymbol(LocationStack, TempAddr)) )
				pElement = new JsrInstr(LocationStack, new SymbolNumber(LocationStack, pSymbol, pThis->TheProg.TheSymbolTable.ResolvedSymbols));
			else
			{
				if(Instr.Jsr.Offset11 & 0x400)
					pElement = new JsrInstr(LocationStack, new IntegerNumber(LocationStack, -((uint64)Instr.Jsr.Offset11 | 0xFFFFFFFFFFFFF800), true));
				else
					pElement = new JsrInstr(LocationStack, new IntegerNumber(LocationStack, Instr.Jsr.Offset11, false));
			}
		}
		else
		{	//JSRR
			if(Instr.Jsrr.Zero2 || Instr.Jsrr.Zero6)
				goto Default;
			pElement = new JsrrInstr(LocationStack, (RegisterEnum)Instr.Jsrr.BaseR);
		}
		break;
	case 0xF:	//vOpcodes[TRAP]:
		if(Instr.Trap.Zeros)
			goto Default;
		if( pThis && (pSymbol = pThis->DisassembleSymbol( LocationStack, (Instr.Trap.TrapVect8 << 1) )) )
			pElement = new TrapInstr(LocationStack, new SymbolNumber(LocationStack, pSymbol, pThis->TheProg.TheSymbolTable.ResolvedSymbols));
		else
			pElement = new TrapInstr(LocationStack, new IntegerNumber(LocationStack, Instr.Trap.TrapVect8, false));
		break;
	case 0xC:	//vOpcodes[JMP]:
		if(Instr.Jmp.Zero3 || Instr.Jmp.Zero6)
			goto Default;
		else if(Instr.Jmp.BaseR == R7)
			pElement = new RetInstr(LocationStack);
		else
			pElement = new JmpInstr(LocationStack, (RegisterEnum)Instr.Jmp.BaseR);
		break;
	case 0xE:	//vOpcodes[LEA]:
		if(Instr.Lea.Offset9 & 0x100)
			TempAddr = Address + 2 + ((uint64)(Instr.Lea.Offset9 << 1) | 0xFFFFFFFFFFFFFC00);
		else
			TempAddr = Address + 2 + (uint64)(Instr.Lea.Offset9 << 1);
		if( pThis && (pSymbol = pThis->DisassembleSymbol(LocationStack, TempAddr)) )
			pElement = new LeaInstr(LocationStack, (RegisterEnum)Instr.Lea.DR, new SymbolNumber(LocationStack, pSymbol, pThis->TheProg.TheSymbolTable.ResolvedSymbols));
		else
		{
			if(Instr.Lea.Offset9 & 0x100)
				pElement = new LeaInstr(LocationStack, (RegisterEnum)Instr.Lea.DR, new IntegerNumber(LocationStack, -((uint64)Instr.Lea.Offset9 | 0xFFFFFFFFFFFFFE00), true));
			else
				pElement = new LeaInstr(LocationStack, (RegisterEnum)Instr.Lea.DR, new IntegerNumber(LocationStack, Instr.Lea.Offset9, false));
		}
		break;
	case 0x2:	//vOpcodes[LDB]:
		pElement = new LdrInstr(LocationStack, LDB, (RegisterEnum)Instr.Ld.DR, (RegisterEnum)Instr.Ld.BaseR, new IntegerNumber(LocationStack, Instr.Ld.Offset6, false));
		break;
	case 0xA:	//vOpcodes[LDI]:
		pElement = new LdrInstr(LocationStack, LDI, (RegisterEnum)Instr.Ld.DR, (RegisterEnum)Instr.Ld.BaseR, new IntegerNumber(LocationStack, Instr.Ld.Offset6, false));
		break;
	case 0x6:	//vOpcodes[LDR]:
		pElement = new LdrInstr(LocationStack, LDR, (RegisterEnum)Instr.Ld.DR, (RegisterEnum)Instr.Ld.BaseR, new IntegerNumber(LocationStack, Instr.Ld.Offset6, false));
		break;
	case 0x3:	//vOpcodes[STB]:
		pElement = new StrInstr(LocationStack, STB, (RegisterEnum)Instr.St.SR, (RegisterEnum)Instr.St.BaseR, new IntegerNumber(LocationStack, Instr.St.Offset6, false));
		break;
	case 0xB:	//vOpcodes[STI]:
		pElement = new StrInstr(LocationStack, STI, (RegisterEnum)Instr.St.SR, (RegisterEnum)Instr.St.BaseR, new IntegerNumber(LocationStack, Instr.St.Offset6, false));
		break;
	case 0x7:	//vOpcodes[STR]:
		pElement = new StrInstr(LocationStack, STR, (RegisterEnum)Instr.St.SR, (RegisterEnum)Instr.St.BaseR, new IntegerNumber(LocationStack, Instr.St.Offset6, false));
		break;
	case 0xD:	//vOpcodes[SHF]:
		if(Instr.Shf.D)	//Shift right
		{
			if(Instr.Shf.A)	//Arithemtic
				pElement = new ShfInstr(LocationStack, RSHFA, (RegisterEnum)Instr.Shf.DR, (RegisterEnum)Instr.Shf.SR, new IntegerNumber(LocationStack, Instr.Shf.Imm4, false));
			else	//Logical
				pElement = new ShfInstr(LocationStack, RSHFL, (RegisterEnum)Instr.Shf.DR, (RegisterEnum)Instr.Shf.SR, new IntegerNumber(LocationStack, Instr.Shf.Imm4, false));
		}
		else	//Shift left
		{
			pElement = new ShfInstr(LocationStack, LSHF, (RegisterEnum)Instr.Shf.DR, (RegisterEnum)Instr.Shf.SR, new IntegerNumber(LocationStack, Instr.Shf.Imm4, false));
		}
		break;
	case 0x9:	//vOpcodes[NOT]:
		if(Instr.Not.Ones != 0x3F)
			goto Default;
		pElement = new NotInstr(LocationStack, (RegisterEnum)Instr.Not.DR, (RegisterEnum)Instr.Not.SR);
		break;
	case 0x8:	//vOpcodes[RTI]:
		if(Instr.Rti.Zeros)
			goto Default;
		pElement = new RtiInstr(LocationStack);
		break;
	default:
Default:	//This is not a valid instruction
		pElement = new Data(LocationStack, DATA2, LC3bISA::Addressability, new IntegerNumber(LocationStack, Instr.Binary, false));
	}
	pElement->Address = Address;
	return pElement;
}

LC3bISA::Instruction::Instruction(const LocationVector &LocationStack) : Element(LocationStack)
{
	ElementType = InstructionElement;
}

unsigned int LC3bISA::Instruction::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	return 0;
}

unsigned int LC3bISA::Instruction::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	return 0;
}

bool LC3bISA::Instruction::IsBranch() const
{
	return false;
}

bool LC3bISA::Instruction::IsMemory() const
{
	return false;
}

/******************************************************************************\
|**********************************   ADD    **********************************|
\******************************************************************************/

LC3bISA::AddInstr::AddInstr(const LocationVector &LocationStack, RegisterEnum dr, RegisterEnum sr1, RegisterEnum sr2) : Instruction(LocationStack)
{
	Opcode = ADD;
	DR = dr;
	SR1 = sr1;
	SR2 = sr2;
	fUseImm = false;
	pSImm5 = NULL;
	Size = 2;
	Length = 1;
}

LC3bISA::AddInstr::AddInstr(const LocationVector &LocationStack, RegisterEnum dr, RegisterEnum sr1, Number *psimm5) : Instruction(LocationStack)
{
	Opcode = ADD;
	DR = dr;
	SR1 = sr1;
	fUseImm = true;
	pSImm5 = psimm5;
	Size = 2;
	Length = 1;
}

LC3bISA::AddInstr::~AddInstr()
{
	if(fUseImm)
		delete pSImm5;
}

Element *LC3bISA::AddInstr::Copy() const
{
	AddInstr *pInstr = new AddInstr(*this);
	if(fUseImm)
		pInstr->pSImm5 = pSImm5->Copy();
	return pInstr;
}

void LC3bISA::AddInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	if(fUseImm && StartIter != EndIter)
	{
		//Void leaves pre-initialization as is
		if((*StartIter)->NumberType != NumVoid)
		{
			delete pSImm5;
			pSImm5 = (*StartIter)->Copy();
		}
		StartIter++;
	}
}

bool LC3bISA::AddInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Add.Opcode = vOpcodes[Opcode];
	Instr.Add.DR = DR;
	Instr.Add.SR1 = SR1;
	if(fUseImm)
	{
		Instr.Add.UseImm = 1;
		uint64 TempInt64;
		sprintf(sMessageBuffer, " for %.31s instruction immediate field", sOpcodes[Opcode]);
		if(!pSImm5->Int(5, true, TempInt64, CallBack, sMessageBuffer))
			fRetVal = false;
		Instr.AddImm.Imm5 = TempInt64;
	}
	else
	{
		Instr.Add.UseImm = 0;
		Instr.Add.Zeros = 0;
		Instr.Add.SR2 = SR2;
	}

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::AddInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(SR1);
	if(!fUseImm)
	{
		vReg.push_back(SR2);
		return 2;
	}
	return 1;
}

unsigned int LC3bISA::AddInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(DR);
	vReg.push_back(PSR);
	return 2;
}

LC3bISA::AddInstr::operator const char *() const
{
	static string sElem;
	ostrstream strElement;

	strElement << sOpcodes[Opcode] << "\t" << sRegisters[DR] << ", " << sRegisters[SR1] << ", " << (fUseImm ? (const char *)*pSImm5 : sRegisters[SR2]) << ends;

	sElem = strElement.str();
	return sElem.c_str();
}

/******************************************************************************\
|**********************************   AND    **********************************|
\******************************************************************************/

LC3bISA::AndInstr::AndInstr(const LocationVector &LocationStack, RegisterEnum dr, RegisterEnum sr1, RegisterEnum sr2) : Instruction(LocationStack)
{
	Opcode = AND;
	DR = dr;
	SR1 = sr1;
	SR2 = sr2;
	fUseImm = false;
	pSImm5 = NULL;
	Size = 2;
	Length = 1;
}

LC3bISA::AndInstr::AndInstr(const LocationVector &LocationStack, RegisterEnum dr, RegisterEnum sr1, Number *psimm5) : Instruction(LocationStack)
{
	Opcode = AND;
	DR = dr;
	SR1 = sr1;
	fUseImm = true;
	pSImm5 = psimm5;
	Size = 2;
	Length = 1;
}

LC3bISA::AndInstr::~AndInstr()
{
	if(fUseImm)
		delete pSImm5;
}

Element *LC3bISA::AndInstr::Copy() const
{
	AndInstr *pInstr = new AndInstr(*this);
	if(fUseImm)
		pInstr->pSImm5 = pSImm5->Copy();
	return pInstr;
}

void LC3bISA::AndInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	if(fUseImm && StartIter != EndIter)
	{
		//Void leaves pre-initialization as is
		if((*StartIter)->NumberType != NumVoid)
		{
			delete pSImm5;
			pSImm5 = (*StartIter)->Copy();
		}
		StartIter++;
	}
}

bool LC3bISA::AndInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.And.Opcode = vOpcodes[Opcode];
	Instr.And.DR = DR;
	Instr.And.SR1 = SR1;
	if(fUseImm)
	{
		Instr.And.UseImm = 1;
		uint64 TempInt64;
		sprintf(sMessageBuffer, " for %.31s instruction immediate field", sOpcodes[Opcode]);
		if(!pSImm5->Int(5, true, TempInt64, CallBack, sMessageBuffer))
			fRetVal = false;
		Instr.AndImm.Imm5 = TempInt64;
	}
	else
	{
		Instr.And.UseImm = 0;
		Instr.And.Zeros = 0;
		Instr.And.SR2 = SR2;
	}

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::AndInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(SR1);
	if(!fUseImm)
	{
		vReg.push_back(SR2);
		return 2;
	}
	return 1;
}

unsigned int LC3bISA::AndInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(DR);
	vReg.push_back(PSR);
	return 2;
}

LC3bISA::AndInstr::operator const char *() const
{
	static string sElem;
	ostrstream strElement;

	strElement << sOpcodes[Opcode] << "\t" << sRegisters[DR] << ", " << sRegisters[SR1] << ", " << (fUseImm ? (const char *)*pSImm5 : sRegisters[SR2]) << ends;

	sElem = strElement.str();
	return sElem.c_str();
}

/******************************************************************************\
|**********************************    BR    **********************************|
\******************************************************************************/

LC3bISA::BrInstr::BrInstr(const LocationVector &LocationStack, OpcodeEnum Instr, Number *psoffset9) : Instruction(LocationStack)
{
	if(Instr < BR || Instr > BRnzp)
		throw "Invalid instruction type given to BrInstr!";
	Opcode = Instr;	
	pSOffset9 = psoffset9;
	Size = 2;
	Length = 1;
}

LC3bISA::BrInstr::~BrInstr()
{
	delete pSOffset9;
}

Element *LC3bISA::BrInstr::Copy() const
{
	BrInstr *pInstr = new BrInstr(*this);
	pInstr->pSOffset9 = pSOffset9->Copy();
	return pInstr;
}

void LC3bISA::BrInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	if(StartIter != EndIter)
	{
		//Void leaves pre-initialization as is
		if((*StartIter)->NumberType != NumVoid)
		{
			delete pSOffset9;
			pSOffset9 = (*StartIter)->Copy();
		}
		StartIter++;
	}
}

bool LC3bISA::BrInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Br.Opcode = vOpcodes[Opcode];
	Instr.Br.N = (Opcode == BR || Opcode == BRn || Opcode == BRnz || Opcode == BRnp || Opcode == BRnzp) ? 1 : 0;
	Instr.Br.Z = (Opcode == BR || Opcode == BRz || Opcode == BRnz || Opcode == BRzp || Opcode == BRnzp) ? 1 : 0;
	Instr.Br.P = (Opcode == BR || Opcode == BRp || Opcode == BRnp || Opcode == BRzp || Opcode == BRnzp) ? 1 : 0;

	//Get the offset
	uint64 TempInt64;
	sprintf(sMessageBuffer, " for %.31s instruction offset field", sOpcodes[Opcode]);
	if(!pSOffset9->Int(9, true, TempInt64, CallBack, sMessageBuffer, true, 1, false, true, Address+2))
		fRetVal = false;
	Instr.Br.Offset9 = TempInt64;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::BrInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(PSR);
	return 1;
}

bool LC3bISA::BrInstr::IsBranch() const
{
	return true;
}

LC3bISA::BrInstr::operator const char *() const
{
	static string sElem;
	ostrstream strElement;

	strElement << sOpcodes[Opcode] << "\t" << (const char *)*pSOffset9 << ends;

	sElem = strElement.str();
	return sElem.c_str();
}

/******************************************************************************\
|**********************************   JSR    **********************************|
\******************************************************************************/

LC3bISA::JsrInstr::JsrInstr(const LocationVector &LocationStack, Number *psoffset11) : Instruction(LocationStack)
{
	Opcode = JSR;	
	pSOffset11 = psoffset11;
	Size = 2;
	Length = 1;
}

LC3bISA::JsrInstr::~JsrInstr()
{
	delete pSOffset11;
}

Element *LC3bISA::JsrInstr::Copy() const
{
	JsrInstr *pInstr = new JsrInstr(*this);
	pInstr->pSOffset11 = pSOffset11->Copy();
	return pInstr;
}

void LC3bISA::JsrInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	if(StartIter != EndIter)
	{
		//Void leaves pre-initialization as is
		if((*StartIter)->NumberType != NumVoid)
		{
			delete pSOffset11;
			pSOffset11 = (*StartIter)->Copy();
		}
		StartIter++;
	}
}

bool LC3bISA::JsrInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Jsr.Opcode = vOpcodes[Opcode];
	Instr.Jsr.JsrType = 1;

	//Get the offset
	uint64 TempInt64;
	sprintf(sMessageBuffer, " for %.31s instruction offset field", sOpcodes[Opcode]);
	if(!pSOffset11->Int(11, true, TempInt64, CallBack, sMessageBuffer, true, 1, false, true, Address+2))
		fRetVal = false;
	Instr.Jsr.Offset11 = TempInt64;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::JsrInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(R7);
	return 1;
}

bool LC3bISA::JsrInstr::IsBranch() const
{
	return true;
}

LC3bISA::JsrInstr::operator const char *() const
{
	static string sElem;
	ostrstream strElement;

	strElement << sOpcodes[Opcode] << "\t" << (const char *)*pSOffset11 << ends;

	sElem = strElement.str();
	return sElem.c_str();
}

/******************************************************************************\
|**********************************   JSRR    **********************************|
\******************************************************************************/

LC3bISA::JsrrInstr::JsrrInstr(const LocationVector &LocationStack, RegisterEnum baser) : Instruction(LocationStack)
{
	Opcode = JSRR;
	BaseR = baser;
	Size = 2;
	Length = 1;
}

LC3bISA::JsrrInstr::~JsrrInstr()
{
}

Element *LC3bISA::JsrrInstr::Copy() const
{
	JsrrInstr *pInstr = new JsrrInstr(*this);
	return pInstr;
}

void LC3bISA::JsrrInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
}

bool LC3bISA::JsrrInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Jsrr.Opcode = vOpcodes[Opcode];
	Instr.Jsrr.JsrType = 0;
	Instr.Jsrr.Zero2 = 0;
	Instr.Jsrr.BaseR = BaseR;
	Instr.Jsrr.Zero6 = 0;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::JsrrInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(R7);
	return 1;
}

unsigned int LC3bISA::JsrrInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(BaseR);
	return 1;
}

bool LC3bISA::JsrrInstr::IsBranch() const
{
	return true;
}

LC3bISA::JsrrInstr::operator const char *() const
{
	sprintf(sElement, "%.31s\t%.15s", sOpcodes[Opcode], sRegisters[BaseR]);
	return sElement;
}

/******************************************************************************\
|**********************************   TRAP   **********************************|
\******************************************************************************/

LC3bISA::TrapInstr::TrapInstr(const LocationVector &LocationStack, Number *ptrapvect8) : Instruction(LocationStack)
{
	Opcode = TRAP;	
	pTrapVect8 = ptrapvect8;
	Size = 2;
	Length = 1;
}

LC3bISA::TrapInstr::~TrapInstr()
{
	delete pTrapVect8;
}

Element *LC3bISA::TrapInstr::Copy() const
{
	TrapInstr *pInstr = new TrapInstr(*this);
	pInstr->pTrapVect8 = pTrapVect8->Copy();
	return pInstr;
}

void LC3bISA::TrapInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	if(StartIter != EndIter)
	{
		//Void leaves pre-initialization as is
		if((*StartIter)->NumberType != NumVoid)
		{
			delete pTrapVect8;
			pTrapVect8 = (*StartIter)->Copy();
		}
		StartIter++;
	}
}

bool LC3bISA::TrapInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Trap.Opcode = vOpcodes[Opcode];
	Instr.Trap.Zeros = 0;

	//Get the address
	uint64 TempInt64;
	sprintf(sMessageBuffer, " for %.31s instruction vector field", sOpcodes[Opcode]);
	if(!pTrapVect8->Int(8, false, TempInt64, CallBack, sMessageBuffer, true, 1, false, true, 0))
		fRetVal = false;
	Instr.Trap.TrapVect8 = TempInt64;

	if(Instr.Trap.TrapVect8 < 0x20)
	{
		sprintf(sMessageBuffer, "%.31s instruction vector should be on or after memory byte location 4x40 (vector 4x20).", sOpcodes[Opcode]);
		CallBack(Warning, sMessageBuffer, LocationStack);
	}

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::TrapInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(R7);
	return 1;
}

bool LC3bISA::TrapInstr::IsBranch() const
{
	return true;
}

bool LC3bISA::TrapInstr::IsMemory() const
{
	return true;
}

LC3bISA::TrapInstr::operator const char *() const
{
	static string sElem;
	ostrstream strElement;

	strElement << sOpcodes[Opcode] << "\t" << (const char *)*pTrapVect8 << ends;

	sElem = strElement.str();
	return sElem.c_str();
}

/******************************************************************************\
|**********************************   JMP    **********************************|
\******************************************************************************/

LC3bISA::JmpInstr::JmpInstr(const LocationVector &LocationStack, RegisterEnum baser) : Instruction(LocationStack)
{
	Opcode = JMP;
	BaseR = baser;
	Size = 2;
	Length = 1;
}

LC3bISA::JmpInstr::~JmpInstr()
{
}

Element *LC3bISA::JmpInstr::Copy() const
{
	JmpInstr *pInstr = new JmpInstr(*this);
	return pInstr;
}

void LC3bISA::JmpInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
}

bool LC3bISA::JmpInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Jmp.Opcode = vOpcodes[Opcode];
	Instr.Jmp.Zero3 = 0;
	Instr.Jmp.BaseR = BaseR;
	Instr.Jmp.Zero6 = 0;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

bool LC3bISA::JmpInstr::IsBranch() const
{
	return true;
}

LC3bISA::JmpInstr::operator const char *() const
{
	sprintf(sElement, "%.31s\t%.15s", sOpcodes[Opcode], sRegisters[BaseR]);
	return sElement;
}

/******************************************************************************\
|**********************************   LEA    **********************************|
\******************************************************************************/

LC3bISA::LeaInstr::LeaInstr(const LocationVector &LocationStack, RegisterEnum dr, Number *psoffset9) : Instruction(LocationStack)
{
	Opcode = LEA;
	DR = dr;
	pSOffset9 = psoffset9;
	Size = 2;
	Length = 1;
}

LC3bISA::LeaInstr::~LeaInstr()
{
	delete pSOffset9;
}

Element *LC3bISA::LeaInstr::Copy() const
{
	LeaInstr *pInstr = new LeaInstr(*this);
	pInstr->pSOffset9 = pSOffset9->Copy();
	return pInstr;
}

void LC3bISA::LeaInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	if(StartIter != EndIter)
	{
		//Void leaves pre-initialization as is
		if((*StartIter)->NumberType != NumVoid)
		{
			delete pSOffset9;
			pSOffset9 = (*StartIter)->Copy();
		}
		StartIter++;
	}
}

bool LC3bISA::LeaInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Lea.Opcode = vOpcodes[Opcode];
	Instr.Lea.DR = DR;

	//Get the offset
	uint64 TempInt64;
	sprintf(sMessageBuffer, " for %.31s instruction offset field", sOpcodes[Opcode]);
	if(!pSOffset9->Int(9, true, TempInt64, CallBack, sMessageBuffer, true, 1, false, true, Address+2))
		fRetVal = false;
	Instr.Lea.Offset9 = TempInt64;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::LeaInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(DR);
	vReg.push_back(PSR);
	return 2;
}

LC3bISA::LeaInstr::operator const char *() const
{
	static string sElem;
	ostrstream strElement;

	strElement << sOpcodes[Opcode] << "\t" << sRegisters[DR] << ", " << (const char *)*pSOffset9 << ends;

	sElem = strElement.str();
	return sElem.c_str();
}

/******************************************************************************\
|**********************************    LD    **********************************|
\******************************************************************************/

LC3bISA::LdrInstr::LdrInstr(const LocationVector &LocationStack, OpcodeEnum Instr, RegisterEnum dr, RegisterEnum baser, Number *psoffset6) : Instruction(LocationStack)
{
	if(Instr < LDB || Instr > LDR)
		throw "Invalid instruction type given to LdrInstr!";
	Opcode = Instr;
	DR = dr;
	BaseR = baser;
	pSOffset6 = psoffset6;
	Size = 2;
	Length = 1;
}

LC3bISA::LdrInstr::~LdrInstr()
{
	delete pSOffset6;
}

Element *LC3bISA::LdrInstr::Copy() const
{
	LdrInstr *pInstr = new LdrInstr(*this);
	pInstr->pSOffset6 = pSOffset6->Copy();
	return pInstr;
}

void LC3bISA::LdrInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	if(StartIter != EndIter)
	{
		//Void leaves pre-initialization as is
		if((*StartIter)->NumberType != NumVoid)
		{
			delete pSOffset6;
			pSOffset6 = (*StartIter)->Copy();
		}
		StartIter++;
	}
}

bool LC3bISA::LdrInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Ld.Opcode = vOpcodes[Opcode];
	Instr.Ld.DR = DR;
	Instr.Ld.BaseR = BaseR;

	//Get Index
	sprintf(sMessageBuffer, " for %.31s instruction index field", sOpcodes[Opcode]);
	uint64 TempInt64;
	if(!pSOffset6->Int(6, true, TempInt64, CallBack, sMessageBuffer, true, Opcode == LDB ? 0 : 1, true))
		fRetVal = false;
	Instr.Ld.Offset6 = TempInt64;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::LdrInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(BaseR);
	return 1;
}

unsigned int LC3bISA::LdrInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(DR);
	vReg.push_back(PSR);
	return 2;
}

bool LC3bISA::LdrInstr::IsMemory() const
{
	return true;
}

LC3bISA::LdrInstr::operator const char *() const
{
	static string sElem;
	ostrstream strElement;

	strElement << sOpcodes[Opcode] << "\t" << sRegisters[DR] << ", " << sRegisters[BaseR] << ", " << (const char *)*pSOffset6 << ends;

	sElem = strElement.str();
	return sElem.c_str();
}

/******************************************************************************\
|**********************************    ST    **********************************|
\******************************************************************************/

LC3bISA::StrInstr::StrInstr(const LocationVector &LocationStack, OpcodeEnum Instr, RegisterEnum sr, RegisterEnum baser, Number *psoffset6) : Instruction(LocationStack)
{
	if(Instr < STB || Instr > STR)
		throw "Invalid instruction type given to StrInstr!";
	Opcode = Instr;
	SR = sr;
	BaseR = baser;
	pSOffset6 = psoffset6;
	Size = 2;
	Length = 1;
}

LC3bISA::StrInstr::~StrInstr()
{
	delete pSOffset6;
}

Element *LC3bISA::StrInstr::Copy() const
{
	StrInstr *pInstr = new StrInstr(*this);
	pInstr->pSOffset6 = pSOffset6->Copy();
	return pInstr;
}

void LC3bISA::StrInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	if(StartIter != EndIter)
	{
		//Void leaves pre-initialization as is
		if((*StartIter)->NumberType != NumVoid)
		{
			delete pSOffset6;
			pSOffset6 = (*StartIter)->Copy();
		}
		StartIter++;
	}
}

bool LC3bISA::StrInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.St.Opcode = vOpcodes[Opcode];
	Instr.St.SR = SR;
	Instr.St.BaseR = BaseR;

	//Get Index
	sprintf(sMessageBuffer, " for %.31s instruction index field", sOpcodes[Opcode]);
	uint64 TempInt64;
	if(!pSOffset6->Int(6, true, TempInt64, CallBack, sMessageBuffer, true, Opcode == STB ? 0 : 1, true))
		fRetVal = false;
	Instr.St.Offset6 = TempInt64;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::StrInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(SR);
	vReg.push_back(BaseR);
	return 2;
}

bool LC3bISA::StrInstr::IsMemory() const
{
	return true;
}

LC3bISA::StrInstr::operator const char *() const
{
	static string sElem;
	ostrstream strElement;

	strElement << sOpcodes[Opcode] << "\t" << sRegisters[SR] << ", " << sRegisters[BaseR] << ", " << (const char *)*pSOffset6 << ends;

	sElem = strElement.str();
	return sElem.c_str();
}

/******************************************************************************\
|**********************************   SHF    **********************************|
\******************************************************************************/

LC3bISA::ShfInstr::ShfInstr(const LocationVector &LocationStack, OpcodeEnum Instr, RegisterEnum dr, RegisterEnum sr, Number *pimm4) : Instruction(LocationStack)
{
	if(Instr < LSHF || Instr > RSHFA)
		throw "Invalid instruction type given to ShfInstr!";
	Opcode = Instr;
	DR = dr;
	SR = sr;
	pImm4 = pimm4;
	Size = 2;
	Length = 1;
}

LC3bISA::ShfInstr::~ShfInstr()
{
	delete pImm4;
}

Element *LC3bISA::ShfInstr::Copy() const
{
	ShfInstr *pInstr = new ShfInstr(*this);
	pInstr->pImm4 = pImm4->Copy();
	return pInstr;
}

void LC3bISA::ShfInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	if(StartIter != EndIter)
	{
		//Void leaves pre-initialization as is
		if((*StartIter)->NumberType != NumVoid)
		{
			delete pImm4;
			pImm4 = (*StartIter)->Copy();
		}
		StartIter++;
	}
}

bool LC3bISA::ShfInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Shf.Opcode = vOpcodes[Opcode];
	Instr.Shf.DR = DR;
	Instr.Shf.SR = SR;
	Instr.Shf.A = Opcode == RSHFL ? 0 : 1;
	Instr.Shf.D = Opcode == LSHF ? 0 : 1;
	sprintf(sMessageBuffer, " for %.31s instruction shamt field", sOpcodes[Opcode]);
	uint64 TempInt64;
	if(!pImm4->Int(4, false, TempInt64, CallBack, sMessageBuffer))
		fRetVal = false;
	Instr.Shf.Imm4 = TempInt64;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::ShfInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(SR);
	return 1;
}

unsigned int LC3bISA::ShfInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(DR);
	vReg.push_back(PSR);
	return 2;
}

LC3bISA::ShfInstr::operator const char *() const
{
	static string sElem;
	ostrstream strElement;

	strElement << sOpcodes[Opcode] << "\t" << sRegisters[DR] << ", " << sRegisters[SR] << ", " << (const char *)*pImm4 << ends;

	sElem = strElement.str();
	return sElem.c_str();
}

/******************************************************************************\
|**********************************   NOT    **********************************|
\******************************************************************************/

LC3bISA::NotInstr::NotInstr(const LocationVector &LocationStack, RegisterEnum dr, RegisterEnum sr) : Instruction(LocationStack)
{
	Opcode = NOT;
	DR = dr;
	SR = sr;
	Size = 2;
	Length = 1;
}

Element *LC3bISA::NotInstr::Copy() const
{
	NotInstr *pInstr = new NotInstr(*this);
	return pInstr;
}

void LC3bISA::NotInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
}

bool LC3bISA::NotInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Not.Opcode = vOpcodes[Opcode];
	Instr.Not.DR = DR;
	Instr.Not.SR = SR;
	Instr.Not.Ones = (Word)-1;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::NotInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(SR);
	return 1;
}

unsigned int LC3bISA::NotInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(DR);
	vReg.push_back(PSR);
	return 2;
}

LC3bISA::NotInstr::operator const char *() const
{
	sprintf(sElement, "%.31s\t%.15s, %.15s", sOpcodes[Opcode], sRegisters[DR], sRegisters[SR]);
	return sElement;
}

/******************************************************************************\
|**********************************   RET    **********************************|
\******************************************************************************/

LC3bISA::RetInstr::RetInstr(const LocationVector &LocationStack) : Instruction(LocationStack)
{
	Opcode = RET;
	Size = 2;
	Length = 1;
}

Element *LC3bISA::RetInstr::Copy() const
{
	RetInstr *pInstr = new RetInstr(*this);
	return pInstr;
}

void LC3bISA::RetInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
}

bool LC3bISA::RetInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Ret.Opcode = vOpcodes[Opcode];
	Instr.Ret.Zero3 = 0;
	Instr.Ret.BaseR = R7;
	Instr.Ret.Zero6 = 0;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::RetInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(R7);
	return 1;
}

bool LC3bISA::RetInstr::IsBranch() const
{
	return true;
}

LC3bISA::RetInstr::operator const char *() const
{
	sprintf(sElement, "%.31s", sOpcodes[Opcode]);
	return sElement;
}

/******************************************************************************\
|**********************************   RTI    **********************************|
\******************************************************************************/

LC3bISA::RtiInstr::RtiInstr(const LocationVector &LocationStack) : Instruction(LocationStack)
{
	Opcode = RTI;
	Size = 2;
	Length = 1;
}

Element *LC3bISA::RtiInstr::Copy() const
{
	RtiInstr *pInstr = new RtiInstr(*this);
	return pInstr;
}

void LC3bISA::RtiInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
}

bool LC3bISA::RtiInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Rti.Opcode = vOpcodes[Opcode];
	Instr.Rti.Zeros = 0;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

unsigned int LC3bISA::RtiInstr::GetSources(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(R6);
	return 1;
}

unsigned int LC3bISA::RtiInstr::GetDestinations(vector<RegisterEnum> &vReg) const
{
	vReg.clear();
	vReg.push_back(R6);
	vReg.push_back(PSR);
	return 2;
}

bool LC3bISA::RtiInstr::IsBranch() const
{
	return true;
}

bool LC3bISA::RtiInstr::IsMemory() const
{
	return true;
}

LC3bISA::RtiInstr::operator const char *() const
{
	sprintf(sElement, "%.31s", sOpcodes[Opcode]);
	return sElement;
}

/******************************************************************************\
|**********************************   NOP    **********************************|
\******************************************************************************/

LC3bISA::NopInstr::NopInstr(const LocationVector &LocationStack) : Instruction(LocationStack)
{
	Opcode = NOP;
	Size = 2;
	Length = 1;
}

Element *LC3bISA::NopInstr::Copy() const
{
	NopInstr *pInstr = new NopInstr(*this);
	return pInstr;
}

void LC3bISA::NopInstr::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
}

bool LC3bISA::NopInstr::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	LC3bInstruction Instr;

	if(Address & 1)
	{
		sprintf(sMessageBuffer, "%.31s instruction address not word aligned.", sOpcodes[Opcode]);
		CallBack(Error, sMessageBuffer, LocationStack);
		fRetVal = false;
	}

	//Generate the instruction
	Instr.Binary = 0;

	//Load the instruction into the RAM
	if(fLittleEndian)
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)Instr.Binary) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)(Instr.Binary >> 8)) );
	}
	else	//Big Endian
	{
		vRam.push_back( RamVector::value_type(Address, (unsigned char)(Instr.Binary >> 8)) );
		vRam.push_back( RamVector::value_type(Address + 1, (unsigned char)Instr.Binary) );
	}

	return fRetVal;
}

LC3bISA::NopInstr::operator const char *() const
{
	sprintf(sElement, "%.31s", sOpcodes[Opcode]);
	return sElement;
}

}	//namespace LC3b
