//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "SettingsWindow.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include "MainWindow.h"
#include "FileWindow.h"

using namespace std;
using namespace JMT;

namespace AshIDE	{

SettingsWindow::SettingsWindow(int X, int Y, int Width, int Height) : Fl_Group(X, Y, Width, Height)
{
	{
		//B is the  border width. PSH is the part of the settings taken up by the project settings. The remainder is the file settings.
		int B = 16, PSX = X+B, PSY = Y+B, PSW = Width-B-B, PSH = 128, FSX = PSX, FSY = PSY+PSH+B, FSW = PSW, FSH = Height-B-PSH-B-B;

		pProjectSettings = new Fl_Group(PSX, PSY, PSW, PSH, "Project Settings");
		{
			pProjectSettings->box(FL_PLASTIC_UP_FRAME);
			pProjectSettings->align(FL_ALIGN_TOP);
			pProjectSettings->labelfont(pProjectSettings->labelfont() | FL_BOLD);

			pProjectSetting[Project::PrintTokens] = new Fl_Check_Button(PSX+4, PSY+4, 2*PSW/5-4, 20, "Print Tokens");
			pProjectSetting[Project::PrintAST] = new Fl_Check_Button(PSX+4, PSY+24, 2*PSW/5-4, 20, "Print AST");
			pProjectSetting[Project::PrintSymbols] = new Fl_Check_Button(PSX+4, PSY+44, 2*PSW/5-4, 20, "Print Symbols");
			pProjectSetting[Project::OutputImage] = new Fl_Check_Button(PSX+4, PSY+64, 2*PSW/5-4, 20, "Output Image");
			pProjectSetting[Project::OutputVHDL] = new Fl_Check_Button(PSX+4, PSY+84, 2*PSW/5-4, 20, "Output VHDL");
			pProjectSetting[Project::UseOptimizations] = new Fl_Check_Button(PSX+4, PSY+104, 2*PSW/5-4, 20, "Use Optimizations");

			pProjectSetting[Project::UseOS] = new Fl_Check_Button(PSX+2*PSW/4, PSY+4, 2*PSW/5-4, 20, "Use Ash Operating System");
			pProjectSetting[Project::OldLC3] = new Fl_Check_Button(PSX+2*PSW/4, PSY+24, 2*PSW/5-4, 20, "Support Old LC3 Syntax");
			pDefines = new Fl_Choice(PSX+2*PSW/4, PSY+64, PSW/4, 20, "Global Defines");
			pDefines->box(FL_PLASTIC_DOWN_BOX);
			pDefines->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
			pDefineAddDlg = new Fl_Button(PSX+3*PSW/4+4, PSY+44, PSW/6, 20, "Add");
			pDefineAddDlg->callback(DefineAddDlgCB, this);
			pDefineRemove = new Fl_Button(PSX+3*PSW/4+4, PSY+64, PSW/6, 20, "Remove");
			pDefineRemove->callback(DefineRemoveCB, this);
			pAssemble = new Fl_Button(PSX+2*PSW/4, PSY+84, PSW/4, 20, "Assemble");
			pAssemble->callback(MainWindow::AssembleCB, NULL);
			pBuild = new Fl_Button(PSX+3*PSW/4+4, PSY+84, PSW/6, 20, "Build");
			pBuild->callback(MainWindow::BuildCB, NULL);
			pSimulate = new Fl_Button(PSX+2*PSW/4, PSY+104, PSW/4, 20, "Simulate");
			pSimulate->callback(MainWindow::SimulateCB, NULL);
			pProjectSetting[Project::DisReg] = new Fl_Check_Button(PSX+3*PSW/4+4, PSY+104, PSW/6, 20, "Dis + Reg");

			for(unsigned int i = 0; i < NUM_PROJECT_SETTINGS; i++)
				pProjectSetting[i]->callback(ProjectSettingCB, (void *)i);
		}
		pProjectSettings->end();

		pFileSettings = new Fl_Group(FSX, FSY, FSW, FSH, "File Settings");
		{
			pFileSettings->box(FL_PLASTIC_UP_FRAME);
			pFileSettings->align(FL_ALIGN_TOP);
			pFileSettings->labelfont(pFileSettings->labelfont() | FL_BOLD);

			pFileSetting[Project::FileData::ExcludeFromBuild] = new Fl_Check_Button(FSX+4, FSY+4, 2*FSW/4-4, 20, "Exclude From Build");
			for(unsigned int i = 0; i < NUM_PROJECT_FILE_SETTINGS; i++)
				pFileSetting[i]->callback(FileSettingCB, (void *)i);
			pAssembleFile = new Fl_Button(FSX+4, FSY+24, FSW/4, 20, "Assemble");
			pAssembleFile->callback(MainWindow::AssembleFileCB, NULL);

			pLanguage = new Fl_Choice(FSX+2*FSW/4, FSY+4, FSW/4, 20, "Language");
			pLanguage->box(FL_PLASTIC_DOWN_BOX);
			pLanguage->callback(LanguageCB, this);
			pLanguage->align(FL_ALIGN_RIGHT);
			pLanguage->add("None");
			pLanguage->add("LC3");
			pLanguage->add("LC3b");
			pSourceType = new Fl_Choice(FSX+2*FSW/4, FSY+24, FSW/4, 20, "Source Type");
			pSourceType->box(FL_PLASTIC_DOWN_BOX);
			pSourceType->callback(SourceTypeCB, this);
			pSourceType->align(FL_ALIGN_RIGHT);
			pSourceType->add("LC3 Source");
			pSourceType->add("LC3 Header");
			pSourceType->add("LC3b Source");
			pSourceType->add("LC3b Header");
			pSourceType->add("Resource");
			pSourceType->add("Dependency");
			pFileSettings->end();
		}
		pFileSettings->hide();
	}
	end();

	Fl_Group *pCurrent = current();
	current(NULL);
	pDefineDlg = new Fl_Window(220, 105, "Global Define");
	{
		pDefineIdentifier = new Fl_Input(70, 10, 140, 25, "Identifier");
		pDefineValue = new Fl_Input(70, 40, 140, 25, "Value");

		pDefineAdd = new Fl_Return_Button(70, 70, 65, 25, "Add");
		pDefineAdd->callback(DefineAddCB, this);

		pDefineCancel = new Fl_Button(145, 70, 65, 25, "Cancel");
		pDefineCancel->callback(DefineCancelCB, this);
	}
	pDefineDlg->end();
	pDefineDlg->callback(DefineCancelCB, this);
	pDefineDlg->set_non_modal();
	current(pCurrent);
}

SettingsWindow::~SettingsWindow()
{
	delete pDefineDlg;
}

bool SettingsWindow::InitProjectSettings()
{
	for(int i = 0; i < NUM_PROJECT_SETTINGS; i++)
		pProjectSetting[i]->value(TheProject.GetSetting((Project::SettingEnum)i));
	pDefines->clear();
	for(Project::DefineList::iterator DefineIter = TheProject.Defines.begin(); DefineIter != TheProject.Defines.end(); DefineIter++)
		pDefines->add((DefineIter->first+"="+DefineIter->second).c_str());
	pDefines->redraw();
	pDefines->value(pDefines->size()-2);
	if(pDefines->size() > 1)
		pDefineRemove->activate();
	else
		pDefineRemove->deactivate();
	return true;
}

void SettingsWindow::DefineAddDlgCB(Fl_Widget *pW, void *pV)
{
	TheMainWindow.pSettingsWindow->DefineAddDlg();
}

bool SettingsWindow::DefineAddDlg()
{
	pDefineDlg->show();
	focus(pDefineIdentifier);
	return true;
}

bool SettingsWindow::DefineRemove()
{
	if(pDefines->size() <= 1)
		//0 could be selected even though the list is emtpy
		return false;

	int Selected = pDefines->value();
	TheProject.RemoveDefine(Selected);
	pDefines->remove(Selected);
	pDefines->value(pDefines->size()-2);
	pDefines->redraw();
	if(pDefines->size() <= 1)
		pDefineRemove->deactivate();
	return true;
}

bool SettingsWindow::DefineAdd()
{
	const char *sIdentifier = pDefineIdentifier->value();
	const char *sValue = pDefineValue->value();
	if(sIdentifier[0] == 0 || sValue[0] == 0)
	{
		fl_alert("You must provide both an identifier and a value.");
		return false;
	}
	for(unsigned int i = 0; i < strlen(sIdentifier); i++)
	{
		char TempC = sIdentifier[i];
		if( !( (TempC >= 'a' && TempC <= 'z') || (TempC >= 'A' && TempC <= 'Z') || TempC == '_'
			|| i > 0 && (TempC >= '0' && TempC <= '9') ) )
		{
			fl_alert("The identifier can only contain the characters a-z, A-Z, '_', and 0-9 (except the first character).");
			return false;
		}
	}
	if(TheProject.AddDefine(sIdentifier, sValue))
	{
		string sDefine;
		sDefine = string(sIdentifier)+"="+sValue;
		pDefines->add(sDefine.c_str());
		pDefines->redraw();
		pDefines->value(pDefines->size()-2);
	}
	else
	{
		pDefines->clear();
		for(Project::DefineList::iterator DefineIter = TheProject.Defines.begin(); DefineIter != TheProject.Defines.end(); DefineIter++)
			pDefines->add((DefineIter->first+"="+DefineIter->second).c_str());
		pDefines->value(pDefines->size()-2);
	}
	if(pDefines->size() > 1)
		pDefineRemove->activate();
	pDefineDlg->hide();
	return true;
}

bool SettingsWindow::DefineCancel()
{
	pDefineDlg->hide();
	return true;
}

bool SettingsWindow::InitFileSettings()
{
	if(TheProject.pFile)
	{
		pFileSettings->show();
		for(int i = 0; i < NUM_PROJECT_FILE_SETTINGS; i++)
			pFileSetting[i]->value(TheProject.pFile->GetSetting((Project::FileData::FileSettingEnum)i));
		if(TheProject.pFile->IsSource())
			pFileSetting[Project::FileData::ExcludeFromBuild]->activate();
		else
			pFileSetting[Project::FileData::ExcludeFromBuild]->deactivate();
		pLanguage->value(TheProject.pFile->GetLanguage());
		pSourceType->value(TheProject.pFile->GetSourceType());
	}
	else
	{
		pFileSettings->hide();
	}
	return true;
}

bool SettingsWindow::Language()
{
	LanguageEnum Lang = TheProject.pFile->GetLanguage();
	if(Lang != TheProject.pFile->SetLanguage((LanguageEnum)pLanguage->value()))
	{
		FileWindow *pFW;
		if(pFW = FileWindow::IsOpen(TheProject.pFile->sFileName.Full))
		{
			string sFileName = TheProject.pFile->sFileName.Full;
			if(fl_ask("You must close and re-open the file for a language change to take effect.\nRe-open the file?"))
			{
				if(pFW->CloseNow())
					TheMainWindow.OpenFile(sFileName);
			}
			else
				pFW->show();
		}
	}
	return true;
}

bool SettingsWindow::SourceType()
{
	LanguageEnum Lang = TheProject.pFile->GetLanguage();
	TheProject.pFile->SetSourceType((Project::SourceEnum)pSourceType->value());
	if(Lang != TheProject.pFile->GetLanguage())
	{
		FileWindow *pFW;
		if(pFW = FileWindow::IsOpen(TheProject.pFile->sFileName.Full))
		{
			string sFileName = TheProject.pFile->sFileName.Full;
			if(fl_ask("You must close and re-open the file for a language change to take effect.\nRe-open the file?"))
			{
				if(pFW->CloseNow())
					TheMainWindow.OpenFile(sFileName);
			}
			else
				pFW->show();
		}
	}
	return true;
}

}	//namespace AshIDE
