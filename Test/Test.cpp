#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

void intsig(int sig)
{
	printf("Hello.\n");
	fflush(NULL);
	signal(SIGINT, intsig);
//	char buff[10];
//	scanf("%s", buff);
//	exit(0);
}

int main()
{

	signal(SIGINT, intsig);

	while(true)
	{
		cout << "<Requesting 1 character>\n";
		bool fLeftOvers = false;
		unsigned int CharsRead = 0, CharsToRead = 1;
		string sBuffer = "";
		cout << flush;
	while(true)
	{
		if(CharsRead == CharsToRead)
		{
			if(sBuffer[CharsRead-1] != '\n')
			{
				//*NOTE: Windows console keeps the CTRL-D(4) and following characters, including the required \n(A)
				//UNIX console returns from get() as soon as CTRL-D(4) is sent.
				//The CTRL-D and all following characters are no longer available to get/peek.
#if !defined UNIX_BUILD
				if(cin.peek() == 4)
				{	//End-Of-Input given after last requested character
					fseek(stdin, 0, SEEK_END);
				}
				else
#else
				cout << endl;
#endif
					fLeftOvers = true;
			}
			break;
		}

		int TempC = fgetc(stdin);
			cout << endl << hex << TempC << endl;

		//*NOTE: CTRL-D on Unix is treated like a CTRL-Z on Windows.
		//-1 is sent if CTRL-D is the only input. CTRL-Z on unix suspends the program instead of treating it as EOF
#if !defined UNIX_BUILD
		if(TempC == 4)	//End-Of-Input CTRL-D
		{
			fseek(stdin, 0, SEEK_END);
			break;
		}
#endif
		if(TempC == -1)	//CTRL-C or CTRL-Z
		{
			clearerr(stdin);
			break;
		}

		sBuffer += (char)TempC;
		CharsRead++;
	}
	}

	return 0;
}
