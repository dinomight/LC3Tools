//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Memory.h"
#include <cstdio>

using namespace std;
using namespace JMT;

namespace Simulator	{

MemoryLocation::MemoryLocation(unsigned char value)
{
//	if(bits > 64)
//		throw "Register cannot have more than 64 bits!";
	Value = value;
}

MemoryLocation::operator unsigned char() const
{
//	uint64 BitMask = ((uint64)1 << Bits) - 1;
	return Value;// & BitMask;
}

bool MemoryLocation::operator==(const MemoryLocation &RValue) const
{
	return Value == RValue.Value;
}

MemoryLocation &MemoryLocation::operator=(unsigned char value)
{
	Value = value;
	return *this;
}

istream &Simulator::operator >>(istream &Input, MemoryLocation &TheMemLoc)
{
	TheMemLoc.Value = Input.get();
	return Input;
}

ostream &Simulator::operator <<(ostream &Output, const MemoryLocation &TheMemLoc)
{
	return Output.put(TheMemLoc.Value);
}



Memory::Memory(const string &sname, uint64 Begin, uint64 End, const vector<short> &BitDivisions) : Array(Begin, End, BitDivisions)
{
	sName = sname;
}

bool Memory::Write(const RamVector &vRam, SimCallBackFunction)
{
	//Buffer for formatting error messages
	char sMessageBuffer[256];

	//Check the memory address range
	RamVector::const_iterator RamIter = vRam.begin();
	if(RamIter != vRam.end() && RamIter->first < Array.Begin())
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%.63s[4x%I64X] is outside of the memory's address range.", sName.c_str(), RamIter->first);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%.63s[4x%llX] is outside of the memory's address range.", sName.c_str(), RamIter->first);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Error, sMessageBuffer);
		return false;
	}
	RamVector::const_reverse_iterator rRamIter = vRam.rbegin();
	if(rRamIter != vRam.rend() && rRamIter->first > Array.End())
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%.63s[4x%I64X] is outside of the memory's address range.", sName.c_str(), rRamIter->first);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%.63s[4x%llX] is outside of the memory's address range.", sName.c_str(), rRamIter->first);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Assign the memory data
	for(RamIter = vRam.begin(); RamIter != vRam.end(); RamIter++)
		Array[RamIter->first] = RamIter->second;

	return true;
}

bool Memory::Read(RamVector &vRam, uint64 Address, uint64 Length, SimCallBackFunction) const
{
	//Buffer for formatting error messages
	char sMessageBuffer[256];

	//Check the memory address range
	if(Address < Array.Begin())
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%.63s[4x%I64X] is outside of the memory's address range.", sName.c_str(), Address);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%.63s[4x%llX] is outside of the memory's address range.", sName.c_str(), Address);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Error, sMessageBuffer);
		return false;
	}
	if(Address + Length - 1 > Array.End())
	{
		#if defined _MSC_VER
			sprintf(sMessageBuffer, "%.63s[4x%I64X] is outside of the memory's address range.", sName.c_str(), Address + Length - 1);
		#elif defined GPLUSPLUS
			sprintf(sMessageBuffer, "%.63s[4x%llX] is outside of the memory's address range.", sName.c_str(), Address + Length - 1);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		SimCallBack(Error, sMessageBuffer);
		return false;
	}

	//Read the memory data
	for(uint64 i = Address; i < Address + Length; i++)
		vRam.push_back( RamVector::value_type(i, Array[i]) );

	return true;
}

void Memory::Clear()
{
	Array.Clear(Array.Begin(), Array.End());
}

MemoryLocation &Memory::operator[](uint64 Index)
{
	return Array[Index];
}

istream &Simulator::operator >>(istream &Input, Memory &TheMem)
{
	return Input >> TheMem.Array;
}

ostream &Simulator::operator <<(ostream &Output, const Memory &TheMem)
{
	//*NOTE: GCC incorrectly calls the non-const << function instead of the const version
	return Output << TheMem.Array;
}

}	//namespace Simulator
