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

; bootstrap code that the AVR writes to start of ram at reset
; then the Z80 is allowed to start executing this code
;
; bootstrap code size is limited to 128 bytes
;
	org	0

	include	"avr.inc"
	include "cpm.inc"

; if SD card is present, and correctly partitioned then
; boot to CP/M, otherwise to system monitor

	ld	a,2
	out	(A_SDC),a  ; get SD card type
	in	a,(A_SDS)
	cp	2	   ; 0 and 1 means no card detected
	jr	nc,bootcpm

bootmon:
	sub	a
	out	(A_MSCC),a ; command 0 - load monitor image
	ld	hl,monitor ; target address
	ld	e,((0xffff-monitor)+1)/256
	ld	sp,hl
	jr	load

bootcpm:
	ld	de,0x80
        xor     a
	ld	(de),a
        out     (A_SD3),a
        out     (A_SD2),a
        out     (A_SD1),a
        out     (A_SD0),a
        out     (A_SDC),a ; read LBA sector 0 to get partition table

rs1:    in      a,(A_SDC) ; poll for command completed
        and     0x80
        jr      z,rs1
        in      a,(A_SDS) ; get status
        or      a
        jr      nz,bootmon ; read failed, boot to monitor
	ld	h,0xfe    ; read to top of ram (0xfe00)
        ld      l,0
        ld      b,0
        ld      c,A_SDD
        inir              ;read 256 bytes
        inir              ;and again

	;typedef struct { // first entry at offset 446
	;  uint8_t status;
	;  uint8_t firstchs[3];
	;  uint8_t type;             0x50
	;  uint8_t lastchs[3];       0x51
	;  uint8_t firstlba[4];      0x54
	;  uint8_t lbacount[4];      0x58
	;} PARTITION;
	
	ld	hl,0xfe00+0x1c2
	ld	a,(HL)
	cp	0x58	   ; check for correct partition type
	jr	nz,bootmon
	ld	bc,12
	ldir		   ; copy last 12 bytes of partition info to 0x80

	ld	a,2
	out	(A_MSCC),a ; command 2 - load cpm image
	ld	hl,ccp
	ld	e,((0xffff-ccp)+1)/256
	ld	sp,ccp+0x1600 ; start address

; transfer either monitor or CP/M image from AVR to Z80 RAM
; command is already issued, E register is number of 256 byte pages to
; transfer, HL is memory address to load to, SP is code start address

load:	ld	b,0        ; full pages
	ld	c,A_MSCD   ; data port
loop:	inir		   ; transfer page from data port
	dec	e	   ; all pages done?
	jr	nz,loop	   ; no, next page
	ld	a,13
	out	(A_COND),a
	ld	a,10
	out	(A_COND),a
	ld	hl,0
	add	hl,sp
	jp	(hl)       ; loaded, jump to start address

	ds	127-$
	nop

