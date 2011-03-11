// Copyright 2002 Ashley Wise
// JediMasterThrash@comcast.net

#include "JMTLib.h"
#include <cstdio>

using namespace std;

namespace JMT	{

string sWorkingDir;
vector<string> InputList;

bool EndianCheck()
{
	unsigned short Test;
	unsigned char *pTest;
	Test = 0x0123;
	pTest = (unsigned char *)&Test;
	if(pTest[0] == 0x23 && pTest[1] == 0x01)
		return true;
	else if(pTest[0] == 0x01 && pTest[1] == 0x23)
		return false;
	else
		throw runtime_error("Unknown endianness in platform!");
}

string ToLower(const string &sString)
{
	bool InQuote = false, InEscape = false, QuoteType = false; //false = double quotes, true = single quotes
	string sResult;

	for(unsigned int i = 0; i < sString.size(); i++)
	{
		sResult += sString[i];	//Speculative

		if(!InQuote)	//Not in quote
		{
			if(sString[i] == '\"')
			{
				QuoteType = false;
				InQuote = true;
				InEscape = false;
			}
			else if(sString[i] == '\'')
			{
				QuoteType = true;
				InQuote = true;
				InEscape = false;
			}
			else
				sResult[i] = tolower(sString[i]);
		}
		else //in quote
		{	
			if(!InEscape)	//Not in escape sequence
			{
				if(sString[i] == '\\' )
					InEscape = true;
				else if(!QuoteType && sString[i] == '\"' )
					InQuote = false;
				else if(QuoteType && sString[i] == '\'' )
					InQuote = false;
			}
			else //In Escape sequence
				InEscape = false;
		}
		if(sString[i] == '\n' || sString[i] == '\r')
		{
			InQuote = false;
			InEscape = false;
			QuoteType = false;
		}
	}

	return sResult;
}

FileName::FileName()
{
}

FileName::FileName(const string &sFN)
{
	Set(sFN);
}

FileName &FileName::operator=(const string &sFN)
{
	return Set(sFN);
}

FileName &FileName::Set(const string &sFN)
{
	//Assign the filename
	Full = sFN;
	string::size_type LastSlash = Full.find_last_of("\\/:");
	string::size_type LastPeriod = Full.find_last_of('.');
	if(LastPeriod < LastSlash)
		LastPeriod = string::npos;
	Name = Full.substr(LastSlash+1);
	Bare = Full.substr(LastSlash+1, LastPeriod-LastSlash-1);
	Path = Full.substr(0, LastSlash+1);
	Ext = LastPeriod != string::npos ? Full.substr(LastPeriod+1) : "";
	return *this;
}

string CreateStandardPath(const string &sCurrent)
{
	string sRetVal = sCurrent;
	string::size_type TempLoc;
	while((TempLoc = sRetVal.find('\\')) != string::npos)
		sRetVal[TempLoc] = '/';
	return sRetVal;
}

string CreateCompactFileName(const string &sCurrent)
{
	string::size_type TempLoc1, TempLoc2, TempLoc3;
	
	if( (TempLoc1 = sCurrent.find("..")) == string::npos )
		return sCurrent;

	//Remove any occurances of "blah/../blah" in sCurrent.
	if(TempLoc1)
	{
		TempLoc2 = sCurrent.substr(0, TempLoc1).find_last_of("\\/:");
		TempLoc3 = sCurrent.substr(TempLoc1 + 2).find_first_of("\\/:");
		if(TempLoc2 == TempLoc1 - 1 && TempLoc3 == 0)
		{
			TempLoc1 = sCurrent.substr(0, TempLoc2).find_last_of("\\/:");
			if(TempLoc2 == 0)
				return CreateCompactFileName(sCurrent.substr(TempLoc2 + 3));
			else if(TempLoc1 == string::npos)
				return CreateCompactFileName(sCurrent.substr(TempLoc2 + 4));
			else
				return CreateCompactFileName(sCurrent.substr(0, TempLoc1+1) + sCurrent.substr(TempLoc2 + 4));
		}
	}

	//Skip past a beginning "../"
	TempLoc3 = sCurrent.substr(TempLoc1 + 2).find_first_of("\\/:");
	if(TempLoc1 == 0 && TempLoc3 == 0)
		return sCurrent.substr(0,3) + CreateCompactFileName(sCurrent.substr(3));

	return sCurrent;
}

string CreateFileNameRelative(const string &scurrent, const string &srelative)
{
	string::size_type TempLoc1, TempLoc2;
	string sCurrent = CreateCompactFileName(scurrent);
	string sRelative = CreateCompactFileName(srelative);
	
	TempLoc1 = sRelative.find_first_of("\\/:");
	TempLoc2 = sRelative.find("..");

	if(TempLoc2 == 0 && TempLoc1  == 2)
	{
		//The relative file name begins with "../"
		TempLoc2 = sCurrent.find_last_of("\\/:");
		if(TempLoc2 != string::npos)
		{
			//the current file includes at least one path
			if(TempLoc2 != 0)
			{
				//current file is "*/filename"
				if(TempLoc2 >= 2 && sCurrent[TempLoc2 - 1] == '.' && sCurrent[TempLoc2 - 2] == '.')
					//We can't remove a "../" because we don't know where we came from
					return sCurrent.substr(0, TempLoc2 + 1) + sRelative;
				else
					return CreateFileNameRelative(sCurrent.substr(0, TempLoc2), sRelative.substr(TempLoc1 + 1));
			}
			else
				//current file is "/filename", wee need to keep the beginning / for an absolute path
				return CreateFileNameRelative(sCurrent.substr(0, TempLoc2 + 1), sRelative.substr(TempLoc1 + 1));
		}
		else
			//the current file has no path
			return sRelative;
	}
	else
	{
		//the file in current path
		TempLoc2 = sCurrent.find_last_of("\\/:");
		if(TempLoc1 == 0)
			//this is an absolute path
			return sRelative;
		else if(TempLoc2 != string::npos)
			//just replace the filename in current
			return sCurrent.substr(0, TempLoc2 + 1) + sRelative;
		else
			return sRelative;
	}
}

string CreateRelativeFileName(const string &scurrent, const string &srelative)
{
	string::size_type TempLoc1, TempLoc2;
	unsigned int SlashCount;
	string sCurrent = CreateCompactFileName(scurrent);
	string sRelative = CreateCompactFileName(srelative);

	TempLoc1 = sCurrent.find_first_of("\\/:");
	TempLoc2 = sRelative.find_first_of("\\/:");
	
	if(TempLoc1 == string::npos)
		//Current has no path, so Relative is already relative to current's path
		return sRelative;

	if(TempLoc1 == TempLoc2 && sCurrent.substr(0, TempLoc1) == sRelative.substr(0, TempLoc2))
		//They share a path, so remove it
		return CreateRelativeFileName(sCurrent.substr(TempLoc1+1), sRelative.substr(TempLoc2+1));

	//Their paths differ at this point.

	//if sCurrent begins with ../ but sRelative does not,
	//then we do not know how to get the sRelative from sCurrent
	if(sCurrent.find("..") == 0)
		throw runtime_error("In CreateRelativeFileName sCurrent begins with ../!");

	TempLoc1 = 0;
	SlashCount = 0;
	while( (TempLoc2 = sCurrent.substr(TempLoc1+1).find_first_of("\\/:")) != string::npos )
	{
		TempLoc1 += TempLoc2 + 1;
		SlashCount++;
	}
	
	//Back up for every folder Current is in.
	for(;SlashCount > 0; SlashCount--)
		sRelative = string("../") + sRelative;

	return sRelative;
}

const LocationVector NullLocationStack;

#ifndef NO_CHAR_NUMBERS
ostream &operator<<(ostream &Output, unsigned char UC)
{
	return Output << (unsigned short)UC;
}

ostream &operator<<(ostream &Output, signed char SC)
{
	return Output << (signed short)SC;
}

istream &operator>>(istream &Input, unsigned char &UC)
{
	unsigned short US;
	Input >> US;
	UC = (unsigned char)US;
	return Input;
}

istream &operator>>(istream &Input, signed char &SC)
{
	signed short SS;
	Input >> SS;
	SC = (signed char)SS;
	return Input;
}
#endif

#if (_MSC_VER < 1300) && !defined GPLUSPLUS
ostream &operator<<(ostream &Output, uint64 UI64)
{
	char Buff[32];
	switch(Output.flags() & ios::basefield)
	{
	case ios::hex:
		#if defined _MSC_VER
			sprintf(Buff, "%I64x", UI64);
		#elif defined GPLUSPLUS
			sprintf(Buff, "%llx", UI64);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		break;
	case ios::dec:
		#if defined _MSC_VER
			sprintf(Buff, "%I64u", UI64);
		#elif defined GPLUSPLUS
			sprintf(Buff, "%llu", UI64);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		break;
	case ios::oct:
		#if defined _MSC_VER
			sprintf(Buff, "%I64o", UI64);
		#elif defined GPLUSPLUS
			sprintf(Buff, "%llo", UI64);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		break;
	}
	return Output << Buff;
}

ostream &operator<<(ostream &Output, int64 SI64)
{
	char Buff[32];
	switch(Output.flags() & ios::basefield)
	{
	case ios::hex:
		#if defined _MSC_VER
			sprintf(Buff, "%I64x", SI64);
		#elif defined GPLUSPLUS
			sprintf(Buff, "%llx", SI64);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		break;
	case ios::dec:
		#if defined _MSC_VER
			sprintf(Buff, "%I64d", SI64);
		#elif defined GPLUSPLUS
			sprintf(Buff, "%lld", SI64);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		break;
	case ios::oct:
		#if defined _MSC_VER
			sprintf(Buff, "%I64o", SI64);
		#elif defined GPLUSPLUS
			sprintf(Buff, "%llo", SI64);
		#else
			#error "Only MSVC and GCC Compilers Supported"
		#endif
		break;
	}
	return Output << Buff;
}

istream &operator>>(istream &Input, uint64 &UI64)
{
	UI64 = LexNumber(Input);
	return Input;
}

istream &operator>>(istream &Input, int64 &SI64)
{
	SI64 = LexNumber(Input);
	return Input;
}
#endif

int64 LexNumber(istream &Input)
{
	bool fPositive = true;
	char Temp;
	uint64 Number = 0;
	enum {Oct, Dec, Hex} Base;

	//Get the sign
	Temp = Input.peek();
	if(Temp == '-')
	{
		Input.ignore();
		fPositive = false;
	}
	else if(Temp == '+')
	{
		Input.ignore();
	}

	//Get the base
	switch(Input.flags() & ios::basefield)
	{
	case ios::hex:
		Base = Hex;
		break;
	case ios::dec:
		Base = Dec;
		break;
	case ios::oct:
		Base = Oct;
		break;
	}

	//Get the number
	switch(Base)
	{
	case Oct:
		while(true)
		{
			Temp = Input.peek();
			if(Temp >= '0' && Temp <= '7' )
			{
				Number = (Number << 3) + (Temp - '0');
				Input.ignore();
			}
			else
				break;
		}
		break;
	case Dec:
		while(true)
		{
			Temp = Input.peek();
			if(Temp >= '0' && Temp <= '9' )
			{
				Number = Number * 10 + (Temp - '0');
				Input.ignore();
			}
			else
				break;
		}
		break;
	case Hex:
		//ignore a 0x before the number
		if(Input.peek() == '0')
		{
			Input.ignore();
			if(Input.peek() == 'x' || Input.peek() == 'X')
				Input.ignore();
		}

		while(true)
		{
			Temp = Input.peek();
			if(Temp >= '0' && Temp <= '9')
			{
				Number = (Number << 4) + (Temp - '0');
				Input.ignore();
			}
			else if(Temp >= 'a' && Temp <= 'f')
			{
				Number = (Number << 4) + 10 + (Temp - 'a');
				Input.ignore();
			}
			else if(Temp >= 'A' && Temp <= 'F')
			{
				Number = (Number << 4) + 10 + (Temp - 'A');
				Input.ignore();
			}
			else
				break;
		}
		break;
	}

	//Return the number
	if(fPositive)
		return Number;
	else	//Negative number
		return -Number;
}

}	//namespace JMT
