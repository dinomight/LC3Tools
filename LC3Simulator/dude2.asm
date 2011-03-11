INCLUDE "../AshOS/AshOS_LC3.ah"

ORIGIN

SEGMENT
rti
	trap getc
	lea r5, DataSegment
	ld r0, Char
	trap out
	ld r0, Char2
	trap out
	lea r0, String
	trap putsp
	lea r0, String2
	trap puts
	trap in
;	trap 4x35
	trap halt

DataSegment:
SEGMENT
	String: data1[] "Hello Worldb\n\0\0\0"
ALIGN 2
	String2: data2[] "Hello World\n\0"
	Char: data1 'A'
ALIGN 2
	Char2: data2 'B'
