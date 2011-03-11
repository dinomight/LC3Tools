//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Symbol.h"
#include <cstdio>

using namespace std;
using namespace JMT;

namespace Assembler	{

const char *const sSymbols[NUM_SYMBOLS] = {"Label", "Extern", "Define", "Macro", "Struct", "Member", "Void"};

Symbol::Symbol(const LocationVector &LS, const string &sS, unsigned char addressability)
{
	sSymbol = sS;
	pLabel = NULL;
	pSymbol = NULL;
	pSegment = NULL;
	SymbolType = SymVoid;
	Addressability = addressability;
	LocationStack = LS;
}

uint64 Symbol::GetValue() const
{
	switch(SymbolType)
	{
	case SymLabel:
	case SymMember:
		if(pLabel->pElement)
			return pLabel->pElement->Address;
		if(pLabel->pSegment)
			return pLabel->pSegment->Address;
		return pLabel->Address;
	case SymExtern:
		return pSymbol->GetValue();
	case SymStruct:
		return 0;
	default:
		throw "GetValue called on unresolved symbol!";
	}
}

uint64 Symbol::GetSegmentRelativeValue() const
{
	switch(SymbolType)
	{
	case SymLabel:
	case SymMember:
		if(pLabel->pElement)
			return pLabel->pElement->Address - pLabel->pSeg->Address;
		if(pLabel->pSegment)
			return pLabel->pSegment->Address - pLabel->pSeg->Address;
		return pLabel->Address - pLabel->pSeg->Address;
	case SymExtern:
		return pSymbol->GetSegmentRelativeValue();
	case SymStruct:
		return 0;
	default:
		throw "GetValue called on unresolved symbol!";
	}
}

Symbol::operator const char *() const
{
	static char sSymb[65 + MAX_IDENTIFIER_CHAR];
	char *psS = sSymb;

	psS += sprintf(psS, "%.63s, %.31s", sSymbol.c_str(), sSymbols[SymbolType]);

	if(SymbolType == SymMacro || SymbolType == SymDefine || SymbolType == SymStruct)
		//These have no value
		return sSymb;

	if(SymbolType == SymMember)
	{
		//These have no segment relative value
		#if defined _MSC_VER
			sprintf(psS, ", 4x%I64X", GetValue() >> Addressability);
		#elif defined GPLUSPLUS
			sprintf(psS, ", 4x%llX", GetValue() >> Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		return sSymb;
	}

	//SymLabel (or SymExtern pointing to label)
	#if defined _MSC_VER
		sprintf(psS, ", 4x%I64X, 4x%I64X", GetValue() >> Addressability, GetSegmentRelativeValue() >> Addressability);
	#elif defined GPLUSPLUS
		sprintf(psS, ", 4x%llX, 4x%llX", GetValue() >> Addressability, GetSegmentRelativeValue() >> Addressability);
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif

	return sSymb;
}

Symbol::~Symbol()
{
}

}	//namespace Assembler

