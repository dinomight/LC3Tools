//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Segment.h"
#include <cstdio>
#include "Data.h"

using namespace std;
using namespace JMT;

namespace Assembler	{

Segment::Segment(const LocationVector &LS, unsigned char addressability)
{
	fDynamicOffset = true;
	Addressability = addressability;
	LocationStack = LS;
	fStruct = false;
	fRecursive = false;
}

Segment::Segment(const LocationVector &LS, unsigned char addressability, uint64 offset)
{
	fDynamicOffset = false;
	Addressability = addressability;
	Offset = offset;
	LocationStack = LS;
	fStruct = false;
	fRecursive = false;
}

Segment::Segment(const LocationVector &LS, unsigned char addressability, const string &sstruct)
{
	fDynamicOffset = true;
	Addressability = addressability;
	LocationStack = LS;
	fStruct = true;
	sStruct = sstruct;
	fRecursive = false;
}

Segment *Segment::Copy() const
{
	Segment *pThis = const_cast<Segment *>(this);
	Segment *pSegment = new Segment(*this);
	pSegment->Sequence.clear();
	for(list<Element *>::const_iterator SequenceIter = Sequence.begin(); SequenceIter != Sequence.end(); SequenceIter++)
		pSegment->Sequence.push_back((*SequenceIter)->Copy());
	return pSegment;
}

bool Segment::ResolveStructDef(CallBackFunction)
{
	bool fRetVal = true;
	uint64 PreviousSize;
	PreviousSize = Size = Address = 0;
	fRecursive = true;

	//Go over each element in the sequence and add up how many bytes they take up.
	for(list<Element *>::iterator SequenceIter = Sequence.begin(); SequenceIter != Sequence.end(); SequenceIter++)
	{
		(*SequenceIter)->Address = Size;

		if((*SequenceIter)->ElementType == StructElement)
		{
			if(!reinterpret_cast<Struct *>(*SequenceIter)->ResolveStructDef(CallBack))
				fRetVal = false;
		}
		else if((*SequenceIter)->ElementType == AlignElement)
		{
			uint64 Alignment = reinterpret_cast<Align *>(*SequenceIter)->Alignment;
			LocationVector AlignLS = reinterpret_cast<Align *>(*SequenceIter)->LocationStack;
			delete *SequenceIter;
			if(Alignment == 0 || Size % Alignment == 0)
			{
				//Remove the alignment element
				list<Element *>::iterator SequenceIter2 = SequenceIter;
				SequenceIter2++;
				Sequence.erase(SequenceIter);
				SequenceIter = SequenceIter2;
				SequenceIter--;
				continue;
			}
			else
			{
				//Put a data array into the alignment's place
				(*SequenceIter) = new Data(AlignLS, DATA1, Addressability, Alignment - Size % Alignment, vector<Number *>());
				SequenceIter--;
				continue;
			}
		}

		//Add the size of this element to the address
		Size += (*SequenceIter)->Size;

		if(Size < PreviousSize)
		{
			CallBack(Error, "Struct size larger than can be counted.", (*SequenceIter)->LocationStack);
			fRetVal = false;
		}

		PreviousSize = Size;
	}
	
	fRecursive = false;
	return fRetVal;
}

bool Segment::ResolveAddresses(uint64 &CurrentAddress, CallBackFunction)
{
	Address = CurrentAddress;
	uint64 PreviousAddress = Address, OrigAddress = CurrentAddress;
	bool fRetVal = true;

	//Go over each element in the sequence and add up how many bytes they take up.
	for(list<Element *>::iterator SequenceIter = Sequence.begin(); SequenceIter != Sequence.end(); SequenceIter++)
	{
		//Update the address of each element
		(*SequenceIter)->Address = CurrentAddress;

		if((*SequenceIter)->ElementType == StructElement)
		{
			if(!reinterpret_cast<Struct *>(*SequenceIter)->ResolveAddresses(CurrentAddress, CallBack))
				fRetVal = false;
		}
		else if((*SequenceIter)->ElementType == AlignElement)
		{
			uint64 Alignment = reinterpret_cast<Align *>(*SequenceIter)->Alignment;
			LocationVector AlignLS = reinterpret_cast<Align *>(*SequenceIter)->LocationStack;
			delete *SequenceIter;
			if(Alignment == 0 || CurrentAddress % Alignment == 0)
			{
				//Remove the alignment element
				list<Element *>::iterator SequenceIter2 = SequenceIter;
				SequenceIter2++;
				Sequence.erase(SequenceIter);
				SequenceIter = SequenceIter2;
				SequenceIter--;
				continue;
			}
			else
			{
				//Put a data array int the alignment's place
				(*SequenceIter) = new Data(AlignLS, DATA1, Addressability, Alignment - CurrentAddress % Alignment, vector<Number *>());
				SequenceIter--;
				continue;
			}
		}
		else
			//Add the size of this element to the address
			CurrentAddress += (*SequenceIter)->Size;

		if(CurrentAddress < PreviousAddress)
		{
			CallBack(Error, "Element at address higher than can be counted.", (*SequenceIter)->LocationStack);
			fRetVal = false;
		}

		PreviousAddress = CurrentAddress;
	}
	
	Size = CurrentAddress - Address;

	return fRetVal;
}

bool Segment::GenerateImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();

	//Go over each element in the sequence and add their image to the Ram.
	for(list<Element *>::const_iterator SequenceIter = Sequence.begin(); SequenceIter != Sequence.end(); SequenceIter++)
	{
		RamVector ElementImage;
		if( !(*SequenceIter)->GetImage(ElementImage, fLittleEndian, CallBack) )
			fRetVal = false;
		vRam.insert(vRam.end(), ElementImage.begin(), ElementImage.end());
	}

	return fRetVal;
}

Segment::operator const char *() const
{
	static char sSegment[32 + MAX_IDENTIFIER_CHAR];

	if(fStruct)
		sprintf(sSegment, "STRUCTDEF %.63s", sStruct.c_str());
	else if(fDynamicOffset)
		sprintf(sSegment, "SEGMENT");
	else
	{
		#if defined _MSC_VER
			sprintf(sSegment, "SEGMENT 4x%I64X", Offset >> Addressability);
		#elif defined GPLUSPLUS
			sprintf(sSegment, "SEGMENT 4x%llX", Offset >> Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
	}

	return sSegment;
}

ostream &operator <<(ostream &Output, Segment &Seg)
{
	//print the segment
	Output << (const char *)Seg << endl;

	//Go over each element in the sequence
	for(list<Element *>::iterator SequenceIter = Seg.Sequence.begin(); SequenceIter != Seg.Sequence.end(); SequenceIter++)
	{
		if((*SequenceIter)->ElementType == LabelElement)
			Output << (const char *)**SequenceIter << endl;
		else
			Output << "\t" << (const char *)**SequenceIter << endl;
	}

	if(Seg.fStruct)
		Output << "END\n";

	return Output;
}

Segment::~Segment()
{
	//Delete all the elements
	//Go over each element in the sequence
	for(list<Element *>::iterator SequenceIter = Sequence.begin(); SequenceIter != Sequence.end(); SequenceIter++)
		delete *SequenceIter;
}

}	//namespace Assembler
