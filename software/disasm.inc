;  The MIT License (MIT)
; 
;  Copyright (c) 2018 Madis Kaal <mast@nomad.ee>
; 
;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
; 
;  The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
; 
;  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.
;

; decode one Z80 instruction at HL
; returns HL adjusted past the decoded instruction
;
decodeinstr:
	push	hl
	call	phex16
	ld	a," "
	call	putc
	pop	hl
	ld	a,(hl)
	call	phex
	ld	a," "
	call	putc
	ld	a,(hl)
	ld	d,h
	ld	e,l
	inc	hl
	push	hl
	cp	0x40
	jr	c,dis1
	cp	0x80
	jr	nc,dis1
	;0x40..0x7f are loads, with exception of HALT
	cp	0x76
	jr	nz,dis2
	ld	a,3
	call	btab
	call	print
	db	"H","A","L","T",0
	pop	hl
	ret

dis2:	push	af
	ld	a,3
	call	btab
	call	print
	db	"L","D"," ",0
	pop	af
	push	af
	rra
	rra
	rra
	call	pregname
	ld	a,","
	call	putc
	pop	af
	call	pregname
	pop	hl
	ret

dis1:	cp	0x80
	jr	c,dis5
	cp	0xc0
	jr	nc,dis5
	; 0x80..0xbf is arithmetic block
	push	af
	ld	a,3
	call	btab
	pop	af
	push	af
	rra
	rra
	rra
	and	0x07
	ld	c,a
	ld	hl,tb2
dis6:	ld	a,c
	or	a
	jr	z,dis7
dis8:	ld	a,(hl)
	inc	hl
	or	a
	jr	nz,dis8
	dec	c
	jr	dis6
dis7:	call	pstr
	pop	af
	call	pregname
	pop	hl
	ret

; two major blocks done, now check for remaining simple one-byte
; instructions
dis5:	ld	hl,tb3
	call	ilookup
	jr	nz,dis9
	ld	a,3
	call	btab
	inc	hl
	call	pstr
	pop	hl
	ret

; check for 3-byte instructions with immediate value in the end
dis9:	ld	a,(de)
	ld	hl,tb4
	call	ilookup
	jr	nz,dis10
	inc	de
	call	twobp
	inc	hl
	call	pstr
	call	twobv
	pop	hl
	inc	hl
	inc	hl
	ret

;check for 2 byte instructions where 1 byte immediate value follows
dis10:	ld	a,(de)
	ld	hl,tb5
	call	ilookup
	jr	nz,dis11
	inc	de
	call	onebp
	inc	hl
	call	pstr
	ld	a,(de)
	call	phex
	pop	hl
	inc	hl
	ret

; check relative jumps
dis11:
	ld	a,(de)
	ld	hl,tb6
	call	ilookup
	jr	nz,dis12
	inc	de
	call	onebp
	inc	hl
	call	pstr
	ld	a,(de)
	ld	e,a
	ld	d,0
	and	0x80
	jr	z,dis111
	ld	d,0xff
dis111:	pop	hl
	inc	hl
	push	hl
	add	hl,de
	call	phex16
	pop	hl
	ret

; 0x22 ld (**),hl
dis12:	ld	a,(de)
	cp	0x22
	jr	nz,dis13
	inc	de
	call	twobp
	call	print
dis121:	db	"L","D"," ","(",0
	call	twobv
	call	print
	db	")",",","H","L",0
dis122:	pop	hl
	inc	hl
	inc	hl
	ret

; 0x23 ld (**),a
dis13:	cp	0x32
	jr	nz,dis14
	inc	de
	call	twobp
	ld	hl,dis121
	call	pstr
	call	twobv
	call	print
	db	")",",","A",0
	jr	dis122

; 0x2a ld hl,(**)
dis14:	cp	0x2a
	jr	nz,dis15
	inc	de
	call	twobp
	call	print
	db	"L","D"," ","H","L",0
dis141:	call	print
	db	",","(",0
	call	twobv
	ld	a,")"
	call	putc
	jr	dis122

; 0x3a ld a,(**)
dis15:	cp	0x3a
	jr	nz,dis16
	inc	de
	call	twobp
	call	print
	db	"L","D"," ","A",0
	jr	dis141

; 0xd3 out (*),a
dis16:	cp	0xd3
	jr	nz,dis17
	inc	de
	call	onebp
	call	print
	db	"O","U","T"," ",0
	call	onebv
	call	print
	db	",","A",0
	pop	hl
	inc	hl
	ret

; 0xdb in a,(*)
dis17:	cp	0xdb
	jr	nz,dis18
	inc	de
	call	onebp
	call	print
	db	"I","N"," ","A",",",0
	call	onebv
	pop	hl
	inc	hl
	ret

; done with main page, check for extended instructions
dis18:	inc	de
	cp	0xed
	jr	nz,dis19
	ld	a,(de)
	cp	0x40
	jr	c,dis181
	cp	0x80
	jr	nc,dis181
	and	15
	cp	3
	jr	z,dis182 ; columns x3,xb are 16 bit register moves
	cp	11
	jr	z,dis183

dis181:	ld	a,(de)
	ld	hl,tb7 ; simple two byte instructions
	call	ilookup
	jp	nz,edunknown
	push	hl
	call	onebp
	pop	hl
	inc	hl
	call	pstr
	pop	hl
	inc	hl
	ret

dis182: ; ld (**),rr
	call	threebp
	call	print
	db	"L","D"," ","(",0
	call	twobv
	dec	de
	call	print
	db	")",",",0
	ld	a,(de)
	call	pregp
dis1821:
	pop	hl
	inc	hl
	inc	hl
	inc	hl
	ret

dis183: ; ld rr,(**)
	ld	a,(de)
	push	af
	call	threebp
	call	print
	db	"L","D"," ",0
	pop	af
	call	pregp
	call	print
	db	",","(",0
	ld	a,(de)
	call	phex
	dec	de
	ld	a,(de)
	call	phex
	ld	a,")"
	call	putc
	jr	dis1821

; check for bit operations page (CB)
dis19:	cp	0xcb
	jp	nz,dis20
	call	onebp
	ld	a,(de)
	call	bitinstr
	ld	a,(de)
	cp	0x40
	jr	c,dis191
	call	bitnum
dis191:	ld	a,(de)
	call	pregname
	pop	hl
	inc	hl
	ret

; check for IX instructions (DD page)
dis20:	cp	0xdd
	jp	nz,dis21
	ld	C,"X"
	jr	dis201

;check for IY instructions (FD page)
dis21:	cp	0xfd
	jp	nz,unknown
	ld	C,"Y"

;decode index page instructions, index register name (X or Y) is
;in C register
;
dis201:	ld	a,(de)
	cp	0x36 ; LD (Ix+*),*
	jr	z,dis2011
	cp	0xcb ; index page bit ops
	jp	z,dis2012
	; decode using tables
	ld	hl,tb9 ; 2 byte instructions
        call    ilookup
        jr      nz,dis2013
	; yes, 2 byte
        call    onebp
        inc     hl
        call    instrexp
        pop     hl
        inc     hl
	ret
dis2013:
	ld	a,(de)
	ld	hl,tb10 ; 3 byte instructions
	call	ilookup
	jr	nz,dis2014
	call	twobp
	inc	hl
	call	instrexp
	pop	hl
	inc	hl
	inc	hl
	ret
dis2014:
	ld	a,(de)
	ld 	hl,tb11 ; 4 byte instructions
	call 	ilookup
	jr	nz,dis20141 
	call 	threebp 
	inc 	hl 
	call	instrexp
	pop 	hl 
	inc 	hl 
	inc 	hl 
	inc 	hl 
	ret

; handle index page unknown opcodes as if they are all
; 2 bytes long
dis20141:
	call	onebp
	call	print
	db	"?","?","?",0
	pop	hl
	inc	hl
	ret
	
dis2011: ; an oddball 4 byte  LD (Ix+*),*
	call	threebp
	call	print
	db	"L","D"," ","(","I",0
	ld	a,c
	call	putc
	ld	a,"+"
	call	putc
	inc	de
	ld	a,(de)
	call	phex
	ld	a,")"
	call	putc
	ld	a,","
	call	putc
	inc	de
	ld	a,(de)
	call	phex
	pop	hl
	inc	hl
	inc	hl
	inc	hl
	ret

; index register bit ops
; C contains "X" or "Y", DE points to 2nd byte in instr
; FD CB ii oo
;
dis2012:
	inc	de
	inc	de
	ld	a,(de)
	dec	de
	dec	de
	and	15
	cp	0x06
	jr	z,dis20122
	cp	0x0e
	jr	z,dis20122
	call	threebp
	call	print
	db	"?","?","?",0
	pop	hl
	inc	hl
	inc	hl
	inc	hl
	ret
	
dis20122:
	push	bc
	call	threebp
	inc	de
	inc	de
	ld	a,(de)
	call	bitinstr
	ld	a,(de)
	dec	de
	cp	0x40
	jr	c,dis20121
	call	bitnum
dis20121:
	call	print
	db	"(","I",0
	pop	bc
	ld	a,c
	call	putc
	ld	a,"+"
	call	putc
	ld	a,(de)
	call	phex
	ld	a,")"
	call	putc
	pop	hl
	inc	hl
	inc	hl
	inc	hl
	ret
		

; catch-all
unknown:
	ld	a,3
unk1:	call	btab
	call	print
	db	"?","?","?",0
	pop	hl
	ret

; ED prefix catch-all
edunknown:
	ld	a,(de)
	pop	hl
	inc	hl
	push	hl
	call	phex
	ld	a," "
	call	putc
	ld	a,2
	jr	unk1

onebv:	ld	a,"("
	call	putc
	ld	a,(de)
	call	phex
	ld	a,")"
	call	putc
	ret

twobv:	ld	a,(de)
	call	phex
	dec	de
	ld	a,(de)
	call	phex
	ret

; prints one byte at (de) + space
; increments de by one
;
bhex:	ld	a,(de)
	inc	de
	call	phex
	ld	a," "
	call	putc
	ret

; II xx __ __
; first instruction byte already displayed, shows one byte
; at (de), and blanks out following two byte spaces
onebp:	push	de
	call	bhex
	pop	de
	ld	a,2
	call	btab
	ret

; II xx aa __
; first instruction byte already displayed, shows two
; bytes (de),(de+1) and blanks out following one byte space
twobp:	call	bhex
	call	bhex
	dec	de
	ld	a,1
	call	btab
	ret

; II xx aa bb
; first instruction byte already displayed, shows 3 bytes
; (de),(de+1),(de+2)
;
threebp:
	call	bhex
	call	bhex
	call	bhex
	dec	de
	ret


; print bit number followed by comma
bitnum:	rra
	rra
	rra
	and	7
	add	"0"
	call	putc
	ld	a,","
	call	putc
	ret

; print bit manipulation instruction name followed by space
;
bitinstr:
	cp	0xc0
	jr	c,bii1
;set bit,r
	call	print
	db	"S","E","T"," ",0
	ret
bii1:	cp	0x80
	jr	c,bii2
;res bit,r
	call	print
	db	"R","E","S"," ",0
	ret
bii2:	cp	0x40
	jr	c,bii3
;bit bit,r
	call	print
	db	"B","I","T"," ",0
	ret
bii3:	rra
	and	0x1c
	ld	c,a
	ld	b,0
	ld	hl,tb8
	add	hl,bc
	call	pstr
	ld	a," "
	call	putc
	ret

; print register pair name
; for ED page 16 bit moves
pregp:	rra
	rra
	rra
	rra
	and	15
	sub	4
	ld	c,a
	add	a
	add	c
	ld	c,a
	ld	b,0
	ld	hl,rpt
	add	hl,bc
	call	pstr
	ret

; print A*3 space characters
;
btab:	push	bc
	ld	c,a
	add	a
	add	c
	ld	c,a
	ld	a," "
btab1:	call	putc
	dec	c
	jr	nz,btab1
	pop	bc
	ret

; instruction lookup from table
; start address in table in HL
; instruction in A.
;
; table is a list of "instruction code, mnemonics, zero byte"
; entries, 0x00 instruction code from second element onwards
; is table end indicator
;
ilookup: cp	(hl)
	ret	z
	inc	hl
	push	af
ilk1:	ld	a,(hl)
	inc	hl
	or	a
	jr	nz,ilk1
	ld	a,(hl)
	or	a
	jr	z,ilk2
	pop	af
	jr	ilookup
ilk2:	pop	af
	ld	a,0xff
	or	a
	ret

; expand instruction template in form "L","D"," ","A",",","(","I","?","+","%",")"
; HL points to template. special characters are replaced as follows:
; ? - content of register C ("X" or "Y")
; % - 8-bit hex of byte following the instruction at (DE)
; # - 16-bit hext of word following the instruction at (DE)
;
iexp3:	call	putc
instrexp:
	ld	a,(hl)
	inc	hl
	or	a
	ret	z
	cp	"?"
	jr	nz,iexp1
	ld	a,c
	call	putc
	jr	instrexp
iexp1:	cp	"%"
	jr	nz,iexp2
	inc	de
	ld	a,(de)
	call	phex
	dec	de
	jr	instrexp
iexp2:	cp	"#"
	jr	nz,iexp3
	inc	de
	inc	de
	ld	a,(de)
	dec	de
	call	phex
	ld	a,(de)
	dec	de
	call	phex
	jr	instrexp

tb1:	db	"B","C","D","E","H","L"," ","A"
tb2:	db	"A","D","D"," ","A",",",0
	db	"A","D","C"," ","A",",",0
	db	"S","U","B"," ",0
	db	"S","B","C"," ","A",",",0
	db	"A","N","D"," ",0
	db	"X","O","R"," ",0
	db	"O","R"," ",0
	db	"C","P"," ",0

tb3:	db	0x00,"N","O","P",0
	db	0x02,"L","D"," ","(","B","C",")",",","A",0
	db	0x03,"I","N","C"," ","B","C",0
	db	0x04,"I","N","C"," ","B",0
	db	0x05,"D","E","C"," ","B",0
	db	0x07,"R","L","C","A",0
	db	0x08,"E","X"," ","A","F",",","A","F","'",0
	db	0x09,"A","D","D"," ","H","L",",","B","C",0
	db	0x0a,"L","D"," ","A",",","(","B","C",")",0
	db	0x0b,"D","E","C"," ","B","C",0
	db	0x0c,"I","N","C"," ","C",0
	db	0x0d,"D","E","C"," ","C",0
	db	0x0f,"R","R","C","A",0
	db	0x12,"L","D"," ","(","D","E",")",",","A",0
	db	0x13,"I","N","C"," ","D","E",0
	db	0x14,"I","N","C"," ","D",0
	db	0x15,"D","E","C"," ","D",0
	db	0x17,"R","L","A",0
	db	0x19,"A","D","D"," ","H","L",",","D","E",0
	db	0x1a,"L","D"," ","A",",","(","D","E",")",0
	db	0x1b,"D","E","C"," ","D","E",0
	db	0x1c,"I","N","C"," ","E",0
	db	0x1d,"D","E","C"," ","E",0
	db	0x1f,"R","R","A",0
	db	0x23,"I","N","C"," ","H","L",0
	db	0x24,"I","N","C"," ","H",0
	db	0x25,"D","E","C"," ","H",0
	db	0x27,"D","A","A",0
	db	0x29,"A","D","D"," ","H","L",",","H","L",0
	db	0x2b,"D","E","C"," ","H","L",0
	db	0x2c,"I","N","C"," ","L",0
	db	0x2d,"D","E","C"," ","L",0
	db	0x2f,"C","P","L",0
	db	0x33,"I","N","C"," ","S","P",0
	db	0x34,"I","N","C"," ","(","H","L",")",0
	db	0x35,"D","E","C"," ","(","H","L",")",0
	db	0x37,"S","C","F",0
	db	0x39,"A","D","D"," ","H","L",",","S","P",0
	db	0x3B,"D","E","C"," ","S","P",0
	db	0x3C,"I","N","C"," ","A",0
	db	0x3d,"D","E","C"," ","A",0
	db	0x3f,"C","C","F",0
	db	0xc0,"R","E","T"," ","N","Z",0
	db	0xc1,"P","O","P"," ","B","C",0
	db	0xc5,"P","U","S","H"," ","B","C",0
	db	0xc7,"R","S","T"," ","0","0",0
	db	0xc8,"R","E","T"," ","Z",0
	db	0xc9,"R","E","T",0
	db	0xcf,"R","S","T"," ","0","8",0
	db	0xd0,"R","E","T"," ","N","C",0
	db	0xd1,"P","O","P"," ","D","E",0
	db	0xd5,"P","U","S","H"," ","D","E",0
	db	0xd7,"R","S","T"," ","1","0",0
	db	0xd8,"R","E","T"," ","C",0
	db	0xd9,"E","X","X",0
	db	0xdf,"R","S","T"," ","1","8",0
	db	0xe0,"R","E","T"," ","P","O",0
	db	0xe1,"P","O","P"," ","H","L",0
	db	0xe3,"E","X"," ","(","S","P",")",",","H","L",0
	db	0xe5,"P","U","S","H"," ","H","L",0
	db	0xe7,"R","S","T"," ","2","0",0
	db	0xe8,"R","E","T"," ","P","E",0
	db	0xe9,"J","P"," ","(","H","L",")",0
	db	0xeb,"E","X"," ","D","E",",","H","L",0
	db	0xef,"R","S","T"," ","2","8",0
	db	0xf0,"R","E","T"," ","P",0
	db	0xf1,"P","O","P"," ","A","F",0
	db	0xf3,"D","I",0
	db	0xf5,"P","U","S","H"," ","A","F",0
	db	0xf7,"R","S","T"," ","3","0",0
	db	0xf8,"R","E","T"," ","M",0
	db	0xf9,"L","D"," ","S","P",",","H","L",0
	db	0xfb,"E","I",0
	db	0xff,"R","S","T"," ","3","8",0
	db	0

tb4:	db	0x01,"L","D"," ","B","C",",",0
	db	0x11,"L","D"," ","D","E",",",0
	db	0x21,"L","D"," ","H","L",",",0
	db	0x31,"L","D"," ","S","P",",",0
	db	0xc2,"J","P"," ","N","Z",",",0
	db	0xc3,"J","P"," ",0
	db	0xc4,"C","A","L","L"," ","N","Z",",",0
	db	0xca,"J","P"," ","Z",",",0
	db	0xcc,"C","A","L","L"," ","Z",",",0
	db	0xcd,"C","A","L","L"," ",0
	db	0xd2,"J","P"," ","N","C",",",0
	db	0xd4,"C","A","L","L"," ","N","C",",",0
	db	0xda,"J","P"," ","C",",",0
	db	0xdc,"C","A","L","L"," ","C",",",0
	db	0xe2,"J","P"," ","P","O",",",0
	db	0xe4,"C","A","L","L"," ","P","O",",",0
	db	0xea,"J","P"," ","P","E",",",0
	db	0xec,"C","A","L","L"," ","P","E",",",0
	db	0xf2,"J","P"," ","P",",",0
	db	0xf4,"C","A","L","L"," ","P",",",0
	db	0xfa,"J","P"," ","M",",",0
	db	0xfc,"C","A","L","L"," ","M",",",0
	db	0

tb5:	db	0x06,"L","D"," ","B",",",0
	db	0x0e,"L","D"," ","C",",",0
	db	0x16,"L","D"," ","D",",",0
	db	0x1e,"L","D"," ","E",",",0
	db	0x26,"L","D"," ","H",",",0
	db	0x2e,"L","D"," ","L",",",0
	db	0x36,"L","D"," ","(","H","L",")",",",0
	db	0x3e,"L","D"," ","A",",",0
	db	0xc6,"A","D","D"," ","A",",",0
	db	0xce,"A","D","C"," ","A",",",0
	db	0xd6,"S","U","B"," ",0
	db	0xde,"S","B","C"," ","A",",",0
	db	0xe6,"A","N","D"," ",0
	db	0xee,"X","O","R"," ",0
	db	0xf6,"O","R"," ",0
	db	0xfe,"C","P"," ",0
	db	0
	
tb6:	db	0x10,"D","J","N","Z"," ",0
	db	0x18,"J","R"," ",0
	db	0x20,"J","R"," ","N","Z",",",0
	db	0x28,"J","R"," ","Z",",",0
	db	0x30,"J","R"," ","N","C",",",0
	db	0x38,"J","R"," ","C",",",0
	db	0	

tb7:	db	0x40,"I","N"," ","B",",","(","C",")",0
	db	0x41,"O","U","T"," ","(","C",")",",","B",0
	db	0x42,"S","B","C"," ","H","L",",","B","C",0
	db	0x44,"N","E","G",0
	db	0x45,"R","E","T","N",0
	db	0x46,"I","M"," ","0",0
	db	0x47,"L","D"," ","I",",","A",0
	db	0x48,"I","N"," ","C",",","(","C",")",0
	db	0x49,"O","U","T"," ","(","C",")",",","C",0
	db	0x4a,"A","D","C"," ","H","L",",","B","C",0
	db	0x4d,"R","E","T","I",0
	db	0x4f,"L","D"," ","R",",","A",0
	db	0x50,"I","N"," ","D",",","(","C",")",0
	db	0x51,"O","U","T"," ","(","C",")",",","D",0
	db	0x52,"S","B","C"," ","H","L",",","D","E",0
	db	0x56,"I","M"," ","1",0
	db	0x57,"L","D"," ","A",",","I",0
	db	0x58,"I","N"," ","E",",","(","C",")",0
	db	0x59,"O","U","T"," ","(","C",")",",","E",0
	db	0x5a,"A","D","C"," ","H","L",",","D","E",0
	db	0x5e,"I","M"," ","2",0
	db	0x5f,"L","D"," ","A",",","R",0
	db	0x60,"I","N"," ","H",",","(","C",")",0
	db	0x61,"O","U","T"," ","(","C",")",",","H",0
	db	0x62,"S","B","C"," ","H","L",",","H","L",0
	db	0x67,"R","R","D",0
	db	0x68,"I","N"," ","L",",","(","C",")",0
	db	0x69,"O","U","T"," ","(","C",")",",","L",0
	db	0x6a,"A","D","C"," ","H","L",",","H","L",0
	db	0x6f,"R","L","D",0
	db	0x72,"S","B","C"," ","H","L",",","S","P",0
	db	0x78,"I","N"," ","A",",","(","C",")",0
	db	0x79,"O","U","T"," ","(","C",")",",","A",0
	db	0x7a,"A","D","C"," ","H","L",",","S","P",0
	db	0xa0,"L","D","I",0
	db	0xa1,"C","P","I",0
	db	0xa2,"I","N","I",0
	db	0xa3,"O","U","T","I",0
	db	0xa8,"L","D","D",0
	db	0xa9,"C","P","D",0
	db	0xaa,"I","N","D",0
	db	0xab,"O","U","T","D",0
	db	0xb0,"L","D","I","R",0
	db	0xb1,"C","P","I","R",0
	db	0xb2,"I","N","I","R",0
	db	0xb3,"O","T","I","R",0
	db	0xb8,"L","D","D","R",0
	db	0xb9,"C","P","D","R",0
	db	0xba,"I","N","D","R",0
	db	0xbb,"O","T","D","R",0
	db	0

tb8:	db	"R","L","C",0
	db	"R","R","C",0
	db	"R","L",0,0
	db	"R","R",0,0
	db	"S","L","A",0
	db	"S","R","A",0
	db	"S","L","L",0
	db	"S","R","L",0

rpt:	db	"B","C",0,"D","E",0,"H","L",0,"S","P",0

; 2 byte index reg instructions
tb9:	db	0x23,"I","N","C"," ","I","?",0
	db	0x09,"A","D","D"," ","I","?",",","B","C",0
	db	0x19,"A","D","D"," ","I","?",",","D","E",0
	db	0x29,"A","D","D"," ","I","?",",","I","?",0
	db	0x2b,"D","E","C"," ","I","?",0
	db	0x39,"A","D","D"," ","I","?",",","S","P",0
	db	0xe1,"P","O","P"," ","I","?",0
	db	0xe3,"E","X"," ","(","S","P",")",",","I","?",0
	db	0xe5,"P","U","S","H"," ","I","?",0
	db	0xe9,"J","P"," ","(","I","?",")",0
	db	0xf9,"L","D"," ","S","P",",","I","?",0
	db	0
; 3 byte index reg instructions
tb10:	db	0x34,"I","N","C"," ","(","I","?","+","%",")",0
	db	0x35,"D","E","C"," ","(","I","?","+","%",")",0
	db	0x46,"L","D"," ","B",",","(","I","?","+","%",")",0
	db	0x4e,"L","D"," ","C",",","(","I","?","+","%",")",0
	db	0x56,"L","D"," ","D",",","(","I","?","+","%",")",0
	db	0x5e,"L","D"," ","E",",","(","I","?","+","%",")",0
	db	0x66,"L","D"," ","H",",","(","I","?","+","%",")",0
	db	0x6e,"L","D"," ","L",",","(","I","?","+","%",")",0
	db	0x70,"L","D"," ","(","I","?","+","%",")",",","B",0
	db	0x71,"L","D"," ","(","I","?","+","%",")",",","C",0
	db	0x72,"L","D"," ","(","I","?","+","%",")",",","D",0
	db	0x73,"L","D"," ","(","I","?","+","%",")",",","E",0
	db	0x74,"L","D"," ","(","I","?","+","%",")",",","H",0
	db	0x75,"L","D"," ","(","I","?","+","%",")",",","L",0
	db	0x77,"L","D"," ","(","I","?","+","%",")",",","A",0
	db	0x7e,"L","D"," ","A",",","(","I","?","+","%",")",0
	db	0x86,"A","D","D"," ","A",",","(","I","?","+","%",")",0
	db	0x8e,"A","D","C"," ","A",",","(","I","?","+","%",")",0
	db	0x96,"S","U","B"," ","(","I","?","+","%",")",0
	db	0x9e,"S","B","C"," ","A",",","(","I","?","+","%",")",0
	db	0xa6,"A","N","D"," ","(","I","?","+","%",")",0
	db	0xae,"X","O","R"," ","(","I","?","+","%",")",0
	db	0xb6,"O","R"," ","(","I","?","+","%",")",0
	db	0xbe,"C","P"," ","(","I","?","+","%",")",0
	db	0
;4 byte index register instructions
tb11:	db	0x21,"L","D"," ","I","?",",","#",0
	db	0x22,"L","D"," ","(","#",")",",","I","?",0
	db	0x2a,"L","D"," ","I","?",",","(","#",")",0
	db	0

; print name of 8 byte register, with register number in
; 3 lsb of A. also handles memory pointer (HL)
pregname:
	and	0x7
	ld	c,a
	ld	b,0
	ld	hl,tb1
	add	hl,bc
	ld	a,(hl)
	cp	" "
	jr	nz,dis3
	call	print
	db	"(","H","L",")",0
	jr	dis4
dis3:	call	putc
dis4:	ret

