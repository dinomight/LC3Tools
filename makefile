ASM_BUILD = Assembler
LC3_ASM_BUILD = LC3Assembler
LC3B_ASM_BUILD = LC3bAssembler
SIM_BUILD = Simulator
LC3_SIM_BUILD = LC3Simulator
LC3B_SIM_BUILD = LC3bSimulator
ASHIDE_BUILD = AshIDE
LC3CONV_BUILD = AsmConvertLC3
LC2CONV_BUILD = AsmConvert2to3
ENDIAN_BUILD = EndianCheck
JMTLIB_BUILD = JMTLib

ASM_OPATH = ${ASM_BUILD}/Obj
LC3_ASM_OPATH = ${LC3_ASM_BUILD}/Obj
LC3B_ASM_OPATH = ${LC3B_ASM_BUILD}/Obj
SIM_OPATH = ${SIM_BUILD}/Obj
LC3_SIM_OPATH = ${LC3_SIM_BUILD}/Obj
LC3B_SIM_OPATH = ${LC3B_SIM_BUILD}/Obj
ASHIDE_OPATH = ${ASHIDE_BUILD}/Obj
LC3CONV_OPATH = ${LC3CONV_BUILD}/Obj
LC2CONV_OPATH = ${LC2CONV_BUILD}/Obj
JMTLIB_OPATH = ${JMTLIB_BUILD}/Obj

ASM_OBJ = ${ASM_OPATH}/AsmLexer.o ${ASM_OPATH}/AsmParser.o ${ASM_OPATH}/AsmToken.o ${ASM_OPATH}/Base.o ${ASM_OPATH}/Data.o ${ASM_OPATH}/Disassembler.o ${ASM_OPATH}/Element.o ${ASM_OPATH}/Label.o ${ASM_OPATH}/Number.o ${ASM_OPATH}/Program.o ${ASM_OPATH}/Segment.o ${ASM_OPATH}/Symbol.o ${ASM_OPATH}/SymbolTable.o
#ASM_OBJ = ${ASM_OBJ} ${ASM_OPATH}/Assembler.o ${ASM_OPATH}/Expander.o
LC3_ASM_OBJ = ${LC3_ASM_OPATH}/AsmUI.o ${LC3_ASM_OPATH}/LC3ISA.o
LC3B_ASM_OBJ = ${LC3B_ASM_OPATH}/AsmUI.o ${LC3B_ASM_OPATH}/LC3bISA.o
SIM_OBJ = ${SIM_OPATH}/Architecture.o ${SIM_OPATH}/Memory.o ${SIM_OPATH}/Pipeline.o ${SIM_OPATH}/Register.o
#SIM_OBJ = ${SIM_OBJ} ${SIM_OPATH}/Simulator.o
LC3_SIM_OBJ = ${LC3_SIM_OPATH}/SimUI.o  ${LC3_SIM_OPATH}/LC3Arch.o
LC3B_SIM_OBJ = ${LC3B_SIM_OPATH}/SimUI.o ${LC3B_SIM_OPATH}/LC3bArch.o
ASHIDE_OBJ = ${ASHIDE_OPATH}/BreakpointWindow.o ${ASHIDE_OPATH}/CallStackWindow.o ${ASHIDE_OPATH}/ConsoleWindow.o ${ASHIDE_OPATH}/DataValuesWindow.o ${ASHIDE_OPATH}/DisassemblyWindow.o ${ASHIDE_OPATH}/FilesWindow.o ${ASHIDE_OPATH}/FileWindow.o ${ASHIDE_OPATH}/InstructionsWindow.o ${ASHIDE_OPATH}/LC3bFileWindow.o ${ASHIDE_OPATH}/LC3FileWindow.o ${ASHIDE_OPATH}/MainWindow.o ${ASHIDE_OPATH}/MemoryBytesWindow.o ${ASHIDE_OPATH}/MessageWindow.o ${ASHIDE_OPATH}/ProgramsWindow.o ${ASHIDE_OPATH}/Project.o ${ASHIDE_OPATH}/ProjectLexer.o ${ASHIDE_OPATH}/ProjectParser.o ${ASHIDE_OPATH}/ProjectToken.o ${ASHIDE_OPATH}/ReadOnlyEditor.o ${ASHIDE_OPATH}/RegistersWindow.o ${ASHIDE_OPATH}/SettingsWindow.o ${ASHIDE_OPATH}/SimulatorWindow.o ${ASHIDE_OPATH}/TextEditor.o ${ASHIDE_OPATH}/WriteDataWindow.o ${ASHIDE_OPATH}/WriteRegisterWindow.o
LC3CONV_OBJ = ${LC3CONV_OPATH}/AsmConvertLC3.o
LC2CONV_OBJ = ${LC2CONV_OPATH}/AsmConvert2to3.o
JMTLIB_OBJ = ${JMTLIB_OPATH}/JMTLib.o ${JMTLIB_OPATH}/Lexer.o ${JMTLIB_OPATH}/Token.o ${JMTLIB_OPATH}/HighlightLexer.o

ASM_CPP = ${ASM_BUILD}/AsmLexer.cpp ${ASM_BUILD}/AsmParser.cpp ${ASM_BUILD}/AsmToken.cpp ${ASM_BUILD}/Base.cpp ${ASM_BUILD}/Data.cpp ${ASM_BUILD}/Disassembler.cpp ${ASM_BUILD}/Element.cpp ${ASM_BUILD}/Label.cpp ${ASM_BUILD}/Number.cpp ${ASM_BUILD}/Program.cpp ${ASM_BUILD}/Segment.cpp ${ASM_BUILD}/Symbol.cpp ${ASM_BUILD}/SymbolTable.cpp
#ASM_CPP = ${ASM_CPP} ${ASM_BUILD}/Assembler.cpp ${ASM_BUILD}/Expander.cpp
LC3_ASM_CPP = ${LC3_ASM_BUILD}/AsmUI.cpp ${LC3_ASM_BUILD}/LC3ISA.cpp
LC3B_ASM_CPP = ${LC3B_ASM_BUILD}/AsmUI.cpp ${LC3B_ASM_BUILD}/LC3bISA.cpp
SIM_CPP = ${SIM_BUILD}/Architecture.cpp ${SIM_BUILD}/Memory.cpp ${SIM_BUILD}/Pipeline.cpp ${SIM_BUILD}/Register.cpp
#SIM_CPP = ${SIM_CPP} ${SIM_BUILD}/Simulator.o
LC3_SIM_CPP = ${LC3_SIM_BUILD}/SimUI.cpp  ${LC3_SIM_BUILD}/LC3Arch.cpp
LC3B_SIM_CPP = ${LC3B_SIM_BUILD}/SimUI.cpp ${LC3B_SIM_BUILD}/LC3bArch.cpp
ASHIDE_CPP = ${ASHIDE_BUILD}/BreakpointWindow.cpp ${ASHIDE_BUILD}/CallStackWindow.cpp ${ASHIDE_BUILD}/ConsoleWindow.cpp ${ASHIDE_BUILD}/DataValuesWindow.cpp ${ASHIDE_BUILD}/DisassemblyWindow.cpp ${ASHIDE_BUILD}/FilesWindow.cpp ${ASHIDE_BUILD}/FileWindow.cpp ${ASHIDE_BUILD}/InstructionsWindow.cpp ${ASHIDE_BUILD}/LC3bFileWindow.cpp ${ASHIDE_BUILD}/LC3FileWindow.cpp ${ASHIDE_BUILD}/MainWindow.cpp ${ASHIDE_BUILD}/MemoryBytesWindow.cpp ${ASHIDE_BUILD}/MessageWindow.cpp ${ASHIDE_BUILD}/ProgramsWindow.cpp ${ASHIDE_BUILD}/Project.cpp ${ASHIDE_BUILD}/ProjectLexer.cpp ${ASHIDE_BUILD}/ProjectParser.cpp ${ASHIDE_BUILD}/ProjectToken.cpp ${ASHIDE_BUILD}/ReadOnlyEditor.cpp ${ASHIDE_BUILD}/RegistersWindow.cpp ${ASHIDE_BUILD}/SettingsWindow.cpp ${ASHIDE_BUILD}/SimulatorWindow.cpp ${ASHIDE_BUILD}/TextEditor.cpp ${ASHIDE_BUILD}/WriteDataWindow.cpp ${ASHIDE_BUILD}/WriteRegisterWindow.cpp
LC3CONV_CPP = ${LC3CONV_BUILD}/AsmConvertLC3.cpp
LC2CONV_CPP = ${LC2CONV_BUILD}/AsmConvert2to3.cpp
JMTLIB_CPP = ${JMTLIB_BUILD}/JMTLib.cpp ${JMTLIB_BUILD}/Lexer.cpp ${JMTLIB_BUILD}/Token.cpp ${JMTLIB_BUILD}/HighlightLexer.cpp

ASM_H = ${ASM_BUILD}/AsmLexer.h ${ASM_BUILD}/AsmParser.h ${ASM_BUILD}/AsmToken.h ${ASM_BUILD}/Assembler.h ${ASM_BUILD}/Assembler.cpp ${ASM_BUILD}/Base.h ${ASM_BUILD}/Data.h ${ASM_BUILD}/Disassembler.h ${ASM_BUILD}/Element.h ${ASM_BUILD}/Expander.h ${ASM_BUILD}/Expander.cpp ${ASM_BUILD}/Label.h ${ASM_BUILD}/Number.h ${ASM_BUILD}/Program.h ${ASM_BUILD}/Segment.h ${ASM_BUILD}/Symbol.h ${ASM_BUILD}/SymbolTable.h
LC3_ASM_H = ${LC3_ASM_BUILD}/AsmUI.h ${LC3_ASM_BUILD}/LC3ISA.h ${LC3_ASM_BUILD}/LC3ISA.def
LC3B_ASM_H = ${LC3B_ASM_BUILD}/AsmUI.h ${LC3B_ASM_BUILD}/LC3bISA.h ${LC3B_ASM_BUILD}/LC3bISA.def
SIM_H = ${SIM_BUILD}/Architecture.h ${SIM_BUILD}/Memory.h ${SIM_BUILD}/Pipeline.h ${SIM_BUILD}/Register.h ${SIM_BUILD}/Simulator.h ${SIM_BUILD}/Simulator.cpp
LC3_SIM_H = ${LC3_SIM_BUILD}/SimUI.h  ${LC3_SIM_BUILD}/LC3Arch.h
LC3B_SIM_H = ${LC3B_SIM_BUILD}/SimUI.h ${LC3B_SIM_BUILD}/LC3bArch.h
ASHIDE_H = ${ASHIDE_BUILD}/BreakpointWindow.h ${ASHIDE_BUILD}/CallStackWindow.h ${ASHIDE_BUILD}/ConsoleWindow.h ${ASHIDE_BUILD}/DataValuesWindow.h ${ASHIDE_BUILD}/DisassemblyWindow.h ${ASHIDE_BUILD}/FilesWindow.h ${ASHIDE_BUILD}/FileWindow.h ${ASHIDE_BUILD}/InstructionsWindow.h ${ASHIDE_BUILD}/LC3bFileWindow.h ${ASHIDE_BUILD}/LC3FileWindow.h ${ASHIDE_BUILD}/MainWindow.h ${ASHIDE_BUILD}/MemoryBytesWindow.h ${ASHIDE_BUILD}/MessageWindow.h ${ASHIDE_BUILD}/ProgramsWindow.h ${ASHIDE_BUILD}/Project.h ${ASHIDE_BUILD}/ProjectLexer.h ${ASHIDE_BUILD}/ProjectParser.h ${ASHIDE_BUILD}/ProjectToken.h ${ASHIDE_BUILD}/ReadOnlyEditor.h ${ASHIDE_BUILD}/RegistersWindow.h ${ASHIDE_BUILD}/SettingsWindow.h ${ASHIDE_BUILD}/SimulatorWindow.h ${ASHIDE_BUILD}/TextEditor.h ${ASHIDE_BUILD}/WriteDataWindow.h ${ASHIDE_BUILD}/WriteRegisterWindow.h
LC3CONV_H = ${LC3CONV_BUILD}/AsmConvertLC3.h
LC2CONV_H = 
JMTLIB_H = ${JMTLIB_BUILD}/JMTLib.h ${JMTLIB_BUILD}/Lexer.h ${JMTLIB_BUILD}/Token.h ${JMTLIB_BUILD}/HighlightLexer.h ${JMTLIB_BUILD}/JMTSys.h ${JMTLIB_BUILD}/SynchLib.h ${JMTLIB_BUILD}/SparseArray.h

#RedHat Linux x86 GCC Options
CFLAGS = -O3 -w -fpermissive -DUNIX_BUILD -DGPLUSPLUS
FLTKCFLAGS = -Ifltk-1.1.7
FLTKLDFLAGS = -Lfltk-1.1.7/lib/Redhat_x86 -lfltk_images -lfltk -L/usr/X11R6/lib -lX11
RELEASE_DIR = Release/Redhat_x86

#Solaris Unix Sparc GCC Options
#CFLAGS = -O3 -w -fpermissive -DUNIX_BUILD -DBIG_ENDIAN_BUILD -DGPLUSPLUS
#FLTKCFLAGS = -Ifltk-1.1.7
#Old options: -lm -lXext -lX11 -lsocket
#FLTKLDFLAGS = -Lfltk-1.1.7/lib/Solaris_Sparc -lfltk_images -lfltk -L/usr/X11R6/lib -lX11 -lsocket
#RELEASE_DIR = Release/Solaris_Sparc

#*NOTE: Make on some versions of Linux always recompiles, even when the dependencies are up-to-date.
All: LC3bTools.set LC3Tools.set AshIDE.set

AshIDE.set: ${RELEASE_DIR} AshIDE.out LC3IDE.out LC3bIDE.out
#AshIDE.set: ${RELEASE_DIR} AshIDE.out
	mv AshIDE.out ${RELEASE_DIR}/AshIDE
	mv LC3IDE.out ${RELEASE_DIR}/LC3IDE
	mv LC3bIDE.out ${RELEASE_DIR}/LC3bIDE
	echo "AshIDE" > AshIDE.set

LC3Tools.set: ${RELEASE_DIR} LC3Assembler.out LC3Simulator.out AsmConvertLC3.out EndianCheck.out
	mv LC3Assembler.out ${RELEASE_DIR}/LC3Assembler
	mv LC3Simulator.out ${RELEASE_DIR}/LC3Simulator
	mv AsmConvertLC3.out ${RELEASE_DIR}/AsmConvertLC3
	mv EndianCheck.out ${RELEASE_DIR}/EndianCheck
	echo "LC3Tools" > LC3Tools.set

#*NOTE: If you build "all", then it tries to move EndianCheck.out twice, once for LC3 and once for LC3b
#The second time, it's not there anymore so the makefile errors out.
LC3bTools.set: ${RELEASE_DIR} LC3bAssembler.out LC3bSimulator.out AsmConvert2to3.out EndianCheck.out
	mv LC3bAssembler.out ${RELEASE_DIR}/LC3bAssembler
	mv LC3bSimulator.out ${RELEASE_DIR}/LC3bSimulator
	mv AsmConvert2to3.out ${RELEASE_DIR}/AsmConvert2to3
#	mv EndianCheck.out ${RELEASE_DIR}/EndianCheck
	echo "LC3bTools" > LC3bTools.set

LC3Assembler.out: ${ASM_OBJ} ${LC3_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${LC3_ASM_BUILD}/main.cpp
	g++ ${CFLAGS} ${ASM_OBJ} ${LC3_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${LC3_ASM_BUILD}/main.cpp -o LC3Assembler.out

LC3bAssembler.out: ${ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3B_ASM_BUILD}/main.cpp
	g++ ${CFLAGS} ${ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3B_ASM_BUILD}/main.cpp -o LC3bAssembler.out

LC3Simulator.out: ${SIM_OBJ} ${LC3_SIM_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${LC3_SIM_BUILD}/main.cpp
	g++ ${CFLAGS} ${SIM_OBJ} ${LC3_SIM_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${LC3_SIM_BUILD}/main.cpp -o LC3Simulator.out

LC3bSimulator.out: ${SIM_OBJ} ${LC3B_SIM_OBJ} ${ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3B_SIM_BUILD}/main.cpp
	g++ ${CFLAGS} ${SIM_OBJ} ${LC3B_SIM_OBJ} ${ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3B_SIM_BUILD}/main.cpp -o LC3bSimulator.out

AshIDE.out: ${ASHIDE_OBJ} ${SIM_OBJ} ${LC3_SIM_OBJ} ${LC3B_SIM_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${ASHIDE_BUILD}/main.cpp
	g++ ${CFLAGS} ${FLTKCFLAGS} ${ASHIDE_OBJ} ${SIM_OBJ} ${LC3_SIM_OBJ} ${LC3B_SIM_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${ASHIDE_BUILD}/main.cpp ${FLTKLDFLAGS} -o AshIDE.out

LC3IDE.out: ${ASHIDE_OBJ} ${SIM_OBJ} ${LC3_SIM_OBJ} ${LC3B_SIM_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${ASHIDE_BUILD}/main.cpp
	g++ ${CFLAGS} ${FLTKCFLAGS} -DDEFAULT_LANG_LC3 ${ASHIDE_OBJ} ${SIM_OBJ} ${LC3_SIM_OBJ} ${LC3B_SIM_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${ASHIDE_BUILD}/main.cpp ${FLTKLDFLAGS} -o LC3IDE.out

LC3bIDE.out: ${ASHIDE_OBJ} ${SIM_OBJ} ${LC3_SIM_OBJ} ${LC3B_SIM_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${ASHIDE_BUILD}/main.cpp
	g++ ${CFLAGS} ${FLTKCFLAGS} -DDEFAULT_LANG_LC3B ${ASHIDE_OBJ} ${SIM_OBJ} ${LC3_SIM_OBJ} ${LC3B_SIM_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${LC3B_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_OBJ} ${ASHIDE_BUILD}/main.cpp ${FLTKLDFLAGS} -o LC3bIDE.out

AsmConvertLC3.out: ${LC3CONV_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_BUILD}/main.cpp
	g++ ${CFLAGS} ${LC3CONV_OBJ} ${ASM_OBJ} ${LC3_ASM_OBJ} ${JMTLIB_OBJ} ${LC3CONV_BUILD}/main.cpp -o AsmConvertLC3.out

AsmConvert2to3.out: ${LC2CONV_OBJ}
	g++ ${CFLAGS} ${LC2CONV_OBJ} -o AsmConvert2to3.out

EndianCheck.out: ${ENDIAN_BUILD}/main.cpp ${JMTLIB_BUILD}/JMTLib.cpp
	g++ ${CFLAGS} ${ENDIAN_BUILD}/main.cpp ${JMTLIB_BUILD}/JMTLib.cpp -o EndianCheck.out

#*NOTE: Some versions of make on linux don't support the following which used to be on the dependency line:
#${ASM_OBJ}: ${ASM_OPATH} ${ASM_BUILD}/$${@F:.o=.cpp}
#${LC3_ASM_OBJ}: ${LC3_ASM_OPATH} ${LC3_ASM_BUILD}/$${@F:.o=.cpp}
#${LC3B_ASM_OBJ}: ${LC3B_ASM_OPATH} ${LC3B_ASM_BUILD}/$${@F:.o=.cpp}
#${SIM_OBJ}: ${SIM_OPATH} ${SIM_BUILD}/$${@F:.o=.cpp}
#${LC3_SIM_OBJ}: ${LC3_SIM_OPATH} ${LC3_SIM_BUILD}/$${@F:.o=.cpp}
#${LC3B_SIM_OBJ}: ${LC3B_SIM_OPATH} ${LC3B_SIM_BUILD}/$${@F:.o=.cpp}
#${LC3CONV_OBJ}: ${LC3CONV_OPATH} ${LC3CONV_BUILD}/$${@F:.o=.cpp}
#${LC2CONV_OBJ}: ${LC2CONV_OPATH} ${LC2CONV_BUILD}/$${@F:.o=.cpp}

${ASM_OBJ}: ${ASM_OPATH} ${ASM_CPP} ${ASM_H}
	g++ -c ${CFLAGS} ${ASM_BUILD}/${*F}.cpp -o $@

${LC3_ASM_OBJ}: ${LC3_ASM_OPATH} ${LC3_ASM_CPP} ${LC3_ASM_H} ${ASM_H} ${LC3CONV_H}
	g++ -c ${CFLAGS} ${LC3_ASM_BUILD}/${*F}.cpp -o $@

${LC3B_ASM_OBJ}: ${LC3B_ASM_OPATH} ${LC3B_ASM_CPP} ${LC3B_ASM_H} ${ASM_H}
	g++ -c ${CFLAGS} ${LC3B_ASM_BUILD}/${*F}.cpp -o $@

${SIM_OBJ}: ${SIM_OPATH} ${SIM_CPP} ${SIM_H} ${ASM_H}
	g++ -c ${CFLAGS} ${SIM_BUILD}/${*F}.cpp -o $@

${LC3_SIM_OBJ}: ${LC3_SIM_OPATH} ${LC3_SIM_CPP} ${LC3_SIM_H} ${SIM_H} ${ASM_H} ${LC3CONV_H}
	g++ -c ${CFLAGS} ${LC3_SIM_BUILD}/${*F}.cpp -o $@

${LC3B_SIM_OBJ}: ${LC3B_SIM_OPATH} ${LC3B_SIM_CPP} ${LC3B_SIM_H} ${SIM_H} ${ASM_H}
	g++ -c ${CFLAGS} ${LC3B_SIM_BUILD}/${*F}.cpp -o $@

${ASHIDE_OBJ}: ${ASHIDE_OPATH} ${ASHIDE_CPP} ${ASHIDE_H} ${SIM_H} ${ASM_H} ${LC3CONV_H}
	g++ -c ${CFLAGS} ${FLTKCFLAGS} ${ASHIDE_BUILD}/${*F}.cpp -o $@

${LC3CONV_OBJ}: ${LC3CONV_OPATH} ${LC3CONV_CPP} ${LC3CONV_H} ${LC3_ASM_H} ${ASM_H}
	g++ -c ${CFLAGS} ${LC3CONV_BUILD}/${*F}.cpp -o $@

${LC2CONV_OBJ}: ${LC2CONV_OPATH} ${LC2CONV_CPP} ${LC2CONV_H}
	g++ -c ${CFLAGS} ${LC2CONV_BUILD}/${*F}.cpp -o $@

${JMTLIB_OBJ}: ${JMTLIB_OPATH} ${JMTLIB_CPP} ${JMTLIB_H}
	g++ -c ${CFLAGS} ${JMTLIB_BUILD}/${*F}.cpp -o $@

${ASM_OPATH}:
	mkdir -p ${ASM_OPATH}

${LC3_ASM_OPATH}:
	mkdir -p ${LC3_ASM_OPATH}

${LC3B_ASM_OPATH}:
	mkdir -p ${LC3B_ASM_OPATH}

${SIM_OPATH}:
	mkdir -p ${SIM_OPATH}

${LC3_SIM_OPATH}:
	mkdir -p ${LC3_SIM_OPATH}

${LC3B_SIM_OPATH}:
	mkdir -p ${LC3B_SIM_OPATH}

${ASHIDE_OPATH}:
	mkdir -p ${ASHIDE_OPATH}

${LC3CONV_OPATH}:
	mkdir -p ${LC3CONV_OPATH}

${LC2CONV_OPATH}:
	mkdir -p ${LC2CONV_OPATH}

${JMTLIB_OPATH}:
	mkdir -p ${JMTLIB_OPATH}

${RELEASE_DIR}:
	mkdir -p ${RELEASE_DIR}

clean:
	rm -f AshIDE.set LC3Tools.set LC3bTools.set ${ASM_OPATH}/* ${LC3_ASM_OPATH}/* ${LC3B_ASM_OPATH}/* ${SIM_OPATH}/* ${LC3_SIM_OPATH}/* ${LC3B_SIM_OPATH}/* ${ASHIDE_OPATH}/* ${LC3CONV_OPATH}/* ${LC2CONV_OPATH}/* ${JMTLIB_OPATH}/* *.out

