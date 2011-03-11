;	Ash LC-3b Operating System 1.00
;	(c) Ashley Wise, 2003
;	awise@crhc.uiuc.edu
;
;	AshOS_LC3.asm must be linked with the user code.
;	AsmOS_LC3.ah should be be included in the user code.
;	R0 is used for passing values to/from OS functions.
;	R6 is expected to be the User Stack Pointer (USP).
;	R7 is overwritten by the TRAP instruction (return address).
;	All other registers are preserved.
;	Remember to NULL-terminate strings.

INCLUDE "AshOS_LC3.ah"

ORIGIN 0
SEGMENT

BootCode:
SEGMENT
	; Jumps over the Trap and Interrupt vector tables.
	trap MAIN
	; Might want to perform some check code after "main" exits
	trap HALT
	
TrapVectorTable:
SEGMENT 4x20

	GETC: data2 _GETC
	OUT: data2 _OUT
	PUTS: data2 _PUTS
	IN: data2 _IN
	PUTSP: data2 _PUTSP
	HALT: data2 _HALT
	MAIN: data2 _MAIN

	STRUCTDEF UndefVector data2 _UNDEF END
	STRUCT UndefVector[4xD9] ?

ExceptionVectorTable:
SEGMENT 4xE0
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
	STRUCTDEF RegisterStack
		Reg: data2[8] ?
	END
IFDEF NO_USP
	MACRO PushRegs0(LocalRegStack)
		st r0, LocalRegStack.Reg[0]
		PushRegs(LocalRegStack)
	END
	MACRO PushRegs(LocalRegStack)
		st r1, LocalRegStack.Reg[1]
		st r2, LocalRegStack.Reg[2]
		st r3, LocalRegStack.Reg[3]
		st r4, LocalRegStack.Reg[4]
		st r5, LocalRegStack.Reg[5]
		st r7, LocalRegStack.Reg[7]
	END
	MACRO PopRegs0(LocalRegStack)
		ld r0, LocalRegStack.Reg[0]
		PopRegs(LocalRegStack)
	END
	MACRO PopRegs(LocalRegStack)
		ld r1, LocalRegStack.Reg[1]
		ld r2, LocalRegStack.Reg[2]
		ld r3, LocalRegStack.Reg[3]
		ld r4, LocalRegStack.Reg[4]
		ld r5, LocalRegStack.Reg[5]
		ld r7, LocalRegStack.Reg[7]
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
		Push(r4)
		Push(r5)
		Push(r7)
	END
	MACRO PopRegs0(LocalRegStack)
		PopRegs(LocalRegStack)
		Pop(r0)
	END
	MACRO PopRegs(LocalRegStack)
		Pop(r7)
		Pop(r5)
		Pop(r4)
		Pop(r3)
		Pop(r2)
		Pop(r1)
	END
END

; GETC function
; Reads a single character from the keyboard. The character is not echoed
; onto the console. Its ASCII code is copied into R0. The high eight bits
; of R0 are cleared.
_GETC:
	PushRegs(GetCRegStack)
	WaitForKB:
		ldi r2, pKBSR
		BRzp WaitForKB
	ldi r0, pKBDR
	PopRegs(GetCRegStack)
	ret
	GetCRegStack: STRUCT RegisterStack ?
	pKBSR: data2 KBSR
	pKBDR: data2 KBDR

; OUT function
; Write the character in R0[7:0] to the console.
_OUT:
	PushRegs0(OutRegStack)
	WaitForDisplay:
		ldi r2, pDSR
		BRzp WaitForDisplay
	sti r0, pDDR
	PopRegs0(OutRegStack)
	ret
	OutRegStack: STRUCT RegisterStack ?

; PUTS function
; Write the string pointed to by R0 to the console.
; There is one character per memory location.
; Make sure the string is NULL-terminated!
_PUTS:
	PushRegs0(PutsRegStack)
	DisplayLoop2:
		ldr r3, r0, 0
		brz DisplayDone2
		WaitForDisplay2:
			ldi r2, pDSR
			BRzp WaitForDisplay2
		sti r3, pDDR
		add r0, r0, 1
		brnzp DisplayLoop2
	DisplayDone2:
	PopRegs0(PutsRegStack)
	ret
	PutsRegStack: STRUCT RegisterStack ?
	pDSR: data2 DSR
	pDDR: data2 DDR

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
	ld r0, ENDL
	trap OUT
	add r0, r1, 0
	PopRegs(InRegStack)
	ret
	InRegStack: STRUCT RegisterStack ?
	InMessage: DATA2[] "\nPlease input a single character: \0"
	ENDL: data2 '\n'

; PUTSP function
; Write the string pointed to by R0 to the console.
; There are two characters per memory location. The low-byte character
; [7:0] is written first. If there are an odd number of characters,
; the high-byte [15:8] is 4x00. There must still be a full 4x0000
; location at the end of the string.
; Make sure the string is NULL-terminated!
_PUTSP:
	PushRegs0(PutspRegStack)
	DisplayLoop3:
		ldr r4, r0, 0
		brz DisplayDone3

		;Write the lower char first
		WaitForDisplay3a:
			ldi r2, pDSR
			BRzp WaitForDisplay3a
		sti r4, pDDR

		;Move upper char into lower char
		ld  r3, UpperChar
		and r4, r4, r3	;clear out the lower part of the current double-char
		and r3, r3, 0	;zero the result
		and r5, r5, 0
		add r5, r5, BitMirrors$LEN	;loop count
		lea r7, BitMirrors	;setup address
		;R5 holds loop count
		;R4 holds upper char value
		;R3 holds result of moving upper char to lower char
		;R7 holds address of current mirror mask
		;R2 holds current mirror mask
		;R1 is temporary use
		;R0 keeps string pointer for PUTSP
		MirrorLoop:
			ldr r2, r7, 0
			add r7, r7, 1
			and r1, r2, r4	;see if this bit is set in the upper char
			BRz BitNotSet
				add r3, r3, r2	;add this bit to the lower char
			BitNotSet:
			add r5, r5, -1
			BRp MirrorLoop

		;Now write the upper char (which is now in the lower char location)
		WaitForDisplay3b:
			ldi r2, pDSR
			BRzp WaitForDisplay3b
		sti r3, pDDR

		add r0, r0, 1
		brnzp DisplayLoop3
	DisplayDone3:
	PopRegs0(PutspRegStack)
	ret
	PutspRegStack: STRUCT RegisterStack ?
	UpperChar: DATA2 4xFF00
	BitMirrors: DATA2[] 4x0101, 4x0202, 4x0404, 4x0808, 4x1010, 4x2020, 4x4040, 4x8080

; HALT function
; Halt execution and print a message to the console.
_HALT:
	PushRegs0(HaltRegStack)
	lea r0, HaltMessage;
	trap PUTS
	ldi r2, pMCR
	ld r0, Mask
	and r0, r0, R2
	sti r0, pMCR
	nop
	PopRegs0(HaltRegStack)
	ret
	HaltRegStack: STRUCT RegisterStack ?
	HaltMessage: DATA2[] "\nStopping program execution.\n\0"
	Mask: DATA2 4x7FFF
	pMCR: data2 MCR

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
	UndefMessage: DATA2[] "\nUndefined trap vector executed.\n\0"

; These functions are not yet implemented because the architectural
; use of the stack pointers is still not defined.
; The simulator will print a message and break on exceptions.

; Privilege exception function
; Halt execution and print a message to the console.
_Privilege:
	rti
	PrivMessage: DATA2[] "\nRTI instruction executed in unprivileged mode. Skipping over instruction.\n\0"

; IllegalInstr exception function
; Halt execution and print a message to the console.
_IllegalInstr:
	rti
	IllMessage: DATA2[] "\nIllegal instruction executed. Skipping over instruction.\n\0"

; Keyboard interrupt function
; Doesn't do anything. The user program should define and install their own handler.
_Keyboard:
	rti

;The start of the user code space
_MAIN:
SEGMENT 4x2E00
