//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef PROJECTPARSER_H
#define PROJECTPARSER_H

#pragma warning (disable:4786)
#include <list>
#include "Project.h"
#include "ProjectToken.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;

namespace AshIDE
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		ProjectParser

		An instance of this class parses an tokenized project settings file into
		a project structure.

		Multiple parser instances can be run simultaneously (multi-threaded).
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class ProjectParser
	{
	protected:
		//Buffer for formatting error messages
		char sMessageBuffer[128 + MAX(2+2*MAX_IDENTIFIER_CHAR, 1+MAX_FILENAME_CHAR)];
		//The project we are parsing
		Project &TheProj;
		//Error callback function
		bool (*CallBack)(MessageEnum, const string &, const LocationVector &);
		//true if shouldn't print message for subsequent errors.
		//This is set if future errors are likely just caused by an error that
		//was already printed.
		bool fMaskErrors;

	public:
		/**********************************************************************\
			ProjectParser( [in-out] symbolic project, [in] error callback function )

			Attaches the program to this parser.
		\******/
		ProjectParser(Project &, CallBackFunction);

		/**********************************************************************\
			Parse( [in-out] start token, [in] end token )
			
			This will parse the stream of tokens and create the Project from it.
			If there is a semantic error in the stream, it will print an error
			message and return false. It will attempt to parse beyond the error.

			Multiple calls to Parse will continue to add to the AST.
		\******/
		virtual bool Parse(const list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			ParseSetting( [in-out] start token, [in] end token )
			
			This will parse project settings in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		virtual bool ParseSetting(list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			ParseDefineDef( [in-out] start token, [in] end token )
			
			This will parse the global define information in the stream of
			tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		virtual bool ParseDefineDef(list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			ParseFileDef( [in-out] start token, [in] end token )
			
			This will parse the file information in the stream of tokens.
			If there is a semantic error in the stream, it will print an error
			message and return false.

			The start token iterator will be updated to point after the parsed
			tokens.
		\******/
		virtual bool ParseFileDef(list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			~ProjectParser()
		\******/
		virtual ~ProjectParser();
	};
}

#endif
