# Microsoft Developer Studio Project File - Name="LC3Simulator" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=LC3Simulator - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LC3Simulator.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LC3Simulator.mak" CFG="LC3Simulator - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LC3Simulator - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "LC3Simulator - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LC3Simulator - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying EXE...
PostBuild_Cmds=copy Release\*.exe ..\Release\Windows_x86\.
# End Special Build Tool

!ELSEIF  "$(CFG)" == "LC3Simulator - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "LC3Simulator - Win32 Release"
# Name "LC3Simulator - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\AsmConvertLC3\AsmConvertLC3.cpp
# End Source File
# Begin Source File

SOURCE=..\LC3Assembler\AsmUI.cpp
# End Source File
# Begin Source File

SOURCE=.\LC3Arch.cpp
# End Source File
# Begin Source File

SOURCE=..\LC3Assembler\LC3ISA.cpp
# End Source File
# Begin Source File

SOURCE=..\LC3Assembler\LC3ISA.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\SimUI.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\AsmConvertLC3\AsmConvertLC3.h
# End Source File
# Begin Source File

SOURCE=..\LC3Assembler\AsmUI.h
# End Source File
# Begin Source File

SOURCE=.\LC3Arch.h
# End Source File
# Begin Source File

SOURCE=..\LC3Assembler\LC3ISA.h
# End Source File
# Begin Source File

SOURCE=.\SimUI.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
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
# PROP Exclude_From_Build 1
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
# PROP Exclude_From_Build 1
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
# PROP Exclude_From_Build 1
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
