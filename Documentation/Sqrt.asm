ORIGIN
SEGMENT

CodeSegment:
SEGMENT

;*****************************
;Square Root Function.
;	r1 = Accumulator
;	r2 = Deaccumulator
;	r3 = CurrentMask2
;	r4 = CurrentMask
;	r5 = CurrentDigit = CurrentBit + 1
;	r6 = Temp
;	r7 = DataSegmentAddress
;	Value = Value to square root
;*****************************
	;Initialize variables
	lea	r7, DataSegment	;R7 contains the address of the data segment
	and	r1, r1, 0		;Acc <- 0
	ldr	r3, r7, CurMask2Init
	ldr	r4, r7, CurMaskInit
	ldr	r5, r7, CurDigitInit
	ldr	r2, r7, Value
	brz	Done			;Done if(Deac == 0)

Loop:
	;Temp = Deacc - CurMask2 - 2 * CurBit * Acc
	;Split into two steps. Shifting and subtraction.

	;Temp = (Acc << CurDigit) ==> In effect Temp = 2 * CurBit * Acc
	str	r5, r7, StoreR5
	add	r6, r1, 0			;Temp <- Acc
ShiftLeft:
		lshf	r6, r6, 1	;Temp <<= 1
		add	r5, r5, -1		;CurDigit--
		brp	ShiftLeft
	ldr	r5, r7, StoreR5

	;Temp = Deacc - CurMask2 - Temp
	add	r6, r6, r3
	not	r6, r6
	add	r6, r6, 1
	add	r6, r6, r2
	
	;if(Temp >= 0)
	brn	Skip
		add	r1, r1, r4	;Acc += CurMask
		add	r2, r6, 0	;Deac = Temp ==> In effect Deac -= CurMask + 2 * CurBit * Acc
		brz	Done		;Done if(Deac == 0)
Skip:
	rshfl	r3, r3, 2		;CurMask2 >>= 2	==> In Effect CurMask2 /= 4
	rshfl	r4, r4, 1		;CurMask >>= 1 ==> In Effect CurMask /= 2
	add	r5, r5, -1		;CurDigit--
	;if(CurDigit > 0)
	brp	Loop

Done:
	str	r1, r7, Result	;Value = Acc^2 + Deacc (r1^2 + r2), so r1 = Sqrt(Value)

Halt:
	br	Halt

DataSegment:
SEGMENT
CurMask2Init:	Data2	4x4000	;2^14
CurMaskInit:	Data2	4x0080	;2^7
CurDigitInit:	Data2	8
StoreR5:		Data2	?
Value:			Data2	144
Result:			Data2	?
