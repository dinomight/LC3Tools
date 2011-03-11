.orig 0
add r0, r0, 1
add r1, r1, -1
add r2, r0, r1
brnp oops
add r0, r0, -2
and r0, r0, 6
and r1, r1, 5
and r2, r0, r1
brnz oops
not r2, r2
brzp oops

jsr foo
lea r6, foo
jsrr r6
trap foop

lea r6, loadstore
jmp r6

oops br oops

foop .fill foo
foo ret

loadstore
ld r3, blah
ldi r4, blahp
lea r5, blahp
ldr r6, r5, blahp[1]$baserel

st r3, wee
add r4, r4, r4
sti r4, weep
lea r5, weep
add r4, r4, r4
str r4, r5, 1

halt
br halt

blahp .fill blah
blah .fill 1
weep .fill wee
wee .blkw 1
