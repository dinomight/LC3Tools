INCLUDE "../AshOS/AshOS_LC3.ah"

ORIGIN 4x3000
SEGMENT

; load the handlers
ld r0, pprivilege[0]
ld r1, pprivilege[1]
str r0, r1, 0
ld r0, pillegalopcode[0]
ld r1, pillegalopcode[1]
str r0, r1, 0
ld r0, pkeyboard[0]
ld r1, pkeyboard[1]
str r0, r1, 0

add r1, r1, 0
data2 4xF4F4
rti
halt:
	add r1, r1, 0
	br halt

privilege:
	add r1, r1, 1
	rti

illegalopcode:
	add r1, r1, -1
	rti

keyboard:
	add r1, r1, 1
	rti

pprivilege: data2[] privilege, 4x100
pillegalopcode: data2[] illegalopcode, 4x101 
pkeyboard: data2[] keyboard, 4x180
