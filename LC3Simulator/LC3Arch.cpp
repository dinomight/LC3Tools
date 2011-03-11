//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "LC3Arch.h"
#include <cstdio>
#include "../Assembler/Number.h"
#include "../Simulator/Simulator.h"
#pragma warning (disable:4244)

using namespace std;
using namespace JMT;
using namespace Assembler;
using namespace Simulator;

namespace LC3	{

#undef R
#define R(Reg) (*pThis->pR[Reg])
#define CHANGE(Reg) (Reg == TR ? NoEvent : ChangeEvent)

LC3Arch::LC3Arch(ArchSim<LC3ISA> &thesim) : TheSim(thesim)
{
	CreateRegisters();
	CreateMemories();
	CreatePipelines();
	KeyboardInterruptVector = 0x80;
}

bool LC3Arch::Reset(const RamVector &MemoryImage)
{
	*pPC = Flags.fUseOS ? 0x3000 : MemoryImage.begin()->first >> 1;
	*pIR = 0;
	*pPSR = 0x8000;
	*pSSP = 0x3000;
	*pUSP = 0xFE00;
	*pKBSR = 0x0001;
	*pKBDR = 0;
	*pDSR = 0x8000;
	*pDDR = 0;
	*pMCR = 0x8000;
	for(int i = 0; i < 8; i++)
		*pR[i] = 0;
	*pR[6] = 0xE000;
	InterruptList.clear();

	pDRAM->Clear();
	if(!pDRAM->Write(MemoryImage, TheSim.SimCallBack))
		return false;

	//The first instruction will otherwise not be registered as an event,
	//since it has to be registered prior to the cycle it occurs 
	TheSim.InstructionEvent(*pPC << LC3ISA::Addressability);

	return true;
}

uint64 LC3Arch::NextInstruction()
{
	return *pPC << LC3ISA::Addressability;
}

bool LC3Arch::DataRead(RamVector &MemoryImage, uint64 Address, uint64 Length)
{
	return pDRAM->Read(MemoryImage, Address, Length, TheSim.SimCallBack);
}

bool LC3Arch::DataWrite(const RamVector &MemoryImage)
{
	return pDRAM->Write(MemoryImage, TheSim.SimCallBack);
}

bool LC3Arch::CreateRegisters()
{
	pair<RegisterSetMap::iterator, bool> RegSetIter;
	RegSetIter = RegisterSets.insert( RegisterSetMap::value_type("control", RegisterSet("control")) );
	pPC = &RegSetIter.first->second.AddRegister("pc", 8*sizeof(LC3ISA::Word));
	pIR = &RegSetIter.first->second.AddRegister("ir", 8*sizeof(LC3ISA::Word));
	pPSR = &RegSetIter.first->second.AddRegister("psr", 8*sizeof(LC3ISA::Word));
	pSSP = &RegSetIter.first->second.AddRegister("ssp", 8*sizeof(LC3ISA::Word));
	pUSP = &RegSetIter.first->second.AddRegister("usp", 8*sizeof(LC3ISA::Word));

	RegSetIter = RegisterSets.insert( RegisterSetMap::value_type("mmio", RegisterSet("mmio")) );
	pKBSR = &RegSetIter.first->second.AddRegister("kbsr", 8*sizeof(LC3ISA::Word));
	pKBDR = &RegSetIter.first->second.AddRegister("kbdr", 8*sizeof(LC3ISA::Word));
	pDSR = &RegSetIter.first->second.AddRegister("dsr", 8*sizeof(LC3ISA::Word));
	pDDR = &RegSetIter.first->second.AddRegister("ddr", 8*sizeof(LC3ISA::Word));
	pMCR = &RegSetIter.first->second.AddRegister("mcr", 8*sizeof(LC3ISA::Word));

	RegSetIter = RegisterSets.insert( RegisterSetMap::value_type("regfile", RegisterSet("regfile")) );
	vector<Register *> vRegs = RegSetIter.first->second.AddRegisterArray("r", 8*sizeof(LC3ISA::Word), 8);
	for(int i = 0; i < 8; i++)
		pR[i] = vRegs[i];

	return true;
}

bool LC3Arch::CreateMemories()
{
	vector<short> BitDivisions;
	BitDivisions.push_back(-48);
	BitDivisions.push_back(8);
	BitDivisions.push_back(8);

	pair<MemoryMap::iterator, bool> MemIter;
	MemIter = Memories.insert( MemoryMap::value_type("dram", Memory("dram", 0, LC3ISA::MaxAddress, BitDivisions)) );
	pDRAM = &MemIter.first->second;
	return true;
}

bool LC3Arch::CreatePipelines()
{
	Pipelines.push_back( Pipeline("datapath") );
	Pipelines[0].AddStage("rtl", DataPath);
	return true;
}

bool LC3Arch::DataPath(Architecture *pArch)
{
	//Make nice aliases
	LC3Arch *pThis = reinterpret_cast<LC3Arch *>(pArch);
	ArchSim<LC3ISA> &TheSim = pThis->TheSim;
	Register &PC = *pThis->pPC, &IR = *pThis->pIR, &PSR = *pThis->pPSR, &SSP = *pThis->pSSP, &USP = *pThis->pUSP, &MCR = *pThis->pMCR, TR("", 64), TPC("", 64);
	Memory &DRAM = *pThis->pDRAM;
	LC3ISA::Word Address, PreviousPC = PC, Temp;
	LC3ISA::LC3Instruction Instr;

	//Make sure the clock is started.
	if(MCR[15] != 1)
	{
		TheSim.SimCallBack(Info, "Restarted the clock.");
		MCR.SetBit(15, 1);
		TheSim.RegisterEvent("mmio", MCR, (EventEnum)(WriteEvent | ValueEvent | ChangeEvent));
	}
	TheSim.RegisterEvent("mmio", MCR, ReadEvent);

	//Get the next instruction
	TR = IR; IR = Instr.Binary = pThis->DataRead(PC, 2, 16);
	TheSim.RegisterEvent("control", IR, (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(IR)));
	TPC = PC;

	//Check for interrupts
	if(pThis->ProcessInterrupt())
		return true;

	//Process the instruction
	switch(Instr.Base.Opcode)
	{
	case 0x1:	//vOpcodes[ADD]:
		if(Instr.Add.UseImm)
		{
			TR = R(Instr.Add.DR); R(Instr.Add.DR) = R(Instr.Add.SR1) + SEXT(Instr.AddImm.Imm5, 5);
			TheSim.RegisterEvent("regfile", R(Instr.Add.SR1), ReadEvent);
		}
		else
		{
			if(Instr.Add.Zeros)
				goto Default;
			TR = R(Instr.Add.DR); R(Instr.Add.DR) = R(Instr.Add.SR1) + R(Instr.Add.SR2);
			TheSim.RegisterEvent("regfile", R(Instr.Add.SR1), ReadEvent);
			TheSim.RegisterEvent("regfile", R(Instr.Add.SR2), ReadEvent);
		}
		TheSim.RegisterEvent("regfile", R(Instr.Add.DR), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(Instr.Add.DR))));

		TR = PSR; pThis->SetCC(R(Instr.Add.DR));
		TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));

		PC = PC + 1;
		break;

	case 0x5:	//vOpcodes[AND]:
		if(Instr.And.UseImm)
		{
			TR = R(Instr.And.DR); R(Instr.And.DR) = R(Instr.And.SR1) & SEXT(Instr.AndImm.Imm5, 5);
			TheSim.RegisterEvent("regfile", R(Instr.And.SR1), ReadEvent);
		}
		else
		{
			if(Instr.And.Zeros)
				goto Default;
			TR = R(Instr.And.DR); R(Instr.And.DR) = R(Instr.And.SR1) & R(Instr.And.SR2);
			TheSim.RegisterEvent("regfile", R(Instr.And.SR1), ReadEvent);
			TheSim.RegisterEvent("regfile", R(Instr.And.SR2), ReadEvent);
		}
		TheSim.RegisterEvent("regfile", R(Instr.And.DR), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(Instr.And.DR))));

		TR = PSR; pThis->SetCC(R(Instr.And.DR));
		TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));

		PC = PC + 1;
		break;

	case 0x0:	//vOpcodes[BR]:
		bool fBranch;
		fBranch = false;
		if(Instr.Br.N)
		{
			if(Instr.Br.Z)
			{
				if(Instr.Br.P)
				{
					if(PSR[N] | PSR[Z] | PSR[P])
						fBranch = true;
				}
				else
				{
					if(PSR[N] | PSR[Z])
						fBranch = true;
				}
			}
			else
			{
				if(Instr.Br.P)
				{
					if(PSR[N] | PSR[P])
						fBranch = true;
				}
				else
				{
					if(PSR[N])
						fBranch = true;
				}
			}
		}
		else
		{
			if(Instr.Br.Z)
			{
				if(Instr.Br.P)
				{
					if(PSR[Z] | PSR[P])
						fBranch = true;
				}
				else
				{
					if(PSR[Z])
						fBranch = true;
				}
			}
			else
			{
				if(Instr.Br.P)
				{
					if(PSR[P])
						fBranch = true;
				}
				else
				{
					//This is a NOP
					fBranch = false;
				}
			}
		}
		TheSim.RegisterEvent("control", PSR, ReadEvent);

		if(fBranch)
			PC = PC + 1 + SEXT(Instr.Br.Offset9, 9);
		else
			PC = PC + 1;
		break;

	case 0x4:	//vOpcodes[JSR]:
		if(Instr.Jsr.JsrType)
		{	//JSR
			TR = R(7); R(7) = PC + 1;
			TheSim.RegisterEvent("regfile", R(7), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(7))));
			PC = PC + 1 + SEXT(Instr.Jsr.Offset11, 11);
			TheSim.SubInEvent(TPC, PC);
		}
		else
		{	//JSRR
			if(Instr.Jsrr.Zero2 || Instr.Jsrr.Zero6)
				goto Default;

			TR = R(7); R(7) = PC + 1;
			TheSim.RegisterEvent("regfile", R(7), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(7))));
			PC = R(Instr.Jsrr.BaseR);
			TheSim.RegisterEvent("regfile", R(Instr.Jsrr.BaseR), ReadEvent);
			TheSim.SubInEvent(TPC, PC);
		}
		break;

	case 0xF:	//vOpcodes[TRAP]:
		if(Instr.Trap.Zeros)
			goto Default;

		TR = R(7); R(7) = PC + 1;
		TheSim.RegisterEvent("regfile", R(7), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(7))));

		PC = pThis->DataRead(Instr.Trap.TrapVect8, 2, 16);
		TheSim.SubInEvent(TPC, PC);
		break;

	case 0xC:	//vOpcodes[JMP]:
		if(Instr.Jmp.Zero3 || Instr.Jmp.Zero6)
			goto Default;

		if(Instr.Jmp.BaseR == LC3ISA::R7)
		{	//Ret
			PC = R(7);
			TheSim.RegisterEvent("regfile", R(7), ReadEvent);
			TheSim.SubOutEvent();
			break;
		}

		PC = R(Instr.Jmp.BaseR);
		TheSim.RegisterEvent("regfile", R(Instr.Jmp.BaseR), ReadEvent);
		break;

	case 0xE:	//vOpcodes[LEA]:
		TR = R(Instr.Lea.DR); R(Instr.Lea.DR) = PC + 1 + SEXT(Instr.Lea.Offset9, 9);
		TheSim.RegisterEvent("regfile", R(Instr.Lea.DR), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(Instr.Lea.DR))));

		TR = PSR; pThis->SetCC(R(Instr.Lea.DR));
		TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));

		PC = PC + 1;
		break;

	case 0x2:	//vOpcodes[LD]:
		TR = R(Instr.Ld.DR); R(Instr.Ld.DR) = pThis->DataRead(PC + 1 + SEXT(Instr.Ld.Offset9, 9), 2, 16);
		TheSim.RegisterEvent("regfile", R(Instr.Ld.DR), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(Instr.Ld.DR))));

		TR = PSR; pThis->SetCC(R(Instr.Ld.DR));
		TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));

		PC = PC + 1;
		break;

	case 0xA:	//vOpcodes[LDI]:
		Address = pThis->DataRead(PC + 1 + SEXT(Instr.Ld.Offset9, 9), 2, 16);

		TR = R(Instr.Ld.DR); R(Instr.Ld.DR) = pThis->DataRead(Address, 2, 16);
		TheSim.RegisterEvent("regfile", R(Instr.Ld.DR), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(Instr.Ld.DR))));

		TR = PSR; pThis->SetCC(R(Instr.Ld.DR));
		TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));

		PC = PC + 1;
		break;

	case 0x6:	//vOpcodes[LDR]:
		TR = R(Instr.Ldr.DR); R(Instr.Ldr.DR) = pThis->DataRead(R(Instr.Ldr.BaseR) + SEXT(Instr.Ldr.Offset6, 6), 2, 16);
		TheSim.RegisterEvent("regfile", R(Instr.Ldr.BaseR), ReadEvent);
		TheSim.RegisterEvent("regfile", R(Instr.Ldr.DR), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(Instr.Ldr.DR))));

		TR = PSR; pThis->SetCC(R(Instr.Ldr.DR));
		TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));

		PC = PC + 1;
		break;

	case 0x3:	//vOpcodes[ST]:
		pThis->DataWrite(PC + 1 + SEXT(Instr.St.Offset9, 9), 2, R(Instr.St.SR), 16);
		TheSim.RegisterEvent("regfile", R(Instr.St.SR), ReadEvent);

		PC = PC + 1;
		break;

	case 0xB:	//vOpcodes[STI]:
		Address = pThis->DataRead(PC + 1 + SEXT(Instr.St.Offset9, 9), 2, 16);

		pThis->DataWrite(Address, 2, R(Instr.St.SR), 16);
		TheSim.RegisterEvent("regfile", R(Instr.St.SR), ReadEvent);

		PC = PC + 1;
		break;

	case 0x7:	//vOpcodes[STR]:
		pThis->DataWrite(R(Instr.Str.BaseR) + SEXT(Instr.Str.Offset6, 6), 2, R(Instr.Str.SR), 16);
		TheSim.RegisterEvent("regfile", R(Instr.Str.BaseR), ReadEvent);
		TheSim.RegisterEvent("regfile", R(Instr.Str.SR), ReadEvent);

		PC = PC + 1;
		break;

	case 0x9:	//vOpcodes[NOT]:
		if(Instr.Not.Ones != 0x3F)
			goto Default;

		TR = R(Instr.Not.DR); R(Instr.Not.DR) = ~R(Instr.Not.SR);
		TheSim.RegisterEvent("regfile", R(Instr.Not.SR), ReadEvent);
		TheSim.RegisterEvent("regfile", R(Instr.Not.DR), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(Instr.Not.DR))));

		TR = PSR; pThis->SetCC(R(Instr.Not.DR));
		TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));

		PC = PC + 1;
		break;

	case 0x8:	//vOpcodes[RTI]:
		if(Instr.Rti.Zeros)
			goto Default;

		if(PSR[15] == 0)
		{
			PC = pThis->DataRead(R(6), 2, 16);
			TR = R(6); R(6) = R(6) + 1;
			TheSim.RegisterEvent("regfile", R(6), (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(R(6))));

			TR = PSR; Temp = pThis->DataRead(R(6), 2, 16);
			TR = R(6); R(6) = R(6) + 1;
			TheSim.RegisterEvent("regfile", R(6), (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(R(6))));
			PSR = Temp;
			TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));
			TheSim.SubOutEvent();

			//Save the supervisor stack
			TR = SSP; SSP = R(6);
			TheSim.RegisterEvent("regfile", R(6), ReadEvent);
			TheSim.RegisterEvent("control", SSP, (EventEnum)(WriteEvent | ValueEvent | CHANGE(SSP)));

			//R6 gets the stack pointer
			if(PSR[15])
			{
				TR = R(6); R(6) = USP;
				TheSim.RegisterEvent("control", USP, ReadEvent);
				TheSim.RegisterEvent("regfile", R(6), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(6))));
			}
			else
			{
				TR = R(6); R(6) = SSP;
				TheSim.RegisterEvent("control", SSP, ReadEvent);
				TheSim.RegisterEvent("regfile", R(6), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(6))));
			}
			TheSim.RegisterEvent("control", PSR, ReadEvent);
			
		}
		else	//Privilege mode exception
		{
			TheSim.RegisterEvent("control", PSR, ReadEvent);
			TheSim.SimCallBack(JMT::Exception, "RTI instruction executed with user privilege.");
			PC = PC + 1;
			pThis->Exception(0);
		}

		break;

	default:
Default:	//This is not a valid instruction
		TheSim.SimCallBack(JMT::Exception, "Invalid instruction.");
		PC = PC + 1;
		pThis->Exception(1);
	}
	//The PC always changes
	TR = TPC;
	TheSim.RegisterEvent("control", PC, (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(PC)));

	//The instruction event for the next instruction is flagged at the end of the preceeding cycle,
	//so that a breakpoint will be able to stop before the registered instruction event
	//happens, rather than after it has happened.
	//The trace file will output registers as they are before the instruction has executed.
	TheSim.InstructionEvent(PC << LC3ISA::Addressability);

	return true;
}

LC3ISA::Word LC3Arch::SEXT(LC3ISA::Word Value, unsigned char Bits)
{
	LC3ISA::Word BitMask = Bits ? ((LC3ISA::Word)1 << (Bits - 1)) : 0;
	if(Value & BitMask)
	{
		LC3ISA::Word SextMask = ~(BitMask - 1);
		return Value | SextMask;
	}
	else
		return Value;
}

void LC3Arch::SetCC(signed short Value)
{
	*pPSR = (*pPSR & ~0x7) | (Value < 0 ? (1 << N) : 0) | (Value == 0 ? (1 << Z) : 0) | (Value > 0 ? (1 << P) : 0);
}

LC3ISA::Word LC3Arch::MakeInt(const RamVector &vData, unsigned char Bits, ArchSim<LC3ISA> &TheSim)
{
	uint64 TempInt64;
	IntegerNumber Int(NullLocationStack, vData, LC3ISA::fLittleEndian);
	Int.Int(Bits, false, TempInt64, TheSim.CallBack);
	return TempInt64;
}

bool LC3Arch::Exception(uint64 Vector)
{
	LC3Arch *pThis = this;
	Register &PC = *pPC, &IR = *pIR, &PSR = *pPSR, &SSP = *pSSP, &USP = *pUSP, TR("", 64), TPC("", 64);
	Register &KBSR = *pKBSR, &KBDR = *pKBDR, &DSR = *pDSR, &DDR = *pDDR, &MCR = *pMCR;

	if(Vector & 0x80)
	{
		throw "Exception vector must be in the range 4x00 to 4x7F!";
		return false;
	}

	switch(Vector)
	{
	case 0:
		//Privilege mode violation
		break;

	case 1:
		//Illegal instruction
		break;

	default:
		throw "Exception undefined!";
	}

	//Convert a vector number into an address
	Vector = (Vector & 0xFF) | 0x100;

	//Save R6/stack pointer
	if(PSR[15])
	{
		TR = USP; USP = R(6);
		TheSim.RegisterEvent("regfile", R(6), ReadEvent);
		TheSim.RegisterEvent("control", USP, (EventEnum)(WriteEvent | ValueEvent | CHANGE(USP)));
	}
	else
	{
		TR = SSP; SSP = R(6);
		TheSim.RegisterEvent("regfile", R(6), ReadEvent);
		TheSim.RegisterEvent("control", SSP, (EventEnum)(WriteEvent | ValueEvent | CHANGE(SSP)));
	}
	TheSim.RegisterEvent("control", PSR, ReadEvent);

	//R6 gets the supervisor stack pointer
	TR = R(6); R(6) = SSP;
	TheSim.RegisterEvent("control", SSP, ReadEvent);
	TheSim.RegisterEvent("regfile", R(6), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(6))));
	
	//Save PSR and PC to the stack
	TR = R(6); R(6) = R(6) + -1;
	TheSim.RegisterEvent("regfile", R(6), (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(R(6))));
	DataWrite(R(6), 2, PSR, 16);
	TheSim.RegisterEvent("control", PSR, ReadEvent);
	TR = R(6); R(6) = R(6) + -1;
	TheSim.RegisterEvent("regfile", R(6), (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(R(6))));
	DataWrite(R(6), 2, PC, 16);
	TheSim.RegisterEvent("control", PC, ReadEvent);

	//Update PSR
	TR = PSR; PSR = PSR & 700;
	TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));

	//Load the handler address into the PC
	TR = PC; PC = DataRead(Vector, 2, 16);
	TheSim.RegisterEvent("control", PC, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PC)));
	TheSim.SubInEvent(TR, PC);

	TheSim.Exception();
	TheSim.InstructionEvent(PC << LC3ISA::Addressability);
	return true;
}

bool LC3Arch::Interrupt(uint64 Vector)
{
	if( !(Vector & 0x80) || Vector > 0x1ff )
	{
		TheSim.SimCallBack(Error, "Interrupt vector must be in the range 4x80 to 4xFF.");
		return false;
	}
	
	InterruptList.push_back(Vector);
	return true;
}

bool LC3Arch::ProcessInterrupt()
{
	LC3Arch *pThis = this;
	Register &PC = *pPC, &IR = *pIR, &PSR = *pPSR, &SSP = *pSSP, &USP = *pUSP, TR("", 64), TPC("", 64);
	Register &KBSR = *pKBSR, &KBDR = *pKBDR, &DSR = *pDSR, &DDR = *pDDR, &MCR = *pMCR;

	if(InterruptList.empty())
		return false;
	uint64 Vector = *InterruptList.begin();

	STDTYPENAME LC3ISA::Word NewPriority;
	switch(Vector)
	{
	case 0x80:
		//Keyboard interrupt, PL4
		NewPriority = 4;
		if( ((PSR & 0x700) >> 8) >= NewPriority )
		{
			//The priority of the executing program is too high, do not process
			TheSim.RegisterEvent("control", PSR, ReadEvent);
/*			if(TheSim.fCheck)
			{
				sprintf(sMessageBuffer, "Executing program priority level (%u) higher than keyboard interrupt priority level (%u); interrupt not processed.", ((PSR & 0x700) >> 8), NewPriority);
				TheSim.SimCallBack(Info, sMessageBuffer);
			}*/
			return false;
		}
		TheSim.RegisterEvent("control", PSR, ReadEvent);

		if(!KBSR[0])
		{
			//Keyboard interrupt is disabled
			TheSim.RegisterEvent("mmio", KBSR, ReadEvent);
//			if(TheSim.fCheck)
//				TheSim.SimCallBack(Info, "Keyboard interrupt is disabled; interrupt not processed.");
			return false;
		}
		TheSim.RegisterEvent("mmio", KBSR, ReadEvent);
		break;

	default:
		sprintf(sMessageBuffer, "Interrupt vector 4x%x undefined.", (STDTYPENAME LC3ISA::Word)Vector);
		TheSim.SimCallBack(Error, sMessageBuffer);
		InterruptList.pop_front();
		return false;
	}
	InterruptList.pop_front();

	//Convert a vector number into an address
	Vector = (Vector & 0xFF) | 0x100;

	//Save R6/stack pointer
	if(PSR[15])
	{
		TR = USP; USP = R(6);
		TheSim.RegisterEvent("regfile", R(6), ReadEvent);
		TheSim.RegisterEvent("control", USP, (EventEnum)(WriteEvent | ValueEvent | CHANGE(USP)));
	}
	else
	{
		TR = SSP; SSP = R(6);
		TheSim.RegisterEvent("regfile", R(6), ReadEvent);
		TheSim.RegisterEvent("control", SSP, (EventEnum)(WriteEvent | ValueEvent | CHANGE(SSP)));
	}
	TheSim.RegisterEvent("control", PSR, ReadEvent);

	//R6 gets the supervisor stack pointer
	TR = R(6); R(6) = SSP;
	TheSim.RegisterEvent("control", SSP, ReadEvent);
	TheSim.RegisterEvent("regfile", R(6), (EventEnum)(WriteEvent | ValueEvent | CHANGE(R(6))));
	
	//Save PSR and PC to the stack
	TR = R(6); R(6) = R(6) + -1;
	TheSim.RegisterEvent("regfile", R(6), (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(R(6))));
	DataWrite(R(6), 2, PSR, 16);
	TheSim.RegisterEvent("control", PSR, ReadEvent);
	TR = R(6); R(6) = R(6) + -1;
	TheSim.RegisterEvent("regfile", R(6), (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(R(6))));
	DataWrite(R(6), 2, PC, 16);
	TheSim.RegisterEvent("control", PC, ReadEvent);

	//Update PSR
	TR = PSR; PSR = (NewPriority << 8);
	TheSim.RegisterEvent("control", PSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PSR)));

	//Load the handler address into the PC
	TR = PC; PC = DataRead(Vector, 2, 16);
	TheSim.RegisterEvent("control", PC, (EventEnum)(WriteEvent | ValueEvent | CHANGE(PC)));
	TheSim.SubInEvent(TR, PC);

	TheSim.InstructionEvent(PC << LC3ISA::Addressability);
	return true;
}

LC3ISA::Word LC3Arch::DataRead(LC3ISA::Word Address, unsigned char Bytes, unsigned char Bits)
{
	LC3ISA::Word BitMask = ((LC3ISA::Word)1 << Bits) - 1;
	LC3ISA::Word Value;
	RamVector vData;
	Register &KBSR = *pKBSR, &KBDR = *pKBDR, &DSR = *pDSR, &DDR = *pDDR, &MCR = *pMCR, TR("", 64);
	bool fRetVal;

	//Second check for Memory-mapped IO
	string sInput;
	switch(Address)
	{
	case KBSR_ADDRESS:
		//Try to read one char from the keyboard
		unsigned int CharsRead;
		fRetVal = TheSim.ReadConsole(sInput, 1, CharsRead);
		if(CharsRead)
		{
			TR = KBSR; KBSR.SetBit(15, 1);
			TheSim.RegisterEvent("mmio", KBSR, (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(KBSR)));
			TR = KBDR; KBDR = sInput[0];
			TheSim.RegisterEvent("mmio", KBDR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(KBDR)));
		}
		else if(!fRetVal)	//CTRL-C gave EOF or Error signal
		{
			TR = KBSR; KBSR.SetBit(15, 1);
			TheSim.RegisterEvent("mmio", KBSR, (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(KBSR)));
			TR = KBDR; KBDR = -1;
			TheSim.RegisterEvent("mmio", KBDR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(KBDR)));
		}
		else	//CTRL-D  gave "no input"
		{
			TR = KBSR; KBSR.SetBit(15, 0);
			TheSim.RegisterEvent("mmio", KBSR, (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(KBSR)));
		}
		Value = KBSR;
		break;
	case KBDR_ADDRESS:
		Value = KBDR;
		TheSim.RegisterEvent("mmio", KBDR, ReadEvent);
		TR = KBSR; KBSR.SetBit(15, 0);
		TheSim.RegisterEvent("mmio", KBSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(KBSR)));
		break;
	case DSR_ADDRESS:
		//The display is always ready
		TR = DSR; DSR.SetBit(15, 1);
		TheSim.RegisterEvent("mmio", DSR, (EventEnum)(ReadEvent | WriteEvent | ValueEvent | CHANGE(DSR)));
		Value = DSR;
		break;
	case DDR_ADDRESS:
		//No Read from this reg
		Value = 0;//DDR;
		break;
	case MCR_ADDRESS:
		Value = MCR;
		TheSim.RegisterEvent("mmio", MCR, ReadEvent);
		break;
	default:	//No MMIO
		if(Address >= 0xFE00)
		{
			//read to undefined MMIO locations
			sprintf(sMessageBuffer, "Read to undefined MMIO location (4x%x).", Address);
			TheSim.SimCallBack(Warning, sMessageBuffer);
		}

		pDRAM->Read(vData, Address << LC3ISA::Addressability, Bytes, TheSim.SimCallBack);
		Value = MakeInt(vData, Bits, TheSim);
		for(uint64 i = 0; i < Bytes; Bytes++)
		{
			TheSim.DataEvent((Address << LC3ISA::Addressability) + i, ReadEvent);
			TheSim.MemoryEvent(*pDRAM, (Address << LC3ISA::Addressability) + i, ReadEvent);
		}
	}

	return Value & BitMask;
}

void LC3Arch::DataWrite(LC3ISA::Word Address, unsigned char Bytes, LC3ISA::Word Value, unsigned char Bits)
{
	Value &= ((LC3ISA::Word)1 << Bits) - 1;
	RamVector vTData, vData;
	Register &KBSR = *pKBSR, &KBDR = *pKBDR, &DSR = *pDSR, &DDR = *pDDR, &MCR = *pMCR, TR("", 64);

	//Second check for Memory-mapped IO
	string sOutput;
	switch(Address)
	{
	case KBSR_ADDRESS:
		//Can only write to interrupt enable bit
		KBSR.SetBit(0, Value & 1);
		TheSim.RegisterEvent("mmio", KBSR, (EventEnum)(WriteEvent | ValueEvent));
		break;
	case KBDR_ADDRESS:
		//No write to this reg
		//KBDR = Value;
		//TheSim.RegisterEvent("mmio", KBDR, (EventEnum)(WriteEvent | ValueEvent));
		break;
	case DSR_ADDRESS:
		//No write to this reg
		//DSR = Value;
		//TheSim.RegisterEvent("control", pDSR->sName, (EventEnum)(WriteEvent | ValueEvent), DSR);
		break;
	case DDR_ADDRESS:
		//Try to write one char to the screen
		TR = DSR; DSR.SetBit(15, 0);
		TheSim.RegisterEvent("mmio", DSR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(DSR)));
		unsigned int CharsWritten;
		sOutput += (char)Value;
		TR = DDR; DDR = Value;
		TheSim.WriteConsole(sOutput, 1, CharsWritten);
		TheSim.RegisterEvent("mmio", DDR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(DDR)));
		break;
	case MCR_ADDRESS:
		TR = MCR; MCR = Value;
		if(!(MCR)[15])
		{
			TheSim.SimCallBack(JMT::Exception, "Program has stopped the clock. Continuing execution will restart the clock.");
			TheSim.Exception();
		}
		TheSim.RegisterEvent("mmio", MCR, (EventEnum)(WriteEvent | ValueEvent | CHANGE(MCR)));
		break;
	default:	//No MMIO
		if(Address >= 0xFE00)
		{
			//write to undefined MMIO locations
			sprintf(sMessageBuffer, "Write to undefined MMIO location (4x%x).", Address);
			TheSim.SimCallBack(Warning, sMessageBuffer);
		}

		pDRAM->Read(vTData, (Address << LC3ISA::Addressability), Bytes, TheSim.SimCallBack);
		uint64 i;
		for(i = 0; i < Bytes; i++) 
			vData.push_back( RamVector::value_type((Address << LC3ISA::Addressability) + i, (Value >> (i*8)) & 0xFF) );
		pDRAM->Write(vData, TheSim.SimCallBack);
		for(i = 0; i < Bytes; i++)
		{
			TheSim.DataEvent((Address << LC3ISA::Addressability) + i, (EventEnum)(WriteEvent | ValueEvent | (vData == vTData ? NoEvent : ChangeEvent)));
			TheSim.MemoryEvent(*pDRAM, (Address << LC3ISA::Addressability) + i, (EventEnum)(WriteEvent | ValueEvent | (vData == vTData ? NoEvent : ChangeEvent)));
			vData.erase(vData.begin());
			vTData.erase(vTData.begin());
		}
	}
}

istream &LC3Arch::operator <<(istream &Input)
{
	return Input;
}
ostream &LC3Arch::operator >>(ostream &Output) const
{
	return Output;
}

LC3Arch::~LC3Arch()
{
}

}	//namespace LC3
