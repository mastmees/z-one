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

; this file can be used to assemble code that contains all documented z80
; instructions for testing disassembler

;
; 0x
	nop
	ld	bc,0x1122
	ld	(bc),a
	inc	bc
	inc	b
	dec	b
	ld	b,0x11
	rlca
	ex	af,af'
	add	hl,bc
	ld	a,(bc)
	dec	bc
	inc	c
	dec	c
	ld	c,0x11
	rrca

;1x
	djnz	$+2+0x11
	ld	de,0x1122
	ld	(de),a
	inc	de
	inc	d
	dec	d
	ld	d,0x11
	rla
	jr	$+2+0x11
	add	hl,de
	ld	a,(de)
	dec	de
	inc	e
	dec	e
	ld	e,0x11
	rra
;2x
	jr	nz,$+2+0x11
	ld	hl,0x1122
	ld	(0x1122),hl
	inc	hl
	inc	h
	dec	h
	ld	h,0x11
	daa
	jr	z,$+2+0x11
	add	hl,hl
	ld	hl,(0x1122)
	dec	hl
	inc	l
	dec	l
	ld	l,0x11
	cpl
;3x
	jr	nc,$+2+11
	ld	sp,0x1122
	ld	(0x1122),a
	inc	sp
	inc	(hl)
	dec	(hl)
	ld	(hl),0x11
	scf
	jr	c,$+2+0x11
	add	hl,sp
	ld	a,(0x1122)
	dec	sp
	inc	a
	dec	a
	ld	a,0x11
	ccf
;4x
	ld	b,b
	ld 	b,c
	ld 	b,d
	ld 	b,e
	ld 	b,h
	ld 	b,l
	ld 	b,(hl)
	ld 	b,a
	ld 	c,b
	ld 	c,c
	ld	c,d
	ld 	c,e
	ld 	c,h
	ld 	c,l
	ld 	c,(hl)
	ld 	c,a
;5x
	ld	d,b
	ld 	d,c
	ld 	d,d
	ld 	d,e
	ld 	d,h
	ld 	d,l
	ld 	d,(hl)
	ld 	d,a
	ld 	e,b
	ld 	e,c
	ld 	e,d
	ld 	e,e
	ld 	e,h
	ld 	e,l
	ld 	e,(hl)
	ld 	e,a
;6x
	ld	h,b
	ld	h,c
	ld 	h,d
	ld 	h,e
	ld 	h,h
	ld 	h,l
	ld 	h,(hl)
	ld 	h,a
	ld 	l,b
	ld	l,c
	ld 	l,d
	ld 	l,e
	ld 	l,h
	ld 	l,l
	ld 	l,(hl)
	ld 	l,a
;7x
	ld 	(hl),b
	ld 	(hl),c
	ld 	(hl),d
	ld 	(hl),e
	ld 	(hl),h
	ld 	(hl),l
	halt
	ld 	(hl),a
	ld 	a,b
	ld 	a,c
	ld 	a,d
	ld 	a,e
	ld 	a,h
	ld 	a,l
	ld 	a,(hl)
	ld 	a,a
;8x
	add 	a,b
	add 	a,c
	add 	a,d
	add 	a,e
	add 	a,h
	add 	a,l
	add 	a,(hl)
	add 	a,a
	adc 	a,b
	adc 	a,c
	adc 	a,d
	adc 	a,e
	adc 	a,h
	adc 	a,l
	adc 	a,(hl)
	adc 	a,a
;9x
	sub 	b
	sub 	c
	sub 	d
	sub 	e
	sub 	h
	sub 	l
	sub 	(hl)
	sub 	a
	sbc 	a,b
	sbc 	a,c
	sbc 	a,d
	sbc 	a,e
	sbc 	a,h
	sbc 	a,l
	sbc 	a,(hl)
	sbc 	a,a
;Ax
	and 	b
	and 	c
	and 	d
	and 	e
	and 	h
	and 	l
	and 	(hl)
	and 	a
	xor 	b
	xor 	c
	xor 	d
	xor 	e
	xor 	h
	xor 	l
	xor 	(hl)
	xor 	a
;Bx
	or 	b
	or 	c
	or 	d
	or 	e
	or 	h
	or 	l
	or 	(hl)
	or 	a
	cp 	b
	cp 	c
	cp 	d
	cp 	e
	cp 	h
	cp 	l
	cp 	(hl)
	cp 	a
;Cx
	ret	nz
	pop 	bc
	jp 	nz,0x1122
	jp 	0x1122
	call 	nz,0x1122
	push 	bc
	add 	a,0x11
	rst 	0x00
	ret 	z
	ret	
	jp 	z,0x1122	
	;BITS
	call 	z,0x1122
	call 	0x1122
	adc 	a,0x11
	rst 	0x08
;Dx
	ret 	nc
	pop 	de
	jp 	nc,0x1122
	out 	(0x11),a
	call 	nc,0x1122
	push 	de
	sub 	0x11
	rst 	0x10
	ret 	c
	exx	
	jp 	c,0x1122
	in 	a,(0x11)
	call 	c,0x1122	
	;IX
	sbc 	a,0x11
	rst 	0x18
;Ex
	ret 	po
	pop 	hl
	jp 	po,0x1122
	ex 	(sp),hl
	call 	po,0x1122
	push 	hl
	and 	0x11
	rst 	0x20
	ret 	pe
	jp 	(hl)
	jp 	pe,0x1122
	ex 	de,hl
	call 	pe,0x1122	
	;EXTD
	xor 	0x11
	rst 	0x28
;Fx
	ret 	p
	pop 	af
	jp 	p,0x1122
	di
	call 	p,0x1122
	push 	af
	or 	0x11
	rst 	0x30
	ret 	m
	ld 	sp,hl
	jp 	m,0x1122
	ei
	call 	m,0x1122	
	;IY
	cp 	0x11
	rst 	0x38

;EXTD
	in 	b,(c)
	out 	(c),b
	sbc 	hl,bc
	ld 	(0x1122),bc
	neg
	retn
	im 	0
	ld 	i,a
	in 	c,(c)
	out 	(c),c
	adc 	hl,bc
	ld 	bc,(0x1122)
	reti
	ld 	r,a

	in 	d,(c)
	out 	(c),d
	sbc 	hl,de
	ld 	(0x1122),de
	im 	1
	ld 	a,i
	in 	e,(c)
	out 	(c),e
	adc 	hl,de
	ld 	de,(0x1122)
	im 	2
	ld 	a,r

	in 	h,(c)
	out 	(c),h
	sbc 	hl,hl
	rrd
	in 	l,(c)
	out 	(c),l
	adc 	hl,hl
	ld 	hl,(0x1122)
	rld

	sbc 	hl,sp
	ld 	(0x1122),sp
	in 	a,(c)
	out 	(c),a
	adc 	hl,sp
	ld 	sp,(0x1122)

	ldi	
	cpi	
	ini	
	outi
	ldd
	cpd
	ind
	outd

	ldir	
	cpir	
	inir	
	otir
	lddr
	cpdr
	indr
	otdr

;BITS

	rlc	b
	rlc	c
	rlc	d
	rlc	e
	rlc	h
	rlc	l
	rlc	(hl)
	rlc	a
	rrc	b
	rrc	c
	rrc	d
	rrc	e
	rrc	h
	rrc	l
	rrc	(hl)
	rrc	a
	
	rl	b
	rl	c
	rl	d
	rl	e
	rl	h
	rl	l
	rl	(hl)
	rl	a
	rr	b
	rr	c
	rr	d
	rr	e
	rr	h
	rr	l
	rr	(hl)
	rr	a
	
	sla	b
	sla	c
	sla	d
	sla	e
	sla	h
	sla	l
	sla	(hl)
	sla	a
	sra	b
	sra	c
	sra	d
	sra	e
	sra	h
	sra	l
	sra	(hl)
	sra	a
	
	srl	b
	srl	c
	srl	d
	srl	e
	srl	h
	srl	l
	srl	(hl)
	srl	a
	
	bit	0,b
	bit	0,c
	bit	0,d
	bit	0,e
	bit	0,h
	bit	0,l
	bit	0,(hl)
	bit	0,a
	bit	1,b
	bit	1,c
	bit	1,d
	bit	1,e
	bit	1,h
	bit	1,l
	bit	1,(hl)
	bit	1,a
	
	bit	2,b
	bit	2,c
	bit	2,d
	bit	2,e
	bit	2,h
	bit	2,l
	bit	2,(hl)
	bit	2,a
	bit	3,b
	bit	3,c
	bit	3,d
	bit	3,e
	bit	3,h
	bit	3,l
	bit	3,(hl)
	bit	3,a
	
	bit	4,b
	bit	4,c
	bit	4,d
	bit	4,e
	bit	4,h
	bit	4,l
	bit	4,(hl)
	bit	4,a
	bit	5,b
	bit	5,c
	bit	5,d
	bit	5,e
	bit	5,h
	bit	5,l
	bit	5,(hl)
	bit	5,a
	
	bit	6,b
	bit	6,c
	bit	6,d
	bit	6,e
	bit	6,h
	bit	6,l
	bit	6,(hl)
	bit	6,a
	bit	7,b
	bit	7,c
	bit	7,d
	bit	7,e
	bit	7,h
	bit	7,l
	bit	7,(hl)
	bit	7,a
	
	res	0,b
	res	0,c
	res	0,d
	res	0,e
	res	0,h
	res	0,l
	res	0,(hl)
	res	0,a
	res	1,b
	res	1,c
	res	1,d
	res	1,e
	res	1,h
	res	1,l
	res	1,(hl)
	res	1,a
	
	res	2,b
	res	2,c
	res	2,d
	res	2,e
	res	2,h
	res	2,l
	res	2,(hl)
	res	2,a
	res	3,b
	res	3,c
	res	3,d
	res	3,e
	res	3,h
	res	3,l
	res	3,(hl)
	res	3,a
	
	res	4,b
	res	4,c
	res	4,d
	res	4,e
	res	4,h
	res	4,l
	res	4,(hl)
	res	4,a
	res	5,b
	res	5,c
	res	5,d
	res	5,e
	res	5,h
	res	5,l
	res	5,(hl)
	res	5,a
	
	res	6,b
	res	6,c
	res	6,d
	res	6,e
	res	6,h
	res	6,l
	res	6,(hl)
	res	6,a
	res	7,b
	res	7,c
	res	7,d
	res	7,e
	res	7,h
	res	7,l
	res	7,(hl)
	res	7,a
	
	set	0,b
	set	0,c
	set	0,d
	set	0,e
	set	0,h
	set	0,l
	set	0,(hl)
	set	0,a
	set	1,b
	set	1,c
	set	1,d
	set	1,e
	set	1,h
	set	1,l
	set	1,(hl)
	set	1,a
	
	set	2,b
	set	2,c
	set	2,d
	set	2,e
	set	2,h
	set	2,l
	set	2,(hl)
	set	2,a
	set	3,b
	set	3,c
	set	3,d
	set	3,e
	set	3,h
	set	3,l
	set	3,(hl)
	set	3,a
	
	set	4,b
	set	4,c
	set	4,d
	set	4,e
	set	4,h
	set	4,l
	set	4,(hl)
	set	4,a
	set	5,b
	set	5,c
	set	5,d
	set	5,e
	set	5,h
	set	5,l
	set	5,(hl)
	set	5,a
	
	set	6,b
	set	6,c
	set	6,d
	set	6,e
	set	6,h
	set	6,l
	set	6,(hl)
	set	6,a
	set	7,b
	set	7,c
	set	7,d
	set	7,e
	set	7,h
	set	7,l
	set	7,(hl)
	set	7,a

;IX
	add	ix,bc
	add	ix,de
	ld 	ix,0x1122
	ld 	(0x1122),ix
	inc 	ix
	add 	ix,ix
	ld 	ix,(0x1122)
	dec 	ix
	inc 	(ix+0x11)
	dec 	(ix+0x11)
	ld 	(ix+0x11),0x22
	add 	ix,sp
	ld 	b,(ix+0x11)
	ld 	c,(ix+0x11)
	ld 	d,(ix+0x11)
	ld 	e,(ix+0x11)
	ld 	h,(ix+0x11)
	ld 	l,(ix+0x11)
	ld 	(ix+0x11),b
	ld 	(ix+0x11),c
	ld 	(ix+0x11),d
	ld 	(ix+0x11),e
	ld 	(ix+0x11),h
	ld 	(ix+0x11),l
	ld 	(ix+0x11),a
	ld 	a,(ix+0x11)
	add 	a,(ix+0x11)
	adc 	a,(ix+0x11)
	sub 	(ix+0x11)
	sbc 	a,(ix+0x11)
	and 	(ix+0x11)
	xor 	(ix+0x11)
	or 	(ix+0x11)
	cp 	(ix+0x11)
	pop 	ix
	ex 	(sp),ix
	push 	ix
	jp 	(ix)
	ld 	sp,ix
;IX BITS
	rlc 	(ix+0x11)
	rrc 	(ix+0x11)
	rl 	(ix+0x11)
	rr 	(ix+0x11)
	sla 	(ix+0x11)
	sra 	(ix+0x11)
	srl 	(ix+0x11)
	bit	0,(ix+0x11)
	bit	1,(ix+0x11)
	bit	2,(ix+0x11)
	bit	3,(ix+0x11)
	bit	4,(ix+0x11)
	bit	5,(ix+0x11)
	bit	6,(ix+0x11)
	bit	7,(ix+0x11)
	res	0,(ix+0x11)
	res	1,(ix+0x11)
	res	2,(ix+0x11)
	res	3,(ix+0x11)
	res	4,(ix+0x11)
	res	5,(ix+0x11)
	res	6,(ix+0x11)
	res	7,(ix+0x11)
	set	0,(ix+0x11)
	set	1,(ix+0x11)
	set	2,(ix+0x11)
	set	3,(ix+0x11)
	set	4,(ix+0x11)
	set	5,(ix+0x11)
	set	6,(ix+0x11)
	set	7,(ix+0x11)

;IY
	add	iy,bc
	add	iy,de
	ld 	iy,0x1122
	ld 	(0x1122),iy
	inc 	iy
	add 	iy,iy
	ld 	iy,(0x1122)
	dec 	iy
	inc 	(iy+0x11)
	dec 	(iy+0x11)
	ld 	(iy+0x11),0x22
	add 	iy,sp
	ld 	b,(iy+0x11)
	ld 	c,(iy+0x11)
	ld 	d,(iy+0x11)
	ld 	e,(iy+0x11)
	ld 	h,(iy+0x11)
	ld 	l,(iy+0x11)
	ld 	(iy+0x11),b
	ld 	(iy+0x11),c
	ld 	(iy+0x11),d
	ld 	(iy+0x11),e
	ld 	(iy+0x11),h
	ld 	(iy+0x11),l
	ld 	(iy+0x11),a
	ld 	a,(iy+0x11)
	add 	a,(iy+0x11)
	adc 	a,(iy+0x11)
	sub 	(iy+0x11)
	sbc 	a,(iy+0x11)
	and 	(iy+0x11)
	xor 	(iy+0x11)
	or 	(iy+0x11)
	cp 	(iy+0x11)
	pop 	iy
	ex 	(sp),iy
	push 	iy
	jp 	(iy)
	ld 	sp,iy
;IX BITS
	rlc 	(iy+0x11)
	rrc 	(iy+0x11)
	rl 	(iy+0x11)
	rr 	(iy+0x11)
	sla 	(iy+0x11)
	sra 	(iy+0x11)
	srl 	(iy+0x11)
	bit	0,(iy+0x11)
	bit	1,(iy+0x11)
	bit	2,(iy+0x11)
	bit	3,(iy+0x11)
	bit	4,(iy+0x11)
	bit	5,(iy+0x11)
	bit	6,(iy+0x11)
	bit	7,(iy+0x11)
	res	0,(iy+0x11)
	res	1,(iy+0x11)
	res	2,(iy+0x11)
	res	3,(iy+0x11)
	res	4,(iy+0x11)
	res	5,(iy+0x11)
	res	6,(iy+0x11)
	res	7,(iy+0x11)
	set	0,(iy+0x11)
	set	1,(iy+0x11)
	set	2,(iy+0x11)
	set	3,(iy+0x11)
	set	4,(iy+0x11)
	set	5,(iy+0x11)
	set	6,(iy+0x11)
	set	7,(iy+0x11)
