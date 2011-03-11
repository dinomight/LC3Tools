//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#pragma warning (disable:4786)
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include "Pipeline.h"
#include "Memory.h"
#include "Register.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;

namespace Simulator
{
	template<class ISA>
	class ArchSim;

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Architecture

		An instance of this class represents one microprocessor system
		architecture.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Architecture
	{
	protected:
		//Buffer for formatting error messages
		char sMessageBuffer[256];

		/**********************************************************************\
			CreateRegisters( [in-out] architecture )

			Adds the registers for this ISA architecture's implementation to
			the simulation architecture.

			All the register set and register names should be lower-case,
			since the ISA is case-insensitive, and will convert all simulator
			command input to lowercase.
		\******/
		virtual bool CreateRegisters() = 0;

		/**********************************************************************\
			CreateMemories( [in-out] architecture )

			Adds the memories for this ISA architecture's implementation to
			the simulation architecture.

			All the memory names should be lower-case,
			since the ISA is case-insensitive, and will convert all simulator
			command input to lowercase.
		\******/
		virtual bool CreateMemories() = 0;

		/**********************************************************************\
			CreatePipelines( [in-out] architecture )

			Adds the pipelines for this ISA architecture's implementation to
			the simulation architecture.

			All the pipeline and stage names should be lower-case,
			since the ISA is case-insensitive, and will convert all simulator
			command input to lowercase.
		\******/
		virtual bool CreatePipelines() = 0;

	public:
		//List of pipelines. Pipelines must be added in order of propagation.
		//Pipelines are run in reverse order.
		typedef vector<Pipeline> PipelineVector;
		PipelineVector Pipelines;
		//List of memories
		typedef map<string, Memory> MemoryMap;
		MemoryMap Memories;
		//List of register sets
		typedef map<string, RegisterSet> RegisterSetMap;
		RegisterSetMap RegisterSets;
		//The keyboard interrupt vector
		uint64 KeyboardInterruptVector;

		/**********************************************************************\
			Architecture( )

			Constructor. The constructor of the inherited architecture should
			take a reference to the simulator as a parameter.
		\******/
		Architecture();

		/**********************************************************************\
			Run( )

			Executes one cycle of the architecture.
		\******/
		virtual bool Run();

		/**********************************************************************\
			Reset( [in] memory image )

			Resets the architecture. Resets memory and register state.
			Uses the imput memory image of the initial program.
		\******/
		virtual bool Reset(const RamVector &) = 0;

		/**********************************************************************\
			NextInstruction( )

			Returns the byte address of the next instruction to be processed.
		\******/
		virtual uint64 NextInstruction() = 0;

		/**********************************************************************\
			Interrupt( [in] interrupt vector )

			Signals an external interrupt to the processor. The parameter
			is a vector number and not an address.
		\******/
		virtual bool Interrupt(uint64) = 0;

		/**********************************************************************\
			DataRead( [in] memory image, [in] starting address, [in] length )

			Reads the memory image from the logical program memory.
			A given location in the logical program memory could be located in
			the architecture's main memory, or it could be loaded into caches
			or registers. Therefore, it is up to the architecture to find out
			(or "know") where this location is, and read from it.
		\******/
		virtual bool DataRead(RamVector &, uint64, uint64) = 0;

		/**********************************************************************\
			DataWrite( [in] memory image )

			Writes the memory image to the logical program memory.
			A given location in the logical program memory could be located in
			the architecture's main memory, or it could be loaded into caches
			or registers. Therefore, it is up to the architecture to find out
			(or "know") where this location is, and write to it.
		\******/
		virtual bool DataWrite(const RamVector &) = 0;

		//saving and loading snapshots
		virtual istream &operator <<(istream &) = 0;
		virtual ostream &operator >>(ostream &) const = 0;

		//destructor
		virtual ~Architecture();
	};
}

#endif
