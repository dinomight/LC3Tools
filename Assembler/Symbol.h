//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef SYMBOL_H
#define SYMBOL_H

#pragma warning (disable:4786)
#include <string>
#include "Segment.h"
#include "Label.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	//These are the enumerations and strings for the symbols
	const unsigned int NUM_SYMBOLS = 7;
	enum SymbolEnum {SymLabel = 0, SymExtern, SymDefine, SymMacro, SymStruct, SymMember, SymVoid};
	extern const char *const sSymbols[NUM_SYMBOLS];

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Symbol

		An instance of this class is a reference to a Label or a value.
		
		A Symbol will resolve to the address of the Label it points to
		or to the value it is a Symbol for.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Symbol
	{
	public:
		SymbolEnum SymbolType;
		//The Symbol might be a reference to a label
		Label *pLabel;
		//The Symbol might be a reference to an external symbol
		Symbol *pSymbol;
		//The Symbol might be a reference to a structure
		Segment *pSegment;
		//Addressability. Specifies the number of bits in a byte address
		//that aren't addressable in this ISA.
		unsigned char Addressability;
		//The input ID and line number the symbol resolved on
		//If the symbol hasn't been resolved yet, this is the first
		//reference.
		LocationVector LocationStack;

		//The name of this symbol
		string sSymbol;

		/**********************************************************************\
			Symbol( [in] symbol name, [in] addressability );
			
			Initialize the symbol
		\******/
		Symbol(const LocationVector &, const string &, unsigned char);

		/**********************************************************************\
			GetValue( [in] error callback function )
			
			Returns the address or offset this is a Symbol for.
		\******/
		uint64 GetValue() const;

		/**********************************************************************\
			GetSegmentRelativeValue( [in] error callback function )
			
			Returns the segment relative address or offset this is a Symbol for.
		\******/
		uint64 GetSegmentRelativeValue() const;

		/**********************************************************************\
			Prints in hex:
			SymbolName, GetValue, GetSegmentRelativeValue
		\******/
		virtual operator const char *() const;

		virtual ~Symbol();
	};
}

#endif
