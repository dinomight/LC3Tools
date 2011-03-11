//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef MEMORY_H
#define MEMORY_H

#pragma warning (disable:4786)
#include <vector>
#include <string>
#include <iostream>
#include "../JMTLib/SparseArray.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace Simulator
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		MemoryLocation

		An instance of this class represents one location in a memory array.

	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class MemoryLocation
	{
	public:
		//The value at this location
		unsigned char Value;

		//Creates default value, no element
		MemoryLocation(unsigned char value = UNINITIALIZED_VALUE);
		//Access the memory location
		operator unsigned char() const;
		//Assign a value ot the location
		bool operator==(const MemoryLocation &) const;
		//Assign a value ot the location
		MemoryLocation &operator=(unsigned char);
		//saving and loading snapshots
		friend istream &operator >>(istream &, MemoryLocation &);
		friend ostream &operator <<(ostream &, const MemoryLocation &);
	};

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Memory

		An instance of this class represents a memory array.

		A memory array can contain up to 2^64 memory locations (64-bit 
		indexability), and uses SparseArray to manage the data.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Memory
	{
	public:
		//The memory array
		SparseArray<uint64, MemoryLocation> Array;
		//Name of the memory
		string sName;

		//Construct (name, start byte index, end byte index, bit divisions)
		Memory(const string &, uint64, uint64, const vector<short> &);
		//Assign the memory image to the memory array
		bool Write(const RamVector &, SimCallBackFunction);
		//Read the memory image from the memory array (image, address, length)
		bool Read(RamVector &, uint64, uint64, SimCallBackFunction) const;
		//Reset the memory
		void Clear();
		//Access an element from the memory
		MemoryLocation &operator[](uint64);
		//saving and loading snapshots
		friend istream &operator >>(istream &, Memory &);
		friend ostream &operator <<(ostream &, const Memory &);
	};
}

#endif
