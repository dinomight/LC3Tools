//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "FilesWindow.h"
#include <FL/Fl.H>
#include "MainWindow.h"

using namespace std;
using namespace JMT;

namespace AshIDE	{

const char *const FilesWindow::sSources[NUM_SOURCE_TYPES] = {"LC3 Source Files", "LC3 Header Files", "LC3b Source Files", "LC3b Header Files", "Resources", "Dependencies"};
const string FilesWindow::sExpanded = "@f[-]\t@b", FilesWindow::sClosed = "@f[+]\t@b";
int FilesWindow::ColumnWidths[4] = {32, 10, 10, 0};

FilesWindow::FilesWindow(int X, int Y, int Width, int Height) : Fl_Hold_Browser(X, Y, Width, Height)
{
	column_widths(ColumnWidths);
}

FilesWindow::~FilesWindow()
{
}

int FilesWindow::handle(int Event)
{
	unsigned int Selected, i, j;
	Project::FileData *pFileData;

	int RetVal = Fl_Hold_Browser::handle(Event);
	switch(Event)
	{
	case FL_RELEASE:
		if( !(Selected = value()) )
		{
			//No item selected
			TheProject.SelectFile(NULL);
			break;
		}

		if(Fl::event_clicks())	//Double click
		{
			if( !(pFileData = (Project::FileData *)data(Selected)) )
			{
				//A source category was selected
				TheProject.SelectFile(NULL);

				//Figure out which category it was
				for(i = 0; i < NUM_SOURCE_TYPES; i++)
				{
					if(Location[i] == Selected)
						break;
				}

				//Change the expanded/closed state of the category
				if(TheProject.GetExpanded((Project::SourceEnum)i))
				{
					//close the category
					text(Selected, (sClosed + sSources[i]).c_str());
					TheProject.SetExpanded((Project::SourceEnum)i, false);
					for(j = Location[i]+1; j < (i+1 < NUM_SOURCE_TYPES ? Location[i+1] : size()+1); j++)
						hide(j);
				}
				else
				{
					//expand the category
					text(Selected, (sExpanded + sSources[i]).c_str());
					TheProject.SetExpanded((Project::SourceEnum)i, true);
					for(j = Location[i]+1; j < (i+1 < NUM_SOURCE_TYPES ? Location[i+1] : size()+1); j++)
						show(j);
				}
			}
			else
			{
				//A file was selected
				TheMainWindow.OpenFile(pFileData->sFileName.Full);
				TheProject.SelectFile(pFileData);
			}
		}
		else	//Single click
		{
			if( !(pFileData = (Project::FileData *)data(Selected)) )
			{
				//A source category was selected
				TheProject.SelectFile(NULL);
			}
			else
			{
				//A file was selected
				//Set it as the current selection
				TheProject.SelectFile(pFileData);
			}
		}
		break;
	}
	return RetVal;
}

bool FilesWindow::UpdateList()
{
	clear();
	unsigned int i;

	//Initialize the source categories
	for(i = 0; i < NUM_SOURCE_TYPES; i++)
	{
		if(TheProject.GetExpanded((Project::SourceEnum)i))
			add((sExpanded + sSources[i]).c_str());
		else
			add((sClosed + sSources[i]).c_str());
		Location[i] = i+1;
		hide(Location[i]);
	}

	//Run through the project files in reverse order. This way alphabetical insertion will always insert at directly after
	//the header line.
	for(Project:: FileMap::reverse_iterator FileIter = TheProject.Files.rbegin(); FileIter != TheProject.Files.rend(); FileIter++)
	{
		Project::FileData &FileData = FileIter->second;
		
		show(Location[FileData.GetSourceType()]);
		insert(Location[FileData.GetSourceType()]+1, (string("\t\t") + FileData.sFileName.Name).c_str(), &FileData);
		if(!TheProject.GetExpanded(FileData.GetSourceType()))
			hide(Location[FileData.GetSourceType()]+1);
		for(int i = FileData.GetSourceType()+1; i < NUM_SOURCE_TYPES; i++)
			Location[i]++;
	}

	//If a file was selected before updating the list, and the file is still in the project, re-select it
	if(TheProject.pFile && TheProject.GetExpanded(TheProject.pFile->GetSourceType()))
	{
		for(i = Location[TheProject.pFile->GetSourceType()]; true; i++)
		{
			if((Project::FileData *)data(i) == TheProject.pFile)
			{
				select(i, 1);
				TheProject.SelectFile(TheProject.pFile);
				break;
			}
		}
	}

	return true;
}

}	//namespace AshIDE
