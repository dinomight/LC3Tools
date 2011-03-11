//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#pragma warning (disable:4786)
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include "../Assembler/Base.h"
#include "Project.h"

using namespace std;
using namespace JMT;

namespace AshIDE
{
	class SettingsWindow : public Fl_Group
	{
	protected:
		friend class MainWindow;

		//Widgets for the project settings
		Fl_Group *pSettings;
		Fl_Group *pProjectSettings;
		Fl_Check_Button *pProjectSetting[NUM_PROJECT_SETTINGS];
		Fl_Group *pFileSettings;
		Fl_Button *pDefineAddDlg, *pDefineRemove;
		Fl_Choice *pDefines;
		Fl_Button *pAssemble, *pBuild, *pSimulate;
		
		//Widgets for the file settings
		Fl_Check_Button *pFileSetting[NUM_PROJECT_FILE_SETTINGS];
		Fl_Choice *pLanguage;
		Fl_Choice *pSourceType;
		Fl_Button *pAssembleFile;

		//Widgets for the pre-processor define dialog
		Fl_Window *pDefineDlg;
		Fl_Input *pDefineIdentifier, *pDefineValue;
		Fl_Return_Button *pDefineAdd;
		Fl_Button *pDefineCancel;

	public:
		/**********************************************************************\
			SettingsWindow( [in] x, [in] y, [in] width, [in] height )

			Constructs the message window.
			This window only displays text, there is no editing.
		\******/
		SettingsWindow(int, int, int, int);

		/**********************************************************************\
			~SettingsWindow( )

			Destructor
		\******/
		virtual ~SettingsWindow();



	//*** Project Settings Management Functions ***//
		/**********************************************************************\
			InitProjectSettings( )

			Initializes the project settings based on the current project.
		\******/
		bool InitProjectSettings();

		static void ProjectSettingCB(Fl_Widget *pW, void *pV)	{	TheProject.SetSetting((Project::SettingEnum)(unsigned int)pV, ((Fl_Button *)pW)->value() != 0);	}
		static void DefineAddDlgCB(Fl_Widget *pW, void *pV);
		bool DefineAddDlg();
		static void DefineRemoveCB(Fl_Widget *pW, void *pV)	{	((SettingsWindow *)pV)->DefineRemove();	}
		bool DefineRemove();
		static void DefineAddCB(Fl_Widget *pW, void *pV)	{	((SettingsWindow *)pV)->DefineAdd();	}
		bool DefineAdd();
		static void DefineCancelCB(Fl_Widget *pW, void *pV)	{	((SettingsWindow *)pV)->DefineCancel();	}
		bool DefineCancel();



	//*** File Settings Management Functions ***//
		/**********************************************************************\
			InitFileSettings( )

			Initializes the file settings based on the current file in the current project.
		\******/
		bool InitFileSettings();

		static void FileSettingCB(Fl_Widget *pW, void *pV)	{	if(TheProject.pFile)	TheProject.pFile->SetSetting((Project::FileData::FileSettingEnum)(unsigned int)pV, ((Fl_Button *)pW)->value() != 0);	}
		static void LanguageCB(Fl_Widget *pW, void *pV)	{	((SettingsWindow *)pV)->Language();	}
		bool Language();
		static void SourceTypeCB(Fl_Widget *pW, void *pV)	{	((SettingsWindow *)pV)->SourceType();	}
		bool SourceType();

	};
}

#endif
