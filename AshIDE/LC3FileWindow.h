//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef LC3FILEWINDOW_H
#define LC3FILEWINDOW_H

#pragma warning (disable:4786)
#include <string>
#include "FileWindow.h"
#include "../LC3Assembler/LC3ISA.h"
#include "../JMTLib/HighlightLexer.h"

using namespace std;
using namespace JMT;
using namespace LC3;

namespace AshIDE
{
	class LC3FileWindow : public FileWindow
	{
	protected:

		//Style stuff
		static Fl_Text_Display::Style_Table_Entry StyleTable[];
		static list<Token *> TheList;
		static LC3ISA::Lexer TheLexer;
		static HighlightLexer TheHighlightLexer;

		virtual ~LC3FileWindow();

	public:
		/**********************************************************************\
			LC3bFileWindow( [in] width, [in] height,
				[in] file name (optional) )

			Constructs the file window. It can either open an existing file
			or create a new file.
		\******/
		LC3FileWindow(int, int, const string &sfilename = "");

		virtual bool GetIdentifier(string &) const;
		virtual bool GetRegister(string &) const;
		virtual bool Help() const;

		virtual bool ParseStyle(const string &, string &);
	};
}

#endif
