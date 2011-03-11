//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef PROJECT_H
#define PROJECT_H

#pragma warning (disable:4786)
#include <string>
#include <map>
#include <vector>
#include "../Assembler/Program.h"
#include "../Assembler/Base.h"
#include "../Simulator/Simulator.h"
#include "../LC3Assembler/LC3ISA.h"
#include "../LC3bAssembler/LC3bISA.h"
#include "../LC3Simulator/LC3Arch.h"
#include "../LC3bSimulator/LC3bArch.h"

using namespace std;
using namespace JMT;
using namespace Assembler;
using namespace Simulator;
using namespace LC3;
using namespace LC3b;

namespace AshIDE
{
	class FileWindow;

	const unsigned int NUM_SOURCE_TYPES = 6;
	const unsigned int NUM_PROJECT_SETTINGS = 9;
	const unsigned int NUM_PROJECT_FILE_SETTINGS = 1;

	class Project
	{
	public:
		//Different types of files in the project
		enum SourceEnum {LC3Source = 0, LC3Header, LC3bSource, LC3bHeader, Resources, Dependencies};
		//True if they are expanded
		bool fExpanded[NUM_SOURCE_TYPES];
		static const char *const sSources[NUM_SOURCE_TYPES];

		//Different types of settings in the project
		enum SettingEnum {PrintTokens = 0, PrintAST, PrintSymbols, OutputImage, OutputVHDL, UseOptimizations, UseOS, OldLC3, DisReg};
		bool fSettings[NUM_PROJECT_SETTINGS];
		static const char *const sSettings[NUM_PROJECT_SETTINGS];
		/*	fAutoProject,	//True if this project is created by default and does not have to be saved.
			fPrintAST,		//True if the AST should be output after compilation.
			fPrintTokens,	//True if the tokens should be output after compilation
			fPrintSymbols,	//True if symbols should be output after compilation.
			fOutputImage,	//True if the binary memory image should be output
			fOutputVHDL,	//True if VHDL vectors should be output
			fUseOptimizations,	//True if optimizations should be used
			fUseOS;			//True if Ash Operating System is automatically included
			fOldLC3;		//True if LC3 files should be run through the OldLC3 translation
			fDisReg;		//True is the disassembly and register windows are opened automatically*/
		bool fAutoProject,	//True if this project is created by default and does not have to be saved.
			fChanged,	//True if there have been changes to the project since the last save.
			fRebuild;	//True if there have been changes to the project since the last assemble/build
		//Holds pairs of <identifier name, token string> for global defines
		typedef list< pair<string, string> > DefineList;
		DefineList Defines;
		//Relative path to the documentation and OS
		FileName sHelpLocation, sOSLocation[NUM_LANGUAGES];
		//The number of original sources for the build. Cannot use InputLists.size() because dependencies
		//are added to the list during assembly.
		unsigned int NumSources[NUM_LANGUAGES];
		//A list of all the input files to a build, including original sources and dependencies
		vector<string> InputLists[NUM_LANGUAGES];
		//A list of all the program objects generated for the original sources
		vector<Program *> Programs[NUM_LANGUAGES];
		//The resultant memory image from a build
		RamVector *pMemoryImages[NUM_LANGUAGES];

		//Simulator data
		bool fSimulating;	//True if simulation is currently happening
		LanguageEnum SimISA;	//The language of the project that is currently being simulated
		ArchSim<LC3ISA> *pLC3Sim;
		ArchSim<LC3bISA> *pLC3bSim;
		LC3Arch *pLC3Arch;
		LC3bArch *pLC3bArch;

		//Quick macros to access the simulator, architecture, and command interpreter
		#define TheSim(Params)\
			(TheProject.SimISA == LangLC3 ? TheProject.pLC3Sim->Params : TheProject.pLC3bSim->Params)
		#define TheArch(Params)\
			(TheProject.SimISA == LangLC3 ? TheProject.pLC3Arch->Params : TheProject.pLC3bArch->Params)
		#define SIM_COMMAND(Params) TheSimulatorWindow.Command(Params)
		#define SIM_COMMAND_UPDATE(Params) TheSimulatorWindow.Command(Params, true)

		//This is a pointer to the currently opened project.
		//Only one project can be opened/loaded at a time.
		#define TheProject	(*Project::pProject)
		static Project *pProject;

	//*** Project Data ***//
		struct FileData
		{
			/******************************************************************\
				FileData( [in] full-path filename )

				Creates a new filedata.
			\******/
			FileData(const string &);
			~FileData();

			FileName sFileName;
			FileWindow *pWindow;
			bool fChanged;	//indicates a new version of the file has been saved since it was last assembled
			unsigned int ProgramNumber;
			LanguageEnum Language;
			SourceEnum SourceType;
			//Different types of settings in the files in the project
			enum FileSettingEnum {ExcludeFromBuild = 0};
			bool fFileSettings[NUM_PROJECT_FILE_SETTINGS];
			static const char *const sFileSettings[NUM_PROJECT_FILE_SETTINGS];
			bool SetSetting(FileSettingEnum, bool);
			bool GetSetting(FileSettingEnum) const;
			LanguageEnum SetLanguage(LanguageEnum);
			LanguageEnum GetLanguage() const;
			SourceEnum SetSourceType(SourceEnum);
			SourceEnum GetSourceType() const;
			bool IsSource() const;
			bool IsBuild() const;

			Program *pProgram;
		};

		//Maps absolute filename to file data
		typedef map<string, FileData> FileMap;
		typedef FileMap::iterator FileMapIter;
		FileMap Files;
		//This is a pointer to the currently active/selected file within the project.
		//This pointer can be NULL at any given time, indicating no current selection.
		FileData *pFile;
		string sWorkingDir;
		FileName sFileName;
		unsigned int Warnings, Errors;


	//*** Project Creation Functions ***//
		/**********************************************************************\
			Project( [in] working directory name )

			Creates a new project. The project is by default an auto-project,
			where the settings are not saved. This allows a user who just wants
			simple asm programming to not have to be bothered with prompts
			to save the project settings or name a project.
		\******/
		Project(const string &);

		/**********************************************************************\
			~Project( )

			Destructor.
		\******/
		~Project();

		/**********************************************************************\
			LoadSettings( [in] project settings file name )

			Loads project settings from a file. All current settings are
			overridden. You cannot call LoadSettings more than once on the same
			project instance. You cannot call LoadSettings on an auto-project
			once changes have been made. The only valid usage is to call
			LoadSettings immediately after constructing an auto-proejct.

			The working directory is changed to be the directory of the project
			file.
		\******/
		bool LoadSettings(const string &);

		/**********************************************************************\
			SaveSettings( [in] project settings file name )

			Saves project settings from a file. If SaveSettings is called
			on an auto-project, the project is no longer auto.
			The working directory is changed to be the directory of the project
			file.
		\******/
		bool SaveSettings(const string &);


		
	//*** Project Management Functions ***//
		/**********************************************************************\
			AddFile( [in] File name )
			AddFile( [in] File data )

			Add a file to the project. If the file is currently open,
			it adds the window to the file data.
		\******/
		bool AddFile(const string &);
		bool AddFile(const FileData &);

		/**********************************************************************\
			RemoveFile( [in] File name )

			Remoevs a file from the project. Does nothing if the file is not
			in the project.
		\******/
		bool RemoveFile(const string &);

		/**********************************************************************\
			GetFile( [in] file name )

			Get the data for a file in the project.
			Returns NULL if the file is not in the project.
		\******/
		FileData *GetFile(const string &);

		/**********************************************************************\
			ListFiles( [out] file name list )

			Get the names of all the files in the project.
			Returns a count of how many files are in the project
		\******/
		unsigned int ListFiles(list<string> &);

		/**********************************************************************\
			AddWindow( [in] file name, [in] window )

			Associate a window with a file in the project.
		\******/
		bool AddWindow(const string &, FileWindow *);

		/**********************************************************************\
			RemoveWindow( [in] file name )

			Remove a window associated from a file.
		\******/
		bool RemoveWindow(const string &);

		/**********************************************************************\
			SetExpanded( [in] source type, [in] true or false )

			Sets the expanded state of the specified source type.
			This refers to the visual file window that displays this project.
			Returns the new expanded state.
		\******/
		bool SetExpanded(SourceEnum, bool);

		/**********************************************************************\
			GetExpanded( [in] source type )

			Gets the expanded state of the specified source type..
		\******/
		bool GetExpanded(SourceEnum);

		/**********************************************************************\
			SetSetting( [in] source type, [in] true or false )

			Sets the specified setting to enabled/disabled.
			Returns the new setting state.
		\******/
		bool SetSetting(SettingEnum, bool);

		/**********************************************************************\
			GetSetting( [in] source type )

			Gets the specified setting state (enabled/disabled).
		\******/
		bool GetSetting(SettingEnum);

		/**********************************************************************\
			AddDefine( [in] identifier, [in] value )

			Adds a new global define to the project.
			Returns false if the identifier was already defined and was
			replaced.
		\******/
		bool AddDefine(const string &, const string &);

		/**********************************************************************\
			RemoveDefine( [in] identifier )
			RemoveDefine( [in] define # )

			Removes a global define from the project.
			The first overload uses the identifier.
			The second overload uses an index into the list.
			Returns true if it found and removed the define.
		\******/
		bool RemoveDefine(const string &);
		bool RemoveDefine(unsigned int);

		/**********************************************************************\
			SelectFile( [in] file data )

			Sets the currently selected file. NULL selects no file.
		\******/
		bool SelectFile(FileData *);


		
	//*** Project Build Functions ***//

		/**********************************************************************\
			Assemble( )
			Assemble( )

			Assembles the entire project.
		\******/
		bool Assemble();

		/**********************************************************************\
			AssembleFile( [in] file data )
			AssembleFile( [in] file name )

			Assembles the specified file.
		\******/
		bool AssembleFile(FileData *);
		bool AssembleFile(const string &);

		/**********************************************************************\
			Build( )
			Build( )

			Builds the entire project.
		\******/
		bool Build();

		/**********************************************************************\
			Simulate( )
			Simulate( )

			Simulates the project.
		\******/
		bool Simulate();

		/**********************************************************************\
			ResetAssembler( )

			Resets the assembler settings. Deletes all pre-build programs
			and resets the files' program numbers.
		\******/
		bool ResetAssembler();

		/**********************************************************************\
			IsSimulating( [in] file name )

			Sees if the file is one of the files with code that is currently
			being simulated.
		\******/
		bool IsSimulating(const string &) const;

	protected:
		/**********************************************************************\
			PrintDefine( [in] define value, [out] expanded value )

			Prints the quoted version of the string with all non-quotable
			characters represented as escape sequences.
		\******/
		void PrintDefine(const string &, string &);

		/**********************************************************************\
			AddOS( )
			RemoveOS( )

			Adds or removes the OS files from the project.
		\******/
		bool AddOS();
		bool RemoveOS();

		/**********************************************************************\
			AssembleX( [in] file data )

			Assembles the file presuming the assembler settings are already
			set up.
		\******/
		bool AssembleLC3(FileData *);
		bool AssembleLC3b(FileData *);

		/**********************************************************************\
			BuildX( )

			Builds the project into a memory image.
		\******/
		bool BuildLC3();
		bool BuildLC3b();
	};
}

#endif
