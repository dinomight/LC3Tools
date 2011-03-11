// JMTSys.h: Cross-platform macros for OS-specific functions
// Copyright 2002 Ashley Wise
// JediMasterThrash@comcast.net

#ifndef JMTSYS_H
#define JMTSYS_H

#include <stdexcept>
using namespace std;

/*============================================================================*\
 *	Macros for cross-platform system functions.
 *
 *	Includes Asynchronous I/O, memory nailing.
\*============================================================================*/

#if defined WIN32
	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0500
	#endif
	#include <windows.h>

	#define SYNCH_SLEEP(Interval)\
		{\
			if(Interval)\
				Sleep(Interval);\
			else\
				if(!SwitchToThread())\
					Sleep(1);\
		}

	#define ASYNC_TYPE HANDLE
	#define OVERLAP_TYPE OVERLAPPED

	#define ASYNC_FILE_OPEN(File, FileName, Overlapped, Event)\
		{\
			if(!(Event = CreateEvent(NULL, FALSE, FALSE, NULL)))\
				throw runtime_error("JMTSys: Failed to create Async Event");\
			if(!(File = CreateFile(FileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL)))\
				throw runtime_error("JMTSys: Failed to create Async File");\
			DWORD Size, SizeHigh;\
			Size = GetFileSize(File, &SizeHigh);\
			Overlapped.Internal = 0;\
			Overlapped.InternalHigh = 0;\
			Overlapped.Offset = Size;\
			Overlapped.OffsetHigh = SizeHigh;\
			Overlapped.hEvent = Event;\
		}

	#define ASYNC_IO_WRITE(File, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten, Overlapped, IOPending)\
		if(!(WriteFile(File, Buffer, NumberOfBytesToWrite, NULL, &Overlapped)))\
		{\
			if(GetLastError() == ERROR_IO_PENDING)\
				IOPending = true;\
		}\
		else\
		{\
			DWORD BytesWritten;\
			GetOverlappedResult(File, &Overlapped, &BytesWritten, FALSE);\
			NumberOfBytesWritten = BytesWritten;\
			Overlapped.Offset += BytesWritten;\
			IOPending = false;\
		}

	#define ASYNC_IO_WAIT(File, Event, NumberOfBytesWritten, Overlapped)\
		switch( WaitForSingleObject(Event, INFINITE) )\
		{\
		case WAIT_ABANDONED:\
		case WAIT_FAILED:\
		case WAIT_TIMEOUT:\
			throw runtime_error("JMTSys: Failed to wait for Async Event");\
		case WAIT_OBJECT_0:\
			DWORD BytesWritten;\
			GetOverlappedResult(File, &Overlapped, &BytesWritten, FALSE);\
			NumberOfBytesWritten = BytesWritten;\
			Overlapped.Offset += BytesWritten;\
			break;\
		}

	#define ASYNC_FILE_CLOSE(File, Event)\
		{\
			if(File)\
				CloseHandle(File);\
			if(Event)\
				CloseHandle(Event);\
		}

	#define NAIL_MEMORY(pointer, size)\
		{\
			if(!VirtualLock(pointer, size))\
				throw runtime_error("JMTSys: Failed to lock memory");\
		}
	#define UNNAIL_MEMORY(pointer, size)\
		{\
			if(!VirtualUnlock(pointer, size))\
				throw runtime_error("JMTSys: Failed to lock memory");\
		}
#elif defined UNIX_BUILD
	#include <unistd.h>
	#include <sys/mman.h>
	#include <sched.h>

	#define SYNCH_SLEEP(Interval)\
		{\
			if(Interval)\
				usleep(Interval * 1000);\
			else\
				if(sched_yield())\
					usleep(1000);\
		}

	#define ASYNC_TYPE unsigned long
	#define OVERLAP_TYPE unsigned long

	#define ASYNC_FILE_OPEN(File, FileName, Overlapped, Event)\
		{\
			throw runtime_error("JMTSys: ASYNCIO not yet implemented for Linux");\
		}

	#define ASYNC_IO_WRITE(File, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten, Overlapped, IOPending)\
		{\
			throw runtime_error("JMTSys: ASYNCIO not yet implemented for Linux");\
		}

	#define ASYNC_IO_WAIT(File, Event, NumberOfBytesWritten, Overlapped)\
		{\
			throw runtime_error("JMTSys: ASYNCIO not yet implemented for Linux");\
		}

	#define ASYNC_FILE_CLOSE(File, Event)\
		{\
			throw runtime_error("JMTSys: ASYNCIO not yet implemented for Linux");\
		}

	#define NAIL_MEMORY(pointer, size)\
		{\
			if(mlock((void *)pointer, size))\
				throw runtime_error("JMTSys: Failed to lock memory");\
		}
	#define UNNAIL_MEMORY(pointer, size)\
		{\
			if(munlock((void *)pointer, size))\
				throw runtime_error("JMTSys: Failed to lock memory");\
		}
#else
	#error "Only WIN32 and UNIX Systems Supported"
#endif

#endif	//JMTSYS_H
