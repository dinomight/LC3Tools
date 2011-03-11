//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Expander.h"
#include <fstream>
#include <cstdio>
#include <ctime>

using namespace std;
using namespace JMT;

namespace Assembler	{

template<class ISA>
Expander<ISA>::Expander(Program &Prog, list<Token *> &Tokens, CallBackFunction) : TheProg(Prog), TokenList(Tokens)
{
	this->CallBack = CallBack;
}

template<class ISA>
bool Expander<ISA>::Expand()
{
	bool fRetVal = true;
	list<Token *>::iterator TokenIter;

	//First add compiler-defined macros to the table
	{
		string sTimeStamp, sTime, sDate;
		time_t Time;
		time(&Time);
		sTimeStamp = ctime(&Time);
		sTime = sTimeStamp.substr(11, 8);
		(sDate = sTimeStamp.substr(4, 7)) += sTimeStamp.substr(20, 4);


		if(!TheProg.TheSymbolTable.ResolveSymbol(TheProg.LocationStack, "__version__", CallBack, SymDefine))
			throw "ResolveSymbol on predefines failed!";
		Defines.insert(DefineMap::value_type("__version__", new RealToken(TheProg.LocationStack, ASM_VER_NUM)));

		if(!TheProg.TheSymbolTable.ResolveSymbol(TheProg.LocationStack, "__version_3__", CallBack, SymDefine))
			throw "ResolveSymbol on predefines failed!";
		Defines.insert(DefineMap::value_type("__version_3__", new RealToken(TheProg.LocationStack, ASM_VER_NUM)));

		if(!TheProg.TheSymbolTable.ResolveSymbol(TheProg.LocationStack, "__file__", CallBack, SymDefine))
			throw "ResolveSymbol on predefines failed!";
		Defines.insert(DefineMap::value_type("__file__", new StringToken(TheProg.LocationStack, TheProg.sFileName.Full)));

		if(!TheProg.TheSymbolTable.ResolveSymbol(TheProg.LocationStack, "__line__", CallBack, SymDefine))
			throw "ResolveSymbol on predefines failed!";
		Defines.insert(DefineMap::value_type("__line__", new IntegerToken(TheProg.LocationStack, 0)));

		if(!TheProg.TheSymbolTable.ResolveSymbol(TheProg.LocationStack, "__time__", CallBack, SymDefine))
			throw "ResolveSymbol on predefines failed!";
		Defines.insert(DefineMap::value_type("__time__", new StringToken(TheProg.LocationStack, sTime)));

		if(!TheProg.TheSymbolTable.ResolveSymbol(TheProg.LocationStack, "__date__", CallBack, SymDefine))
			throw "ResolveSymbol on predefines failed!";
		Defines.insert(DefineMap::value_type("__date__", new StringToken(TheProg.LocationStack, sDate)));
	}

	//Then run through the tokens and add all the new macro definitions to the macro table
	//Ifdef/else constructs are evaluated in serial as they are encountered, based
	//on existing definitions.
	for(TokenIter = TokenList.begin(); TokenIter != TokenList.end();)
	{
		Token *pToken = *TokenIter;
		if((*TokenIter)->TokenType == (TokenEnum)TDirective)
		{
			switch(((DirToken *)(*TokenIter))->Directive)
			{
			case DirInclude:
				if(!ParseInclude(TokenIter))
					fRetVal = false;
				break;
			case DirDefine:
				if(!ParseDefine(TokenIter))
					fRetVal = false;
				break;
			case DirMacro:
				if(!ParseMacro(TokenIter))
					fRetVal = false;
				break;
			case DirIfDef:
				if(!ParseIfDef(TokenIter, true))
					fRetVal = false;
				break;
			case DirIfNDef:
				if(!ParseIfDef(TokenIter, false))
					fRetVal = false;
				break;
			case DirElse:
				CallBack(Error, "Unmatched Else directive.", (*TokenIter)->LocationStack);
				TokenIter = Erase(TokenIter);
				fRetVal = false;
				break;
			default:
				TokenIter++;
			}
		}
		else
			TokenIter++;
	}

	//Run through all the tokens looking for references to defines and macros.
	for(TokenIter = TokenList.begin(); TokenIter != TokenList.end();)
	{
		if(CheckExpand(TokenIter))
			continue;	
		if((*TokenIter)->TokenType != TIdentifier)
		{
			TokenIter++;
			continue;
		}
		DefineMap::iterator DefineIter = Defines.find(((IDToken *)(*TokenIter))->sIdentifier);
		if(DefineIter != Defines.end())
		{
			if(!ExpandDefine(TokenIter, DefineIter))
				fRetVal = false;
			continue;
		}
		MacroMap::iterator MacroIter = Macros.find(((IDToken *)(*TokenIter))->sIdentifier);
		if(MacroIter != Macros.end())
		{
			if(!ExpandMacro(TokenIter, MacroIter))
				fRetVal = false;
			continue;
		}
		TokenIter++;
	}

	return fRetVal;
}

template<class ISA>
bool Expander<ISA>::ParseInclude(list<Token *>::iterator &StartIter)
{
	bool fRetVal = true;
	string sFileName;
	unsigned int IncludeID, i;
	LocationVector IncludeStack;
	ifstream IncludeFile;
	list<Token *> IncludeTokenList;
	typename ISA::Lexer IncludeLex(IncludeTokenList, CallBack, false);
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is the start of the include syntax
	list<Token *>::iterator TokenIter, OrigIter = StartIter;
	TokenIter = StartIter++;

	//Get include filename
	if(StartIter == TokenList.end() || (*StartIter)->TokenType != TString)
	{
		CallBack(Error, "Include directive must be followed by a filename string.", (*TokenIter)->LocationStack);
		goto IncludeFinish;
	}
	sFileName = CreateStandardPath(((StringToken *)(*StartIter))->sString.c_str());
	TokenIter = StartIter++;

	//The filename must be relative to the working directory
	sFileName = CreateRelativeFileName(sWorkingDir, CreateFileNameRelative(sWorkingDir, CreateFileNameRelative(InputList[(*OrigIter)->LocationStack.rbegin()->first], sFileName)));

	//Get the ID for the new input file.
	for(IncludeID = 0; IncludeID < InputList.size(); IncludeID++)
		if(InputList[IncludeID] == sFileName)
			//The file is already in the global input list
			break;
	if(IncludeID == InputList.size())
		//The file is not yet in the global input list
		InputList.push_back(sFileName);

	//Update the location stack to include this new file
	//Use location from the INCLUDE keyword
	IncludeStack = (*OrigIter)->LocationStack;
	//Make sure the file is not already in the current location stack.
	//This could result in an infinite include recursion.
	for(i = 0; i < IncludeStack.size(); i++)
		if(IncludeStack[i].first == IncludeID)
			break;
	if(i != IncludeStack.size())
	{
		//*NOTE: Should this be an error/ignored?
		sprintf(sMessageBuffer, "Recursive include of file %.255s ignored", sFileName.c_str());
		CallBack(Warning, sMessageBuffer, (*TokenIter)->LocationStack);
		goto IncludeFinish;
	}
	IncludeStack.push_back( LocationVector::value_type(IncludeID, 1) );

	//Open the included assembly file
	IncludeFile.open(CreateFileNameRelative(sWorkingDir, sFileName).c_str());
	if(!IncludeFile)
	{
		sprintf(sMessageBuffer, "Unable to open include file %.255s", CreateFileNameRelative(sWorkingDir, sFileName).c_str());
		CallBack(Fatal, sMessageBuffer, (*TokenIter)->LocationStack);
		goto IncludeFinish;
	}

	//Lex the new file, and then add the tokens to this program's tokenlist
	sprintf(sMessageBuffer, "(%.255s)", sFileName.c_str());
	CallBack(Info, sMessageBuffer, LocationVector());
//	CallBack(Info, "Lexing...", IncludeStack);
	if(!IncludeLex.Lex(IncludeStack, IncludeFile))
		fRetVal = false;
	TokenList.insert(StartIter, IncludeTokenList.begin(), IncludeTokenList.end());
	TokenIter++;
	StartIter = Erase(OrigIter, TokenIter);

	return fRetVal;

IncludeFinish:
	Erase(OrigIter, StartIter);
	return false;
}

template<class ISA>
bool Expander<ISA>::ParseDefine(list<Token *>::iterator &StartIter)
{
	string sSymbol;
	Token *pToken;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is the start of the define syntax
	list<Token *>::iterator TokenIter, OrigIter = StartIter;
	TokenIter = StartIter++;

	//Get the symbol
	if(StartIter == TokenList.end() || (*StartIter)->TokenType != TIdentifier)
	{
		CallBack(Error, "Define directive must be followed by a symbol and token.", (*TokenIter)->LocationStack);
		goto DefineFinish;
	}
	sSymbol = ((IDToken *)(*StartIter))->sIdentifier;
	TokenIter = StartIter++;

	//Get the token
	if(StartIter == TokenList.end())
	{
		CallBack(Error, "Define directive must be followed by a symbol and token.", (*TokenIter)->LocationStack);
		goto DefineFinish;
	}
	//Check for illegal tokens in the define:
	else if(	(*StartIter)->TokenType == (TokenEnum)TDirective &&
		( ((DirToken *)(*StartIter))->Directive == DirDefine || ((DirToken *)(*StartIter))->Directive == DirMacro
		|| ((DirToken *)(*StartIter))->Directive == DirIfDef || ((DirToken *)(*StartIter))->Directive == DirIfNDef
		|| ((DirToken *)(*StartIter))->Directive == DirInclude )	)
	{
		CallBack(Error, "Define directive missing token, or invalid use of pre-compiler directive for define token.", (*TokenIter)->LocationStack);
		goto DefineFinish;
	}
	pToken = *StartIter;
	TokenIter = StartIter++;

	//Add the symbol to the symboltable
	//Use DEFINE directive as location
	if(!TheProg.TheSymbolTable.ResolveSymbol((*OrigIter)->LocationStack, sSymbol, CallBack, SymDefine))
		goto DefineFinish;

	//If the define was already in the define or macro table,
	//then it would also have been a resolved symbol, so ResolveSymbol()
	//would have returned false.

	//Add the definition to the define table
	Defines.insert(DefineMap::value_type(sSymbol, pToken));
	//Remove the define tokens except for the token that was added to the define table
	Erase(OrigIter, TokenIter);
	//Remove the used token from the list, but don't delete it
	TokenList.erase(TokenIter);

	return true;

DefineFinish:
	Erase(OrigIter, StartIter);
	return false;
}

template<class ISA>
bool Expander<ISA>::ParseMacro(list<Token *>::iterator &StartIter)
{
	string sSymbol;
	vector<string> ParameterList;
	unsigned int i;
	list<Token *> MacroList;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is the start of the macro syntax
	list<Token *>::iterator TokenIter, OrigIter = StartIter, BeginTokens;
	TokenIter = StartIter++;

	//Get the symbol
	if(StartIter == TokenList.end() || (*StartIter)->TokenType != TIdentifier)
	{
		CallBack(Error, "Macro directive must be followed by a symbol.", (*TokenIter)->LocationStack);
		goto MacroFinish;
	}
	sSymbol = ((IDToken *)(*StartIter))->sIdentifier;
	TokenIter = StartIter++;

	//Get the open parenthesis
	if(StartIter == TokenList.end() || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpOpenParen)
	{
		CallBack(Error, "Macro definition missing open parenthesis.", (*TokenIter)->LocationStack);
		goto MacroFinish;
	}
	TokenIter = StartIter++;

	//get the parameters
	while(StartIter != TokenList.end() && (*StartIter)->TokenType == TIdentifier)
	{
		//Make sure the same parameter name wasn't used already
		for(i = 0; i < ParameterList.size(); i++)
		{
			if(ParameterList[i] == ((IDToken *)(*StartIter))->sIdentifier)
			{
				CallBack(Error, "Macro parameter name used more than once.", (*StartIter)->LocationStack);
				goto MacroFinish;
			}
		}
		ParameterList.push_back(((IDToken *)(*StartIter))->sIdentifier);
		TokenIter = StartIter++;
		//If there's a comma, then there's more parameters
		if(StartIter == TokenList.end() || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
			break;
		TokenIter = StartIter++;
	}

	//Get the close parenthesis
	if(StartIter == TokenList.end() || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseParen)
	{
		CallBack(Error, "Macro definition missing close parenthesis.", (*TokenIter)->LocationStack);
		goto MacroFinish;
	}
	TokenIter = StartIter++;
	BeginTokens = StartIter;

	//Get the tokens, which is everything until the End directive
	while( StartIter != TokenList.end() && !((*StartIter)->TokenType == (TokenEnum)TDirective && ((DirToken *)(*StartIter))->Directive == DirEnd) )
	{
		//Check for illegal tokens in the macro:
		if(	(*StartIter)->TokenType == (TokenEnum)TDirective &&
			( ((DirToken *)(*StartIter))->Directive == DirDefine || ((DirToken *)(*StartIter))->Directive == DirMacro
			|| ((DirToken *)(*StartIter))->Directive == DirIfDef || ((DirToken *)(*StartIter))->Directive == DirIfNDef
			|| ((DirToken *)(*StartIter))->Directive == DirInclude )	)
			break;

		MacroList.push_back(*StartIter);
		TokenIter = StartIter++;
	}

	if(StartIter == TokenList.end() || (*StartIter)->TokenType != (TokenEnum)TDirective || ((DirToken *)(*StartIter))->Directive != DirEnd)
	{
		CallBack(Error, "Macro definition missing End directive, or invalid use of pre-compiler directive inside macro definition.", (*TokenIter)->LocationStack);
		goto MacroFinish;
	}
	TokenIter = StartIter++;

	if(MacroList.empty())
		CallBack(Warning, "Empty Macro definition.", (*OrigIter)->LocationStack);

	//Add the symbol to the symboltable
	//Use location of MACRO directive as symbol location
	if(!TheProg.TheSymbolTable.ResolveSymbol((*OrigIter)->LocationStack, sSymbol, CallBack, SymMacro))
		goto MacroFinish;

	//If the define was already in the define or macro table,
	//then it would also have been a resolved symbol, so ResolveSymbol()
	//would have returned false.

	//Add the definition to the macro table
	Macros.insert( MacroMap::value_type(sSymbol, MacroInfo(ParameterList, MacroList)) );
	//Remove the macro tokens except for the ones that were added to the macro table
	Erase(OrigIter, BeginTokens);
	//Remove the used tokens from the list, but don't delete them
	TokenList.erase(BeginTokens, TokenIter);
	//Remove the end token
	Erase(TokenIter);

	return true;

MacroFinish:
	Erase(OrigIter, StartIter);
	return false;
}

template<class ISA>
bool Expander<ISA>::ParseIfDef(list<Token *>::iterator &StartIter, bool fIfDef)
{
	bool fRetVal = true, fFound;
	const char *sName = fIfDef ? "IfDef" : "IfNDef";
	string sSymbol;
	list<string> ParemeterList;
	list<Token *> MacroList;
	unsigned int EndCount, ElseCount;
	vector<unsigned int> ElseStack;
	LocationVector TempLocationStack = (*StartIter)->LocationStack;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is the start of the ifdef syntax
	list<Token *>::iterator OrigIter = StartIter, TokenIter;
	TokenIter = StartIter++;

	//Get the symbol
	if(StartIter == TokenList.end() || (*StartIter)->TokenType != TIdentifier)
	{
		sprintf(sMessageBuffer, "%.31s directive must be followed by a symbol.", sName);
		CallBack(Error, sMessageBuffer, TempLocationStack);
		TokenList.erase(TokenIter);
		return false;
	}
	sSymbol = ((IDToken *)(*StartIter))->sIdentifier;
	StartIter++;

	//Remove the IfDef definition
	Erase(OrigIter, StartIter);
	//Start iter points to first token of IF region.
	TokenIter = StartIter;
	
	fFound = Defines.find(sSymbol) != Defines.end();
	//If the symbol is defined, use everything inbetween here and the next else or end.
	//Else, use everything between the next else and end, or nothing.

	OrigIter = TokenIter;
	EndCount = 0;
	ElseCount = 0;
	while(	TokenIter != TokenList.end() &&
		!(	(*TokenIter)->TokenType == (TokenEnum)TDirective &&
			( EndCount == 0 && ((DirToken *)(*TokenIter))->Directive == DirEnd || ElseCount == 0 && ((DirToken *)(*TokenIter))->Directive == DirElse )	)	)
	{
		//Check for nested constructs that use an END directive
		if(	(*TokenIter)->TokenType == (TokenEnum)TDirective &&
			( ((DirToken *)(*TokenIter))->Directive == DirIfDef || ((DirToken *)(*TokenIter))->Directive == DirIfNDef
			|| ((DirToken *)(*TokenIter))->Directive == DirMacro || ((DirToken *)(*TokenIter))->Directive == DirStructDef )	)
			EndCount++;
		else if((*TokenIter)->TokenType == (TokenEnum)TDirective && ((DirToken *)(*TokenIter))->Directive == DirEnd)
		{
			if(ElseStack.rbegin() != ElseStack.rend() && (*ElseStack.rbegin()) == EndCount)
			{
				ElseCount--;
				ElseStack.pop_back();
			}
			EndCount--;
		}

		//Check for nested constructs that use an ELSE directive
		if(	(*TokenIter)->TokenType == (TokenEnum)TDirective &&
			( ((DirToken *)(*TokenIter))->Directive == DirIfDef || ((DirToken *)(*TokenIter))->Directive == DirIfNDef )	)
		{
			ElseCount++;
			ElseStack.push_back(EndCount);
		}

		TokenIter++;
	}

	if(!fFound && fIfDef || fFound && !fIfDef)
		//Remove the if part
		Erase(OrigIter, TokenIter);

	if(TokenIter != TokenList.end() && (*TokenIter)->TokenType == (TokenEnum)TDirective && ((DirToken *)(*TokenIter))->Directive == DirElse)
	{
		//Remove the Else token
		TokenIter = Erase(TokenIter);
		if(!fFound && fIfDef || fFound && !fIfDef)
			//Start iter points to first token of ELSE region
			StartIter = TokenIter;
		OrigIter = TokenIter;
		EndCount = 0;
		while(	TokenIter != TokenList.end() &&
			!( EndCount == 0 && (*TokenIter)->TokenType == (TokenEnum)TDirective && ((DirToken *)(*TokenIter))->Directive == DirEnd )	)
		{
			//Check for nested constructs that use an END directive
			if(	(*TokenIter)->TokenType == (TokenEnum)TDirective &&
				( ((DirToken *)(*TokenIter))->Directive == DirIfDef || ((DirToken *)(*TokenIter))->Directive == DirIfNDef
				|| ((DirToken *)(*TokenIter))->Directive == DirMacro || ((DirToken *)(*TokenIter))->Directive == DirStructDef )	)
				EndCount++;
			else if((*TokenIter)->TokenType == (TokenEnum)TDirective && ((DirToken *)(*TokenIter))->Directive == DirEnd)
				EndCount--;
			TokenIter++;
		}

		if(fFound && fIfDef || !fFound && !fIfDef)
			//Remove the else part
			Erase(OrigIter, TokenIter);

		if(TokenIter == TokenList.end())
		{
			sprintf(sMessageBuffer, "%.31s directive missing End directive.", sName);
			CallBack(Error, sMessageBuffer, TempLocationStack);
			return false;
		}
		//Remove the End token
		TokenIter = Erase(TokenIter);
	}
	else if(TokenIter != TokenList.end() && (*TokenIter)->TokenType == (TokenEnum)TDirective && ((DirToken *)(*TokenIter))->Directive == DirEnd)
	{
		//Remove the End token
		TokenIter = Erase(TokenIter);
		if(!fFound && fIfDef || fFound && !fIfDef)
			//Everything ignored
			StartIter = TokenIter;
	}
	else
	{
		sprintf(sMessageBuffer, "%.31s directive missing End directive.", sName);
		CallBack(Error, sMessageBuffer, TempLocationStack);
		if(!fFound && fIfDef || fFound && !fIfDef)
			//Everything ignored
			StartIter = TokenIter;
		return false;
	}

	return true;
}

template<class ISA>
bool Expander<ISA>::ExpandDefine(list<Token *>::iterator &StartIter, const DefineMap::iterator &DefineIter)
{
	//OrigIter is the start of the define syntax
	list<Token *>::iterator OrigIter = StartIter;
	StartIter++;

	//Check for resursive expansion
	for(list<MSInfo>::iterator MSIter = MacroStack.begin(); MSIter != MacroStack.end(); MSIter++)
	{
		if(MSIter->first == DefineIter->first)
		{
			sprintf(sMessageBuffer, "Recursive expansion of Define symbol '%.63s'.", DefineIter->first.c_str());
			CallBack(Error, sMessageBuffer, (*OrigIter)->LocationStack);
			for(list<MSInfo>::reverse_iterator MSIter = MacroStack.rbegin(); MSIter != MacroStack.rend(); MSIter++)
			{
				CallBack(Info, "See Macro expansion", MSIter->second);
			}
			//Remove the define use syntax token
			StartIter = Erase(OrigIter);
			return false;
		}
	}

	Token *pToken = DefineIter->second->Copy();
	//Defines take on the location of the identifier they replace.
	pToken->LocationStack = (*OrigIter)->LocationStack;
	if(DefineIter->first == "__line__")
		((IntegerToken *)(pToken))->Integer = (*OrigIter)->LocationStack.rbegin()->second;

	TokenList.insert(StartIter, pToken);

	//Setup macrostack
	MacroStack.push_back( MSInfo(DefineIter->first, DefineIter->second->LocationStack) );
	TokenList.insert(StartIter, new ExpandToken((*OrigIter)->LocationStack));

	//Remove the define use syntax token
	StartIter = Erase(OrigIter);

	return true;
}

template<class ISA>
bool Expander<ISA>::ExpandMacro(list<Token *>::iterator &StartIter, const MacroMap::iterator &MacroIter)
{
	vector<Token *> ParameterList;
	unsigned int i;
	bool fRecursive = false;
	Token *pToken;
	//TokenIter is a reference to the last-known-good token, which is used for
	//the line number. OrigIter is the start of the macro syntax
	list<Token *>::iterator TokenIter, OrigIter = StartIter, CopyIter;
	TokenIter = StartIter++;
	list<MSInfo>::iterator MSIter;

	CheckExpand(StartIter);

	//Get the open parenthesis
	if(StartIter == TokenList.end() || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpOpenParen)
	{
		CallBack(Error, "Macro usage missing open parenthesis.", (*TokenIter)->LocationStack);
		goto MacroFinish;
	}
	TokenIter = StartIter++;
	CheckExpand(StartIter);

	//get the parameters
	for(i = 0; i < MacroIter->second.first.size(); i++)
	{
		if(StartIter == TokenList.end())
			break;
		//Comma separates parameters
		if(i != 0)
		{
			if((*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpComma)
				break;
			TokenIter = StartIter++;
			CheckExpand(StartIter);
		}
		ParameterList.push_back(*StartIter);
		TokenIter = StartIter++;
		CheckExpand(StartIter);
	}
	//Get the close parenthesis
	if(StartIter == TokenList.end() || (*StartIter)->TokenType != TOperator || ((OpToken *)(*StartIter))->Operator != OpCloseParen)
	{
		CallBack(Error, "Macro usage missing close parenthesis.", (*TokenIter)->LocationStack);
		goto MacroFinish;
	}
	TokenIter = StartIter++;

	if(ParameterList.size() != MacroIter->second.first.size())
	{
		sprintf(sMessageBuffer, "Macro '%.63s' requires %u parameters.", MacroIter->first.c_str(), MacroIter->second.first.size());
		CallBack(Error, sMessageBuffer, (*TokenIter)->LocationStack);
		goto MacroFinish;
	}

	//Let it get this far just to parse past all the ( ... )
	//Check for resursive expansion
	for(MSIter = MacroStack.begin(); MSIter != MacroStack.end(); MSIter++)
	{
		if(MSIter->first == MacroIter->first)
		{
			sprintf(sMessageBuffer, "Recursive expansion of Macro symbol '%.63s'.", MacroIter->first.c_str());
			CallBack(Error, sMessageBuffer, (*OrigIter)->LocationStack);
			for(list<MSInfo>::reverse_iterator MSIter = MacroStack.rbegin(); MSIter != MacroStack.rend(); MSIter++)
			{
				CallBack(Info, "See Macro expansion", MSIter->second);
			}
			goto MacroFinish;
		}
	}

	//Copy the tokens from the macro into the program
	for(CopyIter = MacroIter->second.second.begin(); CopyIter != MacroIter->second.second.end(); CopyIter++)
	{
		if(CopyIter != TokenList.end() && (*CopyIter)->TokenType == TIdentifier)
		{
			//See if the current macro token is one of the parameters
			//*NOTE: This walking of the parameter list for every identifier is expensive
			for(i = 0; i < MacroIter->second.first.size(); i++)
			{
				if(MacroIter->second.first[i] == ((IDToken *)(*CopyIter))->sIdentifier)
				{
					pToken = ParameterList[i]->Copy();
					//Parameters take on the location of the token they replace.
					pToken->LocationStack = (*CopyIter)->LocationStack;
					TokenList.insert(StartIter, pToken);
					break;
				}
			}
			if(i != MacroIter->second.first.size())
				continue;
		}
		//Macro tokens retain their location
		TokenList.insert(StartIter, (*CopyIter)->Copy());
	}

	//Setup macrostack
	MacroStack.push_back( MSInfo(MacroIter->first, (*OrigIter)->LocationStack) );
	TokenList.insert(StartIter, new ExpandToken((*OrigIter)->LocationStack));

	//Remove the macro use syntax tokens
	TokenIter++;
	StartIter = Erase(OrigIter, TokenIter);

	return true;

MacroFinish:
	Erase(OrigIter, StartIter);
	return false;
}

template<class ISA>
list<Token *>::iterator Expander<ISA>::Erase(const list<Token *>::iterator &StartIter)
{
	delete *StartIter;
	return TokenList.erase(StartIter);
}

template<class ISA>
list<Token *>::iterator Expander<ISA>::Erase(const list<Token *>::iterator &StartIter, const list<Token *>::iterator &EndIter)
{
	list<Token *>::iterator TokenIter = StartIter;
	while(TokenIter != EndIter)
	{
		delete *TokenIter;
		TokenIter = TokenList.erase(TokenIter);
	}
	return TokenIter;
}

template<class ISA>
bool Expander<ISA>::CheckExpand(list<Token *>::iterator &StartIter)
{
	bool fExpand = false;
	while(StartIter != TokenList.end() && (*StartIter)->TokenType == (TokenEnum)TExpand)
	{
		fExpand = true;
		if(MacroStack.empty())
			throw "MacroStack empty, unmatched expand token!";
		MacroStack.pop_back();
		delete *StartIter;
		StartIter = TokenList.erase(StartIter);
	}
	return fExpand;
}

template<class ISA>
Expander<ISA>::~Expander()
{
	for(DefineMap::iterator DefineIter = Defines.begin(); DefineIter != Defines.end(); DefineIter++)
		delete DefineIter->second;

	for(MacroMap::iterator MacroIter = Macros.begin(); MacroIter != Macros.end(); MacroIter++)
		for(list<Token *>::iterator TokenIter = MacroIter->second.second.begin(); TokenIter != MacroIter->second.second.end(); TokenIter++)
			delete *TokenIter;
}

}	//namespace Assembler
