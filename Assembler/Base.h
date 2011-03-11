//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef BASE_H
#define BASE_H

#pragma warning (disable:4146)
#pragma warning (disable:4786)
#include <vector>
#include <list>
#include <string>
#include "../JMTLib/JMTLib.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	const char ASM_VER[] = "3.62";
	const double ASM_VER_NUM = 3.62;
	//the largest number of characters a register can have
	const unsigned int MAX_REGISTER_CHAR = 15;
	//The default value for uninitialized data (invalid opcode)
	const unsigned int UNINITIALIZED_VALUE = 0xF4;

	//The size of the console window
	const unsigned int MAX_CONSOLE_WIDTH = 256;
	const unsigned int MIN_CONSOLE_WIDTH = 32;
	const unsigned int CONSOLE_TAB_SIZE = 8;
	extern unsigned int ConsoleWidth;

	//Different languages for an input file
	const unsigned int NUM_LANGUAGES = 3;
	enum LanguageEnum {LangNone = 0, LangLC3, LangLC3b};
	extern const char *const sLanguages[NUM_LANGUAGES];

	//The following are all assembler settings that are setup before compiling
	extern struct Flags_t
	{
		//Assembler/simulator options
		bool fStdout, fConsoleWidth, fPrintTokens, fPrintAST, fPrintSymbols, fOutputImage, fOutputVHDL, fUseOptimizations, fOldLC3, fSimulate, fUseOS;
	} Flags;
	extern LanguageEnum DefaultLang;
	extern string sOutputFileName;
	extern list< pair<string, string> > DefineList;

	//The datatype for RAM entries <address, byte value>
	typedef vector< pair<uint64, unsigned char> > RamVector;

	//The following are useful bit-structures for float data.
	typedef unsigned char Data1;
	typedef unsigned short Data2;
	typedef unsigned int Data4;
	typedef uint64 Data8;
#ifdef BIG_ENDIAN_BUILD
	typedef struct Real1_t
	{
		Data1 Sign : 1;
		Data1 Exponent : 3;
		Data1 Mantissa : 4;
	} Real1;
	typedef struct Real2_t
	{
		Data2 Sign : 1;
		Data2 Exponent : 5;
		Data2 Mantissa : 10;
	} Real2;
	typedef struct Real4_t
	{
		Data4 Sign : 1;
		Data4 Exponent : 8;
		Data4 Mantissa : 23;
	} Real4;
	typedef struct Real8_t
	{
		Data8 Sign : 1;
		Data8 Exponent : 11;
		Data8 Mantissa : 52;
	} Real8;
#else
	typedef struct Real1_t
	{
		Data1 Mantissa : 4;
		Data1 Exponent : 3;
		Data1 Sign : 1;
	} Real1;
	typedef struct Real2_t
	{
		Data2 Mantissa : 10;
		Data2 Exponent : 5;
		Data2 Sign : 1;
	} Real2;
	typedef struct Real4_t
	{
		Data4 Mantissa : 23;
		Data4 Exponent : 8;
		Data4 Sign : 1;
	} Real4;
	typedef struct Real8_t
	{
		Data8 Mantissa : 52;
		Data8 Exponent : 11;
		Data8 Sign : 1;
	} Real8;
#endif
	typedef union Real1Bytes_t
	{
		Real1 Real;
		Data1 Int;
	} Real1Bytes;
	typedef union Real2Bytes_t
	{
		Real2 Real;
		Data2 Int;
	} Real2Bytes;
	typedef union Real4Bytes_t
	{
		Real4 Real;
		Data4 Int;
	} Real4Bytes;
	typedef union Real8Bytes_t
	{
		Real8 Real;
		Data8 Int;
	} Real8Bytes;
}

namespace Simulator
{
	const char SIM_VER[] = "1.61";
	const unsigned int MAX_CALLSTACK_DEPTH = 0x100000;
	const unsigned int MAX_DISPLAY_LENGTH = 256;
	const unsigned int MAX_DISPLAY_LENGTH_LG2 = 17;
	const unsigned int MAX_DISPLAY_ARRAY_LENGTH_LG2 = 17;

	#define SimCallBackFunction bool SimCallBack(MessageEnum, const string &)
	#define SimCommandFunction bool SimCommand(string &)
	#define SimReadConsoleFunction bool SimReadConsole(string &, unsigned int, unsigned int &)
	#define SimWriteConsoleFunction bool SimWriteConsole(const string &, unsigned int, unsigned int &)

	const unsigned int DEFAULT_MEMORY_DISPLAY_LENGTH = 128;

	extern string sProgramDir;
}

#endif
