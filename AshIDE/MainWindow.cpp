//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "MainWindow.h"
#include <FL/Fl_File_Chooser.H>
#include "LC3bFileWindow.h"
#include "LC3FileWindow.h"
#include "FileWindow.h"
#include <fstream>

using namespace std;

namespace AshIDE	{

unsigned int MainWindow::MainWidth = 640, MainWindow::MainHeight = 480;
unsigned int MainWindow::FileWidth = 400, MainWindow::FileHeight = 560;
unsigned int MainWindow::MenuHeight = 28, MainWindow::FilesWidth = 170, MainWindow::SettingsHeight = 224;

Fl_Menu_Item MainWindow::MenuItems[] =
	{
		{ "&Project", 0, 0, 0, FL_SUBMENU },	//0
			{ "&New File",				FL_CTRL + 'n',				NewFileCB },
			{ "&Open File",				FL_CTRL + 'o',				OpenFileCB, 0, FL_MENU_DIVIDER },
			{ "Add &File To Project",	0,							AddFileCB, 0, FL_MENU_DIVIDER | FL_MENU_INVISIBLE },
			{ "New P&roject",			FL_CTRL + FL_SHIFT + 'n',	NewProjectCB },
			{ "Open &Project",			FL_CTRL + FL_SHIFT + 'o',	OpenProjectCB, 0, FL_MENU_DIVIDER },
			{ "&Save Project",			FL_CTRL + 's',				SaveProjectCB, 0, FL_MENU_INVISIBLE },
			{ "Save Project &As",		FL_CTRL + FL_SHIFT + 's',	SaveProjectAsCB, 0, FL_MENU_DIVIDER | FL_MENU_INVISIBLE },
			{ "&Close Project",			0,							CloseProjectCB, 0, FL_MENU_DIVIDER | FL_MENU_INVISIBLE },
			{ "E&xit",					FL_CTRL + 'q',				ExitCB },
			{ 0 },

		{ "&Messages", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//11
			{ "&Save",			0,								SaveMessagesCB, 0, FL_MENU_DIVIDER },
			{ "&Copy",			FL_CTRL + 'c',					CopyMessagesCB, 0, FL_MENU_DIVIDER },
			{ "&Find...",		FL_CTRL + 'f',					FindMessagesCB },
			{ "F&ind Again",	FL_CTRL + 'g',					FindMessagesAgainCB },
			{ 0 },

		{ "&Build", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//17
			{ "&Remove From Project",	FL_Delete,				RemoveFromProjectCB, 0, FL_MENU_INVISIBLE },
			{ "Assemble &File",			FL_SHIFT + FL_F + 2,	AssembleFileCB, 0, FL_MENU_DIVIDER | FL_MENU_INVISIBLE },
			{ "Add &Global Define",		0,						SettingsWindow::DefineAddDlgCB, 0, FL_MENU_DIVIDER },
			{ "&Assemble Project",		FL_F + 2,				AssembleCB },
			{ "&Build Project",			FL_F + 3,				BuildCB },
			{ "R&ebuild All",			0,						RebuildCB },
			{ "Start Simulator",		FL_F + 4,				SimulateCB },
			{ 0 },

		{ "S&imulate", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//26
			{ "&Go",							FL_F + 5,		SimulatorWindow::GoCB },
			{ "Go One &Instruction",			FL_F + 6,		SimulatorWindow::GoOneCB, 0, FL_MENU_DIVIDER },
			{ "Step I&nto Next Function",		FL_F + 8,		SimulatorWindow::StepInCB },
			{ "Step O&ver Next Function",		FL_F + 9,		SimulatorWindow::StepOverCB },
			{ "Step &Out Of Current Function",	FL_F + 10,		SimulatorWindow::StepOutCB },
			{ "&Signal Interrupt",				0,				SimulatorWindow::InterruptCB, 0, FL_MENU_INVISIBLE },
			{ "Brea&k",							FL_F + 11,		SimulatorWindow::BreakCB, 0, FL_MENU_INVISIBLE },
			{ "&Reset",							FL_F + 12,		SimulatorWindow::ResetCB, 0, FL_MENU_INVISIBLE },
			{ "&End Simulation",				FL_CTRL + 'q',	SimulatorWindow::StopCB, 0, FL_MENU_DIVIDER | FL_MENU_INVISIBLE },

			{ "&Breakpoints", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//36
				{ "&Breakpoints",	FL_CTRL + 'b',	SimulatorWindow::BreakpointsCB },
				{ 0 },

			{ "Vie&w", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//39
				{ "&Console I/O",	FL_CTRL + 'i',	SimulatorWindow::ConsoleCB, 0, FL_MENU_DIVIDER },
				{ "&Disassembly",	FL_CTRL + 'd',	SimulatorWindow::DisassemblyCB },
				{ "Call &Stack",	FL_CTRL + 'k',	SimulatorWindow::CallStackCB, 0, FL_MENU_DIVIDER },
				{ "&Instructions",	FL_CTRL + '1',	SimulatorWindow::InstructionsCB },
				{ "D&ata Values",	FL_CTRL + '2',	SimulatorWindow::DataValuesCB },
				{ "&Memory Bytes",	FL_CTRL + '3',	SimulatorWindow::MemoryBytesCB },
				{ "&Registers",		FL_CTRL + '4',	SimulatorWindow::RegistersCB },
				{ "Pipe&lines",		FL_CTRL + '5',	SimulatorWindow::PipelinesCB },
				{ "&Programs",		FL_CTRL + '6',	SimulatorWindow::ProgramsCB },
				{ 0 },

			{ "&Edit && Continue", 0, 0, 0, FL_SUBMENU | FL_MENU_INVISIBLE },	//50
				{ "Edit &Instruction/Data/Memory",	FL_F + 2,	SimulatorWindow::WriteDataCB },
				{ "Edit &Register",					FL_F + 3,	SimulatorWindow::WriteRegisterCB },
				{ 0 },

			{ 0 },

		{ "&Help", 0, 0, 0, FL_SUBMENU },	//55
			{ "&Contents",			FL_F + 1,		MainWindow::HelpCB },
			{ "&About",				0,				MainWindow::AboutCB },
			{ 0 },

		{ 0 }
	};

MainWindow *MainWindow::pMainWindow = NULL;

MainWindow::MainWindow() : Fl_Double_Window(MainWidth, MainHeight)
{
	{
		//Setup the menu
		pMainMenu = new Fl_Menu_Bar(0, 0, w(), MenuHeight);
		pMainMenu->copy(MenuItems, this);
		//Create the project files window
		pFilesWindow = new FilesWindow(0, MenuHeight, FilesWidth, SettingsHeight);
		pFilesWindow->hide();
		//Create the project settings properties
		pSettingsWindow = new SettingsWindow(FilesWidth, MenuHeight, w()-FilesWidth, SettingsHeight);
		pSettingsWindow->hide();
		//Create the message window
		pMessageWindow = new MessageWindow(0, SettingsHeight+MenuHeight, w(), h()-SettingsHeight-MenuHeight);
		pMessageWindow->hide();
	}
	end();
	resizable(pMessageWindow);
	callback(ExitCB, this);

	SimulatorWindow::pSimulatorWindow = NULL;

	pHelpDlg = new Fl_Help_Dialog();

	show();
	SetTitle();
}

MainWindow::~MainWindow()
{
	delete pHelpDlg;
	hide();
}

bool MainWindow::Exit()
{
	if(Fl::event_key() == FL_Escape)
		// ignore Escape
		return false;
	if(!CloseProject())
		return false;
	if(!FileWindow::CloseAll())
		return false;
	//*NOTE: A bug in the FLTK delete_widget process causes the program to hang if
	//you use delete_widget on the last widget
	//delete this;
	Fl::delete_widget(this);
	return true;
}

bool MainWindow::SetTitle()
{
	string sTitle = "AshIDE";

	if(Project::pProject)
		sTitle += string(" - ") + TheProject.sFileName.Bare;

	label(sTitle.c_str());
	return true;
}

bool MainWindow::NewProject()
{
	if(!CloseProject())
		return false;

	char *Buffer;
	string sFileName;
	do
	{
		Buffer = fl_file_chooser("New Project?", "*.aprj", "");
		if(Buffer == NULL)
			return false;

		//Adjust the name to make sure it ends in .aprj
		sFileName = CreateStandardPath(Buffer);
		if(sFileName.substr(MIN(sFileName.size(), sFileName.find_last_of("."))) != ".aprj")
			sFileName += ".aprj";

		ifstream TestNew(sFileName.c_str());
		if(!TestNew.good())
			break;
		fl_alert("Error, project already exists: '%s'", sFileName.c_str());
	}
	while(true);

	{
		//Make the file
		ofstream Test(sFileName.c_str());
	}

	return OpenProject(sFileName);
}

bool MainWindow::OpenProject()
{
	if(!CloseProject())
		return false;

	char *Buffer;
	string sFileName;
	do
	{
		Buffer = fl_file_chooser("Open Project?", "*.aprj", "");
		if(Buffer == NULL)
			return false;

		//Adjust the name to make sure it ends in .aprj
		sFileName = CreateStandardPath(Buffer);
		if(sFileName.substr(MIN(sFileName.size(), sFileName.find_last_of("."))) != ".aprj")
			sFileName += ".aprj";

		ifstream TestNew(sFileName.c_str());
		if(TestNew.good())
			break;
		fl_alert("Error, project does not exist: '%s'", sFileName.c_str());
	}
	while(true);

	return OpenProject(sFileName);
}

bool MainWindow::OpenProject(const string &sFN)
{
	//Make sure the filename is absolute
	char Buffer[256];
	fl_filename_absolute(Buffer, 256, sFN.c_str());
	string sFileName = CreateStandardPath(Buffer);

	//Load the project
	Project::pProject = new Project("");
	bool fRetVal = TheProject.LoadSettings(sFileName);

	//Initialize the project display and make it visable
	pFilesWindow->show();
	pFilesWindow->UpdateList();
	pSettingsWindow->show();
	pSettingsWindow->InitProjectSettings();
	pMessageWindow->show();
	SetTitle();

	//Make the project menu items visable
	pMainMenu->mode(2, pMainMenu->mode(2) & ~FL_MENU_DIVIDER);
	pMainMenu->mode(3, pMainMenu->mode(3) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(6, pMainMenu->mode(6) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(7, pMainMenu->mode(7) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(8, pMainMenu->mode(8) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(11, pMainMenu->mode(11) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(17, pMainMenu->mode(17) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(26, pMainMenu->mode(26) & ~FL_MENU_INVISIBLE);
	//*NOTE: The message menu is not displayed until it is redrawn
	pMainMenu->redraw();

	return fRetVal;
}

bool MainWindow::SaveProject()
{
	if(TheProject.sFileName.Full == "")
		return SaveProjectAs();
	return TheProject.SaveSettings(TheProject.sFileName.Full);
}

bool MainWindow::SaveProjectAs()
{
	char *sNewFile = fl_file_chooser("Save Project As?", "*.aprj", TheProject.sFileName.Name.c_str());
	if(sNewFile == NULL)
		return false;

	TheProject.sFileName = CreateStandardPath(sNewFile);
	if(TheProject.sFileName.Ext != "aprj")
		TheProject.sFileName = TheProject.sFileName.Full + ".aprj";
	TheProject.fAutoProject = false;
	return SaveProject();
}


bool MainWindow::CloseProject()
{
	if(!Project::pProject)
		return true;
	if(TheProject.fSimulating)
	{
//		fl_message("You must terminate the simulator before closing the project.");
		TheSimulatorWindow.Stop();
		return false;
	}
	if(!CheckSaveProject())
		return false;
	delete Project::pProject;
	Project::pProject = NULL;

	//Make the project windows invisable
	pFilesWindow->hide();
	pSettingsWindow->hide();
	pMessageWindow->hide();
	pMessageWindow->Clear();

	//Make the project menu items invisable
	pMainMenu->mode(2, pMainMenu->mode(2) | FL_MENU_DIVIDER);
	pMainMenu->mode(3, pMainMenu->mode(3) | FL_MENU_INVISIBLE);
	pMainMenu->mode(6, pMainMenu->mode(6) | FL_MENU_INVISIBLE);
	pMainMenu->mode(7, pMainMenu->mode(7) | FL_MENU_INVISIBLE);
	pMainMenu->mode(8, pMainMenu->mode(8) | FL_MENU_INVISIBLE);
	pMainMenu->mode(11, pMainMenu->mode(11) | FL_MENU_INVISIBLE);
	pMainMenu->mode(17, pMainMenu->mode(17) | FL_MENU_INVISIBLE);
	pMainMenu->mode(26, pMainMenu->mode(26) | FL_MENU_INVISIBLE);
	pMainMenu->redraw();

	return true;
}

bool MainWindow::CheckSaveProject()
{
	//Don't harass about an unchanged project
	if(!TheProject.fChanged)
		return true;
	//Auto-projects are auto-saved
//	if(TheProject.fAutoProject)
		return SaveProject();

	int Choice = fl_choice("The current project has changes that have not been saved",
		"Cancel", "Save", "Discard");

	switch(Choice)
	{
	case 0:
		return false;
	case 1:
		return SaveProject();
	case 2:
		return true;
	}

	return true;
}

bool MainWindow::NewFile()
{
	char *Buffer;
	string sFileName;
	do
	{
		Buffer = fl_file_chooser("New File?", NULL, "");
		if(Buffer == NULL)
			return false;
		sFileName = CreateStandardPath(Buffer);
		ifstream TestNew(sFileName.c_str());
		if(!TestNew.good())
			break;
		fl_alert("Error, file already exists: '%s'", sFileName.c_str());
	}
	while(true);

	{
		//Make the file
		ofstream Test(sFileName.c_str());
	}

	return OpenFile(sFileName);
}

bool MainWindow::AddFile()
{
	char *Buffer;
	string sFileName;
	do
	{
		Buffer = fl_file_chooser("Add File?", NULL, "");
		if(Buffer == NULL)
			return false;
		sFileName = CreateStandardPath(Buffer);
		ifstream TestNew(sFileName.c_str());
		if(TestNew.good())
			break;
		fl_alert("Error, file does not exist: '%s'", sFileName.c_str());
	}
	while(true);

	return TheProject.AddFile(sFileName);
}

bool MainWindow::OpenFile()
{
	char *Buffer;
	string sFileName;
	do
	{
		Buffer = fl_file_chooser("Open File?", NULL, "");
		if(Buffer == NULL)
			return false;
		sFileName = CreateStandardPath(Buffer);
		ifstream TestNew(sFileName.c_str());
		if(TestNew.good())
			break;
		fl_alert("Error, file does not exist: '%s'", sFileName.c_str());
	}
	while(true);

	return OpenFile(sFileName.c_str());
}

bool MainWindow::OpenFile(const string &sFN)
{
	//Make sure the filename is absolute
	char Buffer[256];
	fl_filename_absolute(Buffer, 256, sFN.c_str());
	string sFileName = CreateStandardPath(Buffer);
	FileWindow *pFW;

	//Check to see if the window is already open
	if(pFW = FileWindow::IsOpen(sFileName))
	{
		pFW->show();
		return true;
	}

	//Check to see if a project is open yet
	if(!Project::pProject)
	{
		//A project does not yet exist. Create a default project.
		string sProjName = sFileName.substr(0, MIN(sFileName.size(), sFileName.find_last_of("."))) + ".aprj";
		{
			ifstream TestNew(sProjName.c_str());
			if(!TestNew.good())
			{
				TestNew.close();
				//Make the project
				ofstream Test(sProjName.c_str());
			}	//Else the project already exists
		}
		OpenProject(sProjName);
		//The project is always an auto project if opened this way, even if
		//a project file already existed.
		TheProject.fAutoProject = true;
	}

	//Check to see if the window is already open
	if(pFW = FileWindow::IsOpen(sFileName))
	{
		TheProject.AddWindow(sFileName, pFW);
		pFW->show();
	}
	else
	{
		LanguageEnum Language;
		Project::FileData *pFileData = TheProject.GetFile(sFileName);
		string sExt;
		if(pFileData)
		{
			Language = pFileData->GetLanguage();
			sExt = pFileData->sFileName.Ext;
		}
		else
		{
			Project::FileData FileData(sFileName);
			Language = FileData.GetLanguage();
			sExt = FileData.sFileName.Ext;
		}

		if(Language == LangLC3b)
			pFW = new LC3bFileWindow(FileWidth, FileHeight, sFileName);
		else if(Language == LangLC3)
			pFW = new LC3FileWindow(FileWidth, FileHeight, sFileName);
		else
			pFW = new FileWindow(FileWidth, FileHeight, sFileName);

		//See if they want to add the file to the project (unless they opened
		//a project settings file)
//		if(ToLower(sExt) != "aprj")
			pFW->AddToProject(!TheProject.fAutoProject);
	}

	return true;
}

bool MainWindow::SelectFile()
{
	if(TheProject.pFile)
	{
		pMainMenu->mode(18, pMainMenu->mode(18) & ~FL_MENU_INVISIBLE);
		pMainMenu->mode(19, pMainMenu->mode(19) & ~FL_MENU_INVISIBLE);
	}
	else
	{
		pMainMenu->mode(18, pMainMenu->mode(18) | FL_MENU_INVISIBLE);
		pMainMenu->mode(19, pMainMenu->mode(19) | FL_MENU_INVISIBLE);
	}
	pMainMenu->redraw();
	return pSettingsWindow->InitFileSettings();
}

bool MainWindow::Simulate()
{
	unsigned int i;

	pSettingsWindow->pAssembleFile->deactivate();
	pSettingsWindow->pAssemble->deactivate();
	pSettingsWindow->pBuild->deactivate();
	pSettingsWindow->pSimulate->deactivate();
	pMainMenu->mode(4, pMainMenu->mode(4) | FL_MENU_INVISIBLE);
	pMainMenu->mode(5, pMainMenu->mode(5) | FL_MENU_INVISIBLE);
	pMainMenu->mode(7, pMainMenu->mode(7) & ~FL_MENU_DIVIDER);
	pMainMenu->mode(8, pMainMenu->mode(8) | FL_MENU_INVISIBLE);
	pMainMenu->mode(9, pMainMenu->mode(9) | FL_MENU_INVISIBLE);
	pMainMenu->mode(17, pMainMenu->mode(17) | FL_MENU_INVISIBLE);
	pMainMenu->mode(31, pMainMenu->mode(31) | FL_MENU_DIVIDER);
	for(i = 32; i <= 35; i++)
		pMainMenu->mode(i, pMainMenu->mode(i) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(36, pMainMenu->mode(36) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(39, pMainMenu->mode(39) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(50, pMainMenu->mode(50) & ~FL_MENU_INVISIBLE);
	pMainMenu->redraw();

	TheProject.Simulate();

	pSettingsWindow->pAssembleFile->activate();
	pSettingsWindow->pAssemble->activate();
	pSettingsWindow->pBuild->activate();
	pSettingsWindow->pSimulate->activate();
	pMainMenu->mode(4, pMainMenu->mode(4) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(5, pMainMenu->mode(5) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(7, pMainMenu->mode(7) | FL_MENU_DIVIDER);
	pMainMenu->mode(8, pMainMenu->mode(8) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(9, pMainMenu->mode(9) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(17, pMainMenu->mode(17) & ~FL_MENU_INVISIBLE);
	pMainMenu->mode(31, pMainMenu->mode(31) & ~FL_MENU_DIVIDER);
	for(i = 32; i <= 35; i++)
		pMainMenu->mode(i, pMainMenu->mode(i) | FL_MENU_INVISIBLE);
	pMainMenu->mode(36, pMainMenu->mode(36) | FL_MENU_INVISIBLE);
	pMainMenu->mode(39, pMainMenu->mode(39) | FL_MENU_INVISIBLE);
	pMainMenu->mode(50, pMainMenu->mode(50) | FL_MENU_INVISIBLE);
	pMainMenu->redraw();

	return true;
}
bool MainWindow::RemoveFromProject()
{
	FileWindow *pFW;
	if(pFW = FileWindow::IsOpen(TheProject.pFile->sFileName.Full))
	{
		return pFW->RemoveFromProject(false);
	}
	else
	{
//		if(!fl_ask("Remove file '%s' from project '%s'?", TheProject.pFile->sFileName.Full.substr(TheProject.pFile->sFileName.Full.find_last_of("\\/:")+1).c_str(), TheProject.sFileName.Bare.c_str()))
//			return false;
		TheProject.RemoveFile(TheProject.pFile->sFileName.Full);
		return true;
	}
}

bool MainWindow::Help(HelpEnum HelpIndex, const string &sindex)
{
	string sIndex = ToLower(sindex);
	//first try to open the documentation file
	while(true)
	{
		if(sHelpLocation.Full == "")
		{
			char Buffer[256];
			fl_filename_absolute(Buffer, 256, (sProgramDir + "LC3ToolsHelp.htm").c_str());
			sHelpLocation = CreateStandardPath(Buffer);
		}
		ifstream HelpFile(sHelpLocation.Full.c_str());
		if(!HelpFile.good())
		{
			if(!fl_ask("The LC3Tools documentation was not found at the expected location.\nIn the next dialog, please select the new path to the documentation:"))
			{
				sHelpLocation = "";
				return false;
			}
			char *Buffer;
			Buffer = fl_file_chooser("Documentation location", "LC3ToolsHelp.htm", "LC3ToolsHelp.htm");
			if(Buffer == NULL)
			{
				sHelpLocation = "";
				return false;
			}
			sHelpLocation = CreateStandardPath(Buffer);
			continue;
		}
		if(Project::pProject)
		{
			TheProject.sHelpLocation = sHelpLocation;
			TheProject.fChanged = true;
		}
		break;
	}
	switch(HelpIndex)
	{
	case NoIndex:
		pHelpDlg->load(sHelpLocation.Full.c_str());
		break;
	case AssemblerIndex:
		pHelpDlg->load((sHelpLocation.Path+"AssemblerTutorial.htm#"+sIndex).c_str());
		break;
	case LC3InstructionIndex:
		pHelpDlg->load((sHelpLocation.Path+"LC3Instructions.htm#"+sIndex).c_str());
		break;
	case LC3bInstructionIndex:
		pHelpDlg->load((sHelpLocation.Path+"LC3bInstructions.htm#"+sIndex).c_str());
		break;
	case SimulatorIndex:
		pHelpDlg->load((sHelpLocation.Path+"SimulatorTutorial.htm#"+sIndex).c_str());
		break;
	case AshIDEIndex:
		pHelpDlg->load((sHelpLocation.Path+"AshIDEUsageGuide.htm#"+sIndex).c_str());
		break;
	}
	pHelpDlg->show();
	return true;
}

bool MainWindow::About()
{
	fl_message("AshIDE 1.14, Assembler %s, and Simulator %s programs and source code:\n(C) 2003 Ashley Wise\n\nLicensed for use by students in a course that uses the LC-3 or LC-3b ISAs\n\nFor the latest version and information, visit:\nhttp://www.crhc.uiuc.edu/~awise/LC3Tools.htm", ASM_VER, SIM_VER);
	return true;
}

}	//namespace AshIDE
