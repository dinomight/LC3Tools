//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include <iostream>
#include "../JMTLib/JMTLib.h"

using namespace std;
using namespace JMT;

int main(int argc, char* argv[])
{
	if(EndianCheck())
		cout << "\nLittle Endian\n\n";
	else
		cout << "\nBig Endian\n\n";
	return 0;
}
