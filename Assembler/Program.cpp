//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Program.h"
#include <cstdio>

using namespace std;
using namespace JMT;

namespace Assembler	{

Program::Program(const LocationVector &LS, const string & sFN, unsigned char addressability) : sFileName(sFN)
{
	fDynamicAddress = true;
	Addressability = addressability;
	TheSymbolTable.Addressability = Addressability;
	LocationStack = LS;
}


bool Program::ResolveStructDefs(CallBackFunction)
{
	bool fRetVal = true;

	//Check structure definitions. Calculate structure sizes. Check for recursion.
	for(list<Segment *>::iterator SegIter = Structures.begin(); SegIter != Structures.end(); SegIter++)
	{
		if(!(*SegIter)->ResolveStructDef(CallBack))
			fRetVal = false;
	}

	return fRetVal;
}

bool Program::ResolveAddresses(uint64 &CurrentAddress, CallBackFunction)
{
	Address = CurrentAddress;
	uint64 PreviousAddress = Address;
	char sMessageBuffer[128];
	bool fRetVal = true;

	//Go over each segment
	for(list<Segment *>::iterator SegmentIter = Segments.begin(); SegmentIter != Segments.end(); SegmentIter++)
	{
		if(!(*SegmentIter)->fDynamicOffset)
		{
			//The segment specified an offset from the previous segment/origin. Make sure it's valid
			uint64 OffsetAddress = PreviousAddress + (*SegmentIter)->Offset;

			if(OffsetAddress < CurrentAddress)
			{
				#if defined _MSC_VER
					sprintf(sMessageBuffer, "Segment address (4x%I64X) is before next unallocated location (4x%I64X)", OffsetAddress >> Addressability, CurrentAddress >> Addressability);
				#elif defined GPLUSPLUS
					sprintf(sMessageBuffer, "Segment address (4x%llX) is before next unallocated location (4x%llX)", OffsetAddress >> Addressability, CurrentAddress >> Addressability);
				#else
					#error "Only MSVC and GCC Compilers Supported"
				#endif
				CallBack(Error, sMessageBuffer, (*SegmentIter)->LocationStack);
				fRetVal = false;
			}
			CurrentAddress = OffsetAddress;
		}
		if(!(*SegmentIter)->ResolveAddresses(CurrentAddress, CallBack))
			fRetVal = false;

		PreviousAddress = (*SegmentIter)->Address;
	}

	Size = CurrentAddress - Address;
	return fRetVal;
}

bool Program::GenerateImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();

	//Go over each segment
	for(list<Segment *>::const_iterator SegmentIter = Segments.begin(); SegmentIter != Segments.end(); SegmentIter++)
	{
		RamVector SegmentImage;
		if( !(*SegmentIter)->GenerateImage(SegmentImage, fLittleEndian, CallBack) )
			fRetVal = false;
		vRam.insert(vRam.end(), SegmentImage.begin(), SegmentImage.end());
	}

	return fRetVal;
}

Program::operator const char *() const
{
	static char sProgram[33 + MAX_FILENAME_CHAR];
	char *psP = sProgram;

	psP += sprintf(psP, "ORIGIN");
	if(!fDynamicAddress)
	{
		#if defined _MSC_VER
			psP += sprintf(psP, " 4x%I64X", Address >> Addressability);
		#elif defined GPLUSPLUS
			psP += sprintf(psP, " 4x%llX", Address >> Addressability);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
	}
	sprintf(psP, "\t; %.255s", sFileName.Full.c_str());

	return sProgram;
}

ostream &operator <<(ostream &Output, Program &Prog)
{
	//print the program
	Output << (const char *)Prog << endl << endl;

	list<Segment *>::iterator SegmentIter;

	//Go over all the externs
	for(SymbolTable::SymbolMap::iterator SymbolIter = Prog.TheSymbolTable.ExternSymbols.begin(); SymbolIter != Prog.TheSymbolTable.ExternSymbols.end(); SymbolIter++)
		Output << "EXTERN " << SymbolIter->first.c_str() << endl;

	Output << endl;

	//Go over each structure
	for(SegmentIter = Prog.Structures.begin(); SegmentIter != Prog.Structures.end(); SegmentIter++)
		Output << **SegmentIter << endl;

	//Go over each segment
	for(SegmentIter = Prog.Segments.begin(); SegmentIter != Prog.Segments.end(); SegmentIter++)
		Output << **SegmentIter << endl;

	return Output;
}

Program::~Program()
{
	//Delete all the elements
	list<Segment *>::iterator SegmentIter;

	//Go over each segment
	for(SegmentIter = Segments.begin(); SegmentIter != Segments.end(); SegmentIter++)
		delete *SegmentIter;

	//Go over each structure
	for(SegmentIter = Structures.begin(); SegmentIter != Structures.end(); SegmentIter++)
		delete *SegmentIter;

}

}	//namespace Assembler
