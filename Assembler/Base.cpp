//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Base.h"
#include <cstdio>
#include <cctype>

namespace Assembler	{

unsigned int ConsoleWidth = 80;
const char *const sLanguages[NUM_LANGUAGES] = {"LANG_NONE", "LANG_LC3", "LANG_LC3B"};
Flags_t Flags = {false, false, false, false, false, false, false, false, false, false};
LanguageEnum DefaultLang = LangNone;
string sOutputFileName;
list< pair<string, string> > DefineList;

}	//namespace Assembler

namespace Simulator {
string sProgramDir;
}	//namespace Simulator