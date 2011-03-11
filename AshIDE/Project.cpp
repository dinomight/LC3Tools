//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Project.h"
#include "ProjectToken.h"
#include "ProjectLexer.h"
#include "ProjectParser.h"
#include "MainWindow.h"
#include "FileWindow.h"
#include "../Assembler/Assembler.h"
#include "../AsmConvertLC3/AsmConvertLC3.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <fstream>

using namespace std;
using namespace JMT;
using namespace Assembler;
using namespace Simulator;
using namespace LC3;
using namespace LC3b;

namespace AshIDE	{

const char *const Project::sSources[NUM_SOURCE_TYPES] = {"LC3_SOURCE", "LC3_HEADER", "LC3B_SOURCE", "LC3B_HEADER", "RESOURCE", "DEPENDENCY"};
const char *const Project::sSettings[NUM_PROJECT_SETTINGS] = {"PRINT_TOKENS", "PRINT_AST", "PRINT_SYMBOLS", "OUTPUT_IMAGE", "OUTPUT_VHDL", "USE_OPTIMIZATIONS", "USE_OS", "OLD_LC3", "DIS_REG"};
const char *const Project::FileData::sFileSettings[NUM_PROJECT_FILE_SETTINGS] = {"EXCLUDE_FROM_BUILD"};

Project::FileData::FileData(const string &sFN) : sFileName(sFN)
{
	pWindow = NULL;
	fChanged = false;

	string sExt = ToLower(sFileName.Ext);
	if(sExt == "lc3b" || DefaultLang == LangLC3b && sExt == "asm")
	{
		Language = LangLC3b;
		SourceType = LC3bSource;
	}
	else if(sExt == "lc3bh" || DefaultLang == LangLC3b && sExt == "ah")
	{
		Language = LangLC3b;
		SourceType = LC3bHeader;
	}
	else if(sExt == "lc3" || sExt == "asm3" || DefaultLang == LangLC3 && sExt == "asm")
	{
		Language = LangLC3;
		SourceType = LC3Source;
	}
	else if(sExt == "lc3h" || DefaultLang == LangLC3 && sExt == "ah")
	{
		Language = LangLC3;
		SourceType = LC3Header;
	}
	else if( (sExt == "obj" || sExt == "bin") && DefaultLang == LangLC3b)
	{
		Language = LangNone;
		SourceType = LC3bSource;
	}
	else if( (sExt == "obj" || sExt == "bin") && DefaultLang == LangLC3)
	{
		Language = LangNone;
		SourceType = LC3Source;
	}
	else
	{
		Language = LangNone;
		SourceType = Resources;
	}

	for(unsigned int i = 0; i < NUM_PROJECT_FILE_SETTINGS; i++)
		fFileSettings[i] = false;
	if(!IsSource())
		fFileSettings[ExcludeFromBuild] = true;
	
	pProgram = NULL;
}

Project::FileData::~FileData()
{
	if(pProgram)
		delete pProgram;
}

bool Project::FileData::SetSetting(FileSettingEnum Setting, bool fState)
{
	if(fFileSettings[Setting] != fState)
		pProject->fRebuild = pProject->fChanged = true;
	return fFileSettings[Setting] = fState;
}

bool Project::FileData::GetSetting(FileSettingEnum Setting) const
{
	return fFileSettings[Setting];
}

LanguageEnum Project::FileData::SetLanguage(LanguageEnum Lang)
{
	if(Language != Lang)
	{
		pProject->fChanged = true;
		bool fSourceBefore = IsBuild();
		Language = Lang;

		if(Language == LangLC3)
		{
			if(SourceType == LC3bHeader)
				SourceType = LC3Header;
			else if(SourceType == LC3bSource)
				SourceType = LC3Source;
		}
		else if(Language == LangLC3b)
		{
			if(SourceType == LC3Header)
				SourceType = LC3bHeader;
			else if(SourceType == LC3Source)
				SourceType = LC3bSource;
		}

		//Check to see if this changed affected the list of source files
		bool fSourceAfter = IsBuild();
		if(fSourceBefore || fSourceAfter)
			pProject->fRebuild = true;

		TheMainWindow.SelectFile();
		TheMainWindow.pFilesWindow->UpdateList();
	}

	return Language;
}

LanguageEnum Project::FileData::GetLanguage() const
{
	return Language;
}

Project::SourceEnum Project::FileData::SetSourceType(SourceEnum Source)
{
	if(SourceType != Source)
	{
		pProject->fChanged = true;
		bool fSourceBefore = IsBuild();
		SourceType = Source;

		if(SourceType == LC3bHeader || SourceType == LC3bSource)
				Language = LangLC3b;
		else if(SourceType == LC3Header || SourceType == LC3Source)
				Language = LangLC3;
		if(IsSource())
			fFileSettings[ExcludeFromBuild] = false;
		else
			fFileSettings[ExcludeFromBuild] = true;

		//Check to see if this changed affected the list of source files
		bool fSourceAfter = IsBuild();
		if(fSourceBefore || fSourceAfter)
			pProject->fRebuild = true;

		TheMainWindow.SelectFile();
		TheMainWindow.pFilesWindow->UpdateList();
	}

	return SourceType = Source;
}

Project::SourceEnum Project::FileData::GetSourceType() const
{
	return SourceType;
}

bool Project::FileData::IsSource() const
{
	return SourceType == LC3Source || SourceType == LC3bSource;
}

bool Project::FileData::IsBuild() const
{
	return IsSource() && !fFileSettings[ExcludeFromBuild];
}

Project *Project::pProject = NULL;

Project::Project(const string &sWD) : sFileName("")
{
	unsigned int i;

	sWorkingDir = sWD;
	fAutoProject = true;
	fChanged = false;
	fRebuild = true;
	fSimulating = false;
	for(i = 0; i < NUM_PROJECT_SETTINGS; i++)
		fSettings[i] = false;
	fSettings[PrintSymbols] = true;
	fSettings[UseOS] = DefaultLang == Assembler::LangLC3b ? false : true;
	fSettings[OldLC3] = DefaultLang == Assembler::LangLC3b ? false : true;
	//Always update the global flag so that a file opened before an assembly will get highlighted (ISA::Lexer uses this flag)
	Flags.fOldLC3 = fSettings[OldLC3];

	for(i = 0; i < NUM_SOURCE_TYPES; i++)
		fExpanded[i] = true;
	pFile = NULL;
	for(i = 0; i < NUM_LANGUAGES; i++)
		pMemoryImages[i] = NULL;
	pLC3Sim = NULL;
	pLC3bSim = NULL;
	pLC3Arch = NULL;
	pLC3bArch = NULL;
}

Project::~Project()
{
	FileMapIter FileIter;

	//Close all file windows associated with this project
	for(FileIter = Files.begin(); FileIter != Files.end(); FileIter++)
	{
		if(FileIter->second.pWindow)
			FileIter->second.pWindow->CloseNow();
	}

	//dissociate any selected file from the main window
	SelectFile(NULL);

	for(unsigned int i = 0; i < NUM_LANGUAGES; i++)
		if(pMemoryImages[i])
			delete pMemoryImages[i];
}

bool Project::LoadSettings(const string &sFN)
{
	if(!fAutoProject)
		throw "LoadSettings called twice on the same project!";
	if(fChanged)
		throw "LoadSettings cannot be called on an AutoProject once changes have been made!";

	bool fRetVal = true;
	Warnings = Errors = 0;
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	ifstream ProjectFile(sFN.c_str());
	if(!ProjectFile.good())
	{
		fl_alert("Error opening file '%s'", sFN.c_str());
		return false;
	}

	sFileName = sFN;
	sWorkingDir = sFileName.Path;

	sprintf(sMessageBuffer, "Loading project %.255s ...", sFileName.Bare.c_str());
	MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());

	//Lexically scan the input
	list<Token *> TokenList;
	list<Token *>::iterator TokenIter;
	ProjectLexer TheLexer(TokenList, MainWindow::MessageCallBack, false);
	InputList.clear();
	InputList.push_back(sFileName.Name);
	LocationVector LocationStack;
	LocationStack.push_back( LocationVector::value_type(0, 1) );

	if(!TheLexer.Lex(LocationStack, ProjectFile))
		fRetVal = false;

	//Parse the TokenList 
	ProjectParser TheParser(*this, MainWindow::MessageCallBack);
	if(!TheParser.Parse(TokenList.begin(), TokenList.end()))
		fRetVal = false;

	//*NOTE: This should go in the constructor, but then the AddOS->AddFile->FileWindow is using a global Project pointer which hasn't been set yet.
	//LoadSettings is always called immediately after the constructor anyway.
	//Also, they could end up getting the "OS not found" dialog before the project is loaded,
	//even when the proj file points to it. So I'm waiting until after the parsing to just re-perform the AddOS
	SetSetting(UseOS, fSettings[UseOS]);

	//get rid of the used tokens
	for(TokenIter = TokenList.begin(); TokenIter != TokenList.end(); TokenIter++)
 		delete *TokenIter;

	if(Errors)
		fl_alert("Error loading project. See message window for details.");

	fAutoProject = false;
	fChanged = false;
	fRebuild = true;

	//Update the main window's help location, or the project's, whichever
	//is most likely to be correct.
	if(TheMainWindow.sHelpLocation.Full == "" && sHelpLocation.Full != "")
		TheMainWindow.sHelpLocation = sHelpLocation;
	else if(TheMainWindow.sHelpLocation.Full != "")
	{
		sHelpLocation = TheMainWindow.sHelpLocation;
		fChanged = true;
	}

	if(!fRetVal)
		return false;
	return true;
}

bool Project::SaveSettings(const string &sFN)
{
	bool fRetVal = true;
	unsigned int i;
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	ofstream ProjectFile(sFN.c_str());
	string sTemp;
	if(!ProjectFile.good())
	{
		fl_alert("Error opening file '%s'", sFN.c_str());
		return false;
	}

	//Re-assign the name. The asm files are now relative
	//to the new project file.
	sFileName = sFN;
	sWorkingDir = sFileName.Path;

	sprintf(sMessageBuffer, "Saving project %.255s ...", sFileName.Bare.c_str());
	MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());

	//Save project settings to file
	ProjectFile << "#Settings for project " << sFileName.Bare.c_str() << "\n\n";
	ProjectFile << "HELP_LOCATION=\"" << sHelpLocation.Full.c_str() << "\"\n";
	ProjectFile << "LC3_OS_LOCATION=\"" << sOSLocation[LangLC3].Full.c_str() << "\"\n";
	ProjectFile << "LC3B_OS_LOCATION=\"" << sOSLocation[LangLC3b].Full.c_str() << "\"\n";
	for(i = 0; i < NUM_PROJECT_SETTINGS; i++)
	{
		if(fSettings[i])
			ProjectFile << sSettings[i] << "=TRUE\n";
		else
			ProjectFile << sSettings[i] << "=FALSE\n";
	}
	for(i = 0; i < NUM_SOURCE_TYPES; i++)
	{
		if(fExpanded[i])
			ProjectFile << "EXPANDED=" << sSources[i] << "\n";
		else
			ProjectFile << "NOTEXPANDED=" << sSources[i] << "\n";
	}
	ProjectFile << "\nDEFINEDEF\n";
	ProjectFile << "{\n";
	for(DefineList::iterator DefineIter = Defines.begin(); DefineIter != Defines.end(); DefineIter++)
	{
		PrintDefine(DefineIter->second, sTemp);
		ProjectFile << "\t" << DefineIter->first.c_str() << "=" << sTemp.c_str() << endl;
	}
	ProjectFile << "}\n\n";
	for(FileMapIter FileIter = Files.begin(); FileIter != Files.end(); FileIter++)
	{
		ProjectFile << "FILEDEF\n";
		ProjectFile << "{\n";
		ProjectFile << "\tFILENAME=\"" << CreateRelativeFileName(sWorkingDir, FileIter->second.sFileName.Full).c_str() << "\"\n";
		ProjectFile << "\tWINDOW=" << (FileIter->second.pWindow ? "TRUE" : "FALSE") << "\n";
		ProjectFile << "\tLANGUAGE=" << sLanguages[FileIter->second.Language] << "\n";
		ProjectFile << "\tSOURCETYPE=" << sSources[FileIter->second.SourceType] << "\n";
		for(i = 0; i < NUM_PROJECT_FILE_SETTINGS; i++)
		{
			if(FileIter->second.fFileSettings[i])
				ProjectFile << "\t" << FileIter->second.sFileSettings[i] << "=TRUE\n";
			else
				ProjectFile << "\t" << FileIter->second.sFileSettings[i] << "=FALSE\n";
		}
		ProjectFile << "}\n\n";
	}
	
//	fAutoProject = false;
	fChanged = false;
	return true;
}

bool Project::AddFile(const string &sFN)
{
	if(Files.find(sFN) == Files.end())
	{
		FileData FD(sFN);
		Files.insert(FileMap::value_type(sFN, FD));
		TheMainWindow.pFilesWindow->UpdateList();
		fChanged = true;
		if(FD.IsBuild())
			fRebuild = true;

		if(FD.sFileName.Ext == "obj" || FD.sFileName.Ext == "bin")
		{
			string sSymbolFile = FD.sFileName.Path + FD.sFileName.Bare + ".Symbols.csv";
			ifstream SymbolFile(sSymbolFile.c_str());
			if(SymbolFile.good())
				AddFile(sSymbolFile);
		}
	}
	AddWindow(sFN, FileWindow::IsOpen(sFN));
	return true;
}

bool Project::AddFile(const FileData &FD)
{
	if(Files.find(FD.sFileName.Full) == Files.end())
	{
		Files.insert(FileMap::value_type(FD.sFileName.Full, FD));
		TheMainWindow.pFilesWindow->UpdateList();
		fChanged = true;
		if(FD.IsBuild())
			fRebuild = true;

		if(FD.sFileName.Ext == "obj" || FD.sFileName.Ext == "bin")
		{
			string sSymbolFile = FD.sFileName.Path + FD.sFileName.Bare + ".Symbols.csv";
			ifstream SymbolFile(sSymbolFile.c_str());
			if(SymbolFile.good())
				AddFile(sSymbolFile);
		}
	}
	AddWindow(FD.sFileName.Full, FileWindow::IsOpen(FD.sFileName.Full));
	return true;
}

bool Project::RemoveFile(const string &sFN)
{
	if(IsSimulating(sFN))
	{
		if(!fl_ask("This file is part of the current simulation. This action will stop the simulator. Do you wish to continue?"))
			return false;
		fSimulating = false;
	}
	FileMap::iterator FileIter = Files.find(sFN);
	if(FileIter == Files.end())
		return false;
	fChanged = true;
	if(FileIter->second.IsBuild())
		fRebuild = true;
	if(&FileIter->second == pFile)
		SelectFile(NULL);
	Files.erase(FileIter);
	TheMainWindow.pFilesWindow->UpdateList();
	return true;
}

Project::FileData *Project::GetFile(const string &sFN)
{
	FileMapIter Find = Files.find(sFN);
	if(Find == Files.end())
		return NULL;
	return &Find->second;
}

unsigned int Project::ListFiles(list<string> &FileList)
{
	for(FileMapIter FileIter = Files.begin(); FileIter != Files.end(); FileIter++)
		FileList.push_back(FileIter->first);
	return Files.size();
}

bool Project::AddWindow(const string &sFN, FileWindow *pAFW)
{
	FileMapIter Find = Files.find(sFN);
	if(Find == Files.end())
		return false;
	Find->second.pWindow = pAFW;
	fChanged = true;
	return true;
}

bool Project::RemoveWindow(const string &sFN)
{
	FileMapIter Find = Files.find(sFN);
	if(Find == Files.end())
		return false;
	Find->second.pWindow = NULL;
	fChanged = true;
	return true;
}

bool Project::SetExpanded(SourceEnum Source, bool fState)
{
	if(fExpanded[Source] != fState)
		fChanged = true;
	return fExpanded[Source] = fState;
}

bool Project::GetExpanded(SourceEnum Source)
{
	return fExpanded[Source];
}

bool Project::SetSetting(SettingEnum Setting, bool fState)
{
	if(Setting == UseOS && fState == true)
	{
		if(!AddOS())
			return fSettings[Setting];
	}
	else if(Setting == UseOS && fState == false)
	{
		if(!RemoveOS())
			return fSettings[Setting];
	}
	if(Setting == OldLC3)
		//Always update the global flag so that a file opened before an assembly will get highlighted (ISA::Lexer uses this flag)
		Flags.fOldLC3 = fState;
	if(fSettings[Setting] != fState)
		fRebuild = fChanged = true;
	return fSettings[Setting] = fState;
}

bool Project::GetSetting(SettingEnum Setting)
{
	return fSettings[Setting];
}

bool Project::AddOS()
{
	bool fRetVal = false;
	//Decide which OS to add
	for(LanguageEnum Lang = (LanguageEnum)(LangNone+1); Lang < NUM_LANGUAGES; Lang = (LanguageEnum)(Lang+1))
	{
		bool fThisLang = false, fThisOS = false;
		for(FileMapIter FileIter = Files.begin(); FileIter != Files.end(); FileIter++)
		{
			if(FileIter->second.IsBuild() && FileIter->second.GetLanguage() == Lang)
			{
				fThisLang = true;
				break;
			}
		}
		if(!fThisLang && Lang != DefaultLang)
			continue;

		while(true)
		{
			if(sOSLocation[Lang].Full == "")
			{
				char Buffer[256];
				fl_filename_absolute(Buffer, 256, (sProgramDir + (Lang == LangLC3 ? "AshOS_LC3.asm" : "AshOS_LC3b.asm")).c_str());
				sOSLocation[Lang] = CreateStandardPath(Buffer);
			}
			ifstream OSFile(sOSLocation[Lang].Full.c_str());
			if(!OSFile.good())
			{
				if(!fl_ask("The %s AshOS was not found at the expected location.\nIn the next dialog, please select the new path to the operating system:", Lang == LangLC3 ? "LC-3" : "LC-3b"))
				{
					sOSLocation[Lang] = "";
					break;
				}
				char *Buffer;
				Buffer = fl_file_chooser("Operating system location", Lang == LangLC3 ? "AshOS_LC3.asm" : "AshOS_LC3b.asm", Lang == LangLC3 ? "AshOS_LC3.asm" : "AshOS_LC3b.asm");
				if(Buffer == NULL)
				{
					sOSLocation[Lang] = "";
					break;
				}
				sOSLocation[Lang] = CreateStandardPath(Buffer);
				continue;
			}
			fThisOS = true;
			break;
		}
		if(fThisOS)
		{
			{
				Project::FileData FileData(sOSLocation[Lang].Path + sOSLocation[Lang].Bare + ".asm");
				FileData.SetLanguage(Lang);
				FileData.SetSourceType(Lang == LangLC3 ? LC3Source : LC3bSource);
				AddFile(FileData);
			}
			{
				Project::FileData FileData(sOSLocation[Lang].Path + sOSLocation[Lang].Bare + ".ah");
				FileData.SetLanguage(Lang);
				FileData.SetSourceType(Lang == LangLC3 ? LC3Header : LC3bHeader);
				AddFile(FileData);
			}
			fChanged = true;
			fRetVal = true;
		}
	}

	return fRetVal;
}

bool Project::RemoveOS()
{
	for(LanguageEnum Lang = (LanguageEnum)(LangNone+1); Lang < NUM_LANGUAGES; Lang = (LanguageEnum)(Lang+1))
	{
		if(sOSLocation[Lang].Full == "")
			continue;
		RemoveFile(sOSLocation[Lang].Path + sOSLocation[Lang].Bare + ".asm");
		RemoveFile(sOSLocation[Lang].Path + sOSLocation[Lang].Bare + ".ah");
	}

	return true;
}

bool Project::AddDefine(const string &sIdentifier, const string &sValue)
{
	fRebuild = fChanged = true;
	if(RemoveDefine(sIdentifier))
	{
		//This is replacing an existing define
		Defines.push_back(DefineList::value_type(sIdentifier, sValue));
		return false;
	}
	//This is a first-time entry
	Defines.push_back(DefineList::value_type(sIdentifier, sValue));
	return true;
}

bool Project::RemoveDefine(const string &sIdentifier)
{
	for(DefineList::iterator DefineIter = Defines.begin(); DefineIter != Defines.end(); DefineIter++)
	{
		if(DefineIter->first == sIdentifier)
		{
			Defines.erase(DefineIter);
			fRebuild = fChanged = true;
			return true;
		}
	}
	return false;
}

bool Project::RemoveDefine(unsigned int Index)
{
	unsigned int i = 0;
	for(DefineList::iterator DefineIter = Defines.begin(); DefineIter != Defines.end(); DefineIter++, i++)
	{
		if(i == Index)
		{
			Defines.erase(DefineIter);
			fRebuild = fChanged = true;
			return true;
		}
	}
	return false;
}

void Project::PrintDefine(const string &sValue, string &sQuoted)
{
	char sToken[8];
	sQuoted = "\"";

	for(unsigned int i = 0; i < sValue.size(); i++)
	{
		switch(sValue[i])
		{
		case '\a':
			sQuoted += "\\a";
			break;
		case '\b':
			sQuoted += "\\b";
			break;
		case '\f':
			sQuoted += "\\f";
			break;
		case '\n':
			sQuoted += "\\n";
			break;
		case '\r':
			sQuoted += "\\r";
			break;
		case '\t':
			sQuoted += "\\t";
			break;
		case '\v':
			sQuoted += "\\v";
			break;
		case '\'':
			sQuoted += "\\'";
			break;
		case '\"':
			sQuoted += "\\\"";
			break;
		case '\\':
			sQuoted += "\\\\";
			break;
		case '\?':
			sQuoted += "\\?";
			break;
		default:
			if(sValue[i] <= 0x1F || sValue[i] >= 0x7F)
			{
				sprintf(sToken, "\\x%X", (unsigned char)sValue[i]);
				sQuoted += sToken;
			}
			else
				sQuoted += sValue[i];
		}
	}
	sQuoted += "\"";
}

bool Project::SelectFile(FileData *pFileData)
{
	pFile = pFileData;
	//Alert the mainwindow to the change
	TheMainWindow.SelectFile();
	return true;
}

bool Project::AssembleFile(const string &sFile)
{
	return AssembleFile(GetFile(sFile));
}

bool Project::AssembleFile(FileData *pFileData)
{
#ifdef BIG_ENDIAN_BUILD
	if(EndianCheck())
	{
		MainWindow::MessageCallBack(Fatal, "Platform is little endian; assembler built for big endian! Aborting...", LocationVector());
		return false;
	}
#else
	if(!EndianCheck())
	{
		MainWindow::MessageCallBack(Fatal, "Platform is big endian; assembler built for little endian! Aborting...", LocationVector());
		return false;
	}
#endif

	//Save the file before assembling it
	FileWindow *pFW;
	if(pFW = FileWindow::IsOpen(pFileData->sFileName.Full))
	{
		if(!pFW->Save())
			return false;
	}

	//Check for an uncompilable type
	if(pFileData->GetLanguage() == LangNone)
	{
		fl_alert("No language specified for '%s'", pFileData->sFileName.Name.c_str());
		return false;
	}
	if(!pFileData->IsSource())
	{
		fl_alert("'%s' is not a source file.", pFileData->sFileName.Name.c_str());
		return false;
	}
	if(pFileData->GetSetting(FileData::ExcludeFromBuild))
	{
		fl_alert("'%s' is excluded from the build.", pFileData->sFileName.Name.c_str());
		return false;
	}

	//Initialize assembler variables
	Warnings = Errors = 0;
	if(fRebuild)
		if(!ResetAssembler())
			return false;

	bool fRetVal = true;
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	TheMainWindow.pMessageWindow->Clear();
	MainWindow::MessageCallBack(Info, "", LocationVector());
	sprintf(sMessageBuffer, "\t\tLC-3%s Assembler %.15s, Ashley Wise", (pFileData->GetLanguage() == LangLC3 ? "" : "b"), ASM_VER);
	MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());
	MainWindow::MessageCallBack(Info, "", LocationVector());
	MainWindow::MessageCallBack(Info, "Compiling...", LocationVector());

	if(pFileData->GetLanguage() == LangLC3)
		fRetVal = AssembleLC3(pFileData);
	else if(pFileData->GetLanguage() == LangLC3b)
		fRetVal = AssembleLC3b(pFileData);

	FileName sOutputFileName(InputList[pFileData->ProgramNumber]);

	MainWindow::MessageCallBack(Info, "", LocationVector());
	sprintf(sMessageBuffer, "%.255s:   %u Error%.1s, %u Warning%.1s", (sOutputFileName.Path+sOutputFileName.Bare).c_str(), Errors, (Errors != 1 ? "s" : ""), Warnings, (Warnings != 1 ? "s" : ""));
	MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());

	if(Errors > 0)
	{
		fl_alert("Errors compiling %s", sOutputFileName.Full.c_str());
		MainWindow::pMainWindow->show();
	}
	return fRetVal;
}

bool Project::Assemble()
{
#ifdef BIG_ENDIAN_BUILD
	if(EndianCheck())
	{
		MainWindow::MessageCallBack(Fatal, "Platform is little endian; assembler built for big endian! Aborting...", LocationVector());
		return false;
	}
#else
	if(!EndianCheck())
	{
		MainWindow::MessageCallBack(Fatal, "Platform is big endian; assembler built for little endian! Aborting...", LocationVector());
		return false;
	}
#endif

	//Save everything
	if(!FileWindow::SaveAll())
		return false;

	//Initialize assembler variables
	if(fRebuild)
		if(!ResetAssembler())
			return false;

	//Reset may update AshOS project settings
	if(!TheMainWindow.SaveProject())
		return false;

	bool fRetVal = true;
	Warnings = Errors = 0;
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	TheMainWindow.pMessageWindow->Clear();

	//Each language gets assembled in its own pass
	for(LanguageEnum Lang = (LanguageEnum)(LangNone+1); Lang < NUM_LANGUAGES; Lang = (LanguageEnum)(Lang+1))
	{
		if(InputLists[Lang].size())
		{
			MainWindow::MessageCallBack(Info, "", LocationVector());
			sprintf(sMessageBuffer, "\t\tLC-3%s Assembler %.15s, Ashley Wise", (Lang == LangLC3 ? "" : "b"), ASM_VER);
			MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());
			MainWindow::MessageCallBack(Info, "", LocationVector());
		}
		else
			continue;

		//Lex, parse, and optimize each input asm file
		bool fOne = false;
		bool fFileRetVal = true;
		for(unsigned int i = 0; i < NumSources[Lang]; i++)
		{
			FileData *pFileData = GetFile(CreateFileNameRelative(sWorkingDir, InputLists[Lang][i]));

			//Check if the program has already been compiled
			if(pFileData->pProgram && !pFileData->fChanged)
				continue;

			if(!fOne)
			{
				MainWindow::MessageCallBack(Info, "Compiling...", LocationVector());
				fOne = true;
			}
			if(pFileData->GetLanguage() == LangLC3)
			{
				if(!AssembleLC3(pFileData))
					fFileRetVal = false;
			}
			else if(pFileData->GetLanguage() == LangLC3b)
			{
				if(!AssembleLC3b(pFileData))
					fFileRetVal = false;
			}
		}
		if(!fFileRetVal)
		{
			fRetVal = false;
			continue;
		}

		if(!fOne)
		{
			sprintf(sMessageBuffer, "Project '%.255s' is up to date.", sFileName.Bare.c_str());
			MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());
		}
	}

	MainWindow::MessageCallBack(Info, "", LocationVector());
	sprintf(sMessageBuffer, "%.255s:   %u Error%.1s, %u Warning%.1s", sFileName.Bare.c_str(), Errors, (Errors != 1 ? "s" : ""), Warnings, (Warnings != 1 ? "s" : ""));
	MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());

	if(Errors > 0)
	{
		fl_alert("Errors compiling %s", sFileName.Bare.c_str());
		MainWindow::pMainWindow->show();
	}

	return fRetVal;
}

bool Project::Build()
{
#ifdef BIG_ENDIAN_BUILD
	if(EndianCheck())
	{
		MainWindow::MessageCallBack(Fatal, "Platform is little endian; assembler built for big endian! Aborting...", LocationVector());
		return false;
	}
#else
	if(!EndianCheck())
	{
		MainWindow::MessageCallBack(Fatal, "Platform is big endian; assembler built for little endian! Aborting...", LocationVector());
		return false;
	}
#endif

	//Save everything
	if(!FileWindow::SaveAll())
		return false;

	//Initialize assembler variables
	if(fRebuild)
		if(!ResetAssembler())
			return false;

	//Reset may update AshOS project settings
	if(!TheMainWindow.SaveProject())
		return false;

	bool fRetVal = true;
	Warnings = Errors = 0;
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	TheMainWindow.pMessageWindow->Clear();

	//Each language gets assembled in its own pass
	for(LanguageEnum Lang = (LanguageEnum)(LangNone+1); Lang < NUM_LANGUAGES; Lang = (LanguageEnum)(Lang+1))
	{
		if(InputLists[Lang].size())
		{
			MainWindow::MessageCallBack(Info, "", LocationVector());
			sprintf(sMessageBuffer, "\t\tLC-3%s Assembler %.15s, Ashley Wise", (Lang == LangLC3 ? "" : "b"), ASM_VER);
			MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());
			MainWindow::MessageCallBack(Info, "", LocationVector());
		}
		else
			continue;

		//Lex, parse, and optimize each input asm file
		bool fOne = false;
		bool fFileRetVal = true;
		for(unsigned int i = 0; i < NumSources[Lang]; i++)
		{
			FileData *pFileData = GetFile(CreateFileNameRelative(sWorkingDir, InputLists[Lang][i]));

			//Check if the program has already been compiled
			if(pFileData->pProgram && !pFileData->fChanged)
				continue;

			if(!fOne)
			{
				MainWindow::MessageCallBack(Info, "Compiling...", LocationVector());
				fOne = true;
			}
			if(pFileData->GetLanguage() == LangLC3)
			{
				if(!AssembleLC3(pFileData))
					fFileRetVal = false;
			}
			else if(pFileData->GetLanguage() == LangLC3b)
			{
				if(!AssembleLC3b(pFileData))
					fFileRetVal = false;
			}
		}
		if(!fFileRetVal)
		{
			fRetVal = false;
			continue;
		}

		//Convert the symbolic assembly program into a memory image
		if(!fOne && pMemoryImages[Lang])
		{
			sprintf(sMessageBuffer, "Project '%.255s' is up to date.", sFileName.Bare.c_str());
			MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());
			continue;
		}

		MainWindow::MessageCallBack(Info, "Linking...", LocationVector());
		if(Lang == LangLC3)
		{
			if(!BuildLC3())
				fRetVal = false;
		}
		else if(Lang == LangLC3b)
		{
			if(!BuildLC3b())
				fRetVal = false;
		}
	}

	MainWindow::MessageCallBack(Info, "", LocationVector());
	sprintf(sMessageBuffer, "%.255s:   %u Error%.1s, %u Warning%.1s", sFileName.Bare.c_str(), Errors, (Errors != 1 ? "s" : ""), Warnings, (Warnings != 1 ? "s" : ""));
	MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());

	if(Errors > 0)
	{
		fl_alert("Errors building %s", sFileName.Bare.c_str());
		MainWindow::pMainWindow->show();
	}

	return fRetVal;
}

bool Project::Simulate()
{
	if(fSimulating)
	{
		TheSimulatorWindow.show();
		return true;
	}

	//Make sure the project is up to date.
	if(!Build())
		return false;

	//Reset the simulator IDs.
	//Simulator IDs allowed multiple simulators to work with the same Ctrl-C breakpoints in console-mode
	//They aren't necessary for the GUI version, nor with any software currently written.
	ArchSim<LC3ISA>::NextSimID = 1;
	ArchSim<LC3bISA>::NextSimID = 1;

	//Decide which ISA to simulate
	if(Programs[LangLC3].size() && !Programs[LangLC3b].size())
	{
		pLC3Sim = new ArchSim<LC3ISA>(MainWindow::SimMessageCallBack, MainWindow::SimCallBack, MainWindow::SimCommand, MainWindow::SimReadConsole, MainWindow::SimWriteConsole);
		pLC3Arch = new LC3Arch(*pLC3Sim);
		InputList = InputLists[LangLC3];
		SimISA = LangLC3;
	}
	else if(!Programs[LangLC3].size() && Programs[LangLC3b].size())
	{
		pLC3bSim = new ArchSim<LC3bISA>(MainWindow::SimMessageCallBack, MainWindow::SimCallBack, MainWindow::SimCommand, MainWindow::SimReadConsole, MainWindow::SimWriteConsole);
		pLC3bArch = new LC3bArch(*pLC3bSim);
		InputList = InputLists[LangLC3b];
		SimISA = LangLC3b;
	}
	else if(Programs[LangLC3].size() && Programs[LangLC3b].size())
	{
		int Choice = fl_choice("Which ISA do you want to simulate?",
			"LC-3", "LC-3b", "Cancel");

		switch(Choice)
		{
		case 0:
			pLC3Sim = new ArchSim<LC3ISA>(MainWindow::SimMessageCallBack, MainWindow::SimCallBack, MainWindow::SimCommand, MainWindow::SimReadConsole, MainWindow::SimWriteConsole);
			pLC3Arch = new LC3Arch(*pLC3Sim);
			InputList = InputLists[LangLC3];
			SimISA = LangLC3;
			break;
		case 1:
			pLC3bSim = new ArchSim<LC3bISA>(MainWindow::SimMessageCallBack, MainWindow::SimCallBack, MainWindow::SimCommand, MainWindow::SimReadConsole, MainWindow::SimWriteConsole);
			pLC3bArch = new LC3bArch(*pLC3bSim);
			InputList = InputLists[LangLC3b];
			SimISA = LangLC3b;
			break;
		case 2:
			return false;
		}
	}
	else
	{
		fl_message("There is nothing to simulate.");
		return false;
	}

	//Begin simulation
	SimulatorWindow::pSimulatorWindow = new SimulatorWindow();
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	MainWindow::SimCallBack(Info, "");
	sprintf(sMessageBuffer, "\t\tLC-3%s Simulator %.15s, Ashley Wise", (SimISA == LangLC3 ? "" : "b"), SIM_VER);
	MainWindow::SimCallBack(Info, sMessageBuffer);
	MainWindow::SimCallBack(Info, "");
/*	MainWindow::SimCallBack(Info, "All simulator functionality is accessible via the GUI menus, windows,");
	MainWindow::SimCallBack(Info, "dialogs, and shortcuts. Advanced users can type commands directly using");
	MainWindow::SimCallBack(Info, "the input below. Enter \"help\" to see a list of all the simulator");
	MainWindow::SimCallBack(Info, "commands, and enter \"help command\" for details about a specific command.");
	MainWindow::SimCallBack(Info, "");
*/
	fSimulating = true;
	TheSim(fDone) = false;
	if(SimISA == LangLC3)
		pLC3Sim->Reset(*pLC3Arch, Programs[SimISA], *pMemoryImages[SimISA]);
	else
		pLC3bSim->Reset(*pLC3bArch, Programs[SimISA], *pMemoryImages[SimISA]);
	FileWindow::SimulatorOptionsAll();

	TheMainWindow.iconize();
	if(fSettings[DisReg])
	{
		SimulatorWindow::pSimulatorWindow->position(20, 30);
		SimulatorWindow::pSimulatorWindow->ShowDisassembly();
		SimulatorWindow::pSimulatorWindow->pDisassemblyWindow->resize(290, 170, 470, 420);
		SimulatorWindow::pSimulatorWindow->AddRegisters();
		SimulatorWindow::pSimulatorWindow->Registers.begin()->second->position(20,360);
	}

	while(fSimulating && !TheSim(fDone))
	{
		//Alternate between executing a simulator cycle and checking for user input
		TheSim(Run());
		TheSimulatorWindow.Update();
		if(TheSim(fBreak))
			Fl::wait();
		else
			Fl::check();
	}

	fSimulating = false;
	FileWindow::SimulatorOptionsAll();
	delete SimulatorWindow::pSimulatorWindow;
	SimulatorWindow::pSimulatorWindow = NULL;
	TheMainWindow.show();

	//Delete the simulation data.
	if(pLC3Sim)
		delete pLC3Sim;
	if(pLC3bSim)
		delete pLC3bSim;
	pLC3Sim = NULL;
	pLC3bSim = NULL;
	if(pLC3Arch)
		delete pLC3Arch;
	if(pLC3bArch)
		delete pLC3bArch;
	pLC3Arch = NULL;
	pLC3bArch = NULL;
	return true;
}

bool Project::ResetAssembler()
{
	if(fSimulating)
	{
		if(!fl_ask("This action will stop the simulator. Do you wish to continue?"))
			return false;
		fSimulating = false;
	}

	FileMapIter FileIter;
	for(FileIter = Files.begin(); FileIter != Files.end();)
	{
		if(FileIter->second.pProgram)
		{
			//Delete all programs; the program numbers will be invalid
			delete FileIter->second.pProgram;
			FileIter->second.pProgram = NULL;
		}
		if(FileIter->second.GetSourceType() == Dependencies)
		{
			//Remove dependencies, they will be re-generated
			FileMapIter TempIter = FileIter;
			FileIter++;
			RemoveFile(TempIter->first);
		}
		else
			FileIter++;
	}

	//Remove memory images
	unsigned int i;
	for(i = 0; i < NUM_LANGUAGES; i++)
	{
		InputLists[i].clear();
		Programs[i].clear();
		if(pMemoryImages[i])
			delete pMemoryImages[i];
		pMemoryImages[i] = NULL;
	}

	//Make sure the OS files are included
	if(fSettings[UseOS])
		AddOS();

	//Put all source files into the input list
	for(FileIter = Files.begin(); FileIter != Files.end(); FileIter++)
	{
		if(FileIter->second.IsBuild())
		{
			FileIter->second.ProgramNumber = InputLists[FileIter->second.GetLanguage()].size();
			InputLists[FileIter->second.GetLanguage()].push_back(CreateRelativeFileName(sWorkingDir, FileIter->second.sFileName.Full));

		}
	}
	for(i = 0; i < NUM_LANGUAGES; i++)
		NumSources[i] = InputLists[i].size();

	Assembler::DefineList.clear();
	for(DefineList::iterator DefineIter = Defines.begin(); DefineIter != Defines.end(); DefineIter++)
	{
		if(NumSources[LangLC3] > 0 && fSettings[OldLC3])
		{
			string sTemp;
			if(!AsmConvertLC3Line(DefineIter->second, sTemp, LocationVector(), MainWindow::MessageCallBack))
				return false;
			DefineIter->second = sTemp;
		}
		Assembler::DefineList.push_back(DefineList::value_type(DefineIter->first, DefineIter->second));
	}

	JMT::sWorkingDir = sWorkingDir;
	Flags.fPrintTokens = fSettings[PrintTokens];
	Flags.fPrintAST = fSettings[PrintAST];
	Flags.fPrintSymbols = fSettings[PrintSymbols];
	Flags.fOutputImage = fSettings[OutputImage];
	Flags.fOutputVHDL = fSettings[OutputVHDL];
	Flags.fUseOptimizations = fSettings[UseOptimizations];
	Flags.fUseOS = fSettings[UseOS];
	Flags.fOldLC3 = fSettings[OldLC3];
	fRebuild = false;
	return true;
}

bool Project::IsSimulating(const string &sFileName) const
{
	if(fSimulating)
	{
		string sRelativeFileName = CreateRelativeFileName(sWorkingDir, sFileName);
		for(vector<string>::iterator FileIter = InputList.begin(); FileIter != InputList.end(); FileIter++)
			if(*FileIter == sRelativeFileName)
				return true;
	}
	return false;
}

bool Project::AssembleLC3(FileData *pFileData)
{
	if(IsSimulating(pFileData->sFileName.Full))
	{
		if(!fl_ask("This file is part of the current simulation. This action will stop the simulator. Do you wish to continue?"))
			return false;
		fSimulating = false;
	}

	//*** Initialize program ***
	InputList = InputLists[LangLC3];
	LocationVector LocationStack;
	LocationStack.push_back( LocationVector::value_type(pFileData->ProgramNumber, 0) );
	if(pFileData->pProgram)
		delete pFileData->pProgram;
	pFileData->pProgram = new Program(LocationStack, InputList[pFileData->ProgramNumber], LC3ISA::Addressability);

	//*** Perform Compilation ***
	//Lex, parse, and optimize each input asm file
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	bool fRetVal = true;

	//Open the assembly file
	if(ToLower(pFileData->sFileName.Ext) == "obj" || ToLower(pFileData->sFileName.Ext) == "bin")
	{
		ifstream AsmFile(pFileData->sFileName.Full.c_str(), ios::in | ios::binary);
		if(!AsmFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", pFileData->sFileName.Full.c_str());
			MainWindow::MessageCallBack(Fatal, sMessageBuffer, LocationVector());
			fRetVal = false;
		}
		//*NOTE: For some reason, MSVC refuses to let "using namespace Assembler" work here
		else if(!Assembler::Assemble<LC3ISA>::Disassemble(AsmFile, *pFileData->pProgram, MainWindow::MessageCallBack))
			fRetVal = false;
	}
	else
	{
		ifstream AsmFile(pFileData->sFileName.Full.c_str());
		if(!AsmFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", pFileData->sFileName.Full.c_str());
			MainWindow::MessageCallBack(Fatal, sMessageBuffer, LocationVector());
			fRetVal = false;
		}
		else
		{
			string sAsm3FileName = pFileData->sFileName.Full + "3";
			if(fSettings[OldLC3])
			{
				ofstream Asm3File(sAsm3FileName.c_str());
				if(!Asm3File.good())
				{
					sprintf(sMessageBuffer, "Unable to open file %.255s", sAsm3FileName.c_str());
					MainWindow::MessageCallBack(Fatal, sMessageBuffer, LocationVector());
					fRetVal = false;
				}
				else if(!AsmConvertLC3(AsmFile, Asm3File, LocationStack, MainWindow::MessageCallBack))
				{
					fRetVal = false;
				}
				else
				{
					AsmFile.close();
					Asm3File.close();
					AsmFile.clear();
					AsmFile.open(sAsm3FileName.c_str());
					if(!AsmFile.good())
					{
						sprintf(sMessageBuffer, "Unable to open file %.255s", sAsm3FileName.c_str());
						MainWindow::MessageCallBack(Fatal, sMessageBuffer, LocationVector());
						fRetVal = false;
					}
				}
			}
			if(fRetVal && !Assembler::Assemble<LC3ISA>::Compile(AsmFile, *pFileData->pProgram, MainWindow::MessageCallBack))
				fRetVal = false;
		}
	}

	//*** Add new resources ***
	if(fSettings[OldLC3])
	{
		string sFile = pFileData->sFileName.Full + "3";
		if(!GetFile(sFile))
		{
			ifstream File(sFile.c_str());
			if(File.good())
			{
				AddFile(sFile);
				FileData *pFD = GetFile(sFile);
				pFD->SetLanguage(LangLC3);
				pFD->SetSourceType(Resources);
			}
		}
	}
	if(fSettings[PrintTokens])
	{
		string sFile = pFileData->sFileName.Path + pFileData->sFileName.Bare + ".Tokens.txt";
		if(!GetFile(sFile))
		{
			ifstream File(sFile.c_str());
			if(File.good())
			{
				AddFile(sFile);
				GetFile(sFile)->SetLanguage(LangLC3);
			}
		}
	}
	if(fSettings[PrintAST])
	{
		string sFile = pFileData->sFileName.Path + pFileData->sFileName.Bare + ".AST.asm";
		if(!GetFile(sFile))
		{
			ifstream File(sFile.c_str());
			if(File.good())
			{
				AddFile(sFile);
				FileData *pFD = GetFile(sFile);
				pFD->SetLanguage(LangLC3);
				pFD->SetSourceType(Resources);
			}
		}
	}

	//*** Add new dependencies ***
	for(unsigned int i = InputLists[LangLC3].size(); i < InputList.size(); i++)
	{
		string sFileName = CreateFileNameRelative(sWorkingDir, InputList[i]);
		FileData *pFD = GetFile(sFileName);
		if(pFD)
		{	//File is already in the project
			pFD->ProgramNumber = i;
		}
		else
		{	//File is not yet in the project
			FileData FD(sFileName);
			FD.SetLanguage(LangLC3);
			FD.SetSourceType(Dependencies);
			FD.ProgramNumber = i;
			AddFile(FD);
			FileWindow *pFW = FileWindow::IsOpen(sFileName);
			if(pFW)
				pFW->AddToProject(true);
		}
		InputLists[LangLC3].push_back(InputList[i]);
	}

	if(!fRetVal)
	{
		delete pFileData->pProgram;
		pFileData->pProgram = NULL;
	}

	//The program has changed, we must re-build.
	//Note that if this was compiled via Assemble*, then Build won't have the fOne info.
	if(pMemoryImages[LangLC3])
		delete pMemoryImages[LangLC3];
	pMemoryImages[LangLC3] = NULL;

	pFileData->fChanged = false;
	return fRetVal;
}

bool Project::AssembleLC3b(FileData *pFileData)
{
	if(IsSimulating(pFileData->sFileName.Full))
	{
		if(!fl_ask("This file is part of the current simulation. This action will stop the simulator. Do you wish to continue?"))
			return false;
		fSimulating = false;
	}

	//*** Initialize program ***
	InputList = InputLists[LangLC3b];
	LocationVector LocationStack;
	LocationStack.push_back( LocationVector::value_type(pFileData->ProgramNumber, 0) );
	if(pFileData->pProgram)
		delete pFileData->pProgram;
	pFileData->pProgram = new Program(LocationStack, InputList[pFileData->ProgramNumber], LC3bISA::Addressability);

	//*** Perform Compilation ***
	//Lex, parse, and optimize each input asm file
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	bool fRetVal = true;

	//Open the assembly file
	if(ToLower(pFileData->sFileName.Ext) == "obj" || ToLower(pFileData->sFileName.Ext) == "bin")
	{
		ifstream AsmFile(pFileData->sFileName.Full.c_str(), ios::in | ios::binary);
		if(!AsmFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", pFileData->sFileName.Full.c_str());
			MainWindow::MessageCallBack(Fatal, sMessageBuffer, LocationVector());
			fRetVal = false;
		}
		else if(!Assembler::Assemble<LC3bISA>::Disassemble(AsmFile, *pFileData->pProgram, MainWindow::MessageCallBack))
			fRetVal = false;
	}
	else
	{
		ifstream AsmFile(pFileData->sFileName.Full.c_str());
		if(!AsmFile.good())
		{
			sprintf(sMessageBuffer, "Unable to open file %.255s", pFileData->sFileName.Full.c_str());
			MainWindow::MessageCallBack(Fatal, sMessageBuffer, LocationVector());
			fRetVal = false;
		}
		else if(!Assembler::Assemble<LC3bISA>::Compile(AsmFile, *pFileData->pProgram, MainWindow::MessageCallBack))
			fRetVal = false;
	}

	//*** Add new resources ***
	if(fSettings[PrintTokens])
	{
		string sFile = pFileData->sFileName.Path + pFileData->sFileName.Bare + ".Tokens.txt";
		if(!GetFile(sFile))
		{
			ifstream File(sFile.c_str());
			if(File.good())
			{
				AddFile(sFile);
				GetFile(sFile)->SetLanguage(LangLC3b);
			}
		}
	}
	if(fSettings[PrintAST])
	{
		string sFile = pFileData->sFileName.Path + pFileData->sFileName.Bare + ".AST.asm";
		if(!GetFile(sFile))
		{
			ifstream File(sFile.c_str());
			if(File.good())
			{
				AddFile(sFile);
				FileData *pFD = GetFile(sFile);
				pFD->SetLanguage(LangLC3b);
				pFD->SetSourceType(Resources);
			}
		}
	}

	//*** Add new dependencies ***
	for(unsigned int i = InputLists[LangLC3b].size(); i < InputList.size(); i++)
	{
		string sFileName = CreateFileNameRelative(sWorkingDir, InputList[i]);
		FileData *pFD = GetFile(sFileName);
		if(pFD)
		{	//File is already in the project
			pFD->ProgramNumber = i;
		}
		else
		{	//File is not yet in the project
			FileData FD(sFileName);
			FD.SetLanguage(LangLC3b);
			FD.SetSourceType(Dependencies);
			FD.ProgramNumber = i;
			AddFile(FD);
			FileWindow *pFW = FileWindow::IsOpen(sFileName);
			if(pFW)
				pFW->AddToProject(true);
		}
		InputLists[LangLC3b].push_back(InputList[i]);
	}

	if(!fRetVal)
	{
		delete pFileData->pProgram;
		pFileData->pProgram = NULL;
	}

	//The program has changed, we must re-build.
	//Note that if this was compiled via Assemble*, then Build won't have the fOne info.
	if(pMemoryImages[LangLC3b])
		delete pMemoryImages[LangLC3b];
	pMemoryImages[LangLC3b] = NULL;

	pFileData->fChanged = false;
	return fRetVal;
}

bool Project::BuildLC3()
{
	//Initialize the build
	InputList = InputLists[LangLC3];
	Programs[LangLC3].clear();
	vector<string>::iterator InputIter;
	for(InputIter = InputList.begin(); InputIter != InputList.end(); InputIter++)
	{
		FileData *pFileData = GetFile(CreateFileNameRelative(sWorkingDir, *InputIter));
		if(!pFileData->IsBuild())
				continue;
		Programs[LangLC3].push_back(pFileData->pProgram);
	}
	if(pMemoryImages[LangLC3])
		delete pMemoryImages[LangLC3];
	pMemoryImages[LangLC3] = new RamVector;


	//Convert the symbolic assembly program into a memory image
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	if(!Assembler::Assemble<LC3ISA>::Link(Programs[LangLC3], *pMemoryImages[LangLC3], MainWindow::MessageCallBack))
	{
		delete pMemoryImages[LangLC3];
		pMemoryImages[LangLC3] = NULL;
		return false;
	}

	if(fSettings[OutputVHDL])
	{
		//Create output file name.
		string sOutputFileName = sFileName.Path + sFileName.Bare + ".vhd";
		//Open the output file.
		ofstream VHDLFile(sOutputFileName.c_str());
		if(!VHDLFile.good())
		{
			sprintf(sMessageBuffer, "Unable to create file %.255s...", sOutputFileName.c_str());
			MainWindow::MessageCallBack(Fatal, sMessageBuffer, LocationVector());
			return false;
		}
		//write the memory image into a VHDL Ram vectors file
		sprintf(sMessageBuffer, "Writing VHDL to %.255s...", CreateRelativeFileName(sWorkingDir, sOutputFileName).c_str());
		MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());
		Assembler::Assemble<LC3ISA>::VHDLWrite(VHDLFile, *pMemoryImages[LangLC3], MainWindow::MessageCallBack);
	}

	//*** Add new resources ***
	for(InputIter = InputList.begin(); InputIter != InputList.end(); InputIter++)
	{
		FileData *pFileData = GetFile(CreateFileNameRelative(sWorkingDir, *InputIter));
		if(!pFileData->IsBuild())
				continue;
		if(fSettings[PrintSymbols])
		{
			string sFile = pFileData->sFileName.Path + pFileData->sFileName.Bare + ".Symbols.csv";
			if(!GetFile(sFile))
			{
				ifstream File(sFile.c_str());
				if(File.good())
				{
					AddFile(sFile);
					GetFile(sFile)->SetLanguage(LangLC3);
				}
			}
		}
		if(fSettings[OutputImage])
		{
			string sFile = pFileData->sFileName.Path + pFileData->sFileName.Bare + ".obj";
			if(!GetFile(sFile))
			{
				ifstream File(sFile.c_str());
				if(File.good())
				{
					AddFile(sFile);
					FileData *pFD = GetFile(sFile);
					pFD->SetLanguage(LangLC3);
					pFD->SetSourceType(Resources);
				}
			}
		}
		if(fSettings[OutputVHDL])
		{
			string sFile = pFileData->sFileName.Path + pFileData->sFileName.Bare + ".vhd";
			if(!GetFile(sFile))
			{
				ifstream File(sFile.c_str());
				if(File.good())
					AddFile(sFile);
			}
		}
	}

	return true;
}

bool Project::BuildLC3b()
{
	//Initialize the build
	InputList = InputLists[LangLC3b];
	Programs[LangLC3b].clear();
	vector<string>::iterator InputIter;
	for(InputIter = InputList.begin(); InputIter != InputList.end(); InputIter++)
	{
		FileData *pFileData = GetFile(CreateFileNameRelative(sWorkingDir, *InputIter));
		if(!pFileData->IsBuild())
				continue;
		Programs[LangLC3b].push_back(pFileData->pProgram);
	}
	if(pMemoryImages[LangLC3b])
		delete pMemoryImages[LangLC3b];
	pMemoryImages[LangLC3b] = new RamVector;

	//Convert the symbolic assembly program into a memory image
	char sMessageBuffer[65 + MAX_FILENAME_CHAR];
	if(!Assembler::Assemble<LC3bISA>::Link(Programs[LangLC3b], *pMemoryImages[LangLC3b], MainWindow::MessageCallBack))
	{
		delete pMemoryImages[LangLC3b];
		pMemoryImages[LangLC3b] = NULL;
		return false;
	}

	if(fSettings[OutputVHDL])
	{
		//Create output file name.
		string sOutputFileName = sFileName.Path + sFileName.Bare + ".vhd";
		//Open the output file.
		ofstream VHDLFile(sOutputFileName.c_str());
		if(!VHDLFile.good())
		{
			sprintf(sMessageBuffer, "Unable to create file %.255s...", sOutputFileName.c_str());
			MainWindow::MessageCallBack(Fatal, sMessageBuffer, LocationVector());
			return false;
		}
		//write the memory image into a VHDL Ram vectors file
		sprintf(sMessageBuffer, "Writing VHDL to %.255s...", CreateRelativeFileName(sWorkingDir, sOutputFileName).c_str());
		MainWindow::MessageCallBack(Info, sMessageBuffer, LocationVector());
		Assembler::Assemble<LC3bISA>::VHDLWrite(VHDLFile, *pMemoryImages[LangLC3b], MainWindow::MessageCallBack);
	}

	//*** Add new resources ***
	for(InputIter = InputList.begin(); InputIter != InputList.end(); InputIter++)
	{
		FileData *pFileData = GetFile(CreateFileNameRelative(sWorkingDir, *InputIter));
		if(!pFileData->IsBuild())
				continue;
		if(fSettings[PrintSymbols])
		{
			string sFile = pFileData->sFileName.Path + pFileData->sFileName.Bare + ".Symbols.csv";
			if(!GetFile(sFile))
			{
				ifstream File(sFile.c_str());
				if(File.good())
				{
					AddFile(sFile);
					GetFile(sFile)->SetLanguage(LangLC3b);
				}
			}
		}
		if(fSettings[OutputImage])
		{
			string sFile = pFileData->sFileName.Path + pFileData->sFileName.Bare + ".obj";
			if(!GetFile(sFile))
			{
				ifstream File(sFile.c_str());
				if(File.good())
				{
					AddFile(sFile);
					FileData *pFD = GetFile(sFile);
					pFD->SetLanguage(LangLC3b);
					pFD->SetSourceType(Resources);
				}
			}
		}
	}
	if(fSettings[OutputVHDL])
	{
		string sFile = sFileName.Path + sFileName.Bare + ".vhd";
		if(!GetFile(sFile))
		{
			ifstream File(sFile.c_str());
			if(File.good())
				AddFile(sFile);
		}
	}

	return true;
}

}	//namespace AshIDE
