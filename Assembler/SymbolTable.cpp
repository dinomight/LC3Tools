//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "SymbolTable.h"
#include <cstdio>

using namespace std;
using namespace JMT;

namespace Assembler	{

Symbol *SymbolTable::ReferenceSymbol(const LocationVector &LocationStack, const string &ssymbol, CallBackFunction)
{
	bool fWarn = false;
	string sSymbol = ssymbol;
	if(sSymbol.size() > MAX_IDENTIFIER_CHAR)
	{
		sSymbol.resize(MAX_IDENTIFIER_CHAR);
		fWarn = true;
	}
	SymbolMap::iterator SymbolIter;

	//See if the symbol has been resolved
	SymbolIter = ResolvedSymbols.find(sSymbol);
	if(SymbolIter != ResolvedSymbols.end())
		//the symbol has been resolved already
		return SymbolIter->second;
	SymbolIter = ExternSymbols.find(sSymbol);
	if(SymbolIter != ExternSymbols.end())
		//the symbol has been resolved already
		return SymbolIter->second;
	else
	{	//The symbol has not been resolved yet
		//see if the symbol has been referenced before
		SymbolIter = UnresolvedSymbols.find(sSymbol);
		if(SymbolIter != UnresolvedSymbols.end())
			//the symbol was referenced already
			return SymbolIter->second;
		else
		{	//this is the first reference to this symbol. Create a new symbol for it
			if(fWarn)
			{
				sprintf(sMessageBuffer, "Identifier '%.63s' truncated to %u characters.", sSymbol.c_str(), MAX_IDENTIFIER_CHAR);
				CallBack(Warning, sMessageBuffer, LocationStack);
			}
			Symbol *pSymbol = new Symbol(LocationStack, sSymbol, Addressability);
			//Add the symbol to the unresolved symbol table
			UnresolvedSymbols.insert( SymbolMap::value_type(sSymbol, pSymbol) );
			return pSymbol;
		}
	}
}

Symbol *SymbolTable::ResolveSymbol(const LocationVector &LocationStack, const string &ssymbol, CallBackFunction, SymbolEnum SymbolType)
{
	Symbol *pSymbol;
	bool fWarn = false;
	string sSymbol = ssymbol;
	if(sSymbol.size() > MAX_IDENTIFIER_CHAR)
	{
		sSymbol.resize(MAX_IDENTIFIER_CHAR);
		fWarn = true;
	}
	SymbolMap::iterator SymbolIter;

	//See if the symbol has been defined before
	SymbolIter = ResolvedSymbols.find(sSymbol);
	if(SymbolIter != ResolvedSymbols.end())
	{	//the symbol has been defined already
		if(SymbolIter->second->SymbolType == SymLabel && SymbolType == SymExtern)
			//extern referring to label in same file
			//This allows both the implementing and using programs to use the same header.
			//*NOTE: Should we give a warning?
			return SymbolIter->second;
		sprintf(sMessageBuffer, "Symbol '%.63s' already defined as %.31s.", SymbolIter->first.c_str(), sSymbols[SymbolIter->second->SymbolType]);
		CallBack(Error, sMessageBuffer, LocationStack);
		sprintf(sMessageBuffer, "See definition of '%.63s'.", SymbolIter->first.c_str());
		CallBack(Info, sMessageBuffer, SymbolIter->second->LocationStack);
		return NULL;
	}
	SymbolIter = ExternSymbols.find(sSymbol);
	if(SymbolIter != ExternSymbols.end())
	{	//the symbol has been defined already

		if(SymbolType == SymLabel)
		{
			//Allow a label to override and extern.
			//This allows both the implementing and using programs to use the same header.
			//*NOTE: Should we give a warning?
			pSymbol = SymbolIter->second;
			ExternSymbols.erase(SymbolIter);
			goto CreateSymbol;
		}
		if(SymbolType == SymExtern)
			//Another extern will just point to the same external label.
			//This could happen from multiple headers declaring the extern.
			return SymbolIter->second;
		else
		{
			sprintf(sMessageBuffer, "Symbol '%.63s' already defined as %.31s.", SymbolIter->first.c_str(), sSymbols[SymbolIter->second->SymbolType]);
			CallBack(Error, sMessageBuffer, LocationStack);
			sprintf(sMessageBuffer, "See definition of '%.63s'.", SymbolIter->first.c_str());
			CallBack(Info, sMessageBuffer, SymbolIter->second->LocationStack);
			return NULL;
		}
	}

	//See if the symbol has been referenced yet
	SymbolIter = UnresolvedSymbols.find(sSymbol);
	if(SymbolIter == UnresolvedSymbols.end())
	{
		//The symbol has not been referenced yet. Create a new symbol for it
		if(fWarn)
		{
			sprintf(sMessageBuffer, "Identifier '%.63s' truncated to %u characters.", sSymbol.c_str(), MAX_IDENTIFIER_CHAR);
			CallBack(Warning, sMessageBuffer, LocationStack);
		}
		pSymbol = new Symbol(LocationStack, sSymbol, Addressability);
	}
	else
	{
		pSymbol = SymbolIter->second;
		UnresolvedSymbols.erase(SymbolIter);
	}

CreateSymbol:
	//Move it into the resolved symbol table.
	pSymbol->LocationStack = LocationStack;
	pSymbol->SymbolType = SymbolType;
	if(SymbolType == SymExtern)
		ExternSymbols.insert(SymbolMap::value_type(pSymbol->sSymbol, pSymbol));
	else
		ResolvedSymbols.insert(SymbolMap::value_type(pSymbol->sSymbol, pSymbol));
	
	return pSymbol;
}

bool SymbolTable::ResolveExternTable(SymbolTable &RefST, CallBackFunction)
{
	bool fRetVal = true;

	//Go through each extern symbol in the parameter symboltable
	for(SymbolMap::iterator ExtSymbolIter = ExternSymbols.begin(); ExtSymbolIter != ExternSymbols.end(); ExtSymbolIter++)
	{
		//For each extern symbol, see if it's resolved in this symboltable
		SymbolMap::iterator SymbolIter = RefST.ResolvedSymbols.find(ExtSymbolIter->first);
		//Also make sure that it is a label. Other types can't be externally linked.
		if(SymbolIter != RefST.ResolvedSymbols.end() && SymbolIter->second->SymbolType == SymLabel)
		{
			//Found it, update the symbol.
			if(ExtSymbolIter->second->pSymbol)
			{
				CallBack(Error, string("Multiply defined external symbol: ")+=(ExtSymbolIter->first), ExtSymbolIter->second->LocationStack);
				CallBack(Info, "See definition.", ExtSymbolIter->second->pSymbol->LocationStack);
				CallBack(Info, "See definition.", SymbolIter->second->LocationStack);
				fRetVal = false;
			}
			else
				ExtSymbolIter->second->pSymbol = SymbolIter->second;
		}
	}

	return fRetVal;
}

bool SymbolTable::Check(CallBackFunction) const
{
	bool fRetVal = true;

	//check all the unresolved Symbols
	for(SymbolMap::const_iterator SymbolIter = UnresolvedSymbols.begin(); SymbolIter != UnresolvedSymbols.end(); SymbolIter++)
	{
		CallBack(Error, string("Unresolved symbol: ")+=(SymbolIter->first), SymbolIter->second->LocationStack);
		fRetVal = false;
	}

	return fRetVal;
}

bool SymbolTable::ClearExtern(CallBackFunction)
{
	//clear all the external symbol references
	for(SymbolMap::const_iterator SymbolIter = ExternSymbols.begin(); SymbolIter != ExternSymbols.end(); SymbolIter++)
		SymbolIter->second->pSymbol = NULL;

	return true;
}

bool SymbolTable::CheckExtern(CallBackFunction) const
{
	bool fRetVal = true;

	//check all the external Symbols
	for(SymbolMap::const_iterator SymbolIter = ExternSymbols.begin(); SymbolIter != ExternSymbols.end(); SymbolIter++)
	{
		if(!SymbolIter->second->pSymbol)
		{
			CallBack(Error, string("Unresolved external symbol: ")+=(SymbolIter->first), SymbolIter->second->LocationStack);
			fRetVal = false;
		}
	}

	return fRetVal;
}

ostream &operator <<(ostream &Output, const SymbolTable &TheST)
{
	SymbolTable::SymbolMap::const_iterator SymbolIter;

	for(SymbolIter = TheST.ResolvedSymbols.begin(); SymbolIter != TheST.ResolvedSymbols.end(); SymbolIter++)
		Output << (const char *)*(SymbolIter->second) << endl;

	for(SymbolIter = TheST.ExternSymbols.begin(); SymbolIter != TheST.ExternSymbols.end(); SymbolIter++)
		Output << (const char *)*(SymbolIter->second) << endl;

	return Output;
}

SymbolTable::~SymbolTable()
{
	SymbolMap::const_iterator SymbolIter;

	//Delete all the Symbols
	for(SymbolIter = ResolvedSymbols.begin(); SymbolIter != ResolvedSymbols.end(); SymbolIter++)
		delete SymbolIter->second;
	for(SymbolIter = UnresolvedSymbols.begin(); SymbolIter != UnresolvedSymbols.end(); SymbolIter++)
		delete SymbolIter->second;
	for(SymbolIter = ExternSymbols.begin(); SymbolIter != ExternSymbols.end(); SymbolIter++)
		delete SymbolIter->second;
}

}	//namespace Assembler
