doh "hello\'\\\" #-1 data2 10 \\world\"".orig x3000;firstline
9doh "hello\'\\\" #-1 data2 10 \\world\""blahx1 add r3, r2, #-1;secondline
doh "hello\'\\\" #-1 data2 10 \\world\"".fill b-010;thirdline
jsrr "hello\'\\\" #-1 data2 10 \\world\"" doh .blkw 	x-af;fourthline
doh^ "hello\'\\\" #-1 data2 10 \\world\"".stringz "hello\'\\\" #-1 .fill 10 \\world\"";fifthline
seg "hello\'\\\" #-1 data2 10 \\world\"".end;sixthline
