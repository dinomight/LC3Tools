//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef LC3BFILEWINDOW_H
#define LC3BFILEWINDOW_H

#pragma warning (disable:4786)
#include <string>
#include "FileWindow.h"
#include "../LC3bAssembler/LC3bISA.h"
#include "../JMTLib/HighlightLexer.h"

using namespace std;
using namespace JMT;
using namespace LC3b;

namespace AshIDE
{
	class LC3bFileWindow : public FileWindow
	{
	protected:

		//Style stuff
		static Fl_Text_Display::Style_Table_Entry StyleTable[];
		static list<Token *> TheList;
		static LC3bISA::Lexer TheLexer;
		static HighlightLexer TheHighlightLexer;
		//This function is just here to satisfy the lexer and will never be called
		static bool AsmCallBack(MessageEnum MessageType, const string &sMessage, const LocationVector &LocationStack)
		{	return true;	}

		virtual ~LC3bFileWindow();

	public:
		/**********************************************************************\
			LC3bFileWindow( [in] width, [in] height,
				[in] file name (optional) )

			Constructs the file window. It can either open an existing file
			or create a new file.
		\******/
		LC3bFileWindow(int, int, const string &sfilename = "");

		virtual bool GetIdentifier(string &) const;
		virtual bool GetRegister(string &) const;
		virtual bool Help() const;

		virtual bool ParseStyle(const string &, string &);
	};
}

#endif
