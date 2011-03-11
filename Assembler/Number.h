//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef NUMBER_H
#define NUMBER_H

#pragma warning (disable:4786)
#include <list>
#include <map>
#include "AsmToken.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	enum NumberEnum {NumInteger, NumReal, NumCharacter, NumSymbol, NumVoid};

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Number

		An instance of this class is a reference to a number.
		
		The number could be any type of signed/unsigned integral/real type/
		character, or a symbol which references an address.

		In order to enable multi-threaded conversions, locks will need to be
		placed around accesses to sMessageBuffer, or create a non-static buffer
		unique to each function that uses it.

		Usage of the operator const char *() must be done serially, not
		multi-threaded.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Number
	{
	protected:
		//Buffer for formatting error messages
		static char sMessageBuffer[129 + MAX_IDENTIFIER_CHAR];
		//Buffer for printing numbers
		static char sNumber[MAX(64, 1+MAX_IDENTIFIER_CHAR)];

	public:
		NumberEnum NumberType;
		LocationVector LocationStack;

		/**********************************************************************\
			Number( [in] location stack )
		\******/
		Number(const LocationVector &);

		/**********************************************************************\
			Make a copy of the Number
		\******/
		virtual Number *Copy() const = 0;

		/**********************************************************************\
			Float( [in] number of exponent bits, [in] number of mantissa bits,
				[out] sign, [out] exponent, [out] mantissa,
				[in] error callback function, [in] message postfix,
				[in] true if warnings should be treated as errors
				*The following parameters only apply if the Number is a symbol
				that resolves to an address.
				[in] number of lower address bits that are not required
				and are not specified by an immediate value. )

			Converts the number to a floating-point of the specified format:
			[1 sign bit][Exp bits][Mantissa bits]
			Each field is returned as the lower X bits of separate parameters.
			The returned format uses bias, nan, inf, and gradual underflow
			the same as the standard IEEE formats.

			If there's an error, sends message and returns false.
		\******/
		virtual bool Float(unsigned char, unsigned char, unsigned char &, unsigned short &, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const = 0;

		/**********************************************************************\
			Char( [out] character, [in] error callback function )

			Converts the number to a character.

			If there's an error, sends message and returns false.
		\******/
		virtual bool Char(char &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const = 0;

		/**********************************************************************\
			Int( [in] number of bits, [in] true if signed else unsigned,
				[out] the resulting integer,
				[in] error callback function, [in] message postfix,
				[in] true if warnings should be treated as errors,
				*The following parameters only apply if the Number is a symbol
				that resolves to an address.
				[in] number of lower address bits that are not required
				and are not specified by an immediate value.
				[in] true if this address should be segment relative
				(symbol only),
				[in] true if this address is address relative (symbol only),
				[in] the relative address (address relative only) )

			Converts the number to an int of the specified bit length. Whether
			or not the desired int is signed only affects warning messages.

			The (Address only) parameters are only necessary if the desired
			int is an address. If segment relative is true and this number is
			a symbol, then the address is segment relative.

			If there's an error, sends message and returns false.
		\******/
		virtual bool Int(unsigned char, bool, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0, bool fSegmentRelative = false, bool fAddressRelative = false, uint64 RelAddress = 0) const = 0;

		/**********************************************************************\
			Prints the number
		\******/
		virtual operator const char *() const = 0;

		virtual ~Number();
	};

	class RealNumber : public Number
	{
	public:
		Real Value;

		RealNumber(const RealToken &);
		RealNumber(const LocationVector &, double);
		//#exponent bits, #mantissa bits, float value (right or LSB justified)
		RealNumber(const LocationVector &, unsigned char, unsigned char, uint64);
		//#exponent bits, #mantissa bits, memory byte data, true if little endian
		RealNumber(const LocationVector &, unsigned char, unsigned char, const RamVector &, bool);

		virtual Number *Copy() const;
		virtual bool Float(unsigned char, unsigned char, unsigned char &, unsigned short &, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Char(char &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Int(unsigned char, bool, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0, bool fSegmentRelative = false, bool fAddressRelative = false, uint64 RelAddress = 0) const;
		virtual operator const char *() const;
	};

	class IntegerNumber : public Number
	{
	protected:
		IntegerNumber(const LocationVector &);

	public:
		uint64 Value;
		bool fNegative;	//true if negative

		IntegerNumber(const IntegerToken &);
		IntegerNumber(const LocationVector &, uint64, bool fnegative = false);
		//memory byte data, true if little endian
		IntegerNumber(const LocationVector &, const RamVector &, bool);

		virtual Number *Copy() const;
		virtual bool Float(unsigned char, unsigned char, unsigned char &, unsigned short &, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Char(char &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Int(unsigned char, bool, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0, bool fSegmentRelative = false, bool fAddressRelative = false, uint64 RelAddress = 0) const;
		virtual operator const char *() const;
	};

	class CharNumber : public Number
	{
	public:
		char Value;

		CharNumber(const CharToken &);
		CharNumber(const LocationVector &, char);

		virtual Number *Copy() const;
		virtual bool Float(unsigned char, unsigned char, unsigned char &, unsigned short &, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Char(char &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Int(unsigned char, bool, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0, bool fSegmentRelative = false, bool fAddressRelative = false, uint64 RelAddress = 0) const;
		virtual operator const char *() const;
	};

	class VoidNumber : public Number
	{
	public:
		VoidNumber(const LocationVector &);

		virtual Number *Copy() const;
		virtual bool Float(unsigned char, unsigned char, unsigned char &, unsigned short &, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Char(char &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Int(unsigned char, bool, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0, bool fSegmentRelative = false, bool fAddressRelative = false, uint64 RelAddress = 0) const;
		virtual operator const char *() const;
	};

	class Symbol;
	class Element;

	//The number type for the symbol, which converts symbols to integer addresses
	//*NOTE: This is declared here, because otherwise the reference back to Symbol causes
	//a recursive include that MSVC can't deal with.
	class SymbolNumber : public IntegerNumber
	{
	public:
		Symbol *pSymbol;
		//A list of offset specifications.
		//True if this entry is an array index, false if this entry is a struct member
		typedef triple<bool, Number *, string> OffsetInfo;
		list<OffsetInfo> OffsetList;
		//A possible attribute
		bool fAttribute;
		AttributeEnum Attribute;
		//Maps symbol name to symbol object
		typedef map<string, Symbol *> SymbolMap;
		//A reference to the symbol map which is needed to lookup struct member accesses.
		SymbolMap &SymbolLookUp;

		SymbolNumber(const LocationVector &, Symbol *, SymbolMap &);

		virtual Number *Copy() const;
		/**********************************************************************\
			ResolveValue( [out] element pointed to,
				[in] error callback function, [in] message postfix,
				[in] true if warnings should be treated as errors,
				*The following parameters only apply if the Number is a symbol
				that resolves to an address.
				[in] number of lower address bits that are not required
				and are not specified by an immediate value.
				[in] true if this address should be segment relative,
				[in] true if this address is address relative,
				[in] the relative address (reladdress only) )

			Creates an address from the symbol's offset/attribute list.
			If the resulting address points to an instruction or data element,
			it returns it in the first parameter.

			If there's an error, sends message and returns false.
		\******/
		virtual bool ResolveValue(Element **, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0, bool fSegmentRelative = false, bool fAddressRelative = false, uint64 RelAddress = 0) const;
		virtual bool Float(unsigned char, unsigned char, unsigned char &, unsigned short &, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Char(char &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0) const;
		virtual bool Int(unsigned char, bool, uint64 &, CallBackFunction, const char *sExtra = "", bool fWarnErrors = false, unsigned char Addressability = 0, bool fSegmentRelative = false, bool fAddressRelative = false, uint64 RelAddress = 0) const;
		virtual operator const char *() const;

		virtual ~SymbolNumber();
	};

}

#endif
