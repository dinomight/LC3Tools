# Microsoft Developer Studio Project File - Name="AshIDE" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=AshIDE - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "AshIDE.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AshIDE.mak" CFG="AshIDE - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AshIDE - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "AshIDE - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "AshIDE - Win32 LC3 Release" (based on "Win32 (x86) Application")
!MESSAGE "AshIDE - Win32 LC3b Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "AshIDE - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /Zm200 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ../fltk-1.1.6/lib/Windows_x86/fltk.lib ../fltk-1.1.6/lib/Windows_x86/fltkimages.lib wsock32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc"
# SUBTRACT LINK32 /debug
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying EXE...
PostBuild_Cmds=copy Release\*.exe ..\Release\Windows_x86\.
# End Special Build Tool

!ELSEIF  "$(CFG)" == "AshIDE - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /Zm200 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../fltk-1.1.6/lib/Windows_x86/fltkd.lib ../fltk-1.1.6/lib/Windows_x86/fltkimagesd.lib wsock32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "AshIDE___Win32_LC3_Release"
# PROP BASE Intermediate_Dir "AshIDE___Win32_LC3_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "AshIDE___Win32_LC3_Release"
# PROP Intermediate_Dir "AshIDE___Win32_LC3_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /Zm200 /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "DEFAULT_LANG_LC3" /YX /FD /Zm200 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ../fltk-1.1.5/lib/Windows_x86/fltk.lib ../fltk-1.1.5/lib/Windows_x86/fltkimages.lib wsock32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 ../fltk-1.1.6/lib/Windows_x86/fltk.lib ../fltk-1.1.6/lib/Windows_x86/fltkimages.lib wsock32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc" /out:"AshIDE___Win32_LC3_Release/LC3IDE.exe"
# SUBTRACT LINK32 /debug
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying EXE...
PostBuild_Cmds=copy AshIDE___Win32_LC3_Release\*.exe ..\Release\Windows_x86\.
# End Special Build Tool

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3b Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "AshIDE___Win32_LC3b_Release"
# PROP BASE Intermediate_Dir "AshIDE___Win32_LC3b_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "AshIDE___Win32_LC3b_Release"
# PROP Intermediate_Dir "AshIDE___Win32_LC3b_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "DEFAULT_LANG_LC3" /YX /FD /Zm200 /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "DEFAULT_LANG_LC3B" /YX /FD /Zm200 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ../fltk-1.1.5/lib/Windows_x86/fltk.lib ../fltk-1.1.5/lib/Windows_x86/fltkimages.lib wsock32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc" /out:"AshIDE___Win32_LC3_Release/LC3IDE.exe"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 ../fltk-1.1.6/lib/Windows_x86/fltk.lib ../fltk-1.1.6/lib/Windows_x86/fltkimages.lib wsock32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc" /out:"AshIDE___Win32_LC3b_Release/LC3bIDE.exe"
# SUBTRACT LINK32 /debug
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying EXE...
PostBuild_Cmds=copy AshIDE___Win32_LC3b_Release\*.exe ..\Release\Windows_x86\.
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "AshIDE - Win32 Release"
# Name "AshIDE - Win32 Debug"
# Name "AshIDE - Win32 LC3 Release"
# Name "AshIDE - Win32 LC3b Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BreakpointWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\CallStackWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\ConsoleWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\DataValuesWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\DisassemblyWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\FilesWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\FileWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\InstructionsWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\LC3bFileWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\LC3FileWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\MainWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\MemoryBytesWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgramsWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\Project.cpp
# End Source File
# Begin Source File

SOURCE=.\ProjectLexer.cpp
# End Source File
# Begin Source File

SOURCE=.\ProjectParser.cpp
# End Source File
# Begin Source File

SOURCE=.\ProjectToken.cpp
# End Source File
# Begin Source File

SOURCE=.\ReadOnlyEditor.cpp
# End Source File
# Begin Source File

SOURCE=.\RegistersWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\SettingsWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\SimulatorWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\TextEditor.cpp
# End Source File
# Begin Source File

SOURCE=.\WriteDataWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\WriteRegisterWindow.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BreakpointWindow.h
# End Source File
# Begin Source File

SOURCE=.\CallStackWindow.h
# End Source File
# Begin Source File

SOURCE=.\ConsoleWindow.h
# End Source File
# Begin Source File

SOURCE=.\DataValuesWindow.h
# End Source File
# Begin Source File

SOURCE=.\DisassemblyWindow.h
# End Source File
# Begin Source File

SOURCE=.\FilesWindow.h
# End Source File
# Begin Source File

SOURCE=.\FileWindow.h
# End Source File
# Begin Source File

SOURCE=.\InstructionsWindow.h
# End Source File
# Begin Source File

SOURCE=.\LC3bFileWindow.h
# End Source File
# Begin Source File

SOURCE=.\LC3FileWindow.h
# End Source File
# Begin Source File

SOURCE=.\MainWindow.h
# End Source File
# Begin Source File

SOURCE=.\MemoryBytesWindow.h
# End Source File
# Begin Source File

SOURCE=.\MessageWindow.h
# End Source File
# Begin Source File

SOURCE=.\ProgramsWindow.h
# End Source File
# Begin Source File

SOURCE=.\Project.h
# End Source File
# Begin Source File

SOURCE=.\ProjectLexer.h
# End Source File
# Begin Source File

SOURCE=.\ProjectParser.h
# End Source File
# Begin Source File

SOURCE=.\ProjectToken.h
# End Source File
# Begin Source File

SOURCE=.\ReadOnlyEditor.h
# End Source File
# Begin Source File

SOURCE=.\RegistersWindow.h
# End Source File
# Begin Source File

SOURCE=.\SettingsWindow.h
# End Source File
# Begin Source File

SOURCE=.\SimulatorWindow.h
# End Source File
# Begin Source File

SOURCE=.\TextEditor.h
# End Source File
# Begin Source File

SOURCE=.\WriteDataWindow.h
# End Source File
# Begin Source File

SOURCE=.\WriteRegisterWindow.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Asm Src Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Assembler\AsmLexer.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\AsmParser.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\AsmToken.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\Assembler.cpp

!IF  "$(CFG)" == "AshIDE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3b Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Assembler\Base.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\Data.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\Disassembler.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\Element.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\Expander.cpp

!IF  "$(CFG)" == "AshIDE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3b Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Assembler\Label.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\Number.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\Program.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\Segment.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\Symbol.cpp
# End Source File
# Begin Source File

SOURCE=..\Assembler\SymbolTable.cpp
# End Source File
# End Group
# Begin Group "Asm Hdr Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Assembler\AsmLexer.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\AsmParser.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\AsmToken.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Assembler.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Base.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Data.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Disassembler.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Element.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Expander.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Label.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Number.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Program.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Segment.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\Symbol.h
# End Source File
# Begin Source File

SOURCE=..\Assembler\SymbolTable.h
# End Source File
# End Group
# Begin Group "LC3 Src Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\AsmConvertLC3\AsmConvertLC3.cpp
# End Source File
# Begin Source File

SOURCE=..\LC3Simulator\LC3Arch.cpp
# End Source File
# Begin Source File

SOURCE=..\LC3Assembler\LC3ISA.cpp
# End Source File
# Begin Source File

SOURCE=..\LC3Assembler\LC3ISA.def

!IF  "$(CFG)" == "AshIDE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3b Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "LC3 Hdr Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\AsmConvertLC3\AsmConvertLC3.h
# End Source File
# Begin Source File

SOURCE=..\LC3Simulator\LC3Arch.h
# End Source File
# Begin Source File

SOURCE=..\LC3Assembler\LC3ISA.h
# End Source File
# End Group
# Begin Group "LC3b Src Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\LC3bSimulator\LC3bArch.cpp
# End Source File
# Begin Source File

SOURCE=..\LC3bAssembler\LC3bISA.cpp
# End Source File
# Begin Source File

SOURCE=..\LC3bAssembler\LC3bISA.def

!IF  "$(CFG)" == "AshIDE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3b Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "LC3b Hdr Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\LC3bSimulator\LC3bArch.h
# End Source File
# Begin Source File

SOURCE=..\LC3bAssembler\LC3bISA.h
# End Source File
# End Group
# Begin Group "Sim Src Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Simulator\Architecture.cpp
# End Source File
# Begin Source File

SOURCE=..\Simulator\Memory.cpp
# End Source File
# Begin Source File

SOURCE=..\Simulator\Pipeline.cpp
# End Source File
# Begin Source File

SOURCE=..\Simulator\Register.cpp
# End Source File
# Begin Source File

SOURCE=..\Simulator\Simulator.cpp

!IF  "$(CFG)" == "AshIDE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "AshIDE - Win32 LC3b Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Sim Hdr Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Simulator\Architecture.h
# End Source File
# Begin Source File

SOURCE=..\Simulator\Memory.h
# End Source File
# Begin Source File

SOURCE=..\Simulator\Pipeline.h
# End Source File
# Begin Source File

SOURCE=..\Simulator\Register.h
# End Source File
# Begin Source File

SOURCE=..\Simulator\Simulator.h
# End Source File
# End Group
# Begin Group "JMTLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\JMTLib\HighlightLexer.cpp
# End Source File
# Begin Source File

SOURCE=..\JMTLib\JMTLib.cpp
# End Source File
# Begin Source File

SOURCE=..\JMTLib\Lexer.cpp
# End Source File
# Begin Source File

SOURCE=..\JMTLib\Token.cpp
# End Source File
# End Group
# End Target
# End Project
