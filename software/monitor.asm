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

	include "avr.inc"

	org	monitor

MONITORSTART:
	ld	sp,MONITORTOP
	call	print
	db	13,10
	db	"Z","-","O","n","e"," ","m","o","n","i","t","o","r"," ","v","e","r","s","i","o","n"," ","2",".","0",13,10
	db	"(","C",")"," ","M","a","d","i","s"," ","K","a","a","l"
	db	" ","2","0","1","7",",","2","0","1","8",0

loop:	call	getline
	call	print
	db	13,10,0
	ld	hl,linebuf
	call	skipspace
	call	ucase
	push	af
	call	skipword
	ld	a,(hl)
	call	isxdigit
	jr	c,loop3 ; there is a parameter
	ld	hl,(defaultparm)
	ld	d,0
	ld	e,0
	ld	c,0
	ld	b,0
	jr	loop2
loop3:	call	gethex ; HL adjusted, DEBC contain value
	push	de
	push	bc
	call	gethex ; get second value to BC
	push	bc
	ld	a,(hl)
	call	isxdigit ; end of input?
	jr	nc,loop1
	call	gethex ;3rd value in BC
	pop	de ;2nd value in de
	pop	hl ;1st value in hl
	inc	sp
	inc	sp ;discard first pushed de
	jr	loop2
	; 2 parameters
loop1:	pop	bc
	pop	hl
	pop	de
loop2:	pop	af
;
; now have command in A and parameters in HL,DE;BC as follows:
; if 1 parameter is entered then it is in HL
; if 3 parameters were entered then they are in HL,DE,BC
; if 2 parameters were entered then they are in DEHL,BC
; in case of 2 parameters, first can be 64 bit
;
	cp	"D"
	jp	z,dump
	cp	"U"
	jp	z,unassemble
	cp	"I"
	jp	z,portin
	cp	"O"
	jp	z,portout
	cp	"F"
	jp	z,fill
	cp	"C"
	jp	z,copy
	cp	"E"
	jp	z,edit
	cp	"G"
	jp	z,execute
	cp	"?"
	jp	z,help
	cp	"X"
	jp	z,registers
	cp	"R"
	jp	z,read
	cp	"W"
	jp	z,write
	cp	"S"
	jp	z,sdcard
	cp	"H"
	jp	z,recvfile
	jp	loop

; receive intel hex file from aux port at 9600 bps
;
recvfile:
	call	aux_on
	call	print
	db	"S","e","n","d"," ","I","n","t","e","l"
	db	" ","h","e","x"," ","@","9","6","0","0","b","p","s",13,10,0
rfile1:
	call	getlineraw
	call	print
	db	13,10,0
	ld	hl,linebuf
	call	skipspace
	ld	a,(hl)
	or	a
	jr	z,rfile1
	call	unihex
	jr	c,rfile1
	call	aux_off
	jp	loop

; disassemble next 20 instructions
;
unassemble:
	ld	c,20
una1:	push	bc
	call	decodeinstr
	ld	a,10
	call	putc
	pop	bc
	dec	c
	jr	nz,una1
	ld	(defaultparm),hl
	jp	loop

; re-detect SD card
;
sdcard:	ld	a,4
	out	(A_SDC),a
sdca1:	in	a,(A_SDC)
	and	0x80
	jr	z,sdca1
	call	print
	db	"T","y","p","e",":",0
	in	a,(A_SDS)
	call	phex
	cp	2
	jp	c,loop
	ld	a,3
	out	(A_SDC),a
	call	print
	db	13,10,"S","i","z","e",":",0
	in	a,(A_SD3)
	call	phex
	in	a,(A_SD2)
	call	phex
	in	a,(A_SD1)
	call	phex
	in	a,(A_SD0)
	call	phex
	call	print
	db	13,10,0
	jp	loop

; print registers
registers:
	call	printreg
	jp	loop

; read 512 byte sector from card
;
read:	call	readsector

diskres: or	a
	jp	z,loop
	push	af
	call	print
	db	"E","r","r"," ",0
	pop	af
	call	phex
	jp	loop

; write 512 byte sector to card
;
write:	call	writesector
	jr	diskres


; execute code from address, if called code returns
; then saves registers and comes back to monitor
;
execute:push	hl	; copy target adr to bc
	pop	bc
	ld	hl,0
	add	hl,sp   ; copy current stack pointer to de
	push	hl
	pop	de
	ld	hl,(r_sp) ; switch to user stack
	ld	sp,hl
	push	de	; store bios stack pointer
	ld	de,execute1
	push	de ; in case the it returns
	push	bc ; address where to go
	ld	hl,(r_bc)
	push	hl
	pop	bc
	ld	de,(r_de)
	push	hl
	pop	de
	ld	hl,(r_af)
	push	hl
	pop	af
	ld	hl,(r_hl)
	ret		; jump to target

; the target code returned, switch back to bios stack and
; store registers
execute1:
	ld	(r_hl),hl
	push	af
	pop	hl
	ld	(r_af),hl
	ld	hl,0
	add	hl,sp
	inc	hl
	inc	hl
	ld	(r_sp),hl
	pop	hl	; get bios stack point back
	ld	sp,hl
	push	bc
	pop	hl
	ld	(r_bc),hl
	push	de
	pop	hl
	ld	(r_de),hl
	call	print
	db	13,10,0
	call	printreg
	jp	loop

; print stored registers
printreg:
	call	print
	db	"A","F",":",0
	ld	hl,(r_af)
	call	phex16
	call	print
	db	" ","B","C",":",0
	ld	hl,(r_bc)
	call	phex16
	call	print
	db	" ","D","E",":",0
	ld	hl,(r_de)
	call	phex16
	call	print
	db	" ","H","L",":",0
	ld	hl,(r_hl)
	call	phex16
	call	print
	db	" ","S","P",":",0
	ld	hl,(r_sp)
	call	phex16
	ret

; copy memory contents
;
copy:	ldir
	jp	loop

; fill memory with constant value
;
fill:	ld	a,c
	ld	(hl),a
	ld	a,h
	cp	d
	jr	c,fill1
	ld	a,l
	cp	e
	jp	nc,loop
fill1:	inc	hl
	jr	fill

; read and print value from I/O port
;
portin:
	ld	c,l
	in	a,(c)
	call	phex
	jp	loop

; write value to I/O port
;
portout:
	ld	a,c
	ld	c,l
	out	(c),a
	jp	loop

; edit memory contents
;
edit:	call	phex16
	push	hl
	call	print
	db	" ",0
	pop	hl
	ld	a,(hl)
	call	phex
	push	hl
	call	print
	db	"-",0
	call	getlineraw
	ld	hl,linebuf
	call	skipspace
	push	af
	call	gethex ; get value to bc
	pop	af
	pop	hl
	and	a
	jp	z,loop ; empty string terminates input
	ld	(hl),c
	inc	hl
	push	hl
	call	print
	db	13,10,0
	pop	hl
	jr	edit

; hex and ascii dump 256 bytes of memory
;
dump:	ld	b,1
	ld	c,0
	push	hl
	add	hl,bc
	ld	(defaultparm),hl
	pop	hl
	ld	b,0
dump1:	push	hl
	call	phex16
	push	hl
	call	print
	db	" ",0
	pop	hl
	ld	c,0
dump2:	ld	a,(hl)
	inc	hl
	call	phex
	push	hl
	call	print
	db	" ",0
	pop	hl
	inc	c
	ld	a,c
	cp	16
	jr	c,dump2
	pop	hl
	ld	c,0
dump4:	ld	a,(hl)
	inc	hl
	cp	" "
	jr	c,dump5
	cp	127
	jr	c,dump3
dump5:	ld	a,"."
dump3:	out	(A_COND),a
	inc	c
	ld	a,c
	cp	16
	jr	c,dump4
	push	hl
	call	print
	db	13,10,0
	pop	hl
	inc	b
	ld	a,b
	cp	16
	jr	c,dump1
	jp	loop

; show monitor command help
;
help:	call	print
	mdat	"monitorhelp.txt"
	db	0
	jp	loop

	include "disasm.inc"

; get hex value from string at HL to DEBC
; on return HL points past delimiter or at string end
; any non-hexdigit character works as delimiter
gethex:
	ld	bc,0
	ld	de,0
gh2:	ld	a,(hl)
	call	isxdigit
	jr	nc,gh1
	push	hl
	push	af
	ld	a,4
	; now need to shift DEBC 4 bits to left
gh3:	ld	h,b
	ld	l,c
	add	hl,hl ;*2, bit15 to C
	ld	b,h
	ld	c,l
	ld	h,d
	ld	l,e
	adc	hl,de
	ld	d,h
	ld	e,l
	dec	a
	jr	nz,gh3
	pop	af
	or	c
	ld	c,a ; add new digit
	;
	pop	hl
	inc	hl
	jr	gh2
gh1:	ld	a,(hl)
	and	a
	ret	z
	inc	hl
	ld	a,(hl)
	call	isxdigit
	ret	c
	jr	gh1

; check if character in A is hex digit, return C set if it is
; and then A will be converted to binary value
;
isxdigit:
	call	ucase
	cp	"0"
	jr	c,isxd1
	cp	"9"+1
	jr	c,xtobin
	cp	"F"+1
	jr	nc,isxd1
	cp	"A"
	jr	nc,xtobin
isxd1:	scf
	ccf
	ret

; convert uppercase hex character in A to binary
; returns with C set
;
xtobin:	sub	"0"
	cp	10
	ret	c
	sub	7
	scf
	ret

; unhexlify two characters at HL, return with binary
; in A, HL=HL+2, C=C+A
;
unhex:	push	bc
	ld	a,(hl)
	call	isxdigit
	jr	nc,uhex1
	inc	hl
	add	a,a
	add	a,a
	add	a,a
	add	a,a
	ld	c,a
	ld	a,(hl)
	call	isxdigit
	jr	nc,uhex1
	inc	hl
	or	c
	pop	bc
	push	af
	add	a,c
	ld	c,a
	pop	af
	scf
	ret
uhex1:	sub	a
	pop	bc
	ret	

; parse a line of intel hex file at HL
;
unihex:
	ld	a,":"
	cp	(hl)
	jr	nz,unierr
	inc	hl
	ld	c,0		; initialize checksum
	call	unhex
	jr	nc,unierr
	ld	b,a		; bytecount
	call	unhex
	jr	nc,unierr
	ld	d,a		; adr hi
	call	unhex
	jr	nc,unierr
	ld	e,a		; adr low
	call	unhex		; get type
	jr	nc,unierr
	cp	0
	jr	z,unidata	; data record
	cp	1
	jr	z,unidone	; end marker
	cp	2
	jr	z,uniexts	; extended segment adr
	cp	4
	jr	z,uniextl	; extended linear address
unicont:
	scf
	ret			; return carry set for all
				; records except end marker

unierr:
	call	print
	db	"I","n","v","a","l","i","d"," ","d","a","t","a",0
	jr	unidone

uniexts:
uniextl:
	call	print
	db	"C","a","n","n","o","t"," ","h","a","n","d","l","e"," "
	db	"e","x","t","e","n","d","e","d"," ","a","d","r",0
unidone:
	scf
	ccf
	ret			; end marker or error, return carry clear

unidata:
	call	unhex
	jr	nc,unierr
	ld	(de),a
	inc	de
	djnz	unidata
	call	unhex
	jr	nc,unierr
	ld	a,c
	or	a
	jr	z,unicont
	call	print
	db	"I","n","v","a","l","i","d"
	db	" ","c","h","e","c","k","s","u","m",0
	jr	unidone


; convert char in A to upper case
;
ucase:	cp	"a"
	ret	c
	cp	"z"+1
	ret	nc
	sub	0x20 ; convert to uppercase
	ret

; skip over spaces in string at HL
; returns first non-space char in A
skipspace:
	ld	a,(hl)
	and	a
	ret	z
	cp	" "+1
	ret	nc
	inc	hl
	jr	skipspace

; skip over word until space is found, and then over space too
;
skipword:
	ld	a,(hl)
	and	a
	ret	z
	cp	" "+1
	jr	c,skipspace
	inc	hl
	jr	skipword

aux_on:	ld	a,0x85
	out	(A_AUXS),a
auxon1:	in	a,(A_AUXS)
	and	0x40
	ret	z
	in	a,(A_AUXD)
	jr	auxon1
	
aux_off: ld	a,0x05
	out	(A_AUXS),a
	ret

; display a prompt and collect line of text into linebuf
;

getline:call	print
	db	13,10,"*"," ",0
getlineraw:
	ld	c,0
	ld	hl,linebuf
gl1:	in	a,(A_CONS)
	and	1
	jr	nz,gl2
	;check if aux input enabled and data ready
	in	a,(A_AUXS)
	and	0xc0 ; enabled and rxrdy?
	cp	0xc0
	jr	nz,gl1
	in	a,(A_AUXD)
	jr	gl3
gl2:	in	a,(A_COND)
gl3:	cp	8
	jr	z,glbs
	cp	13
	jr	z,glcr
	cp	10
	jr	z,glcr
	cp	" "
	jr	c,gl1
	cp	127
	jr	nc,gl1
	ld	b,a
	ld	a,126 ; check if room in buffer
	cp	c
	jr	c,gl1
	ld	(hl),b ; store and output
	inc	hl
	inc	c
	ld	a,b
	out	(A_COND),a
	jr	gl1
glcr:	sub	a
	ld	(hl),a
	ret
glbs:	sub	a
	cp	c
	jr	z,gl1
	dec	hl
	dec	c
	push	hl
	call	print
	db	8," ",8,0
	pop	hl
	jr	gl1

; print HL as hex
phex16:
	ld	a,h
	call	phex
	ld	a,l
; print A as hex
phex:	push	af
	rra
	rra
	rra
	rra
	call	pdigit
	pop	af
pdigit:	and	0x0f
	add	"0"
	cp	"9"+1
	jr	c,pd1
	add	7
pd1:	out	(A_COND),a
	ret

putc:	out	(A_COND),a
	cp	10
	ret	nz
	ld	a,13
	out	(A_COND),a
	ret

; print zero terminated string at HL
pstr:	ld	a,(hl)
	and	a
	ret	z
	call	putc
	inc	hl
	jr	pstr

; print zero terminated string following the call
print:	pop	hl
	call	pstr
	inc	hl
	jp	(hl)

; reading and writing is done by first setting sector number, then writing
; command (0 for read, 1 for write) to A_SDC. for write the next step is to
; write 512 bytes to da data register. After that A_SDC needs to be
; read until its high bit becomes set to indicate operation has completed.
; zero value read from A_SDS means that operation was successful, and for
; read operation 512 data bytes may now be read from A_SDD.
;

; write data from BC to physical sector given by DEHL
; returns result code in A. 0 means success
;
writesector:
	call	setsadr
	ld	a,1
	out	(A_SDC),a
	ld	h,b
	ld	l,c
	ld	b,0
	ld	c,A_SDD
	otir		; write 256 data bytes
	otir		; and second half
ws1:	in	a,(A_SDC)
	and	0x80
	jr	z,ws1   ; not done yet
	in	a,(A_SDS)
	and	a
	ret

; read physical sector given by DEHL to address given by BC
; returns result code in A, if result is 0 then data is read
;
readsector:
	call	setsadr
	xor	a
	out	(A_SDC),a
rs1:	in	a,(A_SDC) ; poll for command completed
	and	0x80
	jr	z,rs1
	in	a,(A_SDS) ; get status
	or	a
	ret	nz
	ld	h,b
	ld	l,c
	ld	b,0
	ld	c,A_SDD
	inir		  ;read 256 bytes
	inir		  ;and again
	and	a
	ret

setsadr: ld	a,d
	out	(A_SD3),a
	ld	a,e
	out	(A_SD2),a
	ld	a,h
	out	(A_SD1),a
	ld	a,l
	out	(A_SD0),a
	ret

r_af:	dw	0
r_bc:	dw	0
r_de:	dw	0
r_hl:	dw	0
r_sp:	dw	0x0080 ; call stack is cp/m stack
defaultparm: dw 0

linebuf:ds	128
	ds	32
MONITORTOP:
;

	ds	0xffff-$
	db	0x55
