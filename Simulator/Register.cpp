//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Register.h"
#include <cmath>
#include <cstdio>
#include <strstream>
#include "../Assembler/Number.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace Simulator	{

Register::Register(const string &sname, unsigned char bits, uint64 value)
{
	if(bits > 64)
		throw "Register cannot have more than 64 bits!";
	sName = sname;
	Bits = bits;
	fFloat = false;
	//*NOTE: GODAWFUL GCC BUG! See Number.cpp for more details...
	Value = value & (Bits == 64 ? ~(uint64)0:(((uint64)1 << Bits) - 1));
}

Register::Register(const string &sname, unsigned char exponentbits, unsigned char mantissabits, uint64 value)
{
	sName = sname;
	ExponentBits = exponentbits;
	MantissaBits = mantissabits;
	Bits = ExponentBits + MantissaBits + 1;
	if(Bits > 64)
		throw "Register cannot have more than 64 bits!";
	fFloat = true;
	//*NOTE: GODAWFUL GCC BUG! See Number.cpp for more details...
	Value = value & (Bits == 64 ? ~(uint64)0:(((uint64)1 << Bits) - 1));
}

bool Register::operator==(const Register &Reg) const
{
	return Value == Reg.Value;
}

unsigned char Register::operator[](int Bit) const
{
	if(Bit >= Bits)
		throw "Register requested bit out of range!";

	return (unsigned char)((Value >> Bit) & 1);
}

Register &Register::operator=(const Register &Reg)
{	return operator=((uint64)Reg);	}

Register &Register::operator=(uint64 value)
{
//	uint64 BitMask = ((uint64)1 << Bits) - 1;
//	if(Value & ~BitMask)
//		throw "Register assigned too many bits!";
	//*NOTE: GODAWFUL GCC BUG! See Number.cpp for more details...
	Value = value & (Bits == 64 ? ~(uint64)0:(((uint64)1 << Bits) - 1));
	return *this;
}

Register Register::operator+(const Register &Reg)
{	return operator+((uint64)Reg);	}

Register Register::operator+(uint64 value)
{
	//*NOTE: GODAWFUL GCC BUG! See Number.cpp for more details...
	Register TempReg("", 64, (Value + value) & (Bits == 64 ? ~(uint64)0:(((uint64)1 << Bits) - 1)));
	return TempReg;
}

Register Register::operator+(int64 value)
{	return operator+((uint64)value);	}

Register Register::operator+(unsigned long value)
{	return operator+((uint64)value);	}

Register Register::operator+(signed long value)
{	return operator+((uint64)value);	}

Register Register::operator+(unsigned int value)
{	return operator+((uint64)value);	}

Register Register::operator+(signed int value)
{	return operator+((uint64)value);	}

Register Register::operator+(unsigned short value)
{	return operator+((uint64)value);	}

Register Register::operator+(signed short value)
{	return operator+((uint64)value);	}

Register Register::operator+(unsigned char value)
{	return operator+((uint64)value);	}

Register Register::operator+(signed char value)
{	return operator+((uint64)value);	}

Register &Register::SetBit(unsigned char Bit, unsigned char value)
{
	if(Bit >= Bits)
		throw "Register requested bit out of range!";

	uint64 BitMask = (uint64)1 << Bit;
	if(value)
		Value |= BitMask;
	else
		Value &= ~BitMask;

	return *this;
}

Register::operator uint64() const
{
	//*NOTE: GODAWFUL GCC BUG! See Number.cpp for more details...
	return Value & (Bits == 64 ? ~(uint64)0:(((uint64)1 << Bits) - 1));
}

Register::operator const char *() const
{
	static char sRegister[128];
	if(fFloat)
	{
		RealNumber Real(NullLocationStack, ExponentBits, MantissaBits, Value);
		sprintf(sRegister, "%.63s (%.16e)", (const char *)Real, Real.Value.DBL);
	}
	else
	{
		int DisplayLength = (Bits+3)/4;
		#if defined _MSC_VER
			sprintf(sRegister, "4x%.*I64X (%I64u)", DisplayLength, (uint64)*this, (uint64)*this);
		#elif defined GPLUSPLUS
			sprintf(sRegister, "4x%.*llX (%llu)", DisplayLength, (uint64)*this, (uint64)*this);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
	}
	return sRegister;
}

istream &operator >>(istream &Input, Register &TheReg)
{
	JMT::ByteData ByteData;
	for(int i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
		ByteData.Bytes[sizeof(uint64) - i - 1] = Input.get();
#else
		ByteData.Bytes[i] = Input.get();
#endif
	TheReg.Value = ByteData.UI64;
	return Input;
}

ostream &operator <<(ostream &Output, const Register &TheReg)
{
	JMT::ByteData ByteData;
	ByteData.UI64 = TheReg.Value;
	for(int i = 0; i < sizeof(uint64); i++)
#ifdef BIG_ENDIAN_BUILD
		Output.put(ByteData.Bytes[sizeof(uint64) - i - 1]);
#else
		Output.put(ByteData.Bytes[i]);
#endif
	return Output;
}



RegisterSet::RegisterSet(const string &sname)
{
	sName = sname;
}

Register &RegisterSet::AddRegister(const string &sRegName, unsigned char Bits, uint64 Value)
{
	pair<RegisterMap::iterator, bool> RegIter = Registers.insert(RegisterMap::value_type(sRegName, Register(sRegName, Bits, Value)));

	//check to see if name already used
	if(!RegIter.second)
		throw "RegisterSet duplicate register name!";

	return RegIter.first->second;
}

vector<Register *> RegisterSet::AddRegisterArray(const string &sPrefix, unsigned char Bits, unsigned int Length, uint64 Value)
{
	char sBuffer[33 + MAX_IDENTIFIER_CHAR];

	vector<Register *> vRegs;
	pair<RegisterMap::iterator, bool> RegIter;
	for(unsigned int i = 0; i < Length; i++)
	{
		sprintf(sBuffer, "%.63s%u", sPrefix.c_str(), i);
		RegIter = Registers.insert(RegisterMap::value_type(sBuffer, Register(sBuffer, Bits, Value)));
		//check to see if name already used
		if(!RegIter.second)
			throw "RegisterSet duplicate register name!";
		vRegs.push_back(&RegIter.first->second);
	}

	return vRegs;
}

Register &RegisterSet::operator[](unsigned int Reg)
{
	if(Reg >= Registers.size())
		throw "RegisterSet requested register out of range!";

	RegisterMap::iterator RegIter = Registers.begin();
	for(unsigned int i = 0; i < Reg; i++)
		RegIter++;
		
	return RegIter->second;
}

const Register &RegisterSet::operator[](unsigned int Reg) const
{
	if(Reg >= Registers.size())
		throw "RegisterSet requested register out of range!";

	RegisterMap::const_iterator RegIter = Registers.begin();
	for(unsigned int i = 0; i < Reg; i++)
		RegIter++;
		
	return RegIter->second;
}

Register &RegisterSet::operator[](const string &sRegName)
{
	RegisterMap::iterator RegIter = Registers.find(sRegName);
	if(RegIter != Registers.end())
		return RegIter->second;
	throw "RegisterSet requested register does not exist!";
}

const Register &RegisterSet::operator[](const string &sRegName) const
{
	RegisterMap::const_iterator RegIter = Registers.find(sRegName);
	if(RegIter != Registers.end())
		return RegIter->second;
	throw "RegisterSet requested register does not exist!";
}

RegisterSet::operator const char *() const
{
	static string sRegisterSet;
	ostrstream strRegisterSet;

	strRegisterSet << sName.c_str() << "\t";
	for(RegisterMap::const_iterator RegIter = Registers.begin(); RegIter != Registers.end(); RegIter++)
	{
		if(RegIter != Registers.begin())
			strRegisterSet << ",\t";
		strRegisterSet << RegIter->second.sName.c_str() << ": " << (const char *)RegIter->second;
	}

	strRegisterSet << ends;
	sRegisterSet = strRegisterSet.str();
	return sRegisterSet.c_str();
}

istream &operator >>(istream &Input, RegisterSet &TheRegSet)
{
	for(RegisterSet::RegisterMap::iterator RegIter = TheRegSet.Registers.begin(); RegIter != TheRegSet.Registers.end(); RegIter++)
		Input >> RegIter->second;
	return Input;
}

ostream &operator <<(ostream &Output, const RegisterSet &TheRegSet)
{
	for(RegisterSet::RegisterMap::const_iterator RegIter = TheRegSet.Registers.begin(); RegIter != TheRegSet.Registers.end(); RegIter++)
		Output << RegIter->second;
	return Output;
}

}	//namespace Simulator
