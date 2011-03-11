//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef SEGMENT_H
#define SEGMENT_H

#pragma warning (disable:4786)
#include <list>
#include <iostream>
#include "Element.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Segment

		An instance of this class is a symbolic representation of a segment
		in an assembly program.

		A segment begins at a memory address, and contains a sequence of
		elements, such as instructions and data.

		Usage of the operator const char *() must be done serially, not
		multi-threaded.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Segment
	{
	public:
		//True if the segment offset is determined dynamically
		bool fDynamicOffset;
		//A specified offset from the beginning of the previous segment or orig
		uint64 Offset;
		//The memory address this segment begins at
		uint64 Address;
		//The size of this segment
		uint64 Size;
		//Addressability. Specifies the number of bits in a byte address
		//that aren't addressable in this ISA.
		unsigned char Addressability;
		//True if the segment is a struct definition and does not create any
		//memory image
		bool fStruct;
		string sStruct;
		//True if creating a in the process of copying structure
		bool fRecursive;
		//The sequence of elements in this segment
		list<Element *> Sequence;
		//The input ID and line number this segment was found on
		LocationVector LocationStack;

		//unsigned char param is addressability
		//Dynamic address
		Segment(const LocationVector &, unsigned char);
		//Offset specified at compile-time
		Segment(const LocationVector &, unsigned char, uint64);
		//This segment is a structure definition (param is struct name)
		Segment(const LocationVector &, unsigned char, const string &);
		//Copy constructor
		Segment *Copy() const;

		/**********************************************************************\
			ResolveStructDef( [in] error callback function )
			
			This runs through the elements in the sequence, calculating and
			updating the size of every element. Updates the size of the
			structure. Checks for recursive structure definitions

			ResolveStructs is only called on structure definitions. Sizes of
			structures are not valid until this has been called.
		\******/
		virtual bool ResolveStructDef(CallBackFunction);

		/**********************************************************************\
			ResolveAddresses( [in-out] current address, 
				[in] error callback function )
			
			This runs through the elements in the sequence, calculating and
			updating the address of every element. Updates the size of the
			segment.

			The starting address of this segment is given in the parameter,
			and it returns the next unallocated address after the segment image.

			ResolveAddresses is not called on structure definitions.
		\******/
		virtual bool ResolveAddresses(uint64 &, CallBackFunction);

		/**********************************************************************\
			GenerateImage( [out] array of memory bytes, [in] endian-ness
				[in] error callback function )

			This runs through the elements and gets their images
			and combines them into a segment image. The data can be
			stored in little endian (true) or big endian (false) format.

			ResolveAddresses must be called before this. Otherwise symbols may
			have the wrong value when they are generated.

			If there is an error in the data, it will print an error message.
			It will return false if it was unable to generate the whole image.
		\******/
		virtual bool GenerateImage(RamVector &, bool, CallBackFunction) const;

		/**********************************************************************\
			Prints the segment
		\******/
		virtual operator const char *() const;

		/**********************************************************************\
			Prints the segment and all it's elements
		\******/
		friend ostream &operator <<(ostream &, Segment &);

		/**********************************************************************\
			~Program()
			
			Deletes the elements in the sequence
		\******/
		virtual ~Segment();
	};
}

#endif
