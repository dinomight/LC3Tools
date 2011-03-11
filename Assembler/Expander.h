//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef EXPANDER_H
#define EXPANDER_H

#pragma warning (disable:4786)
#pragma warning (disable:4503)
#include <map>
#include <vector>
#include <list>
#include "Program.h"
#include "AsmToken.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Expander

		An instance of this class parses the macros in a tokenized assembly
		program, and expands the references to these macros.

		This is a one-use instance class. When created, Expand should be called
		exactly once. And then there is nothing more it can do. The reason
		this was created as a class instead of a function was to make it
		easier to store the MACRO structures and communicate between Expand
		functions.

		Multiple expander instances can be run simultaneously (multi-threaded).
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	template<class ISA>
	class Expander
	{
	protected:
		//Buffer for formatting error messages
		char sMessageBuffer[128 + MAX(2+2*MAX_IDENTIFIER_CHAR, 1+MAX_FILENAME_CHAR)];
		//Error callback function
		bool (*CallBack)(MessageEnum, const string &, const LocationVector &);
		Program &TheProg;
		list<Token *> &TokenList;
		//Stores defines
		//Maps symbol name to token
		typedef map<string, Token *> DefineMap;
		DefineMap Defines;
		//Stoers macros
		//Maps symbol name to macro info
		//Macro info is a list of parameters and a list of tokens
		typedef pair< vector<string>, list<Token *> > MacroInfo;
		typedef map<string, MacroInfo> MacroMap;
		MacroMap Macros;
		//Store a stack of expanded defines and macros so we can tell if there
		//is a recursive expansion
		typedef pair<string, LocationVector> MSInfo;
		list<MSInfo> MacroStack;


		/**********************************************************************\
			ParseInclude( [in-out] start token )
			
			This will parse include directives in the stream of tokens and
			lex the included files and add their tokens to this program's
			tokenlist.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		bool ParseInclude(list<Token *>::iterator &);

		/**********************************************************************\
			ParseDefine( [in-out] start token )
			
			This will parse the define macro in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		bool ParseDefine(list<Token *>::iterator &);

		/**********************************************************************\
			ParseMacro( [in-out] start token )
			
			This will parse the function macro in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.

			*NOTE: While compiling messages are better with the LocationStack
			pointing to the macro definition, linking messages are better with
			the LocationStack pointing to the macro instance. Might want to
			do something about this.
		\******/
		bool ParseMacro(list<Token *>::iterator &);

		/**********************************************************************\
			ParseIfDef( [in-out] start token, [in] ifdef or ifndef )
			
			This will parse the ifdef/ifndef construct in the stream of tokens.
			True for ifdef, false for ifndef.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		bool ParseIfDef(list<Token *>::iterator &, bool);

		/**********************************************************************\
			ExpandDefine( [in-out] start token, [in] define entry )
			
			This will parse the define macro in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		bool ExpandDefine(list<Token *>::iterator &, const DefineMap::iterator &);

		/**********************************************************************\
			ExpandMacro( [in-out] start token, [in] macro entry )
			
			This will parse the function macro in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		bool ExpandMacro(list<Token *>::iterator &, const MacroMap::iterator &);

		/**********************************************************************\
			Erase( [in] token )
			
			This will erase the parameter token from the token list, and return
			the next token after that token.
		\******/
		list<Token *>::iterator Erase(const list<Token *>::iterator &);

		/**********************************************************************\
			Erase( [in] start token, [in] end token )
			
			This will erase the tokens [start, end) from the token list,
			and return the next token after the deleted tokens.
		\******/
		list<Token *>::iterator Erase(const list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			CheckExpand( [in-out] start token )
			
			This check to see if the next tokens are Expand tokens. If so, it
			deletes and erases them, and pops the macrostack.

			Start token is updated to point to the next non-expand token.
			Returns true if an expand token was removed.
		\******/
		bool CheckExpand(list<Token *>::iterator &);

	public:
		/**********************************************************************\
			Expander( [in-out] symbolic program,
				[in-out] tokenized program, [in] error callback function )

			Attaches the program and tokenlist to this expander.
		\******/
		Expander(Program &, list<Token *> &, CallBackFunction);

		/**********************************************************************\
			Expand()
			
			This will parse the stream of tokens for macros, and then expand
			the macros where they are used. The symbols used to identify macros
			will be added to the program's symboltable.
			If there is an error in the stream, it will print an error
			message and return false. It will attempt to parse beyond the error.

			Don't call this function more than once per instance.
		\******/
		bool Expand();

		/**********************************************************************\
			~Expand()
		\******/
		virtual ~Expander();
	};
}

#include "Expander.cpp"

#endif
