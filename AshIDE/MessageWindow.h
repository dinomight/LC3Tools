//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Menu_Bar.H>
#include <string>
#include "ReadOnlyEditor.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;

namespace AshIDE
{
	class MessageWindow : public ReadOnlyEditor
	{
	protected:
		friend class MainWindow;

		static Fl_Text_Display::Style_Table_Entry StyleTable[];

		/**********************************************************************\
			FileNameLine( [in] file name, [in] line number )
			PlainText( [in] text )
			MessageType( [in] message type )

			Common code used by Message to update both the text buffer
			and style buffer at the same time.
		\******/
		bool FileNameLine(const string &, unsigned int);
		bool PlainText(const string &);
		bool MessageID(MessageEnum);

	public:
		/**********************************************************************\
			MessageWindow( [in] x, [in] y, [in] width, [in] height )

			Constructs the message window.
			This window only displays text, there is no editing.
		\******/
		MessageWindow(int, int, int, int);

		/**********************************************************************\
			~MessageWindow( )

			Destructor
		\******/
		virtual ~MessageWindow();

		/**********************************************************************\
			handle( [in] event )

			All events pass through here.
		\******/
		virtual int handle(int);



	//*** Message Management Functions ***//
		/**********************************************************************\
			Message( [in] Message type, [in] message, [in] location stack )

			Adds a message to the window text.
		\******/
		bool Message(MessageEnum, const string &, const LocationVector &);
	};
}

#endif
