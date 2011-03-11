//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef ELEMENT_H
#define ELEMENT_H

#pragma warning (disable:4786)
#include "Number.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	enum ElementEnum {LabelElement, DataElement, StructElement, AlignElement, InstructionElement};
	typedef vector< pair<uint64, unsigned char> > RamVector;

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Element

		An instance of this class is a symbolic representation of an element
		in an assembly program.

		This element is something that belongs in a control flow or
		program sequence.

		The relative locations of Elements could by changed by
		optimization procedures.

		In order to know where the element originated from, each element stores
		a location stack. The last entry in the stack is the
		<input ID, line number> that the element came from. Input ID is
		the index into the InputList which holds filenames of input assembly
		files. Line number is the line within that file.
		All previous entries in the stack recursive locations each file was
		included from in other assembly files.

		In order to enable multi-threaded GetImage(), locks will need to be
		placed around accesses to sMessageBuffer, or create a non-static buffer
		unique to each function that uses it.

		Usage of the operator const char *() must be done serially, not
		multi-threaded.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Element
	{
	protected:
		//Buffer for formatting error messages
		static char sMessageBuffer[130 + 2*MAX_IDENTIFIER_CHAR];
		//Buffer for printing elements
		static char sElement[129 + MAX_IDENTIFIER_CHAR];

	public:
		//The type of element this is
		ElementEnum ElementType;
		//The byte address of this element in memory
		uint64 Address;
		//The number of bytes this element will take up in memory
		uint64 Size;
		//The number of array elements in this element. If this element is an
		//array, the actaul size of each element is Size/Length
		uint64 Length;
		//The input ID and line number the element was found on
		LocationVector LocationStack;
		//An identifer for a location this element was put in (defined by the
		//implementation
		unsigned int LocationID;

		/**********************************************************************\
			Element( [in] location stack )
		\******/
		Element(const LocationVector &);

		/**********************************************************************\
			Make a copy of the Element
		\******/
		virtual Element *Copy() const = 0;

		/**********************************************************************\
			AssignValues( [in-out] start iter, [in] end iter )

			Assign Number objects to this element. Uses numbers starting at
			start iter, and ending at end iter, or when all required numbers
			used. start iter is updated to the first unused number.

			If a number was already given at construction, that number will be
			deleted first, and then the new number takes its place. A copy
			of the new number of made.
		\******/
		virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &) = 0;

		/**********************************************************************\
			GetImage( [out] byte array, [in] endian-ness,
				[in] error callback function )
			
			Computes the binary image that this element represents. It returns
			it in the byte array. The data is converted to bytes based on
			whether it is little endian (true) or big endian (false).

			If there is an error in the data, it will print an error message and
			return false.
		\******/
		virtual bool GetImage(RamVector &, bool, CallBackFunction) const = 0;

		/**********************************************************************\
			Prints the element
		\******/
		virtual operator const char *() const = 0;

		virtual ~Element() = 0;
	};
}

#endif

	/**************************************************************************\
		SetAddress( [in] Address )
		
		Sets a new address. If a symbol is referencing this element,
		it updates the symbol with this new address.
	virtual void SetAddress(unsigned int);
	\******/

	/**************************************************************************\
		GetAddress()
		
		Returns the address this element is at.
	virtual unsigned int GetAddress()	{	return Address;	}
	\******/

	/**************************************************************************\
		GetSize()
		
		Returns the number of bytes this element will take up in memory.
	virtual unsigned int GetSize()	{	return Size;	}
	\******/
