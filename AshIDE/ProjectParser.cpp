//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "ProjectParser.h"
#include <cstdio>
#include "LC3bFileWindow.h"
#include "LC3FileWindow.h"
#include "FileWindow.h"
#include "MainWindow.h"

using namespace std;
using namespace JMT;

namespace AshIDE	{

ProjectParser::ProjectParser(Project &Proj, CallBackFunction) : TheProj(Proj)
{
	this->CallBack = CallBack;
	fMaskErrors = false;
}

bool ProjectParser::Parse(const list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	bool fRetVal = true;
	list<Token *>::iterator TokenIter = StartIter;
	fMaskErrors = false;

	while(TokenIter != EndIter)
	{
		switch((*TokenIter)->TokenType)
		{
		case TKeyword:
			if(!ParseSetting(TokenIter, EndIter))
			{
				fMaskErrors = true;
				fRetVal = false;
			}
			else
				fMaskErrors = false;
			break;
		default:
			if(!fMaskErrors)
			{
				sprintf(sMessageBuffer, "Unexpected token: %.127s", (const char *)**TokenIter);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			}
			TokenIter++;
			fMaskErrors = true;
			fRetVal = false;
		}
	}

	return fRetVal;
}

bool ProjectParser::ParseSetting(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	bool fRetVal = true;
	//TokenIter is a reference to the last-known-good token, which is used for
	list<Token *>::iterator TokenIter = StartIter;
	KeywordEnum Keyword;
	bool fTrue;

	switch(((KeyToken *)(*StartIter))->Keyword)
	{
	case KeyDefineDef:	//Specifies the global defines in the project
		TokenIter = StartIter++;
		return ParseDefineDef(StartIter, EndIter);

	case KeyFileDef:	//Specifies a file in the project
		TokenIter = StartIter++;
		return ParseFileDef(StartIter, EndIter);

	case KeyExpanded:
	case KeyNotExpanded:
		Keyword = ((KeyToken *)(*StartIter))->Keyword;
		TokenIter = StartIter++;

		//Get the equals sign
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
		{
			sprintf(sMessageBuffer, "%.31s project setting missing equals sign.", sKeywords[Keyword]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		TokenIter = StartIter++;

		//Get the source type
		if(StartIter == EndIter || (*StartIter)->TokenType != TKeyword || ((KeyToken *)(*StartIter))->Keyword < FirstSource || ((KeyToken *)(*StartIter))->Keyword > LastSource)
		{
			sprintf(sMessageBuffer, "%.31s project setting missing source type.", sKeywords[Keyword]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		TheProj.SetExpanded((Project::SourceEnum)(((KeyToken *)(*StartIter))->Keyword - FirstSource), (Keyword == KeyExpanded ? true : false));
		TokenIter = StartIter++;
		break;

	case KeySymbols:
	case KeyAST:
	case KeyTokens:
	case KeyOptimizations:
	case KeyImage:
	case KeyVHDL:
	case KeyUseOS:
	case KeyOldLC3:
	case KeyDisReg:
		Keyword = ((KeyToken *)(*StartIter))->Keyword;
		TokenIter = StartIter++;

		//Get the equals sign
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
		{
			sprintf(sMessageBuffer, "%.31s project setting missing equals sign.", sKeywords[Keyword]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		TokenIter = StartIter++;

		//Get true or false
		if(StartIter == EndIter || (*StartIter)->TokenType != TKeyword || ( ((KeyToken *)(*StartIter))->Keyword != KeyTrue && ((KeyToken *)(*StartIter))->Keyword != KeyFalse) )
		{
			sprintf(sMessageBuffer, "%.31s project setting missing TRUE or FALSE.", sKeywords[Keyword]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		fTrue = ((KeyToken *)(*StartIter))->Keyword == KeyTrue;
		TokenIter = StartIter++;

		TheProj.SetSetting((Project::SettingEnum)(Keyword - FirstSetting), fTrue);
		break;

	case KeyHelpLocation:
		TokenIter = StartIter++;

		//Get the equals sign
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
		{
			sprintf(sMessageBuffer, "%.31s project setting missing equals sign.", sKeywords[KeyHelpLocation]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		TokenIter = StartIter++;

		//Get the path
		if(StartIter == EndIter || (*StartIter)->TokenType != TString )
		{
			sprintf(sMessageBuffer, "%.31s project setting missing file path string.", sKeywords[KeyHelpLocation]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		TheProj.sHelpLocation = ((StringToken *)(*StartIter))->sString;
		TokenIter = StartIter++;
		break;

	case KeyLC3OSLocation:
	case KeyLC3bOSLocation:
		Keyword = ((KeyToken *)(*StartIter))->Keyword;
		TokenIter = StartIter++;

		//Get the equals sign
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
		{
			sprintf(sMessageBuffer, "%.31s project setting missing equals sign.", sKeywords[Keyword]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		TokenIter = StartIter++;

		//Get the path
		if(StartIter == EndIter || (*StartIter)->TokenType != TString )
		{
			sprintf(sMessageBuffer, "%.31s project setting missing file path string.", sKeywords[Keyword]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		TheProj.sOSLocation[Keyword == KeyLC3OSLocation ? LangLC3 : LangLC3b] = ((StringToken *)(*StartIter))->sString;
		TokenIter = StartIter++;
		break;

	default:
		if(!fMaskErrors)
		{
			sprintf(sMessageBuffer, "Keyword %.31s not a valid project setting.", sKeywords[((KeyToken *)(*StartIter))->Keyword]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
		}
		TokenIter = StartIter++;
	}

	return fRetVal;
}

bool ProjectParser::ParseDefineDef(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	bool fRetVal = true;
	fMaskErrors = false;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is the start of the filedef syntax
	list<Token *>::iterator TokenIter = StartIter;
	string sDefineIdentifier, sDefineValue;

	//Check for open brace
	if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpOpenBrace)
	{
		sprintf(sMessageBuffer, "Global defines definition missing open brace.");
		CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
		return false;
	}
	TokenIter = StartIter++;

	//check for keywords
	while(StartIter != EndIter && (*StartIter)->TokenType == TKeyword || StartIter != EndIter && (*StartIter)->TokenType == TIdentifier)
	{
		if((*StartIter)->TokenType == TKeyword)
			sDefineIdentifier = sKeywords[((KeyToken *)(*StartIter))->Keyword];
		else
			sDefineIdentifier = ((IDToken *)(*StartIter))->sIdentifier;
		TokenIter = StartIter++;

		//Get the equals sign
		if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
		{
			sprintf(sMessageBuffer, "Global define missing equals sign.");
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		TokenIter = StartIter++;

		//Get the value string
		if(StartIter == EndIter || (*StartIter)->TokenType != TString)
		{
			sprintf(sMessageBuffer, "Global define missing value string.");
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			return false;
		}
		sDefineValue = ((StringToken *)(*StartIter))->sString;
		TokenIter = StartIter++;

		if(!TheProj.AddDefine(sDefineIdentifier, sDefineValue))
		{
			sprintf(sMessageBuffer, "Global define identifier defined multiple times.");
			CallBack(Warning, sMessageBuffer, (*TokenIter)->LocationStack);
		}
	}

	//Check for close brace
	if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseBrace)
	{
		sprintf(sMessageBuffer, "Global defines definition missing close brace.");
		CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
		return false;
	}
	TokenIter = StartIter++;

	return fRetVal;
}

bool ProjectParser::ParseFileDef(list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	bool fRetVal = true;
	fMaskErrors = false;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is the start of the filedef syntax
	list<Token *>::iterator TokenIter = StartIter;
	string sFileName;
	KeywordEnum Keyword;
	bool fTrue;
	unsigned int i;
	bool fWindowSpecified = false, fLanguageSpecified = false, fSourceSpecified = false, fSettingSpecified[NUM_PROJECT_FILE_SETTINGS];
	for(i = 0; i < NUM_PROJECT_FILE_SETTINGS; i++)
		fSettingSpecified[i] = false;
	bool fWindow = false;
	LanguageEnum Language;
	Project::SourceEnum SourceType;
	bool fFileSettings[NUM_PROJECT_FILE_SETTINGS];

	//Check for open brace
	if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpOpenBrace)
	{
		sprintf(sMessageBuffer, "File definition missing open brace.");
		CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
		return false;
	}
	TokenIter = StartIter++;

	//check for keywords
	while(StartIter != EndIter && (*StartIter)->TokenType == TKeyword)
	{
		switch(((KeyToken *)(*StartIter))->Keyword)
		{
		case KeyFileName:	//Specifies a filename relative to the project path
			TokenIter = StartIter++;

			if(sFileName != "")
			{
				sprintf(sMessageBuffer, "%.31s file setting specified multiple times.", sKeywords[KeyFileName]);
				CallBack(Warning, sMessageBuffer, (*TokenIter)->LocationStack);
			}

			//Get the equals sign
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
			{
				sprintf(sMessageBuffer, "%.31s file setting missing equals sign.", sKeywords[KeyFileName]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the filename
			if(StartIter == EndIter || (*StartIter)->TokenType != TString)
			{
				sprintf(sMessageBuffer, "%.31s file setting missing filename string.", sKeywords[KeyFileName]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			sFileName = CreateStandardPath(((StringToken *)(*StartIter))->sString);
			TokenIter = StartIter++;
			break;

		case KeyWindow:	//Specifies the file was open in a window
			Keyword = ((KeyToken *)(*StartIter))->Keyword;
			TokenIter = StartIter++;

			//Get the equals sign
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
			{
				sprintf(sMessageBuffer, "%.31s file setting missing equals sign.", sKeywords[Keyword]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get true or false
			if(StartIter == EndIter || (*StartIter)->TokenType != TKeyword || ( ((KeyToken *)(*StartIter))->Keyword != KeyTrue && ((KeyToken *)(*StartIter))->Keyword != KeyFalse) )
			{
				sprintf(sMessageBuffer, "%.31s file setting missing TRUE or FALSE.", sKeywords[Keyword]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			fTrue = ((KeyToken *)(*StartIter))->Keyword == KeyTrue;
			TokenIter = StartIter++;

			switch(Keyword)
			{
			case KeyWindow:
				fWindow = fTrue;
				fWindowSpecified = true;
				break;
			}
			break;

		case KeyLanguage:	//Specifies the language of the file
			TokenIter = StartIter++;

			//Get the equals sign
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
			{
				sprintf(sMessageBuffer, "%.31s file setting missing equals sign.", sKeywords[KeyLanguage]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the language type
			if(StartIter == EndIter || (*StartIter)->TokenType != TKeyword || !( ((KeyToken *)(*StartIter))->Keyword >= FirstLang && ((KeyToken *)(*StartIter))->Keyword <= LastLang) )
			{
				sprintf(sMessageBuffer, "%.31s file setting missing language type.", sKeywords[KeyLanguage]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			Language = (LanguageEnum)(((KeyToken *)(*StartIter))->Keyword - FirstLang);
			fLanguageSpecified = true;
			TokenIter = StartIter++;
			break;

		case KeySourceType:	//Specifies the source type of the file
			TokenIter = StartIter++;

			//Get the equals sign
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
			{
				sprintf(sMessageBuffer, "%.31s file setting missing equals sign.", sKeywords[KeyLanguage]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get the source type
			if(StartIter == EndIter || (*StartIter)->TokenType != TKeyword || !( ((KeyToken *)(*StartIter))->Keyword >= FirstSource && ((KeyToken *)(*StartIter))->Keyword <= LastSource) )
			{
				sprintf(sMessageBuffer, "%.31s file setting missing language type.", sKeywords[KeyLanguage]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			SourceType = (Project::SourceEnum)(((KeyToken *)(*StartIter))->Keyword - FirstSource);
			fSourceSpecified = true;
			TokenIter = StartIter++;
			break;

		case KeyExcludeFromBuild:
			Keyword = ((KeyToken *)(*StartIter))->Keyword;
			TokenIter = StartIter++;

			//Get the equals sign
			if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpEquals)
			{
				sprintf(sMessageBuffer, "%.31s file setting missing equals sign.", sKeywords[Keyword]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			TokenIter = StartIter++;

			//Get true or false
			if(StartIter == EndIter || (*StartIter)->TokenType != TKeyword || ( ((KeyToken *)(*StartIter))->Keyword != KeyTrue && ((KeyToken *)(*StartIter))->Keyword != KeyFalse) )
			{
				sprintf(sMessageBuffer, "%.31s file setting missing TRUE or FALSE.", sKeywords[Keyword]);
				CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
				return false;
			}
			fTrue = ((KeyToken *)(*StartIter))->Keyword == KeyTrue;
			TokenIter = StartIter++;

			fFileSettings[(Project::FileData::FileSettingEnum)(Keyword - FirstFileSetting)] = fTrue;
			fSettingSpecified[(Keyword - FirstFileSetting)] = true;
			break;

		default:
			sprintf(sMessageBuffer, "Keyword %.31s not a valid file setting.", sKeywords[((KeyToken *)(*StartIter))->Keyword]);
			CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
			TokenIter = StartIter++;
			fRetVal = false;		
		}
	}

	//Check for close brace
	if(StartIter == EndIter || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseBrace)
	{
		sprintf(sMessageBuffer, "File definition missing close brace.");
		CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
		return false;
	}
	TokenIter = StartIter++;

	if(sFileName == "")
	{
		sprintf(sMessageBuffer, "File definition missing file name.");
		CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
		return false;
	}

	if(fRetVal)
	{
		Project::FileData FileData(CreateFileNameRelative(TheProj.sWorkingDir, sFileName));
		if(fLanguageSpecified)
			FileData.SetLanguage(Language);
		if(fSourceSpecified)
			FileData.SetSourceType(SourceType);
		for(i = 0; i < NUM_PROJECT_FILE_SETTINGS; i++)
			if(fSettingSpecified[i])
				FileData.SetSetting((Project::FileData::FileSettingEnum)i, fFileSettings[i]);
		TheProj.AddFile(FileData);
		if(fWindowSpecified && fWindow)
			TheMainWindow.OpenFile(FileData.sFileName.Full);
	}

	return fRetVal;
}

ProjectParser::~ProjectParser()
{
}

}	//namespace AshIDE
