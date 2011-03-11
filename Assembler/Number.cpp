//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Number.h"
#include "Symbol.h"
#include "Data.h"
#include <cstdio>
#include <cmath>
#include <strstream>

using namespace std;
using namespace JMT;

namespace Assembler	{

//*NOTE: GODAWFUL GCC BUG!
//If you use a variable as the shift amount in a left shift,
//stupid GCC 3.x on Linux implements a rotate shifter instead of a left shifter!!!
//AAAAAAAARRRRGGGGGGHHHHHHHH!!!
//So if you expect:
//shamt = 64;
//((uint64)-1 << shamt) - 1
//to give you 0 - 1 = 0xFF..F
//give up, because you'll get 
//1 - 1 = 0
//instead!

char Number::sMessageBuffer[129 + MAX_IDENTIFIER_CHAR];
char Number::sNumber[MAX(64, 1+MAX_IDENTIFIER_CHAR)];

Number::Number(const LocationVector &LS)
{
	LocationStack = LS;
}

Number::~Number()
{
}

RealNumber::RealNumber(const RealToken &RT) : Number(RT.LocationStack)
{
	NumberType = NumReal;
	Value = RT.RealVal;

}

RealNumber::RealNumber(const LocationVector &LocationStack, double value) : Number(LocationStack)
{
	NumberType = NumReal;
	Value.DBL = value;
}

RealNumber::RealNumber(const LocationVector &LocationStack, unsigned char ExpBits, unsigned char MantBits, uint64 Float) : Number(LocationStack)
{
	NumberType = NumReal;
	if(ExpBits > 11 || MantBits > 52)
		throw "Invalid floating point parameters given to RealNumber constructor!";

	uint64 MantissaMask = ((uint64)1 << MantBits) - 1;
	uint64 ExponentMask = ((uint64)1 << (ExpBits + MantBits)) - 1 - MantissaMask;
	int64 Exponent = ((Float & ExponentMask) >> MantBits) - (ExponentMask >> (MantBits + 1));
	uint64 Mantissa = Float & MantissaMask;

	//Check for infinity and NAN
	if((Float & ExponentMask) == ExponentMask)
	{
		if((Float & MantissaMask) == 0)
		{	//Inifinty
			if(Float >> (ExpBits + MantBits))
				Value.UI64 = 0xFFF0000000000000;
			else
				Value.UI64 = 0x7FF0000000000000;
		}
		else	//NAN
		{
			Value.UI64 = 0x7FFFFFFFFFFFFFFF;
		}
		return;
	}

	//Adjust for gradual underflow
	if(!(Float & ExponentMask))
		Exponent++;
	else
		Mantissa |= (uint64)1 << MantBits;

	//Calculate the exponent
	Value.DBL = ldexp(UINT64_TO_DOUBLE(Mantissa), (int)(Exponent - MantBits));
	if(Float >> (ExpBits + MantBits))
		Value.DBL = -Value.DBL;

}

RealNumber::RealNumber(const LocationVector &LocationStack, unsigned char ExpBits, unsigned char MantBits, const RamVector &vData, bool fLittleEndian) : Number(LocationStack)
{
	NumberType = NumReal;
	JMT::ByteData ByteData;
	unsigned int i;

	if(vData.size() > sizeof(uint64))
		throw "RealNumber: vData larger than uint64!";
	
	for(i = 0; i < vData.size(); i++)
	{
		if(fLittleEndian)
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = vData[i].second;
#else
			ByteData.Bytes[i] = vData[i].second;
#endif
		else	//Big Endian
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = vData[vData.size() - i - 1].second;
#else
			ByteData.Bytes[i] = vData[vData.size() - i - 1].second;
#endif
	}
	for(i = vData.size(); i < sizeof(uint64); i++)
	{
		if(fLittleEndian)
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = 0;
#else
			ByteData.Bytes[i] = 0;
#endif
		else	//Big Endian
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = 0;
#else
			ByteData.Bytes[i] = 0;
#endif
	}

	RealNumber Real(LocationStack, ExpBits, MantBits, ByteData.UI64);
	Value = Real.Value;
}

Number *RealNumber::Copy() const
{
	return new RealNumber(*this);
}

bool RealNumber::Float(unsigned char ExpBits, unsigned char MantBits, unsigned char &Sign, unsigned short &Exponent, uint64 &Mantissa, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	if(ExpBits > 11 || MantBits > 52)
		throw "Invalid floating point parameters given to Number::Float conversion!";

	bool fRetVal = true;
	uint64 RealMant, MantMask;
	unsigned int i;
	short RealExp, Bias, NewBias, ExpMask;
	bool fLostPrecision = false;

	Sign = (unsigned char)((Value.UI64 & 0x8000000000000000) >> 63);
	RealMant = Value.UI64 & 0x000FFFFFFFFFFFFF;
	RealExp = (short)((Value.UI64 & 0x7FF0000000000000) >> 52);
	Bias = 0x3FF;
	NewBias = (1 << (ExpBits-1)) - 1;
	ExpMask = (1 << ExpBits) - 1;
	MantMask = ((uint64)1 << MantBits) - 1;

	if(RealExp == 0x7FF)
	{
		if(RealMant == 0)
		{
			//New FP format has less or equal magnitude, so infinity remains
			Exponent = ExpMask;
			Mantissa = 0;
			return fRetVal;
		}
		else
		{
			//The only way a NAN can happen is if the number was already a NAN
			Sign = 0;
			Exponent = ExpMask;
			Mantissa = MantMask;
			return fRetVal;
		}
	}

	//Add leading digit and adjust exponent for gradual underflow so that we now
	//have a true exponent and fraction.
	if(RealExp != 0)
		RealMant |= 0x0010000000000000;
	else
		RealExp += 1;

	Exponent = RealExp - Bias + NewBias;
	Mantissa = RealMant;
	
	//Find exponent for new FP format
	if((short)Exponent >= ExpMask)
	{
		Exponent = ExpMask;
		Mantissa = 0;
		sprintf(sMessageBuffer, "Magnitude too large for real exponent's %u bits, treated as infinity%.63s.", ExpBits, sExtra);
		CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
		return !fWarnErrors;
	}

	//If the exponent is too small, shift the mantissa to increase the exponent
	if((short)Exponent <= 0)
	{
		while((short)Exponent <= 0 && Mantissa)
		{
			if(Mantissa & 1)
				fLostPrecision = true;
			Mantissa >>= 1;
			Exponent++;
		}
		Exponent = 1;
		if( !Mantissa && RealMant || !(Mantissa >> (52 - MantBits)) )
		{
			Exponent = 0;
			Mantissa = 0;
			sprintf(sMessageBuffer, "Magnitude too small for real exponent's %u bits, treated as zero%.63s.", ExpBits, sExtra);
			CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
			return !fWarnErrors;
		}
	}

	//Remove leading digit and adjust exponent for gradual underflow
	if(!(Mantissa & 0x0010000000000000))
		Exponent = 0;
	else
		Mantissa &= ~0x0010000000000000;

	//Find mantissa for new FP format
	//*NOTE: Should add code to implement rounding here
	for(i = 52; i > MantBits; i--)
	{
		if(Mantissa & 1)
			fLostPrecision = true;
		Mantissa >>= 1;
	}

	if(fLostPrecision)
	{
		sprintf(sMessageBuffer, "Floating point fraction's precision truncated to %u bits%.63s.", MantBits+1, sExtra);
		CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
		fRetVal = fWarnErrors ? false : fRetVal;
	}

	return fRetVal;
}

bool RealNumber::Char(char &RetChar, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	bool fRetVal = true;
	char CharValue = (char)Value.DBL;
	if((double)CharValue != Value.DBL)
	{
		sprintf(sMessageBuffer, "Conversion from real to character resulted in loss of data%.63s.", sExtra);
		CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
		fRetVal = fWarnErrors ? false : fRetVal;
	}
	RetChar = CharValue;
	return fRetVal;
}

bool RealNumber::Int(unsigned char Bits, bool fSigned, uint64 &RetInt, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability, bool fSegmentRelative, bool fAddressRelative, uint64 RelAddress) const
{
	bool fRetVal = true;
	bool fSign = (Value.DBL < 0);
	uint64 IntValue = (uint64)fabs(Value.DBL);
	//*NOTE: This comparison returns false if DBL is NAN
	if(UINT64_TO_DOUBLE(IntValue) != fabs(Value.DBL) || Value.UI64 == 0x7FFFFFFFFFFFFFFF)
	{
		sprintf(sMessageBuffer, "Conversion from real to integer resulted in loss of data%.63s.", sExtra);
		CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
		fRetVal = fWarnErrors ? false : fRetVal;
	}

	if(!IntegerNumber(LocationStack, IntValue, fSign).Int(Bits, fSigned, RetInt, CallBack, sExtra, fWarnErrors, Addressability, fSegmentRelative, fAddressRelative, RelAddress))
		return false;
	return fRetVal;
}

RealNumber::operator const char *() const
{
	char *psN = sNumber;
	uint64 RealMant = Value.UI64 & 0x000FFFFFFFFFFFFF;
	short RealExp = (short)((Value.UI64 & 0x7FF0000000000000) >> 52);
	uint64 LeftMant, RightMant;
	unsigned int LeftBits;

	//*NOTE: This comparison returns true if DBL is NAN
	if(Value.DBL < 0 && Value.UI64 != 0x7FFFFFFFFFFFFFFF)
		sprintf(psN++, "-");

	if(RealExp == 0)
	{
		LeftMant = (RealMant >> 50);
		RightMant = (RealMant << 2) & 0x000FFFFFFFFFFFFF;
		#if defined _MSC_VER
			sprintf(psN, "4x%I64X.%.13I64Xg-FF", LeftMant, RightMant);
		#elif defined GPLUSPLUS
			sprintf(psN, "4x%llX.%.13llXg-FF", LeftMant, RightMant);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
	}
	else if(RealExp == 0x7FF)
	{
		if(RealMant == 0)
			sprintf(psN, "4x1.0gINF");
		else
			sprintf(psN, "4x1.0gNAN");
	}
	else
	{
		RealExp -= 0x3FF;
		LeftBits = RealExp % 4;
		RealMant |= 0x0010000000000000;
		LeftMant = (RealMant >> (52 - LeftBits));
		RightMant = LeftBits >= 64 ? 0 : ((RealMant << LeftBits) & 0x000FFFFFFFFFFFFF);
		#if defined _MSC_VER
			if(RealExp < 0)
				sprintf(psN, "4x%I64X.%.13I64Xg-%X", LeftMant, RightMant, -RealExp/4);
			else
				sprintf(psN, "4x%I64X.%.13I64Xg%X", LeftMant, RightMant, RealExp/4);
		#elif defined GPLUSPLUS
			if(RealExp < 0)
				sprintf(psN, "4x%llX.%.13llXg-%X", LeftMant, RightMant, -RealExp/4);
			else
				sprintf(psN, "4x%llX.%.13llXg%X", LeftMant, RightMant, RealExp/4);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
	}

	return sNumber;
}



IntegerNumber::IntegerNumber(const LocationVector &LocationStack) : Number(LocationStack)
{
}

IntegerNumber::IntegerNumber(const IntegerToken &IT) : Number(IT.LocationStack)
{
	NumberType = NumInteger;
	Value = IT.Integer;
	fNegative = IT.fNegative;
}

IntegerNumber::IntegerNumber(const LocationVector &LocationStack, uint64 value, bool fnegative) : Number(LocationStack)
{
	NumberType = NumInteger;
	Value = value;
	fNegative = fnegative;
}

IntegerNumber::IntegerNumber(const LocationVector &LocationStack, const RamVector &vData, bool fLittleEndian) : Number(LocationStack)
{
	NumberType = NumInteger;
	fNegative = false;
	JMT::ByteData ByteData;
	unsigned int i;

	if(vData.size() > sizeof(uint64))
		throw "IntegerNumber: vData larger than uint64!";
	
	for(i = 0; i < vData.size(); i++)
	{
		if(fLittleEndian)
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = vData[i].second;
#else
			ByteData.Bytes[i] = vData[i].second;
#endif
		else	//Big Endian
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = vData[vData.size() - i - 1].second;
#else
			ByteData.Bytes[i] = vData[vData.size() - i - 1].second;
#endif
	}
	for(i = vData.size(); i < sizeof(uint64); i++)
	{
		if(fLittleEndian)
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = 0;
#else
			ByteData.Bytes[i] = 0;
#endif
		else	//Big Endian
#ifdef BIG_ENDIAN_BUILD
			ByteData.Bytes[sizeof(uint64) - i - 1] = 0;
#else
			ByteData.Bytes[i] = 0;
#endif
	}

	Value = ByteData.UI64;
}

Number *IntegerNumber::Copy() const
{
	return new IntegerNumber(*this);
}

bool IntegerNumber::Float(unsigned char ExpBits, unsigned char MantBits, unsigned char &Sign, unsigned short &Exponent, uint64 &Mantissa, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	bool fRetVal = true;
	double RealValue = UINT64_TO_DOUBLE(Value);
	if((uint64)RealValue != Value)
	{
		sprintf(sMessageBuffer, "Conversion from integer to real resulted in loss of data%.63s.", sExtra);
		CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
		fRetVal = fWarnErrors ? false : fRetVal;
	}
	if(fNegative)
		RealValue = -RealValue;

	if(!RealNumber(LocationStack, RealValue).Float(ExpBits, MantBits, Sign, Exponent, Mantissa, CallBack, sExtra, fWarnErrors, Addressability))
		return false;
	return fRetVal;
}

bool IntegerNumber::Char(char &RetChar, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	bool fRetVal = true;
	char CharValue = (char)(fNegative ? -Value : Value);
	if((uint64)(fNegative ? -CharValue : CharValue) != Value)
	{
		sprintf(sMessageBuffer, "Conversion from integer to character resulted in loss of data%.63s.", sExtra);
		CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
		fRetVal = fWarnErrors ? false : fRetVal;
	}
	RetChar = CharValue;
	return fRetVal;
}

bool IntegerNumber::Int(unsigned char Bits, bool fSigned, uint64 &RetInt, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability, bool fSegmentRelative, bool fAddressRelative, uint64 RelAddress) const
{
	if(Bits > 64)
		throw "Invalid bit length given to Number::Int conversion!";

	bool fRetVal = true;
	uint64 NewInt = Value, Mask = (Bits >= 64 ? ~(uint64)0 : ((uint64)1 << Bits) - 1);
	const char *sSigned = fSigned ? "signed" : "unsigned";

	if(!fSigned && ( (NewInt & ~Mask) || fNegative && (NewInt & ~(Mask >> 1)) && !( NewInt == (Mask & ~(Mask >> 1)) ) )
		||
		fSigned && (NewInt & ~(Mask >> 1)) && !(fNegative && NewInt == (Mask & ~(Mask >> 1))))
	{
		sprintf(sMessageBuffer, "Integer truncated to %u %.15s bits%.63s.", Bits, sSigned, sExtra);
		CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
		fRetVal = fWarnErrors ? false : fRetVal;
	}

	if(fNegative)
		NewInt = -NewInt;

	RetInt = NewInt & Mask;
	return fRetVal;
}

IntegerNumber::operator const char *() const
{
	char *psN = sNumber;

	if(fNegative)
		sprintf(psN++, "-");

	#if defined _MSC_VER
		sprintf(psN, "4x%I64X", Value);
	#elif defined GPLUSPLUS
		sprintf(psN, "4x%llX", Value);
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif

	return sNumber;
}



CharNumber::CharNumber(const CharToken &CT) : Number(CT.LocationStack)
{
	NumberType = NumCharacter;
	Value = CT.Character;
}

CharNumber::CharNumber(const LocationVector &LocationStack, char value) : Number(LocationStack)
{
	NumberType = NumCharacter;
	Value = value;
}

Number *CharNumber::Copy() const
{
	return new CharNumber(*this);
}

bool CharNumber::Float(unsigned char ExpBits, unsigned char MantBits, unsigned char &Sign, unsigned short &Exponent, uint64 &Mantissa, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	bool fRetVal = true;
	double RealValue = Value;
	if((char)RealValue != Value)
	{
		sprintf(sMessageBuffer, "Conversion from char to real resulted in loss of data%.63s.", sExtra);
		CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
		fRetVal = fWarnErrors ? false : fRetVal;
	}

	if(!RealNumber(LocationStack, RealValue).Float(ExpBits, MantBits, Sign, Exponent, Mantissa, CallBack, sExtra, fWarnErrors, Addressability))
		return false;
	return fRetVal;
}

bool CharNumber::Char(char &RetChar, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	RetChar = Value;
	return true;
}

bool CharNumber::Int(unsigned char Bits, bool fSigned, uint64 &RetInt, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability, bool fSegmentRelative, bool fAddressRelative, uint64 RelAddress) const
{
	bool fRetVal = true;
	bool fSign = (Value < 0);
	uint64 IntValue = abs(Value);
	if((unsigned char)IntValue != (unsigned char)abs(Value))
	{
		sprintf(sMessageBuffer, "Conversion from char to integer resulted in loss of data%.63s.", sExtra);
		CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
		fRetVal = fWarnErrors ? false : fRetVal;
	}

	if(!IntegerNumber(LocationStack, IntValue, fSign).Int(Bits, fSigned, RetInt, CallBack, sExtra, fWarnErrors, Addressability, fSegmentRelative, fAddressRelative, RelAddress))
		return false;
	return fRetVal;
}

CharNumber::operator const char *() const
{
	char *sValue;
	switch(Value)
	{
	case '\a':
		sValue = "\\a";
		break;
	case '\b':
		sValue = "\\b";
		break;
	case '\f':
		sValue = "\\f";
		break;
	case '\n':
		sValue = "\\n";
		break;
	case '\r':
		sValue = "\\r";
		break;
	case '\t':
		sValue = "\\t";
		break;
	case '\v':
		sValue = "\\v";
		break;
	case '\'':
		sValue = "\\'";
		break;
	case '\"':
		sValue = "\\\"";
		break;
	case '\\':
		sValue = "\\\\";
		break;
	case '\?':
		sValue = "\\?";
		break;
	default:
		if(Value <= 0x1F || Value >= 0x7F)
			sprintf(sNumber, "'\\x%X'", (unsigned char)Value);
		else
			sprintf(sNumber, "'%c'", Value);
		return sNumber;
	}
	sprintf(sNumber, "'%s'", sValue);
	return sNumber;
}



VoidNumber::VoidNumber(const LocationVector &LocationStack) : Number(LocationStack)
{
	NumberType = NumVoid;
}

Number *VoidNumber::Copy() const
{
	return new VoidNumber(*this);
}

bool VoidNumber::Float(unsigned char ExpBits, unsigned char MantBits, unsigned char &Sign, unsigned short &Exponent, uint64 &Mantissa, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	Sign = false;
	Exponent = 0;
	Mantissa = 0;
	return true;
}

bool VoidNumber::Char(char &RetChar, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	RetChar = 0;
	return true;
}

bool VoidNumber::Int(unsigned char Bits, bool fSigned, uint64 &RetInt, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability, bool fSegmentRelative, bool fAddressRelative, uint64 RelAddress) const
{
	RetInt = 0;
	return true;
}

VoidNumber::operator const char *() const
{
	sprintf(sNumber, "?");
	return sNumber;
}



SymbolNumber::SymbolNumber(const LocationVector &LocationStack, Symbol *psymbol, SymbolMap &SM) : IntegerNumber(LocationStack), SymbolLookUp(SM)
{
	NumberType = NumSymbol;
	if(!psymbol)
		throw "NULL Symbol given to SymbolNumber!";
	pSymbol = psymbol;
	fAttribute = false;
}

Number *SymbolNumber::Copy() const
{
	return new SymbolNumber(*this);
}

bool SymbolNumber::ResolveValue(Element **ppElement, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability, bool fSegmentRelative, bool fAddressRelative, uint64 RelAddress) const
{
	bool fRetVal = true, fTrueAddress = true, fLastArray = false;
	SymbolNumber *pThis = const_cast<SymbolNumber *>(this);
	//The current symbol is the current thing we're looking at to get an offset or address from
	//Accessing a structure member will cause a new symbol to become current.
	//If this is an external symbol, set the current symbol to the actual external symbol
	Symbol *pCurrentSymbol = pSymbol->pSymbol ? pSymbol->pSymbol : pSymbol;
	if(ppElement)
		*ppElement = NULL;

	if(fAttribute && Attribute == AttrBaseRel)
	{
		pThis->Value = 0;
		fTrueAddress = false;
	}
	else if( (fAttribute && Attribute == AttrAbs) || !fSegmentRelative && (!fAttribute || Attribute != AttrSegRel) )
	{
		pThis->Value = pCurrentSymbol->GetValue();
	}
	else
	{
		pThis->Value = pCurrentSymbol->GetSegmentRelativeValue();
		fTrueAddress = false;
	}
	pThis->fNegative = false;

	if(pCurrentSymbol->SymbolType == SymStruct)
		fTrueAddress = false;

	//For each offset identifier
	for(list<OffsetInfo>::const_iterator OffsetIter = OffsetList.begin(); OffsetIter != OffsetList.end(); OffsetIter++)
	{
		if(OffsetIter->first)
		{	//Array index
			if(fLastArray)
				throw "Multidimensional array in SymbolNumber::ResolveValue!";
			if(pCurrentSymbol->SymbolType == SymStruct)
			{
				uint64 TempInt64;
				sprintf(sMessageBuffer, " for '%.63s' array index.", pCurrentSymbol->sSymbol.c_str());
				if(!OffsetIter->second->Int(64, false, TempInt64, CallBack, sMessageBuffer, fWarnErrors))
					return false;
				pThis->Value += pCurrentSymbol->pSegment->Size * TempInt64;
			}
			else	//SymLabel or SymMember
			{
				if(pCurrentSymbol->pLabel->pElement)
				{
					uint64 TempInt64;
					sprintf(sMessageBuffer, " for '%.63s' array index.", pCurrentSymbol->sSymbol.c_str());
					if(!OffsetIter->second->Int(64, false, TempInt64, CallBack, sMessageBuffer, fWarnErrors))
						return false;
					if(TempInt64 >= pCurrentSymbol->pLabel->pElement->Length)
					{
						#if defined _MSC_VER
							sprintf(sMessageBuffer, "Symbol '%.63s' indexed (4x%I64X) past allocated bounds (4x0-4x%I64X).", pCurrentSymbol->sSymbol.c_str(), TempInt64, pCurrentSymbol->pLabel->pElement->Length-1);
						#elif defined GPLUSPLUS
							sprintf(sMessageBuffer, "Symbol '%.63s' indexed (4x%llX) past allocated bounds (4x0-4x%llX).", pCurrentSymbol->sSymbol.c_str(), TempInt64, pCurrentSymbol->pLabel->pElement->Length-1);
						#else
							#error "Only MSVC and GCC Compilers Supported"
						#endif
						CallBack(Warning, sMessageBuffer, LocationStack);
					}
					pThis->Value += (pCurrentSymbol->pLabel->pElement->Size / pCurrentSymbol->pLabel->pElement->Length) * TempInt64;
				}
				else if(pCurrentSymbol->pLabel->pSegment)
				{
					sprintf(sMessageBuffer, "Symbol '%.63s' references segment, cannot use array index.", pCurrentSymbol->sSymbol.c_str());
					CallBack(Error, sMessageBuffer, LocationStack);
					return false;
				}
				else
				{
					sprintf(sMessageBuffer, "Symbol '%.63s' references nothing, cannot use array index.", pCurrentSymbol->sSymbol.c_str());
					CallBack(Error, sMessageBuffer, LocationStack);
					return false;
				}
			}
			fLastArray = true;
		}
		else
		{	//Struct member access
			//The first time, the symbol could be anything.
			//Subsequent times, the symbol will always be pointing to a StructElement.
			string sStructMember;

			if(pCurrentSymbol->SymbolType != SymStruct)	//SymLabel or SymMember
			{
				if(pCurrentSymbol->pLabel->pElement)
				{
					if(pCurrentSymbol->pLabel->pElement->ElementType != StructElement)
					{
						sprintf(sMessageBuffer, "Symbol '%.63s' does not reference structure, cannot use member access.", pCurrentSymbol->sSymbol.c_str());
						CallBack(Error, sMessageBuffer, LocationStack);
						return false;
					}
					if(!fLastArray && pCurrentSymbol->pLabel->pElement->Length > 1)
					{
						sprintf(sMessageBuffer, "Symbol '%.63s' member access missing array index.", pCurrentSymbol->sSymbol.c_str());
						CallBack(Error, sMessageBuffer, LocationStack);
						return false;
					}
					pCurrentSymbol = reinterpret_cast<Struct *>(pCurrentSymbol->pLabel->pElement)->pSymbol;
				}
				else if(pCurrentSymbol->pLabel->pSegment)
				{
					sprintf(sMessageBuffer, "Symbol '%.63s' references segment, cannot use member access.", pCurrentSymbol->sSymbol.c_str());
					CallBack(Error, sMessageBuffer, LocationStack);
					return false;
				}
				else
				{
					sprintf(sMessageBuffer, "Symbol '%.63s' references nothing, cannot use member access.", pCurrentSymbol->sSymbol.c_str());
					CallBack(Error, sMessageBuffer, LocationStack);
					return false;
				}
			}
			//Create the mangled name of the struct member
			((sStructMember = pCurrentSymbol->sSymbol) += ".") += OffsetIter->third;
			SymbolMap::iterator MemberIter = SymbolLookUp.find(sStructMember);
			if(MemberIter == SymbolLookUp.end())
			{
				sprintf(sMessageBuffer, "Symbol '%.63s' is not a member of struct '%.63s'.", OffsetIter->third.c_str(), pCurrentSymbol->sSymbol.c_str());
				CallBack(Error, sMessageBuffer, LocationStack);
				return false;
			}
			//Update the current symbol to point to struct member
			pCurrentSymbol = MemberIter->second;
			//We know this is a struct member label because the only way a "." character
			//could get in a symbol name is from parsing a struct member label.
			pThis->Value += pCurrentSymbol->pLabel->Address;
			fLastArray = false;
		}
	}

	if(fAttribute)
	{
		switch(Attribute)
		{
		case AttrSize:
			fTrueAddress = false;
			if(pCurrentSymbol->SymbolType == SymStruct)
				pThis->Value = pCurrentSymbol->pSegment->Size;
			else	//SymLabel or SymMember
			{
				if(pCurrentSymbol->pLabel->pElement)
				{
					//If the last access was an array index, then use the size of an element, not the whole array.
					if(fLastArray)
					{
						if(!pCurrentSymbol->pLabel->pElement->Length)
							throw "Element length is zero!";
						pThis->Value = pCurrentSymbol->pLabel->pElement->Size / pCurrentSymbol->pLabel->pElement->Length;
					}
					else
						pThis->Value = pCurrentSymbol->pLabel->pElement->Size;
				}
				else if(pCurrentSymbol->pLabel->pSegment)
					pThis->Value = pCurrentSymbol->pLabel->pSegment->Size;
				else
				{
					sprintf(sMessageBuffer, "Symbol '%.63s' references nothing, cannot use Size attribute.", pCurrentSymbol->sSymbol.c_str());
					CallBack(Error, sMessageBuffer, LocationStack);
					return false;
				}
			}
			break;

		case AttrLen:
			fTrueAddress = false;
			if(pCurrentSymbol->SymbolType == SymStruct)
			{
				sprintf(sMessageBuffer, "Symbol '%.63s' is a structure name, cannot use Length attribute.", pCurrentSymbol->sSymbol.c_str());
				CallBack(Error, sMessageBuffer, LocationStack);
				return false;
			}
			else	//SymLabel or SymMember
			{
				if(pCurrentSymbol->pLabel->pElement)
				{
					if(fLastArray)
						pThis->Value = 1;
//						CallBack(Error, "Index into array cannot use Length attribute.", LocationStack);
//						return false;
					else
						pThis->Value = pCurrentSymbol->pLabel->pElement->Length;
				}
				else if(pCurrentSymbol->pLabel->pSegment)
				{
					sprintf(sMessageBuffer, "Symbol '%.63s' references segment, cannot use Length attribute.", pCurrentSymbol->sSymbol.c_str());
					CallBack(Error, sMessageBuffer, LocationStack);
					return false;
				}
				else
				{
					sprintf(sMessageBuffer, "Symbol '%.63s' references nothing, cannot use Length attribute.", pCurrentSymbol->sSymbol.c_str());
					CallBack(Error, sMessageBuffer, LocationStack);
					return false;
				}
			}
			break;

		case AttrSeg:
			if(pSymbol->SymbolType == SymStruct)
			{
				sprintf(sMessageBuffer, "Symbol '%.63s' is a structure name, cannot use Segment attribute.", pCurrentSymbol->sSymbol.c_str());
				CallBack(Error, sMessageBuffer, LocationStack);
				return false;
			}
			else	//SymLabel or SymMember
				pThis->Value = pSymbol->pLabel->pSeg->Address;
			break;
		}
	}

	if(fAddressRelative)
	{
		if(fTrueAddress)
		{
			if(RelAddress > pThis->Value)
			{
				pThis->Value = RelAddress - pThis->Value;
				pThis->fNegative = true;
			}
			else
				pThis->Value -= RelAddress;
		}
		else
		{
			sprintf(sMessageBuffer, "Symbolic target%.63s is not an absolute address.", sExtra);
			CallBack(Warning, sMessageBuffer, LocationStack);
		}
	}

	//Assign the element that this symbol points to.
	if(ppElement && !fAttribute && pCurrentSymbol->SymbolType != SymStruct && pCurrentSymbol->pLabel->pElement && pCurrentSymbol->pLabel->pElement->ElementType != StructElement)
		*ppElement =  pCurrentSymbol->pLabel->pElement;


	//Shift the address
	if(!fAttribute || Attribute != AttrLen)
	{
		if(pThis->Value & ((1 << Addressability)-1))
		{
			sprintf(sMessageBuffer, "Symbol '%.63s' is not %u-byte aligned%.63s.", pCurrentSymbol->sSymbol.c_str(), 1 << Addressability, sExtra);
			CallBack(fWarnErrors ? Error : Warning, sMessageBuffer, LocationStack);
			return false;
		}
		pThis->Value >>= Addressability;
	}

	return true;
}

bool SymbolNumber::Float(unsigned char ExpBits, unsigned char MantBits, unsigned char &Sign, unsigned short &Exponent, uint64 &Mantissa, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	if(!ResolveValue(NULL, CallBack, sExtra, fWarnErrors, Addressability))
		return false;

	return IntegerNumber::Float(ExpBits, MantBits, Sign, Exponent, Mantissa, CallBack, sExtra, fWarnErrors, Addressability);
}

bool SymbolNumber::Char(char &RetChar, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability) const
{
	if(!ResolveValue(NULL, CallBack, sExtra, fWarnErrors, Addressability))
		return false;

	return IntegerNumber::Char(RetChar, CallBack, sExtra, fWarnErrors, Addressability);
}

bool SymbolNumber::Int(unsigned char Bits, bool fSigned, uint64 &RetInt, CallBackFunction, const char *sExtra, bool fWarnErrors, unsigned char Addressability, bool fSegmentRelative, bool fAddressRelative, uint64 RelAddress) const
{
	if(!ResolveValue(NULL, CallBack, sExtra, fWarnErrors, Addressability, fSegmentRelative, fAddressRelative, RelAddress))
		return false;

	return IntegerNumber::Int(Bits, fSigned, RetInt, CallBack, sExtra, fWarnErrors,
		Addressability, fSegmentRelative, fAddressRelative, RelAddress);
}

SymbolNumber::operator const char *() const
{
	static string strNumber;
	ostrstream strNum;

	strNum << pSymbol->sSymbol.c_str();
	for(list<OffsetInfo>::const_iterator OffsetIter = OffsetList.begin(); OffsetIter != OffsetList.end(); OffsetIter++)
	{
		if(OffsetIter->first)
			strNum << '[' << (const char *)*OffsetIter->second << ']';
		else
			strNum << "." << OffsetIter->third.c_str();
	}
	
	if(fAttribute)
		strNum << "$" << sAttributes[Attribute];

	strNum << ends;
	strNumber = strNum.str();
	return strNumber.c_str();
}

SymbolNumber::~SymbolNumber()
{
	//For each offset identifier
	for(list<OffsetInfo>::const_iterator OffsetIter = OffsetList.begin(); OffsetIter != OffsetList.end(); OffsetIter++)
	{
		if(OffsetIter->first)	//Array index
			delete OffsetIter->second;
	}
}

}	//namespace Assembler
