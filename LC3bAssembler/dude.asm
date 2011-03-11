origin
segment

real2 __version__
real2 __version_3__
data1 [] __file__
data1[] __line__, '\x80'
data1 [] __time__
data1 [] __date__

MACRO Inc(Reg)
ADD Reg, Reg, 1
END

ALIGN 2
Inc(R1)
Inc(R2)

DEFINE One Two
DEFINE Four RET
MACRO Two(A, B) A B END
MACRO Three(A) A END
One(Three,()Four)

three(three)(four)

STRUCTDEF Row	  X: DATA1[16] ?		END
STRUCTDEF Frame  Y: STRUCT Row[16] ?	END
Z: STRUCT Frame[16] ?
DATA2 Z
DATA2 Z$size
DATA2 Frame.Y$size
DATA2 Row.X$size
DATA2 Z[Z$LEN]

IFDEF wah
	macro eet()
		define whoa
	end
	eet()
	structdef asf
		first:	data1 ?
		second:	data2 ?
		third: struct lsk[] {1, 2}, ?, ?
	end

	IFNDEF wee
		define barf ?
		IFDEF barf
			structdef lsk
				first: data1 3
				second: data2 4
			end
;		ELSE
;			structdef lsk
;			first: data4 3
;			second: data8 4
;			end
		END
;	ELSE
		structdef lsk
			first: data2 3
			second: data1 4
		end
	END
	DEFINE wee ?

	array:
	data2[4] ?
	segment

	struct lsk {5, 6}
	blah:
	struct asf[] {"hello word wow this is long yaaaaaaaayyyyyyyy"}, {"hello word wow this is long yaaaaaaaayyyyyyyy"}
	data1 ?
	br blah[1].second$size

	data2 array$len
	data2 array[array$len]
END