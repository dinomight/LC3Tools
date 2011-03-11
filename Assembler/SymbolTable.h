//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#pragma warning (disable:4786)
#include <map>
#include <string>
#include "Symbol.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		SymbolTable

		Keeps track of all symbols in a program. Keeps track of whether they
		have been resolved or not.

		Can have multi-threaded access if std::map is threadsafe.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class SymbolTable
	{
	public:
		//Buffer for formatting error messages
		char sMessageBuffer[129 + MAX_IDENTIFIER_CHAR];
		//Maps symbol name to symbol object
		typedef map<string, Symbol *> SymbolMap;
		//Stores symbols that have been resolved (defined)
		SymbolMap ResolvedSymbols;
		//Stores symbols that have been referenced but not yet defined
		SymbolMap UnresolvedSymbols;
		//Stores symbols that have been defined as extern.
		SymbolMap ExternSymbols;
		//Addressability. Specifies the number of bits in a byte address
		//that aren't addressable in this ISA.
		unsigned char Addressability;

		/**********************************************************************\
			ReferenceSymbol( [in] location stack, [in] symbol name,
				[in] error callback function );
			
			If the symbol is already in the table, it returns a pointer to it.
			Otherwise it adds a new symbol to the table and returns a pointer.

			The string will be truncated to the maximum characters if necessary;
		\******/
		virtual Symbol *ReferenceSymbol(const LocationVector &, const string &, CallBackFunction);

		/**********************************************************************\
			ResolveSymbol( [in] location stack, [in] symbol name,
				[in] error callback function, [in] the type of symbol );
			
			Makes sure the symbol has not already been resolved.
			If not, transfers it from the unresolved to the resolved symbol
			table, and returns the symbol pointer. Returns NULL if it was
			already resolved. If it has not yet been referenced, it makes the
			reference first.

			If the reference is external, transfers it from the unresolved to
			the extern symbol table.

			The string will be truncated to the maximum characters if necessary;
		\******/
		virtual Symbol *ResolveSymbol(const LocationVector &, const string &, CallBackFunction, SymbolEnum);

		/**********************************************************************\
			ResolveExternTable( [in-out] other symbol table);
			
			Sees if any external references in this symboltable
			were defined in the parameter symboltable.
		\******/
		virtual bool ResolveExternTable(SymbolTable &, CallBackFunction);

		/**********************************************************************\
			Check( [in] error callback function )
			
			This checks to see if there are any unresolved symbols.
			If there are, it prints an error message and returns false.
		\******/
		virtual bool Check(CallBackFunction) const;

		/**********************************************************************\
			ClearExtern( [in] error callback function )
			
			Clears all external references (pointers to symbols in other
			programs). The actual external symbols are still in the database,
			they have just been "reset". Call this function before calling
			ResolveExternTable for the first time with a new build, to make
			sure there are no invalid pointers stuck in the database.
		\******/
		virtual bool ClearExtern(CallBackFunction);

		/**********************************************************************\
			CheckExtern( [in] error callback function )
			
			This checks to see if there are any unresolved external symbols.
			If there are, it prints an error message and returns false.
		\******/
		virtual bool CheckExtern(CallBackFunction) const;

		/**********************************************************************\
			Prints the symbol table as a comma-delimited spread sheet of the
			following format:

			Symbol Name, GetValue, GetSegmentRelativeValue\n
			...
			Unresolved Symbol Name\n
			...
		\******/
		friend ostream &operator <<(ostream &, const SymbolTable &);

		virtual ~SymbolTable();
	};
}

#endif
