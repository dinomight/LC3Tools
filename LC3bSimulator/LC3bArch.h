//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef LC3BARCH_H
#define LC3BARCH_H

#pragma warning (disable:4786)
#include <map>
#include <list>
#include "../Simulator/Architecture.h"
#include "../LC3bAssembler/LC3bISA.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;
using namespace Simulator;

namespace LC3b
{
	//*NOTE: Some bug in MSVC refuses to believe that
	//"const unsigned int BLAH" is actually constant. When you use it in:
	//switch(){ case BLAH:
	//it gives a compile error about BLAH not being constant.
	const unsigned int KBSR_ADDRESS = 0xFE00;
	const unsigned int KBDR_ADDRESS = 0xFE02;
	const unsigned int DSR_ADDRESS = 0xFE04;
	const unsigned int DDR_ADDRESS = 0xFE06;
	const unsigned int MCR_ADDRESS = 0xFFFE;

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		LC3bArch

		The following defines a sipmle LC-3b architecture implementation.

		It contains functions to initialize the architecture, as well as
		functions to execute pipeline stage RTL for the ISA.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class LC3bArch : public Architecture
	{
	protected:
		ArchSim<LC3bISA> &TheSim;

		//Registers;
		Register *pPC, *pIR, *pPSR, *pSSP, *pUSP, *pR[8], *pKBSR, *pKBDR, *pDSR, *pDDR, *pMCR;
		//MMIO registers
		//Memories
		Memory *pDRAM;
		enum CCEnum {N = 2, Z = 1, P = 0};
		//Pending external interrupts
		list<uint64> InterruptList;

		//Pipeline stages
		//Current implementation only has one pipeline with one stage.
		static bool DataPath(Architecture *);

		/**********************************************************************\
			SEXT( [in] value, [in] bits )

			Sign extends the input value. The input value is "bits" long
			(so "bits"-1 is the sign bit). The output word is 16 bits long.
		\******/
		static LC3bISA::Word SEXT(LC3bISA::Word, unsigned char);

		/**********************************************************************\
			SetCC( [in] value )

			Checks the input value for positive, zero, and negative, and sets
			the appropriate condition codes.
		\******/
		void SetCC( signed short);

		/**********************************************************************\
			MakeInt( [in] data vector, [in] bits, [in] simulator )

			Takes a data vector and converts it into an unsigned int of the
			specified bit length and returns it.
		\******/
		static LC3bISA::Word MakeInt(const RamVector &, unsigned char, ArchSim<LC3bISA> &);

		/**********************************************************************\
			Exception( [in] exception vector )

			Signals an internal exception to the processor. The parameter
			is a vector number and not an address.
		\******/
		bool Exception(uint64);

		/**********************************************************************\
			ProcessInterrupt( )

			Check for and process pending external interrupts.
			Returns true if an interrupt is processed, else false.
		\******/
		bool ProcessInterrupt();

		/**********************************************************************\
			DataRead( [in] address, [in] address alignment,
				[in] # bytes to read, [in] value bits, [in] id string )

			Performs a read of the logical memory address. Calls the public
			DataRead which will convert the logical address into necessary
			cache memory reads. Checks the address to see if it is an MMIO
			address. If so, it reads from the MMIO register instead of DataRead
			and logs a register event with the simulator. Else, it logs a data
			event with the simulator. It also checks the alignment of the
			address, and flags an exception if the address is unaligned.

			The address alignment specifies how many LSB bits should be zero.
			The value bits specifies how many bits the read value should be.
			Any additional bits will be set to zero.
		\******/
		LC3bISA::Word DataRead(LC3bISA::Word, unsigned char, unsigned char, unsigned char, const string &);

		/**********************************************************************\
			DataWrite( [in] address, [in] address alignment,
				[in] # bytes to write, [in] value, [in] value bits, [in] id string )

			Performs a write to the logical memory address. Calls the public
			DataWrite which will convert the logical address into necessary
			cache memory writes. Checks the address to see if it is an MMIO
			address. If so, it writes to the MMIO register instead of DataWrite
			and logs a register event with the simulator. Else, it logs a data
			event with the simulator. It also checks the alignment of the
			address, and flags an exception if the address is unaligned.

			The address alignment specifies how many LSB bits should be zero.
			The value bits specifies how many bits the write value should be.
			Any additional bits will be set to zero.
		\******/
		void DataWrite(LC3bISA::Word, unsigned char, unsigned char, LC3bISA::Word, unsigned char, const string &);

	public:
		LC3bArch(ArchSim<LC3bISA> &);
		virtual bool Reset(const RamVector &);
		virtual uint64 NextInstruction();
		virtual bool Interrupt(uint64);
		virtual bool DataRead(RamVector &, uint64, uint64);
		virtual bool DataWrite(const RamVector &);
		virtual bool CreateRegisters();
		virtual bool CreateMemories();
		virtual bool CreatePipelines();
		virtual istream &operator <<(istream &);
		virtual ostream &operator >>(ostream &) const;
		virtual ~LC3bArch();
	};
}

#endif
