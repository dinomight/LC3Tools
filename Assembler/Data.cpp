//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Data.h"
#include <cstdio>
#include <strstream>

using namespace std;
using namespace JMT;

namespace Assembler	{

const unsigned char vDataBytes[NUM_DATATYPES] = {1, 2, 4, 8, 1 ,2 ,4, 8, 1};
const unsigned char vExponentBits[NUM_DATATYPES] = {0, 0, 0, 0, 3, 5, 8, 11, 11};
const unsigned char vMantissaBits[NUM_DATATYPES] = {0, 0, 0, 0, 4, 10, 23, 52, 52};

Data::Data(const LocationVector &LocationStack, DataEnum datatype, unsigned char addressability, Number *Data) : Element(LocationStack)
{
	ElementType = DataElement;
	DataType = datatype;
	Addressability = addressability;
	vData.push_back(Data);
	Size = vDataBytes[DataType];
	Length = 1;
}

Data::Data(const LocationVector &LocationStack, DataEnum datatype, unsigned char addressability, uint64 length, const vector<Number *> &Data) : Element(LocationStack)
{
	ElementType = DataElement;
	DataType = datatype;
	Addressability = addressability;
	vData = Data;
	Size = length * vDataBytes[DataType];
	Length = length;
	if(Length < vData.size())
		throw "Data array size larger than array length!";
}

Element *Data::Copy() const
{
	Data *pData = new Data(*this);
	for(vector<Number *>::iterator DataIter = pData->vData.begin(); DataIter != pData->vData.end(); DataIter++)
		*DataIter = (*DataIter)->Copy();
	return pData;
}

void Data::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	vector<Number *>::iterator DataIter = vData.begin();
	for(uint64 i = 0; i < Length; i++)
	{
		if(StartIter == EndIter)
			//No more data to assign
			return;
		if(DataIter == vData.end())
			//No more pre-initialized data to overwrite
			vData.push_back((*StartIter)->Copy());
		else if((*StartIter)->NumberType != NumVoid)
		{
			//Need to overwrite pre-initialized data
			delete *DataIter;
			*DataIter = (*StartIter)->Copy();
			DataIter++;
		}
		StartIter++;
	}
}

bool Data::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	Data1 *TempData;
	unsigned char TempSign;
	unsigned short TempExponent;
	uint64 TempMantissa;
	Data1 TempData1;
	Data2 TempData2;
	Data4 TempData4;
	Data8 TempData8;
	Real1 TempReal1;
	Real2 TempReal2;
	Real4 TempReal4;
	Real8 TempReal8;
	uint64 TempInt64, CurrentAddress = Address;

	vRam.clear();

	for(vector<Number *>::const_iterator DataIter = vData.begin(); DataIter != vData.end(); DataIter++)
	{
		if((*DataIter)->NumberType == NumVoid)
			continue;

		switch(DataType)
		{
		case DATA1:
			if(!(*DataIter)->Int(8, false, TempInt64, CallBack, " for DATA1", false, Addressability))
				fRetVal = false;
			TempData1 = (Data1)TempInt64;
			TempData = (Data1 *)&TempData1;
			break;
		case DATA2:
			if(!(*DataIter)->Int(16, false, TempInt64, CallBack, " for DATA2", false, Addressability))
				fRetVal = false;
			TempData2 = (Data2)TempInt64;
			TempData = (Data1 *)&TempData2;
			break;
		case DATA4:
			if(!(*DataIter)->Int(32, false, TempInt64, CallBack, " for DATA4", false, Addressability))
				fRetVal = false;
			TempData4 = (Data4)TempInt64;
			TempData = (Data1 *)&TempData4;
			break;
		case DATA8:
			if(!(*DataIter)->Int(64, false, TempInt64, CallBack, " for DATA8", false, Addressability))
				fRetVal = false;
			TempData8 = TempInt64;
			TempData = (Data1 *)&TempData8;
			break;
		case REAL1:
			if(!(*DataIter)->Float(3, 4, TempSign, TempExponent, TempMantissa, CallBack, " for REAL1", false, Addressability))
				fRetVal = false;
			TempReal1.Sign = TempSign;
			TempReal1.Exponent = TempExponent;
			TempReal1.Mantissa = TempMantissa;
			TempData = (Data1 *)&TempReal1;
			break;
		case REAL2:
			if(!(*DataIter)->Float(5, 10, TempSign, TempExponent, TempMantissa, CallBack, " for REAL2", false, Addressability))
				fRetVal = false;
			TempReal2.Sign = TempSign;
			TempReal2.Exponent = TempExponent;
			TempReal2.Mantissa = TempMantissa;
			TempData = (Data1 *)&TempReal2;
			break;
		case REAL4:
			if(!(*DataIter)->Float(8, 23, TempSign, TempExponent, TempMantissa, CallBack, " for REAL4", false, Addressability))
				fRetVal = false;
			TempReal4.Sign = TempSign;
			TempReal4.Exponent = TempExponent;
			TempReal4.Mantissa = TempMantissa;
			TempData = (Data1 *)&TempReal4;
			break;
		case REAL8:
			if(!(*DataIter)->Float(11, 52, TempSign, TempExponent, TempMantissa, CallBack, " for REAL8", false, Addressability))
				fRetVal = false;
			TempReal8.Sign = TempSign;
			TempReal8.Exponent = TempExponent;
			TempReal8.Mantissa = TempMantissa;
			TempData = (Data1 *)&TempReal8;
			break;
		}

		for(unsigned int i = 0; i < vDataBytes[DataType]; i++)
		{
			if(fLittleEndian)
#ifdef BIG_ENDIAN_BUILD
				vRam.push_back(RamVector::value_type(CurrentAddress + i, TempData[vDataBytes[DataType] - i - 1]));
#else
				vRam.push_back(RamVector::value_type(CurrentAddress + i, TempData[i]));
#endif
			else	//Big Endian
#ifdef BIG_ENDIAN_BUILD
				vRam.push_back(RamVector::value_type(CurrentAddress + i, TempData[i]));
#else
				vRam.push_back(RamVector::value_type(CurrentAddress + i, TempData[vDataBytes[DataType] - i - 1]));
#endif
		}
		CurrentAddress += vDataBytes[DataType];
	}

	return fRetVal;
}

Data::operator const char *() const
{
	static string sData;
	ostrstream strData;

	strData << sDataTypes[DataType];
	if(Length > 1)
	{
		#if defined _MSC_VER
			sprintf(sElement, "4x%I64X", Length);
		#elif defined GPLUSPLUS
			sprintf(sElement, "4x%llX", Length);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		strData << "[" << sElement << "]";
	}
	strData << "\t";

	unsigned int Count = 0;
	for(vector<Number *>::const_iterator DataIter = vData.begin(); DataIter != vData.end(); DataIter++, Count++)
	{
		if(DataIter != vData.begin())
		{
			strData << ", ";
			//Insert line breaks occasionally so we don't go over the assembler's max input line length
			if(Count % 16 == 0)
				strData << "\n";
		}
		strData << (const char *)(**DataIter);
	}

	strData << ends;
	sData = strData.str();
	return sData.c_str();
}

Data::~Data()
{
	for(vector<Number *>::iterator DataIter = vData.begin(); DataIter != vData.end(); DataIter++)
		delete *DataIter;
}

Struct::Struct(const LocationVector &LocationStack, Symbol *psymbol, const vector<Number *> &Data) : Element(LocationStack)
{
	ElementType = StructElement;
	pSymbol = psymbol;
	vvData.push_back(Data);
	Length = 1;
}

Struct::Struct(const LocationVector &LocationStack, Symbol *psymbol, uint64 length, const vector< vector<Number *> > &Data) : Element(LocationStack)
{
	ElementType = StructElement;
	pSymbol = psymbol;
	vvData = Data;
	Length = length;
	if(Length < vvData.size())
		throw "Struct array size larger than array length!";
}

bool Struct::ResolveStructDef(CallBackFunction)
{
	bool fRetVal = true;

	if(pSymbol->SymbolType != SymStruct)
	{
		sprintf(sMessageBuffer, "Symbol '%.63s' is not a structure.", pSymbol->sSymbol.c_str());
		CallBack(Error, sMessageBuffer, LocationStack);
		return false;
	}

	if(pSymbol->pSegment->fRecursive)
	{
		sprintf(sMessageBuffer, "Recursive definition of structure '%.63s'.", pSymbol->sSymbol.c_str());
		CallBack(Error, sMessageBuffer, LocationStack);
		return false;
	}

	if(!pSymbol->pSegment->ResolveStructDef(CallBack))
		fRetVal = false;

	Size = pSymbol->pSegment->Size * Length;
	return fRetVal;
}

bool Struct::ResolveAddresses(uint64 &CurrentAddress, CallBackFunction)
{
	bool fRetVal = true;
	uint64 i;
	vector<Segment *>::iterator SegmentIter;
	vector< vector<Number *> >::iterator vDataIter;

	if(pSymbol->SymbolType != SymStruct)
	{
		sprintf(sMessageBuffer, "Symbol '%.63s' is not a structure.", pSymbol->sSymbol.c_str());
		CallBack(Error, sMessageBuffer, LocationStack);
		return false;
	}

	//Delete any previously created structure instantiations. If this function is
	//being called again, then likely something has changed.
	for(SegmentIter = Segments.begin(); SegmentIter != Segments.end(); SegmentIter++)
		delete *SegmentIter;
	Segments.clear();

	//Go over each structure (segment) in the array
	vDataIter = vvData.begin();
	for(i = 0; i < Length; i++)
	{
		//Create a copy of the structure definition. Store it as a segment.
		Segment *pSegment = pSymbol->pSegment->Copy();
		Segments.push_back(pSegment);

		//Copy the data initialization into the structure
		if(vDataIter != vvData.end())
		{
			//Go over each element in the sequence and assign the data to it.
			vector<Number *>::iterator DataIter = vDataIter->begin();
			for(list<Element *>::iterator SequenceIter = pSegment->Sequence.begin(); SequenceIter != pSegment->Sequence.end(); SequenceIter++)
			{
				(*SequenceIter)->AssignValues(DataIter, vDataIter->end());
				if(DataIter == vDataIter->end())
					break;
			}
			//See if the initialization gave more data than the structure holds
			if( DataIter != vDataIter->end() && !(vDataIter->size() == 1 && (*DataIter)->NumberType == NumVoid) )
			{
				if(Length > 1)
					sprintf(sMessageBuffer, "More data supplied to structure '%.63s[%u]' initialization than used.", pSymbol->sSymbol.c_str(), i);
				else
					sprintf(sMessageBuffer, "More data supplied to structure '%.63s' initialization than used.", pSymbol->sSymbol.c_str());
				CallBack(Warning, sMessageBuffer, (*DataIter)->LocationStack);
			}
			vDataIter++;
		}
		if(!pSegment->ResolveAddresses(CurrentAddress, CallBack))
			fRetVal = false;
	}

	Size = CurrentAddress - Address;
	return fRetVal;
}

Element *Struct::Copy() const
{
	Struct *pStruct = new Struct(*this);
	for(vector< vector<Number *> >::iterator vDataIter = pStruct->vvData.begin(); vDataIter != pStruct->vvData.end(); vDataIter++)
		for(vector<Number *>::iterator DataIter = vDataIter->begin(); DataIter != vDataIter->end(); DataIter++)
			*DataIter = (*DataIter)->Copy();
	return pStruct;
}

void Struct::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
	//*NOTE: Still need to think up some way to initialize nested structures.
}

bool Struct::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();

	for(vector<Segment *>::const_iterator SegmentIter = Segments.begin(); SegmentIter != Segments.end(); SegmentIter++)
	{
		RamVector SegmentImage;
		if(!(*SegmentIter)->GenerateImage(SegmentImage, fLittleEndian, CallBack))
			fRetVal = false;
		vRam.insert(vRam.end(), SegmentImage.begin(), SegmentImage.end());
	}

	return true;
}

Struct::operator const char *() const
{
	static string sData;
	ostrstream strData;

	strData << sDataTypes[STRUCT] << " " << pSymbol->sSymbol;
	if(Length > 1)
	{
		#if defined _MSC_VER
			sprintf(sElement, "4x%I64X", Length);
		#elif defined GPLUSPLUS
			sprintf(sElement, "4x%llX", Length);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		strData << "[" << sElement << "]";
	}
	strData << "\t";

	for(vector< vector<Number *> >::const_iterator vDataIter = vvData.begin(); vDataIter != vvData.end(); vDataIter++)
	{
		if(vDataIter != vvData.begin())
			strData << ",\n";
		strData << "{";
		unsigned int Count = 0;
		for(vector<Number *>::const_iterator DataIter = vDataIter->begin(); DataIter != vDataIter->end(); DataIter++, Count++)
		{
			if(DataIter != vDataIter->begin())
			{
				strData << ", ";
				//Insert line breaks occasionally so we don't go over the assembler's max input line length
				if(Count % 16 == 0)
					strData << "\n";
			}
			strData << (const char *)(**DataIter);
		}
		strData << "}";
	}

	strData << ends;
	sData = strData.str();
	return sData.c_str();
}

Struct::~Struct()
{
	for(vector< vector<Number *> >::iterator vDataIter = vvData.begin(); vDataIter != vvData.end(); vDataIter++)
		for(vector<Number *>::iterator DataIter = vDataIter->begin(); DataIter != vDataIter->end(); DataIter++)
			delete *DataIter;

	for(vector<Segment *>::iterator SegmentIter = Segments.begin(); SegmentIter != Segments.end(); SegmentIter++)
		delete *SegmentIter;
}

Align::Align(const LocationVector &LocationStack, uint64 alignment) : Element(LocationStack)
{
	ElementType = AlignElement;
	Alignment = alignment;
}

Element *Align::Copy() const
{
	Align *pAlign = new Align(*this);
	return pAlign;
}

void Align::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
}

bool Align::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	bool fRetVal = true;
	vRam.clear();
	//No image
	return true;
}

Align::operator const char *() const
{
	#if defined _MSC_VER
		sprintf(sElement, "ALIGN 4x%I64X", Alignment);
	#elif defined GPLUSPLUS
		sprintf(sElement, "ALIGN 4x%llX", Alignment);
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif
	return sElement;
}

Align::~Align()
{
}

}	//namespace Assembler
