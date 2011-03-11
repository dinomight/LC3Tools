#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include "../Assembler/Base.h"
using namespace std;

namespace LC3bTools
{
bool SimCommand(string &sCommand);
}
using namespace LC3bTools;

bool fLeftOvers;

int main()
{
	char c = 0x92, *p = &c;
	int i = c;
	int j = (unsigned char)c;
	int k = (unsigned int)c;
	int l = (int)c;
	int m = c & 255;
	int n = *p;
	int o = (unsigned int)*p;
	int q = *p&255;
	return 0;
	char *buff = "awise@crhc.uiuc.edu";
	for(unsigned int i = 0; i < strlen(buff); i++)
	{
		printf("%%%.2x", (unsigned int)buff[i]);
	}
	return 0;
	char buff1[256], buff2[256];
	cin >> buff1;
	cin >> buff2;
	cout << CreateFileNameRelative(buff1, buff2).c_str() << endl;
	return 0;

//	ifstream blah;
//	blah.rdbuf()->open(stdin);
	fLeftOvers = false;
	string sCommand;
	SimCommand(sCommand);
	cout << sCommand.c_str() << endl;
	SimCommand(sCommand);
	cout << sCommand.c_str() << endl;
	return 0;
}

bool LC3bTools::SimCommand(string &sCommand)
{
	if(fLeftOvers)
	{
		cin.seekg(0, ios::end);
		fLeftOvers = false;
	}
	cout << "1 good=" << cin.good() << " bad=" << cin.bad() << " fail=" << cin.fail() << " eof=" << cin.eof() << endl;
	cout << "sim> ";
	fflush(NULL);

	//Temporary string storage
	char sTempBuff[128+2];
	bool fRetVal = true;

	cout << "2 good=" << cin.good() << " bad=" << cin.bad() << " fail=" << cin.fail() << " eof=" << cin.eof() << endl;
	sTempBuff[128+1] = 1;
	//get a line from the assembly file
	fgets(sTempBuff, 128+1, stdin);
	cout << "3 good=" << cin.good() << " bad=" << cin.bad() << " fail=" << cin.fail() << " eof=" << cin.eof() << endl;
	//*NOTE: MSVC's STL version of get() sets the failbit if it gets no characters.
	if(!sTempBuff[0])
	{
	cout << "c good=" << cin.good() << " bad=" << cin.bad() << " fail=" << cin.fail() << " eof=" << cin.eof() << endl;

		cin.clear(cin.rdstate() & ~ios::failbit);
	}

	cout << "4 good=" << cin.good() << " bad=" << cin.bad() << " fail=" << cin.fail() << " eof=" << cin.eof() << endl;
//cin.seekg(1);//cin.ignore(20);	
	fseek(stdin, 0, SEEK_END);
//		cin.clear(cin.rdstate() & ~ios::failbit);
	cout << "5 good=" << cin.good() << " bad=" << cin.bad() << " fail=" << cin.fail() << " eof=" << cin.eof() << endl;

	if(cin.bad())
	{
		cout << "Error reading command." << endl;
		cin.clear();
		return false;
	}
	if(cin.eof())
		cin.clear();
	cout << "6 good=" << cin.good() << " bad=" << cin.bad() << " fail=" << cin.fail() << " eof=" << cin.eof() << endl;
	
	//Check to see if the line was too long
	if(!sTempBuff[128+1])
	{
		sTempBuff[128] = 0;
		cout << "Line exceeds %u characters. Excess ignored." << endl;
	}

	//Return the input
	sCommand = sTempBuff;

	return true;
}
