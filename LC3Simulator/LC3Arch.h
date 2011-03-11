//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef LC3ARCH_H
#define LC3ARCH_H

#pragma warning (disable:4786)
#include "../Simulator/Architecture.h"
#include "../LC3Assembler/LC3ISA.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;
using namespace Simulator;

namespace LC3
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
		LC3Arch

		The following defines a sipmle LC-3 architecture implementation.

		It contains functions to initialize the architecture, as well as
		functions to execute pipeline stage RTL for the ISA.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class LC3Arch : public Architecture
	{
	protected:
		ArchSim<LC3ISA> &TheSim;

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
		static LC3ISA::Word SEXT(LC3ISA::Word, unsigned char);

		/**********************************************************************\
			SetCC( [in] value )

			Checks the input value for positive, zero, and negative, and sets
			the appropriate condition codes.
		\******/
		void SetCC(signed short);

		/**********************************************************************\
			MakeInt( [in] data vector, [in] bits, [in] simulator )

			Takes a data vector and converts it into an unsigned int of the
			specified bit length and returns it.
		\******/
		static LC3ISA::Word MakeInt(const RamVector &, unsigned char, ArchSim<LC3ISA> &);

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
			DataRead( [in] address,
				[in] # bytes to read, [in] value bits )

			Performs a read of the logical memory address. Calls the public
			DataRead which will convert the logical address into necessary
			cache memory reads. Checks the address to see if it is an MMIO
			address. If so, it reads from the MMIO register instead of DataRead
			and logs a register event with the simulator. Else, it logs a data
			event with the simulator.

			The address is expected to be an ISA word address. it will be
			converted to a byte address when sent to the public DataRead and
			to the simulator's Event functions.

			The value bits specifies how many bits the read value should be.
			Any additional bits will be set to zero.
		\******/
		LC3ISA::Word DataRead(LC3ISA::Word, unsigned char, unsigned char);

		/**********************************************************************\
			DataWrite( [in] address,
				[in] # bytes to write, [in] value, [in] value bits )

			Performs a write to the logical memory address. Calls the public
			DataWrite which will convert the logical address into necessary
			cache memory writes. Checks the address to see if it is an MMIO
			address. If so, it writes to the MMIO register instead of DataWrite
			and logs a register event with the simulator. Else, it logs a data
			event with the simulator.

			The address is expected to be an ISA word address. it will be
			converted to a byte address when sent to the public DataWrite and
			to the simulator's Event functions.

			The value bits specifies how many bits the write value should be.
			Any additional bits will be set to zero.
		\******/
		void DataWrite(LC3ISA::Word, unsigned char, LC3ISA::Word, unsigned char);


	public:
		LC3Arch(ArchSim<LC3ISA> &);
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
		virtual ~LC3Arch();
	};
}

#endif
