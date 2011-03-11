INCLUDE "../AshOS/AshOS_LC3b.ah"

ORIGIN

SEGMENT
rti
	trap getc
	lea r5, DataSegment
	ldr r0, r5, Char
	trap out
	lea r0, String
	trap puts
	trap in
	trap 4x35
	trap halt

DataSegment:
SEGMENT
	String: data1[] "Hello World\n\0"
	ALIGN 2
	Char: data2 'A'
text: