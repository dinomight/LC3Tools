//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef SIMULATOR_H
#define SIMULATOR_H

#pragma warning (disable:4786)
#include <map>
#include <list>
#include <vector>
#include <fstream>
#include <string>
#include "Architecture.h"
#include "../Assembler/Program.h"
#include "../Assembler/Symbol.h"
#include "../Assembler/Number.h"
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;
using namespace Assembler;

namespace Simulator
{
	const unsigned int NUM_EVENTTYPES = 7;
	enum EventEnum {NoEvent = 0x0, ReadEvent = 0x1, WriteEvent = 0x2, ChangeEvent = 0x4, ValueEvent = 0x8, InstrEvent = 0x10, GotoEvent = 0x20};
	//*NOTE due to the .cpp being #included in the template header, it doesn't work to create this as extern.
	//So it's just declared multiple times in each header
//	extern const char *const sEventTypes[ValueEvent+1];
	const char *const sEventTypes[ValueEvent+1] = {"NOEVENT", "READEVENT", "WRITEEVENT", "", "CHANGEEVENT", "", "", "", "VALUEEVENT"};

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		ArchSim

		An instance of this class simulates a given architecture.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	template<class ISA>
	class ArchSim
	{
	public:
		//Buffer for formatting error messages
		char sMessageBuffer[65 + MAX_FILENAME_CHAR];
		Architecture *pArch;
		vector<Program *> *pPrograms;
		RamVector *pMemoryImage;

		/**********************************************************************\
			NullCallBack( [in] message type, [in] message,
				[in] locaton stack )
			
			Ignores the message.
		\******/
		static bool NullCallBack(MessageEnum, const string &, const LocationVector &);
		bool (*SimCommand)(string &);
		bool (*SimReadConsole)(string &, unsigned int, unsigned int &);
		bool (*SimWriteConsole)(const string &, unsigned int, unsigned int &);

		//Each Simulator class owns one bit of this flag. 64 simultaneous simulators
		//(possibly simulating a multi-processor system) and be paused
		//asynchronously.
		static uint64 fControlC;
		//This is the next ID for simulator instances. So far only used for the
		//pause flag (fControlC)
		static uint64 NextSimID;
		uint64 SimulatorID;
		//Store old signal handler
		void (*OldSig)(int);
//		friend class Architecture;
		//True if should break to the simulator UI before the next run
		bool fBreak;
		//True if this is this is the first time that SimCommand is being run for a given breakpoint
		//This way we don't redisplay the instruction everytime they hit enter.
		bool fFirstBreak;
		//True if the simulation is terminated
		bool fDone;
		//True if currently tracing execution
		bool fTrace;
		//True if checking for execution anomolies, such as PC out of range.
		bool fCheck;
		ofstream TraceFile;
		list<string> TraceRegisterSets;
		//current cycle
		uint64 SimCycle;
		//current instruction
		uint64 SimInstruction;
		//Callstack <file name, line number, address, label>
		typedef tetra<string, unsigned int, uint64, string> CallStackInfo;
		typedef list<CallStackInfo> CallStackList;
		CallStackList CallStack;
		//The number of instructions before and after the current instruction to print
		unsigned int PreInstructionCount, PostInstructionCount;

	//*** Data for breakpoints ***

		//These are all of the breakpoint event types
		//*NOTE: Template bug in MSVC doesn't find the implementation if done elsewhere
		struct EventInfo
		{
			EventEnum Events;
			list<Number *> ValueList;
			EventInfo()
			{
				Events = NoEvent;
			}
			EventInfo(EventEnum events, list<Number *> valuelist)
			{
				Events = events;
				ValueList = valuelist;
			}
			//Adds additional event info
			EventInfo &operator+=(EventInfo &OtherEventInfo)
			{
				Events = (EventEnum)(Events | OtherEventInfo.Events);

				for(list<Number *>::iterator ValueIter = OtherEventInfo.ValueList.begin(); ValueIter != OtherEventInfo.ValueList.end(); ValueIter++)
					ValueList.push_back(*ValueIter);
				OtherEventInfo.ValueList.clear();

				return *this;
			}
			//Prints the event info
			operator const char *()
			{
				static string sEvents;
				sEvents = "";
				if(Events == NoEvent)
					(sEvents += " ") += sEventTypes[NoEvent];
				if(Events & ReadEvent)
					(sEvents += " ") += sEventTypes[ReadEvent];
				if(Events & WriteEvent)
					(sEvents += " ") += sEventTypes[WriteEvent];
				if(Events & ChangeEvent)
					(sEvents += " ") += sEventTypes[ChangeEvent];
				if(Events & ValueEvent)
				{
					for(list<Number *>::iterator ValueIter = ValueList.begin(); ValueIter != ValueList.end(); ValueIter++)
						(((sEvents += " ") += sEventTypes[ValueEvent]) += " ") += (const char *)**ValueIter;
				}
				return sEvents.c_str();
			}
			~EventInfo()
			{
				for(list<Number *>::iterator ValueIter = ValueList.begin(); ValueIter != ValueList.end(); ValueIter++)
					delete *ValueIter;
			}
		};

		//	<True if line info exists, program number, line number>
		typedef triple<bool, unsigned int, unsigned int> LineInfo;
		//	<symbolic address of instruction, line of instruction, Permanent or temporary (goto) breakpoint>
		typedef triple<Number *, LineInfo, EventEnum> InstrInfo;
		//	<address of instruction, info>
		typedef map<uint64, InstrInfo> InstrMap;
		InstrMap InstrBreakpoints;

		//	<symbolic address of data, data type for value events, event info>
		typedef tetra<Number *, DataEnum, LineInfo, EventInfo *> DataInfo;
		//	<address of data, info>
		typedef map<uint64, DataInfo> DataMap;
		DataMap DataBreakpoints;

		//	<data type for value events, event info>
		typedef pair<DataEnum, EventInfo *> MemoryInfo;
		//	<address of memory location, event info>
		typedef map<uint64, MemoryInfo> MemoryAddrMap;
		//	<memory name, memory address map>
		typedef map<string, MemoryAddrMap> MemoryMap;
		MemoryMap MemoryBreakpoints;

		//	<register name, event info>
		typedef map<string, EventInfo *> RegisterMap;
		//	<register set name, register map>
		typedef map<string, RegisterMap> RegisterSetMap;
		RegisterSetMap RegisterBreakpoints;

		//Cycle breakpoint
		uint64 BreakCycle;
		//Instruction breakpoint
		uint64 BreakInstruction;
		//Subroutine in, over and out breakpoints
		bool fInBreakpoint, fOutBreakpoint, fOverBreakpoint;
		unsigned int DepthCount;

		/**********************************************************************\
			ControlC( [in] signal type )

			Called when Ctrl-C is pressed (used for pausing/breaking simulation)
		\******/
		static void ControlC(int sig);
	
	//*** Functions to process simulation commands ***

		//*NOTE: In a more interactive UI, it could be made so that the UI calls the
		// individual functions itself, bypassing the parsing and command syntax

		/**********************************************************************\
			Command( )

			This runs the command-line input capabilities of the simulator.
		\******/
		virtual bool Command();

		/**********************************************************************\
			DoCommand( [in] command string )

			Lexes, parses, and performs the command in the string.
		\******/
		virtual bool DoCommand(const string &);

		/**********************************************************************\
			ParseCommand( [in-out] start token, [in] end token )

			Parses the simulator command.
		\******/
		virtual bool ParseCommand(list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			ParseEventList( [in-out] start token, [in] end token )

			Parses an event list out of the stream of tokens.
		\******/
		virtual EventInfo *ParseEventList(list<Token *>::iterator &, const list<Token *>::iterator &);

		/**********************************************************************\
			ParseProgramNumber( [in-out] start token, [in] end token,
				[out] true if number specified, [out] program number )

			Parses a program number out of the stream of tokens.
		\******/
		virtual bool ParseProgramNumber(list<Token *>::iterator &, const list<Token *>::iterator &, bool &, unsigned int &);

		/**********************************************************************\
			ParseValue( [in-out] start token, [in] end token,
				[in] string identifying value for error messages,
				[out] program number )

			Parses a value out of the stream of tokens. If the value is a
			symbol, it picks which program to resolve the symbol, and verfies
			the symbol and any attributes. Returns NULL if errors.
			Returns the number of the program used.
		\******/
		virtual Number *ParseValue(list<Token *>::iterator &, const list<Token *>::iterator &, const string &, unsigned int &);

		/**********************************************************************\
			ParseElements( [in-out] start token, [in] end token,
				[in] string identifying value for error messages,
				[out] program number, [out] memory image,
				[in] starting address )

			Parses elements out of a stream of tokens. Returns the
			memory image those elements build. Only ISA and Data elements
			allowed.

			If starting address is not provided, it parses the first token
			as the address.
		\******/
		virtual bool ParseElements(list<Token *>::iterator &, const list<Token *>::iterator &, const string &, unsigned int &, RamVector &, Number *pNumber = NULL);

		/**********************************************************************\
			LineToAddress( [in] true if program number specified,
				[in-out] program number, [in-out] line number,
				[in] string identifying value for error messages )

			Converts a program number (optional) and line number into an
			address. If the program number is not specified, it guesses the
			program number.

			If the line number does not point to an element, it will be
			moved to the next available element, and the line number is updated
			to the new one.
		\******/
		virtual Number *LineToAddress(bool, unsigned int &, unsigned int &, const string &);

		/**********************************************************************\
			AddressToElement( [in] address, [in] true if inside element
				[in] true if find label element )

			Find the element that corresponds to the given address.
			If the second parameter is true, then it will allow an element
			where the address points to the inside of the element, but not to the
			start of the element.
			If the third parameter is true, then it will allow a label
			element, if found. It will only return labels that point to data
			or instructions, not to structs or segments. Otherwise it will only
			return an instruction or data element.
		\******/
		virtual Element *AddressToElement(uint64, bool fInside = true, bool fLabel = false);

		/**********************************************************************\
			PrintInstruction( [in] output stream, [in] element to print,
				[in] address, [in] true if print file info,
				[in] true if also print the element a label points to,
				[in] true if also print binary value of instruction )

			Prints the instruction at the given address to the provided output
			stream. Format:
			filename(linenumber):\t4xADDR:\tLabel:\tInstruction
			If fPrintFile is true, then the filename and line number will be
			printed.

			If the element is provided, it prints that element. If the element
			is null, then it does not print file location and label information.
			Element should be data, instruction, or label that points to data
			or instruction only. It should not be a struct or label that points
			to a struct.
		\******/
		virtual bool PrintInstruction(ostream &, Element *, uint64, bool fPrintFile = true, bool fPrintLabelElement = true, bool fPrintValue = false);

		/**********************************************************************\
			SetConsoleWidth( [in] console width )

			Sets the console width to a specified number of characters.
			Only used for word wrap when printing to console, and for
			printing memory image byte data.
		\******/
		virtual bool SetConsoleWidth(unsigned int);

		/**********************************************************************\
			SetPrintI( [in] pre-instruction print count,
				[in] post-instruction print count )

			Sets the pre- and post-instruction print counts.
			When the current instruction is printed, this number of
			additional adjacent instructions will also be printed.
		\******/
		virtual bool SetPrintI(unsigned int, unsigned int);

		/**********************************************************************\
			Reset( [in-out] architecture, [in-out] program,
				[in-out] memory image)

			This initializes the specified architecture using the specified
			program and associated memory image.
		\******/
		virtual bool Reset(Architecture &, vector<Program *> &, RamVector &);	

		/**********************************************************************\
			TraceOn( [in] file name, [in] trace register set list )

			Opens the given file and sets it up as a trace file.
			Every instruction along with a snapshot of the specified register
			sets will be traced to the file.
		\******/
		virtual bool TraceOn(string, list<string> &);

		/**********************************************************************\
			TraceOff( )

			Stops tracing and closes the trace file.
		\******/
		virtual bool TraceOff();

		/**********************************************************************\
			CheckOn( )

			Turns on runtime checking of dynamic program execution.
		\******/
		virtual bool CheckOn();

		/**********************************************************************\
			CheckOff( )

			Disables runtime checking of dynamic program execution.
		\******/
		virtual bool CheckOff();

		/**********************************************************************\
			SaveState( [in] file name )

			Opens the given file and writes the simulator state to it.
			This includes writing all rams and register sets, as well as
			any architecture state not already in the registers, and any
			required simulator state. Each saved component is written to a
			different file using the component name in the extension.
		\******/
		virtual bool SaveState(string);

		/**********************************************************************\
			LoadState( [in] file name )

			Opens the given file and reads the simulator state from it.
			This includes loading all rams and register sets, as well as
			any architecture state not already in the registers, and any
			required simulator state. Each saved component is read from a
			different file using the component name in the extension.

			If load state returns false (failure), it is highly recommended
			that the simulator be reset, since the state of the simulator
			could be undefined.
		\******/
		virtual bool LoadState(string);

		/**********************************************************************\
			SaveData( [in] file name, [in] address, [in] length,
				[in] datatype, [in] true if opposite endian-ness )

			Opens the given file and writes a data image to it.
			The data image begins at the given address and is of the given
			length number of data elements.
			
			The datatype specifies how to interpret the data in this
			image, and if the data should be written in the opposite
			endian-ness, then the final bool is true. If datatype is STRUCT,
			then the data is interpreted as ISA data.

			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool SaveData(string, Number *, Number *, DataEnum, bool);

		/**********************************************************************\
			LoadData( [in] file name, [in] address, [in] length,
				[in] datatype, [in] true if opposite endian )

			Opens the given file and reads a data image from it.
			The data image begins at the given address and is of the given
			length number of data elements.
			
			The datatype specifies how to interpret the data in this
			image, and if the data should be loaded in the opposite
			endian-ness, then the final bool is true. If datatype is STRUCT,
			then the data is interpreted as ISA data.
			
			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool LoadData(string, Number *, Number *, DataEnum, bool);

		/**********************************************************************\
			SaveObject( [in] file name, [in] address, [in] length )

			Opens the given file and writes a program object to it.
			The program object begins at the given address and is of the given
			length number of data elements.
			
			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool SaveObject(string, Number *, Number *);

		/**********************************************************************\
			LoadData( [in] file name, [in] address, [in] length,
				[in] datatype, [in] true if opposite endian )

			Opens the given file and reads a data image from it.
			The data image begins at the given address and is of the given
			length number of data elements.
			
			The datatype specifies how to interpret the data in this
			image, and if the data should be loaded in the opposite
			endian-ness, then the final bool is true. If datatype is STRUCT,
			then the data is interpreted as ISA data.
			
			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool LoadObject(string);

		/**********************************************************************\
			Go( )

			Turns off the break. The simulator will break again at the next
			breakpoint.

			Cycle count can be NULL.
		\******/
		virtual bool Go();

		/**********************************************************************\
			Go( [in] cycle count )

			Turns off the break. The simulator will break again at the next
			breakpoint or when cycle count is reached.
		\******/
		virtual bool Go(Number *);

		/**********************************************************************\
			Go( [in] instruction count )

			Turns off the break. The simulator will break again at the next
			breakpoint or when instruction count is reached.
		\******/
		virtual bool GoI(Number *);

		/**********************************************************************\
			GoIn( )

			Turns off the break. The simulator will break again at the next
			breakpoint or when the program enters a subroutine.
		\******/
		virtual bool GoIn();

		/**********************************************************************\
			GoOver( )

			Turns off the break. The simulator will break again at the next
			breakpoint or when the program enters and then exits a subroutine.
		\******/
		virtual bool GoOver();

		/**********************************************************************\
			GoOut( )

			Turns off the break. The simulator will break again at the next
			breakpoint or when the program exits the current subroutine.
		\******/
		virtual bool GoOut();

		/**********************************************************************\
			GotoL( [in] true if program number specified, [in] program number,
				[in] line number )

			Turns off the break. The simulator will break again at the next
			breakpoint or when the instruction at the specified line is reached.
		\******/
		virtual bool GotoL(bool, unsigned int, unsigned int);

		/**********************************************************************\
			GotoI( )

			Turns off the break. The simulator will break again at the next
			breakpoint or when the specified instruction is reached.
		\******/
		virtual bool GotoI(Number *, unsigned int);

		/**********************************************************************\
			BreakpointLine( [in] true if program number specified,
				[in] program number, [in] line number, [in] event info )

			Sets a breakpoint for the specified line number.
		\******/
		virtual bool BreakpointLine(bool, unsigned int, unsigned int, EventEnum);

		/**********************************************************************\
			BreakpointInstruction( [in] instruction address,
				[in] true if has line info, [in] program number,
				[in] line number, [in] event info )

			Sets a breakpoint for the given instruction location.
			This is only a "PC-at" breakpoint.
			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool BreakpointInstruction(Number *, bool, unsigned int, unsigned int, EventEnum);

		/**********************************************************************\
			BreakpointInstructionClear( )

			Clears all instruction breakpoints.
		\******/
		virtual bool BreakpointInstructionClear();

		/**********************************************************************\
			BreakpointData( [in] data address, [in] data type,
				[in] event info )

			Sets a breakpoint for the given data location.
			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool BreakpointData(Number *, DataEnum, EventInfo *);

		/**********************************************************************\
			BreakpointDataClear( )

			Clears all data breakpoints.
		\******/
		virtual bool BreakpointDataClear();

		/**********************************************************************\
			BreakpointMemory( [in] memory name, [in] memory address,
				[in] data type, [in] event info )

			Sets a breakpoint for the given memory location.
		\******/
		virtual bool BreakpointMemory(const string &, Number *, DataEnum, EventInfo *);

		/**********************************************************************\
			BreakpointMemoryClear( )

			Clears all memory breakpoints.
		\******/
		virtual bool BreakpointMemoryClear();

		/**********************************************************************\
			BreakpointRegister( [in] register set name, [in] register name,
				[in] event info )

			Sets a breakpoint for the given register.
		\******/
		virtual bool BreakpointRegister(const string &, const string &, EventInfo *);

		/**********************************************************************\
			BreakpointRegisterClear( )

			Clears all register breakpoints.
		\******/
		virtual bool BreakpointRegisterClear();

		/**********************************************************************\
			DisplayHelp( [in] command name )

			Displays help for the specified command, or displays all commands
			if the command name is invalid.
		\******/
		virtual bool DisplayHelp(string);

		/**********************************************************************\
			DisplayInstructionLine( [in] true if program number specified,
				[in] program number, [in] line number, [in] display length )

			Displays the disassembled instruction at the specified line
			number for length number of instructions.
			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool DisplayInstructionLine(bool, unsigned int, unsigned int, Number *);

		/**********************************************************************\
			DisplayInstruction( [in] instruction address, [in] display length )

			Displays the disassembled instruction at the specified address
			for length number of instructions.
			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool DisplayInstruction(Number *, Number *);

		/**********************************************************************\
			DisplayLine( [in] true if program number specified,
				[in] program number, [in] line number, [in] display length,
				[in] data type, [in] true if signed datatype )

			Displays data beginning at the specified line number for
			length number of data elements. If datatype is STRUCT, they
			are displayed as they were declared in the program.
			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool DisplayLine(bool, unsigned int, unsigned int, Number *, DataEnum, bool);

		/**********************************************************************\
			DisplayLineArray( [in] true if program number specified,
				[in] line number, [in] display length )

			Displays the data like the MSVC memory window.
			Parameter must be an line number.
			Length is in bytes.
		\******/
		virtual bool DisplayLineArray(bool, unsigned int, unsigned int, Number *);

		/**********************************************************************\
			DisplayData( [in] data address, [in] display length, [in] data type,
				[in] true if signed datatype )

			Displays data beginning at the specified address for
			length number of data elements. If datatype is STRUCT, they
			are displayed as they were declared in the program.
			Parameter must be an ISA address (which might not be a byte address)
		\******/
		virtual bool DisplayData(Number *, Number *, DataEnum, bool);

		/**********************************************************************\
			DisplayDataArray( [in] data address, [in] display length )

			Displays the data like the MSVC memory window.
			Parameter must be an ISA address (which might not be a byte address).
			Length is in bytes.
		\******/
		virtual bool DisplayDataArray(Number *, Number *);

		/**********************************************************************\
			DisplayMemory( )

			Displays the names of all the memories.
		\******/
		virtual bool DisplayMemory();

		/**********************************************************************\
			DisplayMemory( [in] memory name, [in] starting address,
				[in] display length, [in] data type,
				[in] true if signed datatype )

			Displays memory at the starting address for the specified length
			number of data elements. If datatype is STRUCT, they are displayed
			as ISA words.
		\******/
		virtual bool DisplayMemory(const string &, Number *, Number *, DataEnum, bool);

		/**********************************************************************\
			DisplayMemoryArray( [in] memory name, [in] starting address,
				[in] display length, [in] data type,
				[in] true if signed datatype )

			Displays the memory like the MSVC memory window.
		\******/
		virtual bool DisplayMemoryArray(const string &, Number *, Number *);

		/**********************************************************************\
			RegisterSetDisplay( )

			Displays the names of all the register sets and the names of
			all the registers in the sets.
		\******/
		virtual bool DisplayRegisterSet();

		/**********************************************************************\
			RegisterSetDisplay( [in] register set name )

			Displays the contents of all the registers in a set.
		\******/
		virtual bool DisplayRegisterSet(const string &);

		/**********************************************************************\
			DisplayRegister( [in] register set name, [in] register name )

			Displays the contents of the specified register.
		\******/
		virtual bool DisplayRegister(const string &, const string &);

		/**********************************************************************\
			DisplayPipelines( )

			Displays the names of all the pipelines and their pipeline stages.
		\******/
		virtual bool DisplayPipelines();

		/**********************************************************************\
			DisplayCycleInstruction( )

			Displays the current cycle and instruction number.
		\******/
		virtual bool DisplayCycleInstruction();

		/**********************************************************************\
			DisplayCallStack( )

			Displays the contents of the callstack.
		\******/
		virtual bool DisplayCallStack();

		/**********************************************************************\
			DisplayPrograms( )

			Displays the filenames and program numbers of all the programs
			that built into the memory image currently being simulated.
		\******/
		virtual bool DisplayPrograms();

		/**********************************************************************\
			DisplayBreakpoints( )

			Displays all breakpoints.
		\******/
		virtual bool DisplayBreakpoints();

		/**********************************************************************\
			WriteData( [in] memory image )

			Performs a write to program memory by the user interface.
		\******/
		virtual bool WriteData(const RamVector &);

		/**********************************************************************\
			WriteMemory( [in] memory name, [in] memory image )

			Performs a write to memory by the user interface.
		\******/
		virtual bool WriteMemory(const string &, const RamVector &);

		/**********************************************************************\
			WriteRegister( [in] register set name, [in] register name,
				[in] number )

			Performs a write to a register by the user interface.
		\******/
		virtual bool WriteRegister(const string &, const string &, Number *);

		/**********************************************************************\
			Interrupt( [in] interrupt number )

			Signals an external interrupt to the processor.
		\******/
		virtual bool Interrupt(Number *);

	public:
		/**********************************************************************\
			ArchSim( [in] assembler message callback,
				[in] simulator message callback,
				[in] simulator command callback, [in] read console function,
				[in] write console function)

			LocationStacks given to the assembler message callback will be
			empty.
		\******/
		ArchSim(CallBackFunction, SimCallBackFunction, SimCommandFunction, SimReadConsoleFunction, SimWriteConsoleFunction);

		/**********************************************************************\
			CommandRun( [in-out] architecture, [in-out] program,
				[in-out] memory image)

			This runs the simulator on the specified architecture, using the
			specified program and associated memory image.
			The simulator begins in command-line mode. It remains in command-
			line mode until a command is given to run the architecture.
			The architecture runs until a breakpoint occurs
		\******/
		virtual bool CommandRun(Architecture &, vector<Program *> &, RamVector &);	

		/**********************************************************************\
			Run( )

			This runs the simulator on the specified architecture until
			a break condition is encountered.

			The simulator MUST have been initialized via a call to Reset()
			first.
		\******/
		virtual bool Run();	

		//Destructor
		~ArchSim();

		//The sim callback is accessible to the architecture
		bool (*CallBack)(MessageEnum, const string &, const LocationVector &);
		bool (*SimCallBack)(MessageEnum, const string &);

	//*** Functions called by the architecture during simulation ***

		/**********************************************************************\
			SubInEvent( [in] address of calling instruction,
				[in] address of subroutine )

			Registers a jump into a subroutine.
		\******/
		bool SubInEvent(uint64, uint64);

		/**********************************************************************\
			SubOutEvent( )

			Registers a jump out of a subroutine.
		\******/
		bool SubOutEvent();

		/**********************************************************************\
			InstructionEvent( [in] memory address )

			Checks to see if a breakpoint is waiting on this instruction event.
			Parameter must be a byte address.
		\******/
		bool InstructionEvent(uint64);

		/**********************************************************************\
			DataEvent( [in] memory address, [in] event type )

			Checks to see if a breakpoint is waiting on this data event.
			Parameter must be a byte address.
		\******/
		bool DataEvent(uint64, EventEnum);

		/**********************************************************************\
			MemoryEvent( [in] the memory, [in] memory address,
				[in] event type )

			Checks to see if a breakpoint is waiting on this memory event.
		\******/
		bool MemoryEvent(const Memory &, uint64, EventEnum);

		/**********************************************************************\
			RegisterEvent( [in] register set name, [in] the register,
				[in] event type )

			Checks to see if a breakpoint is waiting on this register event.
		\******/
		bool RegisterEvent(const string &, const Register &, EventEnum);

		/**********************************************************************\
			ReadConsole( [out] input buffer, [in] characters to read,
				[out] characters read)

			Reads the number of characters from the console buffer, places it
			in the input buffer, returns the number of characters actually read.
			Removes the input characters from the console buffer.
		\******/
		bool ReadConsole(string &, unsigned int, unsigned int &);

		/**********************************************************************\
			WriteConsole( [in] output buffer, [in] characters to write,
				[out] characters written)

			Writes the number of characters from the output buffer, places it
			in the console buffer, returns the number of characters actually
			written.
		\******/
		bool WriteConsole(const string &, unsigned int, unsigned int &);

		/**********************************************************************\
			Exception( )

			Breaks simulation due to a processor exception.
			The architecture should have printed a message explaining why.
		\******/
		bool Exception();

		//Additional I/O functions can be added for other things such as:
		//mouse input, keyboard input, etc.
		//Other things such as file I/O, graphics, etc, can be done by the
		//architecture itself.
	};

}

#include "Simulator.cpp"

#endif
