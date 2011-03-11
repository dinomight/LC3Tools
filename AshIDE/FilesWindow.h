//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef FILESWINDOW_H
#define FILESWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Hold_Browser.H>
#include <string>
#include <list>
#include "../Assembler/Base.h"
#include "Project.h"

using namespace std;
using namespace JMT;

namespace AshIDE
{
	class FilesWindow : public Fl_Hold_Browser
	{
	protected:
		friend class MainWindow;

		//Locations of the file type dividers
		unsigned int Location[NUM_SOURCE_TYPES];
		static const char *const sSources[NUM_SOURCE_TYPES];
		static const string sExpanded, sClosed;
		//Column widths
		static int ColumnWidths[4];

	public:
		/**********************************************************************\
			FilesWindow( [in] x, [in] y, [in] width, [in] height )

			Constructs the message window.
			This window only displays text, there is no editing.
		\******/
		FilesWindow(int, int, int, int);

		/**********************************************************************\
			~FilesWindow( )

			Destructor
		\******/
		virtual ~FilesWindow();

		/**********************************************************************\
			handle( [in] event )

			All events pass through here.
		\******/
		virtual int handle(int);




	//*** File List Management Functions ***//
		/**********************************************************************\
			UpdateList( )

			Updates the displayed list of files to mach what is currently in
			the project.
		\******/
		bool UpdateList();



	};
}

#endif
