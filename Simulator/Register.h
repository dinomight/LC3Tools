//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef REGISTER_H
#define REGISTER_H

#pragma warning (disable:4786)
#include <map>
#include <string>
#include <iostream>
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;

namespace Simulator
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Register

		An instance of this class represents a single register.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Register
	{
	public:
		//The name of the register
		string sName;
		//The value of the register
		uint64 Value;
		//The number of bits in the register
		unsigned char Bits, ExponentBits, MantissaBits;
		//true if this is a floating point register, false if integral
		bool fFloat;

		//Construct (name, #bits, intial value)
		Register(const string &, unsigned char, uint64 Value = 0);
		//Construct (name, #exponent bits, #mantissa bits, intial value)
		Register(const string &, unsigned char, unsigned char, uint64 Value = 0);
		//Compare to a register
		bool operator==(const Register &) const;
		//Access a bit from the register
		//*NOTE: MSVC complains about "2 operators with similiar conversions"
		//if "unsigned char" is used as the index
		unsigned char operator[](int) const;
		//Assign value to the register
		Register &operator=(const Register &);
		Register &operator=(uint64);
		//Add two registers
		//*NOTE: MSVC has trouble with the + operator ambiguities.
		Register operator+(const Register &);
		Register operator+(uint64);
		Register operator+(int64);
		Register operator+(unsigned long);
		Register operator+(signed long);
		Register operator+(unsigned int);
		Register operator+(signed int);
		Register operator+(unsigned short);
		Register operator+(signed short);
		Register operator+(unsigned char);
		Register operator+(signed char);
		//Assign value to a bit of the register (Bit, Value). Value can be bool.
		Register &SetBit(unsigned char, unsigned char);
		//Convert the register to a value
		operator uint64() const;
		//Print the contents of the register
		operator const char *() const;
		//saving and loading snapshots
		friend istream &operator >>(istream &, Register &);
		friend ostream &operator <<(ostream &, const Register &);
	};

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		RegisterSet

		An instance of this class represents a set of registers.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class RegisterSet
	{
	public:
		//Array of registers
		typedef map<string, Register> RegisterMap;
		RegisterMap Registers;
		//Name of the register set
		string sName;

		//Construct (name)
		RegisterSet(const string &);
		//Add a register to the set (name, #bits, initial value)
		Register &AddRegister(const string &, unsigned char, uint64 Value = 0);
		//Add an array of registers to the set (name prefix, #bits, #registers, initial value)
		//Names will be prefix0 - prefix#registers.
		vector<Register *> AddRegisterArray(const string &, unsigned char, unsigned int, uint64 Value = 0);
		//Access a register from the set
		Register &operator[](unsigned int);
		const Register &operator[](unsigned int) const;
		Register &operator[](const string &);
		const Register &operator[](const string &) const;
		//Print the contents of the registerset
		operator const char *() const;
		//saving and loading snapshots
		friend istream &operator >>(istream &, RegisterSet &);
		friend ostream &operator <<(ostream &, const RegisterSet &);
	};
}

#endif
