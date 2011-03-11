;	Ash LC-3b Operating System 1.00
;	(c) Ashley Wise, 2003
;	awise@crhc.uiuc.edu
;
;	AshOS_LC3b.asm must be linked with the user code.
;	AsmOS_LC3b.ah should be be included in the user code.
;	R0 is used for passing values to/from OS functions.
;	R6 is expected to be the User Stack Pointer (USP).
;	R7 is overwritten by the TRAP instruction (return address).
;	All other registers are preserved.
;	Remember to NULL-terminate strings.

INCLUDE "AshOS_LC3b.ah"

ORIGIN 0
SEGMENT

BootCode:
SEGMENT
	; Jumps over the trap vector table.
	trap MAIN
	; Might want to perform some check code after "main" exits
	trap HALT
	
TrapVectorTable:
SEGMENT 4x40

	GETC: data2 _GETC
	OUT: data2 _OUT
	PUTS: data2 _PUTS
	IN: data2 _IN
	HALT: data2 _HALT
	MAIN: data2 _MAIN

	STRUCTDEF UndefVector data2 _UNDEF END
	STRUCT UndefVector[4x5A] ?

ExceptionVectorTable:
SEGMENT 4xC0
; The program will load the pointer to their handlers here.
	Privilege: data2 _Privilege
	IllegalInstr: data2 _IllegalInstr

InterruptVectorTable:
SEGMENT 4x80
; The program will load the pointer to their handlers here.
	Keyboard: data2 _Keyboard

TrapFunctions:
SEGMENT 4x80

; Note that in order for the trap functions to work correctly,
; the user code must have preserved R6 as the User Stack Pointer
; (USP). R6 is initialized to this value when the simulator boots.
; If you clobber R6, trap functions might not work.
; You can define NO_USP (as a global define, not in your code)
; which will tell the OS to use local storage rather than the USP.
; However, due to a limitation of the LC-3b ISA, the TRAP functions
; will clobber any value in R5 to achieve this. So if you define
; NO_USP, don't expect R5 to be preserved.
	STRUCTDEF RegisterStack
		Reg: data2[8] ?
	END
IFDEF NO_USP
	MACRO PushRegs0(LocalRegStack)
		lea r5, LocalRegStack
		str r0, r5, RegisterStack.Reg[0]
		PushRegs(LocalRegStack)
	END
	MACRO PushRegs(LocalRegStack)
		lea r5, LocalRegStack
		str r1, r5, RegisterStack.Reg[1]
		str r2, r5, RegisterStack.Reg[2]
		str r3, r5, RegisterStack.Reg[3]
		str r7, r5, RegisterStack.Reg[7]
	END
	MACRO PopRegs0(LocalRegStack)
		lea r5, LocalRegStack
		ldr r0, r5, RegisterStack.Reg[0]
		PopRegs(LocalRegStack)
	END
	MACRO PopRegs(LocalRegStack)
		lea r5, LocalRegStack
		ldr r1, r5, RegisterStack.Reg[1]
		ldr r2, r5, RegisterStack.Reg[2]
		ldr r3, r5, RegisterStack.Reg[3]
		ldr r7, r5, RegisterStack.Reg[7]
		and r5, r5, 0
	END
ELSE
	MACRO PushRegs0(LocalRegStack)
		Push(r0)
		PushRegs(LocalRegStack)
	END
	MACRO PushRegs(LocalRegStack)
		Push(r1)
		Push(r2)
		Push(r3)
		Push(r7)
	END
	MACRO PopRegs0(LocalRegStack)
		PopRegs(LocalRegStack)
		Pop(r0)
	END
	MACRO PopRegs(LocalRegStack)
		Pop(r7)
		Pop(r3)
		Pop(r2)
		Pop(r1)
	END
END

	; Pointers to the MMIO register locations
	pDSR: data2 DSR
	pDDR: data2 DDR
	pKBSR: data2 KBSR
	pKBDR: data2 KBDR
	pMCR: data2 MCR

; GETC function
; Reads a single character from the keyboard. The character is not echoed
; onto the console. Its ASCII code is copied into R0. The high eight bits
; of R0 are cleared.
_GETC:
	PushRegs(GetCRegStack)
	lea r1, TrapFunctions
	WaitForKB:
		ldi r2, r1, pKBSR
		BRzp WaitForKB
	ldi r0, r1, pKBDR
	PopRegs(GetCRegStack)
	ret
	GetCRegStack: STRUCT RegisterStack ?

; OUT function
; Write the character in R0[7:0] to the console
_OUT:
	PushRegs0(OutRegStack)
	lea r1, TrapFunctions
	WaitForDisplay:
		ldi r2, r1, pDSR
		BRzp WaitForDisplay
	sti r0, r1, pDDR
	PopRegs0(OutRegStack)
	ret
	OutRegStack: STRUCT RegisterStack ?

; PUTS function
; Write the string pointed to by R0 to the console.
; Make sure the string is NULL-terminated!
_PUTS:
	PushRegs0(PutsRegStack)
	lea r1, TrapFunctions
	DisplayLoop2:
		ldb r3, r0, 0
		brz DisplayDone2
		WaitForDisplay2:
			ldi r2, r1, pDSR
			BRzp WaitForDisplay2
		sti r3, r1, pDDR
		add r0, r0, 1
		brnzp DisplayLoop2
	DisplayDone2:
	PopRegs0(PutsRegStack)
	ret
	PutsRegStack: STRUCT RegisterStack ?

; IN function
; Print a prompt to the screen and read a single character from the keyboard.
; The character is echoed onto the console along with a newline, and its
; ASCII code is copied into R0. The high eight bits of R0 are cleared.
_IN:
	PushRegs(InRegStack)
	lea r0, InMessage;
	trap PUTS
	trap GETC
	trap OUT
	add r1, r0, 0
	lea r2, ENDL
	ldb r0, r2, ENDL$BASEREL
	trap OUT
	add r0, r1, 0
	PopRegs(InRegStack)
	ret
	InRegStack: STRUCT RegisterStack ?
	InMessage: DATA1[] "\nPlease input a single character: \0"
	ALIGN 2
	ENDL: data1 '\n'

ALIGN 2
; HALT function
; Halt execution and print a message to the console.
_HALT:
	PushRegs0(HaltRegStack)
	lea r0, HaltMessage;
	trap PUTS
	and r0, r0, 0
	lea r1, TrapFunctions
	ldi r2, r1, pMCR
	lea r3, Mask
	ldr r0, r3, Mask$BASEREL
	and r0, r0, r2
	sti r0, r1, pMCR
	nop
	PopRegs0(HaltRegStack)
	ret
	HaltRegStack: STRUCT RegisterStack ?
	HaltMessage: DATA1[] "\nStopping program execution.\n\0"
	ALIGN 2
	Mask: DATA2 4x7FFF

ALIGN 2
; Undef function
; Default function if an undefined trap vector is called.
; Halt execution and print a message to the console.
_UNDEF:
	PushRegs0(UndefRegStack)
	lea r0, UndefMessage;
	trap PUTS
	and r0, r0, 0
	PopRegs0(UndefRegStack)
	ret
	UndefRegStack: STRUCT RegisterStack ?
	UndefMessage: DATA1[] "\nUndefined trap vector executed.\n\0"

; These functions are not yet implemented because the architectural
; use of the stack pointers is still not defined.
; The simulator will print a message and break on exceptions.

; Privilege exception function
; Halt execution and print a message to the console.
_Privilege:
	rti
	PrivMessage: DATA1[] "\nRTI instruction executed in unprivileged mode. Skipping over instruction.\n\0"

ALIGN 2
; IllegalInstr exception function
; Halt execution and print a message to the console.
_IllegalInstr:
	rti
	IllMessage: DATA1[] "\nIllegal instruction executed. Skipping over instruction.\n\0"

ALIGN 2
; Keyboard interrupt function
; Doesn't do anything. The user program should define and install their own handler.
_Keyboard:
	rti

;The start of the user code space
_MAIN:
SEGMENT 4x2E00
