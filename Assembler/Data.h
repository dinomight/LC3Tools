//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef DATA_H
#define DATA_H

#pragma warning (disable:4786)
#include <vector>
#include "Segment.h"
#include "Symbol.h"
#include "Number.h"
#include "Element.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	//This holds the number of bytes for each type of data
	extern const unsigned char vDataBytes[NUM_DATATYPES];
	//This holds the number of exponent and mantissa bits for each type of real data
	extern const unsigned char vExponentBits[NUM_DATATYPES];
	extern const unsigned char vMantissaBits[NUM_DATATYPES];

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Data

		An instance of this class is a symbolic representation of an data
		element in an assembly program.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Data : public Element
	{
	public:
		//The array of data
		vector<Number *> vData;
		//The type of data this is
		DataEnum DataType;
		//Addressability. Specifies the number of bits in a byte address
		//that aren't addressable in this ISA.
		unsigned char Addressability;
		/**********************************************************************\
			Data( [in] location stack, [in] data type, [in] addressability,
				[in] value )
			Data( [in] location stack, [in] data type, [in] addressability,
				[in] array length, [in] array of data )
			
			Stores the data as an array of numbers.
			The array cannot be longer than the specified length.

			The first constructor specifies one piece of
			data, and the second constructor specifies an array of data.
		\******/
		Data(const LocationVector &, DataEnum, unsigned char, Number *);
		Data(const LocationVector &, DataEnum, unsigned char, uint64, const vector<Number *> &);

		virtual Element *Copy() const;
		virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
		virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
		virtual operator const char *() const;

		virtual ~Data();
	};

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Struct

		An instance of this class is a symbolic representation of a structure
		element in an assembly program.

		Struct elements come in two flavors. The first kind is an instance
		that is declared in a structure definition. This is the "golden" copy
		of the structure, and is never actually built.

		The second kind is a regular data instance of a structure. This kind
		will make a copy of the "golden" definition.

		Sizes of structures are not valid until ResolveStructDef has been
		called.
		The structure elements are not created until ResolveAddresses is called.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Struct : public Element
	{
	public:
		//The array of data
		vector< vector<Number *> > vvData;
		//Points to the definition of the structure
		Symbol *pSymbol;
		//The instantiation of the structure (a copy of the definition)
		//Each segment is one element in an array of structures.
		vector<Segment *> Segments;
		/**********************************************************************\
			Struct( [in] location stack, [in] struct definition symbol,
				[in] array of data )
			Struct( [in] location stack, [in] struct definition symbol,
				[in] array length, [in] array of array of data)
			
			Stores the data as an array of an array of numbers.
			The major array cannot be longer than the specified length.

			The first constructor specifies data for a whole structure, and
			the second constructor specifies data for an array of structures.
		\******/
		Struct(const LocationVector &, Symbol *, const vector<Number *> &);
		Struct(const LocationVector &, Symbol *, uint64, const vector< vector<Number *> > &);

		/**********************************************************************\
			ResolveStructDef( [in] error callback function )
			
			This runs through the elements in the sequence, calculating and
			updating the size of every element. Updates the size of the
			structure.

			ResolveStructDef is only called on structure definitions.
		\******/
		virtual bool ResolveStructDef(CallBackFunction);

		/**********************************************************************\
			ResolveAddresses( [in-out] current address, 
				[in] error callback function )
			
			This runs through the elements in the structure, calculating and
			updating the address of every element. Updates the size of the
			structure.

			The starting address of this structure is given in the parameter,
			and it returns the next unallocated address after the structure
			image.

			ResolveAddresses is only called on structure instantiations.
		\******/
		virtual bool ResolveAddresses(uint64 &, CallBackFunction);

		virtual Element *Copy() const;
		virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
		virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
		virtual operator const char *() const;

		virtual ~Struct();
	};

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Align

		An instance of this class is a symbolic representation of an align
		element in an assembly program.

		This element does not build to an image, all it does is make sure
		that the next element is aligned on the specified boundary. It is
		either removed completely from the program, or replaced with byte
		data during the build. A label will never point to an Align element.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Align : public Element
	{
	public:
		uint64 Alignment;

		/**********************************************************************\
			Data( [in] location stack, [in] alignment boundary )
			
			Stores the data as an array of numbers.
			The array cannot be longer than the specified length.

			The first constructor specifies one piece of
			data, and the second constructor specifies an array of data.
		\******/
		Align(const LocationVector &, uint64);

		virtual Element *Copy() const;
		virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
		virtual bool GetImage(RamVector &, bool, CallBackFunction) const;
		virtual operator const char *() const;

		virtual ~Align();
	};

}

#endif
