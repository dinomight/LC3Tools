//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

/*
	*Remove*
	#

	*Change*
	.DEFINE		-> DEFINE
	.ORIG		-> ORIGIN
	.SEGMENT	-> SEGMENT

	*Mangle*	-> _m
	abs
	align
	baserel
	data4
	data8
	define
	else
	end
	extern
	ifdef
	ifndef
	include
	len
	macro
	nop
	origin
	real1
	real4
	real8
	seg
	segment
	segrel
	size
	struct
	structdef
*/

int Dot(istream &, char *, ostream &);
int DotD(istream &, char *, ostream &);
int DotO(istream &, char *, ostream &);
int DotS(istream &, char *, ostream &);
int A(istream &, char *, ostream &);
int AB(istream &, char *, ostream &);
int AL(istream &, char *, ostream &);
int B(istream &, char *, ostream &);
int D(istream &, char *, ostream &);
int DA(istream &, char *, ostream &);
int DE(istream &, char *, ostream &);
int E(istream &, char *, ostream &);
int EL(istream &, char *, ostream &);
int EN(istream &, char *, ostream &);
int EX(istream &, char *, ostream &);
int I(istream &, char *, ostream &);
int IF(istream &, char *, ostream &);
int IFD(istream &, char *, ostream &);
int IFN(istream &, char *, ostream &);
int IN(istream &, char *, ostream &);
int L(istream &, char *, ostream &);
int M(istream &, char *, ostream &);
int N(istream &, char *, ostream &);
int O(istream &, char *, ostream &);
int R(istream &, char *, ostream &);
int S(istream &, char *, ostream &);
int SE(istream &, char *, ostream &);
int SI(istream &, char *, ostream &);
int ST(istream &, char *, ostream &);

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		cout << "\nProvide Asm file name as parameter.\n\n";
		return 0;
	}
	string sInFileName = argv[1];
	string sOutFileName;
	unsigned int Period = sInFileName.find_last_of('.');
	unsigned int ForSlash = sInFileName.find_last_of('/');
	unsigned int BackSlash = sInFileName.find_last_of('\\');

	if(ForSlash != string::npos && Period > ForSlash && (BackSlash == string::npos || Period > BackSlash)
		|| BackSlash != string::npos && Period > BackSlash && (ForSlash == string::npos || Period > ForSlash)
		|| ForSlash == string::npos && BackSlash == string::npos)
		(sOutFileName = sInFileName.substr(0, Period) += "_v3.asm");
	else
		(sOutFileName = sInFileName) += "_v3.asm";


	ifstream InputFile(sInFileName.c_str());
	if(!InputFile.good())
	{
		cout << "\nUnable to open file " << sInFileName.c_str() << "\n\n";
		return 0;
	}
	ofstream OutputFile(sOutFileName.c_str());
	if(!InputFile.good())
	{
		cout << "\nUnable to create file " << sOutFileName.c_str() << "\n\n";
		return 0;
	}

	bool fInComment = false;
	bool fInWord = false;
	bool fInQuote = false;
	bool fInChar = false;
	bool fInEscape = false;
	int TempChar, RetChar = -1;
	char sCurrent[10];
	while(InputFile.good())
	{
		if(RetChar == -1)
			TempChar = InputFile.get();
		else
			TempChar = RetChar;
		RetChar = -1;
		if(TempChar == -1)	//EOF
			break;
		sCurrent[0] = TempChar;
		sCurrent[1] = 0;

		switch(TempChar)
		{
		case '\n':
		case '\r':
			fInComment = false;
			fInWord = false;
			fInQuote = false;
			fInChar = false;
			fInEscape = false;
		case ' ':
		case '\t':
		case ',':
		case '[':
		case ']':
		case '?':
		case ':':
			fInWord = false;
			OutputFile.put((char)TempChar);
			break;
		case ';':
			fInWord = false;
			if(!fInQuote && !fInChar)
				fInComment = true;
			OutputFile.put((char)TempChar);
			break;
		case '"':
			fInWord = false;
			if(!fInComment && !fInChar && !fInEscape)
			{
				if(fInQuote)
					fInQuote = false;
				else
					fInQuote = true;
			}
			OutputFile.put((char)TempChar);
			break;
		case '\'':
			fInWord = false;
			if(!fInComment && !fInQuote && !fInEscape)
			{
				if(fInChar)
					fInChar = false;
				else
					fInChar = true;
			}
			OutputFile.put((char)TempChar);
			break;
		case '\\':
			fInWord = false;
			if(!fInComment && (fInQuote || fInChar))
			{
				if(fInEscape)
					fInEscape = false;
				else if(fInChar || fInQuote)
					fInEscape = true;
			}
			OutputFile.put((char)TempChar);
			break;
		case '#':
			fInWord = false;
			break;
		case '.':
			fInWord = false;
			if(!fInComment && !fInQuote && !fInChar)
				RetChar = Dot(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
			
		case 'a':
		case 'A':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = A(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 'b':
		case 'B':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = B(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 'd':
		case 'D':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = D(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 'e':
		case 'E':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = E(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 'i':
		case 'I':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = I(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 'l':
		case 'L':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = L(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 'm':
		case 'M':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = M(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 'n':
		case 'N':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = N(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 'o':
		case 'O':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = O(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 'r':
		case 'R':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = R(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		case 's':
		case 'S':
			if(!fInComment && !fInWord && !fInQuote && !fInChar)
				RetChar = S(InputFile, sCurrent, OutputFile);
			else
				OutputFile.put((char)TempChar);
			break;
		default:
			OutputFile.put((char)TempChar);
		}

		if(TempChar >= 'a' && TempChar <= 'z' || TempChar >= 'A' && TempChar <= 'Z' || TempChar == '_')
			fInWord = true;
		if(TempChar != '\\')
			fInEscape = false;
	}

	return 0;
}

int Dot(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int TempChar = InputFile.get();
	if(TempChar == -1)	//EOF
	{
		OutputFile << sCurrent;
		return -1;
	}
	sCurrent[1] = TempChar;
	sCurrent[2] = 0;
	switch(TempChar)
	{
	case 'd':
	case 'D':
		return DotD(InputFile, sCurrent, OutputFile);
	case 'o':
	case 'O':
		return DotO(InputFile, sCurrent, OutputFile);
	case 's':
	case 'S':
		return DotS(InputFile, sCurrent, OutputFile);
	default:
		sCurrent[1] = 0;
		OutputFile << sCurrent;
		return TempChar;
	}
}

int DotD(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'f' && TempChar != 'F')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'i' && TempChar != 'I')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'n' && TempChar != 'N')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;

	OutputFile << "DEFINE";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int DotO(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'r' && TempChar != 'R')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'i' && TempChar != 'I')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'g' && TempChar != 'G')
		goto Done;

	OutputFile << "ORIGIN";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int DotS(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'g' && TempChar != 'G')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'm' && TempChar != 'M')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'n' && TempChar != 'N')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 't' && TempChar != 'T')
		goto Done;

	OutputFile << "SEGMENT";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int A(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int TempChar = InputFile.get();
	if(TempChar == -1)	//EOF
	{
		OutputFile << sCurrent;
		return -1;
	}
	sCurrent[1] = TempChar;
	sCurrent[2] = 0;
	switch(TempChar)
	{
	case 'b':
	case 'B':
		return AB(InputFile, sCurrent, OutputFile);
	case 'l':
	case 'L':
		return AL(InputFile, sCurrent, OutputFile);
	default:
		sCurrent[1] = 0;
		OutputFile << sCurrent;
		return TempChar;
	}
}

int AB(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 1;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 's' && TempChar != 'S')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int AL(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 1;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'i' && TempChar != 'I')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'g' && TempChar != 'G')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'n' && TempChar != 'N')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int B(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 1;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'a' && TempChar != 'A')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 's' && TempChar != 'S')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'r' && TempChar != 'R')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'l' && TempChar != 'L')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int D(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int TempChar = InputFile.get();
	if(TempChar == -1)	//EOF
	{
		OutputFile << sCurrent;
		return -1;
	}
	sCurrent[1] = TempChar;
	sCurrent[2] = 0;
	switch(TempChar)
	{
	case 'a':
	case 'A':
		return DA(InputFile, sCurrent, OutputFile);
	case 'e':
	case 'E':
		return DE(InputFile, sCurrent, OutputFile);
	default:
		sCurrent[1] = 0;
		OutputFile << sCurrent;
		return TempChar;
	}
}

int DA(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 't' && TempChar != 'T')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'a' && TempChar != 'A')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != '4' && TempChar != '8')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int DE(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'f' && TempChar != 'F')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'i' && TempChar != 'I')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'n' && TempChar != 'N')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int E(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int TempChar = InputFile.get();
	if(TempChar == -1)	//EOF
	{
		OutputFile << sCurrent;
		return -1;
	}
	sCurrent[1] = TempChar;
	sCurrent[2] = 0;
	switch(TempChar)
	{
	case 'l':
	case 'L':
		return EL(InputFile, sCurrent, OutputFile);
	case 'n':
	case 'N':
		return EN(InputFile, sCurrent, OutputFile);
	case 'x':
	case 'X':
		return EX(InputFile, sCurrent, OutputFile);
	default:
		sCurrent[1] = 0;
		OutputFile << sCurrent;
		return TempChar;
	}
}

int EL(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 's' && TempChar != 'S')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int EN(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'd' && TempChar != 'D')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int EX(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 't' && TempChar != 'T')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'r' && TempChar != 'R')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'n' && TempChar != 'N')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int I(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int TempChar = InputFile.get();
	if(TempChar == -1)	//EOF
	{
		OutputFile << sCurrent;
		return -1;
	}
	sCurrent[1] = TempChar;
	sCurrent[2] = 0;
	switch(TempChar)
	{
	case 'f':
	case 'F':
		return IF(InputFile, sCurrent, OutputFile);
	case 'n':
	case 'N':
		return IN(InputFile, sCurrent, OutputFile);
	default:
		sCurrent[1] = 0;
		OutputFile << sCurrent;
		return TempChar;
	}
}

int IF(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int TempChar = InputFile.get();
	if(TempChar == -1)	//EOF
	{
		OutputFile << sCurrent;
		return -1;
	}
	sCurrent[2] = TempChar;
	sCurrent[3] = 0;
	switch(TempChar)
	{
	case 'd':
	case 'D':
		return IFD(InputFile, sCurrent, OutputFile);
	case 'n':
	case 'N':
		return IFN(InputFile, sCurrent, OutputFile);
	default:
		sCurrent[2] = 0;
		OutputFile << sCurrent;
		return TempChar;
	}
}

int IFD(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 3;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'f' && TempChar != 'F')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int IFN(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 3;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'f' && TempChar != 'F')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int IN(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'c' && TempChar != 'C')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'l' && TempChar != 'L')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'u' && TempChar != 'U')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'd' && TempChar != 'D')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int L(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 1;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'n' && TempChar != 'N')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int M(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 1;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'a' && TempChar != 'A')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'c' && TempChar != 'C')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'r' && TempChar != 'R')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'o' && TempChar != 'O')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int N(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 1;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'o' && TempChar != 'O')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'p' && TempChar != 'P')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int O(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 1;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'r' && TempChar != 'R')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'i' && TempChar != 'I')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'g' && TempChar != 'G')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'i' && TempChar != 'i')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'n' && TempChar != 'N')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int R(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 1;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'a' && TempChar != 'A')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'l' && TempChar != 'L')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != '1' && TempChar != '2' && TempChar != '4' && TempChar != '8')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int S(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int TempChar = InputFile.get();
	if(TempChar == -1)	//EOF
	{
		OutputFile << sCurrent;
		return -1;
	}
	sCurrent[1] = TempChar;
	sCurrent[2] = 0;
	switch(TempChar)
	{
	case 'e':
	case 'E':
		return SE(InputFile, sCurrent, OutputFile);
	case 'i':
	case 'I':
		return SI(InputFile, sCurrent, OutputFile);
	case 't':
	case 'T':
		return ST(InputFile, sCurrent, OutputFile);
	default:
		sCurrent[1] = 0;
		OutputFile << sCurrent;
		return TempChar;
	}
}

int SE(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'g' && TempChar != 'G')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int SI(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'z' && TempChar != 'Z')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'e' && TempChar != 'E')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}

int ST(istream &InputFile, char *sCurrent, ostream &OutputFile)
{
	int i = 2;
	int TempChar;

	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'r' && TempChar != 'R')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'u' && TempChar != 'U')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 'c' && TempChar != 'C')
		goto Done;
	sCurrent[i++] = TempChar = InputFile.get();
	if(TempChar != 't' && TempChar != 'T')
		goto Done;

	sCurrent[i] = 0;
	OutputFile << sCurrent << "_m";
	return -1;

Done:
	sCurrent[--i] = 0;
	OutputFile << sCurrent;
	return TempChar;
}
