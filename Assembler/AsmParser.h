//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef ASMPARSER_H
#define ASMPARSER_H

#pragma warning (disable:4786)
#include <list>
#include "Program.h"
#include "AsmToken.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		AsmParser

		An instance of this class parses an tokenized assembly program into
		an abstract syntrax tree.

		Identifiers are converted into Symbols.

		Multiple parser instances can be run simultaneously (multi-threaded).
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class AsmParser
	{
	protected:
		//Buffer for formatting error messages
		char sMessageBuffer[128 + MAX(2+2*MAX_IDENTIFIER_CHAR, 1+MAX_FILENAME_CHAR)];
		//The program we are parsing
		Program &TheProg;
		//Error callback function
		bool (*CallBack)(MessageEnum, const string &, const LocationVector &);
		bool fStructDef, fOrigin;
		//Stores the most recently defined label until an element or segment
		//claims it.
		list<Label *> LabelList, SaveLabelList;
		//true if shouldn't print message for subsequent errors.
		//This is set if future errors are likely just caused by an error that
		//was already printed.
		bool fMaskErrors;

	public:
		/**********************************************************************\
			AsmParser( [in-out] symbolic program, [in] error callback function )

			Attaches the program to this parser.
		\******/
		AsmParser(Program &, CallBackFunction);

		/**********************************************************************\
			Parse( [in-out] start token, [in] end token )
			
			This will parse the stream of tokens and create the Program from it.
			If there is a semantic error in the stream, it will print an error
			message and return false. It will attempt to parse beyond the error.

			Multiple calls to Parse will continue to add to the AST.
		\******/
		virtual bool Parse(const list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			ParseDirective( [in-out] start token, [in] end token )
			
			This will parse the directive in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		virtual bool ParseDirective(list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			ParseInstruction( [in-out] start token, [in] end token )
			
			This will parse the ISA instruction in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		virtual bool ParseInstruction(list<Token *>::iterator &, const list<Token *>::iterator &) = 0;

		/**********************************************************************\
			ParseData( [in-out] start token, [in] end token )
			
			This will parse the data definition in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		virtual bool ParseData(list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			ParseStruct( [in-out] start token, [in] end token )
			
			This will parse the struct declaration in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		virtual bool ParseStruct(list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			ParseNumber( [in-out] last good token, [in-out] start token,
				[in] end token, [in] true if allow symbol,
				[in] true if allow void )
			
			This will parse the number definition in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return NULL.

			The number could be an integer, real, character, or symbol. It
			returns a number object.

			The start token iterator will be updated to point after the parsed
			tokens. The last good token will be updated to the last token
			successfully parsed.
		\******/
		virtual Number *ParseNumber(list<Token *>::iterator &, list<Token *>::iterator &, const list<Token *>::iterator &, bool, bool);

		/**********************************************************************\
			ParseLabel( [in-out] start token, [in] end token )
			
			This will parse the label in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		virtual bool ParseLabel(list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			~AsmParser()
		\******/
		virtual ~AsmParser();
	};
}

#endif
