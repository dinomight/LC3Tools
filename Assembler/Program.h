//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef PROGRAM_H
#define PROGRAM_H

#pragma warning (disable:4786)
#include <list>
#include <string>
#include <iostream>
#include "SymbolTable.h"
#include "Segment.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Program

		An instance of this class is a symbolic representation of an assembly
		program.
		
		A program contains a a series of segments. Each segment contains
		a sequence of elements, such as data, instructions, and labels.

		Usage of the operator const char *() must be done serially, not
		multi-threaded.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Program
	{
	public:
		FileName sFileName;

		//True if the program address is determined dynamically
		bool fDynamicAddress;
		//The starting address of the program (also of the first segment)
		uint64 Address;
		//The size of this program
		uint64 Size;
		//Addressability. Specifies the number of bits in a byte address
		//that aren't addressable in this ISA.
		unsigned char Addressability;
		//Keeps track of resolved and unresolved symbols
		SymbolTable TheSymbolTable;
		//list of segments in the program.
		list<Segment *> Segments;
		//list of structures in the program.
		list<Segment *> Structures;
		//The input ID and line number this program was found on
		LocationVector LocationStack;

		/**********************************************************************\
			Program( [in] location stack, [in] file name, [in] addressability )
			
			This initializes the data.
			By default the program begins at address 0.
		\******/
		Program(const LocationVector &, const string &, unsigned char);

		/**********************************************************************\
			ResolveStructDefs( [in] error callback function )
			
			This runs through the structs, calculating and updating the size of
			every structure. Checks for recursive definitions.
		\******/
		virtual bool ResolveStructDefs(CallBackFunction);

		/**********************************************************************\
			ResolveAddresses( [in-out] current address, 
				[in] error callback function )
			
			This runs through the segments. If the segment's uses a dynamic
			address, it updates it's address. Calls ResolveAddresses on the
			segments.Updates the size of the segment.

			The starting address of this segment is given in the parameter,
			and it returns the next unallocated address after the segment image.
		\******/
		bool ResolveAddresses(uint64 &, CallBackFunction);

		/**********************************************************************\
			GenerateImage( [out] array of memory bytes, [in] endian-ness
				[in] error callback function )

			This runs through the segments and generates the segment images
			and combines them into a program image. The data can be
			stored in little endian (true) or big endian (false) format.

			Uninitialized memory address will be zero-filled

			ResolveAddresses must be called before this. Otherwise symbols may
			have the wrong value when they are generated.

			If there is an error in the data, it will print an error message.
			It will return false if it was unable to generate the whole image.
		\******/
		bool GenerateImage(RamVector &, bool, CallBackFunction) const;

		/**********************************************************************\
			Prints the program
		\******/
		virtual operator const char *() const;

		/**********************************************************************\
			Prints the program and all its segments
		\******/
		friend ostream &operator <<(ostream &, Program &);

		/**********************************************************************\
			~Program()
			
			Deletes all the Segments.
		\******/
		virtual ~Program();
	};
}

#endif
