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

	org	0x100

SOH:	equ	0x01
STX:	equ	0x02
EOT:	equ	0x04
ACK:	equ	0x06
NAK:	equ	0x15
CAN:	equ	0x18

	ld	sp,stack
	jp	main

aux_open:
	ld      a,0x86
        out     (A_AUXS),a
auxopn1: in      a,(A_AUXS)
        and     0x40
        ret     z
        in      a,(A_AUXD)
        jr      auxopn1

; get a byte with 3 second timeout
; return nz if times out
;
aux_get:
	push	bc
	ld	b,30
aux_g1:
        in      a,(A_AUXS)
        and     0x40 ;rxrdy?
	jr	nz,aux_g2
	call	sleep100
	djnz	aux_g1
	inc	b	; make NZ
	pop	bc
	ret
aux_g2:
	call	aux_receive
	ld	b,a
	xor	a	; set Z
	ld	a,b
	pop	bc
	ret
	
        
aux_close:
	ld	a,0x05
        out     (A_AUXS),a
        ret

; wait until all data sent
;
aux_flush:
	ld	b,30
aux_f1:	call	aux_empty
	ret	z
	call	sleep100
	djnz	aux_f1
	ret

; wait until room in transmitter and send byte from A
;
aux_send:
	push	af
aux_s1:	in	a,(A_AUXS)
	and	0x10
	jr	nz,aux_s1
	pop	af
	out	(A_AUXD),a
	ret

; check if transmitter is empty
; returns Z if empty
aux_empty:
	in	a,(A_AUXS)
	and	0x20
	ret

; returns Z if data is waiting to
; be received
aux_ready:
        ;check if aux input data ready
        in      a,(A_AUXS)
        and     0x40 ;rxrdy?
        cp      0x40
	ret

; wait for byte to arrive and return it in A
aux_receive:
        in      a,(A_AUXS)
        and     0x40 ;rxrdy?
	jr	z,aux_receive
	in	a,(A_AUXD)
	ret

main:	call	aux_open
	call	crlfprint
	db	"Y","M","O","D","E","M"," ","b","a","t","c","h"," "
	db	"r","e","c","e","i","v","e","r",13,10
	db	"C","o","p","y","r","i","g","h","t"," ","(","C",")"," "
	db	"M","a","d","i","s"," ","K","a","a","l"," ","2","0","1","8",13,10,10,0
mainloop:
	call	crlfprint
	db	"R","e","a","d","y"," ","t","o"," ","r","e","c","e","i","v","e"
	db	" ","a","t"," ","1","9","2","0","0","N","8","1"
	db	13,10,"P","r","e","s","s"," ","a","n","y"," ","k","e","y"," ","t","o"," ","e","x","i","t",0
	call	ymserver
	ld	c,11
	call	5	;get console status
	or	a
	jr	z,mainloop
	ld	c,1
	call	5	;eat the char
	call	aux_close
	ld	c,0
	call	5

ymserver:
	ld	hl,_init
	ld	(state),hl
yms1:	ld	c,11	;exit if any key pressed
	call	5
	or	a
	ret	nz
	ld	hl,yms1	;otherwise push return address
	push	hl
	ld	hl,(state)
	jp	(hl)	;and 'call' the state handler

;this state terminates the ymserver
_endserver:
	call	closefile
	pop	hl
	ret

state:	dw	0

; initial state for server, set to the variables
_init:
	xor	a
	ld	(havefile),a
	ld	(ymodem),a
	;begin by sending 'C' few times
_begin:
	ld	a,5
	ld	(counter),a
	ld	hl,_trycrc
	ld	(state),hl
	ret

;send "C" to indicate crc capability, then listen for incoming packet
;stay in this state for 5 tries, then switch to checksum mode
;
_trycrc:
	ld	a,1
	ld	(xmcrc),a
	ld	a,"C"
	call	aux_send
	ld	hl,recbuf
	call	xmrecv
	call	z,_checkff	;got something, check what it is
	ret	z		;satisfied
	ld	a,(counter)
	dec	a
	ld	(counter),a
	ret	nz
	ld	a,2
	ld	(counter),a
	ld	hl,_trynak
	ld	(state),hl
	ret

;sender did not respond to "C", perhaps only supports checksums
;so try sending NAK few times too
;
_trynak:
	xor	a
	ld	(xmcrc),a
	ld	a,NAK
	call	aux_send
	ld	hl,recbuf
	call	xmrecv
	call	z,_checkff
	ret	z
	ld	a,(counter)
	dec	a
	ld	(counter),a
	ret	nz
	ld	a,5
	ld	(counter),a
	ld	hl,_trycrc	;no response, back to trying crc
	ld	(state),hl
	ret

;got first valid packet, see what it is
;return Z if satisfied to indicate go-ahead
;
_checkff:
	ld	a,(packettype)
	cp	EOT
	jr	nz,_chkff1
	ld	hl,_finish	;if EOT received then done
	ld	(state),hl
	xor	a
	ret
_chkff1:
	cp	SOH
	jr	z,_chkff2
	cp	STX
	jr	z,_chkff2
	ld	hl,_begin
	ld	(state),hl	;unknown packet, start over (return NZ)
	ret
	;got valid data packet, now see if it is ymodem name packet
_chkff2:
	ld	a,(packetnum)
	or	a
	jr	z,_chkff3	;got name
	ld	hl,_begin
	ld	(state),hl
	dec	a		;see if it is #1
	ret	nz		;otherwise start over
	;the first packet is data packet, assume xmodem
	;need to generate filename
	call	crlfprint
	db	"X","M","O","D","E","M"," ",0
	ld	hl,tempfname
	call	createfile
	jr	z,_chkff4
	call	writerec
	jr	z,_chkff4
	call	notify
	ld	a,1
	ld	(epacket),a
	ld	hl,_ackpacket
	ld	(state),hl
	xor	a
	ret
	;got filename packet, create file
_chkff3:
	ld	a,1
	ld	(ymodem),a
	call	crlfprint
	db	"Y","M","O","D","E","M"," ",0
	ld	hl,recbuf
	call	createfile
	jr	z,_chkff4
	ld	a,ACK
	call	aux_send	; acknowledge filename
	ld	a,5
	ld	(counter),a
	call	notify
	ld	hl,_waitdata
	ld	(state),hl
	xor	a
	ret
_chkff4:
	ld	hl,_abort
	ld	(state),hl
	xor	a
	inc	a
	ret

notify:	call	sprint
	db	"r","e","c","e","i","v","i","n","g"," ",0
	call	printfn
	call	crlfprint
	db	0
	ret

tempfname:
	db	"T","E","M","P",".","$","$","$",0

;start sending "C" again after getting filename
_waitdata:
        ld      a,"C"
        call    aux_send
        ld      hl,recbuf
        call    xmrecv
	jr	z,_wda1
_wda2:	ld	a,(counter)
	dec	a
	ld	(counter),a
	ret	nz
	ld	hl,_abort
	ld	(state),hl
	ret
_wda1:	ld	a,(packettype)
	cp	EOT
	jr	z,_wda3
	ld	a,(packetnum)
	cp	1
	jr	nz,_wda2
	call	writerec
	jr	z,_wda4
	ld	a,1
	ld	(epacket),a
	ld	hl,_ackpacket
	ld	(state),hl
	ret
_wda3:	ld	hl,_finish
	ld	(state),hl
	ret
_wda4:	ld	hl,_abort
	ld	(state),hl
	ret

_nakpacket:
	ld	a,NAK
	call	aux_send
	ld	hl,_waitnext
	ld	(state),hl
	ret

;packet received and written, ack and advance to next
_ackpacket:
	call	printkb
	ld	a,(epacket)
	inc	a
	ld	(epacket),a
	jr	_ack1
;acknowledge packet that we already have (dont advance)
_reackpacket:
_ack1:	ld	a,ACK
	call	aux_send
	ld	a,3
	ld	(counter),a
	ld	hl,_waitnext
	ld	(state),hl
	ret

;print KB received
printkb:
	ld	hl,(rrecords)
	ld	b,3
pkb1:	or	a
	ld	a,h
	rra
	ld	h,a
	ld	a,l
	rra
	ld	l,a
	djnz	pkb1	;divide by 8 to get kbytes
	ld	a,13
	call	putc
	call	pdec16
	call	sprint
	db	"K","B",0
	ret

; wait for another data packet
_waitnext:
	ld	hl,recbuf
	call	xmrecv
	jr	z,_wn1
	ld	a,(counter)
	dec	a
	ld	(counter),a
	jr	z,_wn5
	ld	hl,_nakpacket
	ld	(state),hl
	ret
_wn5:	ld	hl,_abort
	ld	(state),hl
	ret
	;see if EOT?
_wn1:	ld	a,(packettype)
	cp	EOT
	jr	nz,_wn4
	ld	hl,_finish	;yes, we are done with the file
	ld	(state),hl
	ret
	;another data packet, verify that it is the one we expect
_wn4:	ld	a,(packetnum)
	ld	hl,epacket
	cp	(hl)	;expected packet?
	jr	z,_wn2	;yes, write and ack
	dec	a
	cp	(hl)	;resent previous?
	jr	z,_wn3
	ld	hl,_nakpacket
	ld	(state),hl
	ret
_wn2:	call	writerec
	jr	z,_wn6
	ld	hl,_ackpacket
	ld	(state),hl
	xor	a	
	ret
_wn3:	ld	hl,_reackpacket
	ld	(state),hl
	xor	a
	ret
_wn6:	ld	hl,_abort
	ld	(state),hl
	xor	a
	inc	a
	ret

; abort processing, sent 4 CAN bytes to abort
_abort:	call	crlfprint
	db	"A","b","o","r","t","i","n","g",0
	ld	a,CAN
	call	terminate
	ld	hl,_endserver
	ld	(state),hl
	ret

; finish processing - send ACK for EOT and close file
_finish:ld	a,ACK
	call	term1
	call	crlfprint
	db	"D","o","n","e",0
	ld	hl,_endserver
	ld	(state),hl
	ret

; send 4 times byte from A, then close file if open
terminate:
	call	aux_send
	call	aux_send
	call	aux_send
term1:
	call	aux_send
	call	aux_flush
	call	closefile
	ret

;-------------------------------------------------------------------

		
; receive xmodem packet to recbuf
; return Z if good packet received
;
; xerrno set to:
; 0 no error
; 1 timeout waiting for packet
; 2 timeout waiting for packet #
; 3 timeout waiting for rame # complement
; 4 packet number does not match its complement
; 5 timeout during packet data
; 6 timeout waiting for checksum or first crc byte
; 7 invalid checksum
; 8 invalid crc
; 9 timeout waiting for second crc byte
;
xmrecv:	ld	hl,recbuf
	ld	a,1
	ld	(xerrno),a
	xor	a
	ld	(packettype),a
	call	aux_get
	ret	nz
	ld	(packettype),a
	xor	a
	ld	(xerrno),a
	ld	a,(packettype)
	cp	EOT
	ret	z
	cp	CAN
	ret	z
	cp	SOH
	jr	z,xmsoh
	cp	STX
	jr	nz,xmrecv
	ld	a,8
	ld	(rcount),a
	jr	xmr1
xmsoh:	ld	a,1
	ld	(rcount),a
xmr1:	ld	a,2
	ld	(xerrno),a
	call	aux_get	
	ret	nz
	ld	b,a
	ld	(packetnum),a
	ld	a,3
	ld	(xerrno),a
	call	aux_get
	ret	nz
	cpl
	push	af
	ld	a,4
	ld	(xerrno),a
	pop	af
	cp	b
	ret	nz	; packet # mismatch
	ld	a,5
	ld	(xerrno),a
	ld	c,0
xmrl2:	ld	b,128
xmrl:	call	aux_get
	ret	nz
	ld	(hl),a
	add	a,c
	ld	c,a	; update checksum
	inc	hl
	djnz	xmrl
	ld	a,(rcount)
	dec	a
	ld	(rcount),a
	jr	nz,xmrl2
	ld	a,6
	ld	(xerrno),a
	call	aux_get
	ret	nz
	ld	b,a
	ld	a,(xmcrc)
	or	a	; crc mode?
	jr	nz,xmrc ; yes, get one more byte
	ld	a,7
	ld	(xerrno),a
	ld	a,c
	cp	b	; otherwise check checksum
	ret	nz
	xor	a
	ld	(xerrno),a
	ret
xmrc:	ld	d,b
	ld	a,9
	call	aux_get ; get crc low byte
	ret	nz
	ld	e,a	; DE is now received crc
	push	de
	ld	hl,recbuf
	ld	de,0
	ld	c,1
	ld	a,(packettype)
	cp	STX
	jr	nz,xmrc1
	ld	c,8
xmrc1:	ld	b,128
xmrc2:	ld	a,(hl)
	inc	hl
	call	crc16de
	djnz	xmrc2
	dec	c
	jr	nz,xmrc1
	pop	hl
	ld	a,8
	ld	(xerrno),a
	ld	a,h	; check crc high byte
	cp	d
	ret	nz
	ld	a,l
	cp	e
	ret	nz
	xor	a
	ld	(xerrno),a
	ret

; send xmodem packet (packet number in A) at HL, BC is number of
; data bytes (128 or 1024)
;
xmsend:	push	af
	ld	a,1
	ld	(rcount),a
	ld	a,b
	or	a
	ld	a,SOH
	jr	z,xmsml
	ld	a,8
	ld	(rcount),a
	ld	a,STX
xmsml:	call	aux_send
	pop	af
	call	aux_send ; packet #
	cpl
	call	aux_send ; complemented packet #	
	ld	c,0	;checksum clear
	ld	de,0	;crc clear
xms2:	ld	b,128
xms1:	ld	a,(hl)
	call	aux_send
	push	af
	call	crc16de
	pop	af
	add	a,c
	ld	c,a
	inc	hl
	djnz	xms1
	ld	a,(rcount)
	dec	a
	ld	(rcount),a
	jr	nz,xms2
	ld	a,(xmcrc)
	or	a
	jr	nz,xmsc
	ld	a,c
	call	aux_send
	ret
xmsc:	ld	a,d
	call	aux_send
	ld	a,e
	call	aux_send
	ret

; update CRC16 in DE with byte in A
; derived from http://mdfs.net/Info/Comp/Comms/CRC16.htm
;
crc16de:
	push	bc
	xor	d
	ld	b,8	;bit counter
crcl:	sla	e
	adc	a,a
	jr	nc,crc0
	ld	d,a
	ld	a,e
	xor	0x21
	ld	e,a
	ld	a,d
	xor	0x10
crc0:	djnz	crcl
	ld	d,a
	pop	bc
	ret

; write record from recbuf to file, writes 128 or 1024 based on record type
; return Z on failure
;
writerec:
	ld	a,1
	ld	(rcount),a
	ld	de,recbuf
	ld	a,(packettype)
	cp	STX
	jr	nz,wrrec1
	ld	a,8
	ld	(rcount),a
wrrec1:	push	de
	ld	c,26
	call	5
	ld	de,fcb
	ld	c,21
	call	5
	pop	de
	or	a
	jr	z,fook
	push	af
	call	crlfprint
	db	"w","r","i","t","e"," ","e","r","r",0
	pop	af
	call	phex
	xor	a	;set Z for failure
	ret
fook:	ld	hl,(rrecords)
	inc	hl
	ld	(rrecords),hl
	ld	hl,128
	add	hl,de
	ex	de,hl
	ld	a,(rcount)
	dec	a
	ld	(rcount),a
	jr	nz,wrrec1
	xor	a
	inc	a
	ret

;read record from file, HL points to buffer
;return Z on failure
readrec:
	ex	de,hl
	ld	c,26
	call	5
	ld	de,fcb
	ld	c,20
	call	5
	or	a
	jr	z,rfook
	xor	a	;set Z on failure
	ret
rfook:	inc	a
	ret

;delete file, HL points to file name
;
deletefile:
	call	parsefn
	ld	a,1
	ld	(havefile),a
	ld	de,fcb
	ld	c,19
	call	5	;try erasing first
	ret		;0xff when fails

;creates or overwrites a file
;returns Z on failure. on success sets (havefile) to nonzero
;
createfile:
	push	hl
	call	deletefile
	ld	hl,0
	ld	(rrecords),hl
	pop	hl
	call	parsefn		
	ld	de,fcb
	ld	c,22
	call	5
	push	af
	xor	a
	ld	(fcb+32),a	;zero current record
	pop	af
	cp	0xff
	ret	nz
	push	af
	push	af
	call	crlfprint
	db	"C","r","e","a","t","e"," ","f","a","i","l","e","d",":",0
	call	printfn
	pop	af
	call	phex
	xor	a
	ld	(havefile),a
	pop	af
	ret	

; open existing file, HL points to file name, terminated by space or zero
; FCB set up at fcb
; returns Z on failure
;
openfile:
	call	parsefn
	ld	a,1
	ld	(havefile),a
	ld	de,fcb
	ld	c,15
	call	5
	push	af
	xor	a
	ld	(fcb+32),a	;zero current record
	pop	af
	cp	0xff
	ret	nz
	push	af
	push	af
	call	crlfprint
	db	"O","p","e","n"," ","f","a","i","l","e","d",":",0
	call	printfn
	pop	af
	call	phex
	xor	a
	ld	(havefile),a
	pop	af
	ret	

; close file, FCB expected at fcb
; returns Z on failure
closefile:
	ld	a,(havefile)
	or	a
	ret	z
	xor	a
	ld	(havefile),a
	ld	de,fcb
	ld	c,16
	call	5
	cp	0xff
	ret	nz
	push	af
	push	af
	call	crlfprint
	db	"C","l","o","s","e"," ","f","a","i","l","e","d",":",0
	pop	af
	call	phex
	pop	af
	ret

;clear FCB at DE
clrfcb:
	push	bc
	push	hl
	ld	h,d
	ld	l,e
	xor	a
	ld	(hl),a
	inc	hl
	ld	a," "
	ld	b,11
clrfcb1:
	ld	(hl),a
	inc	hl
	djnz	clrfcb1
	xor	a
	ld	b,24
clrfcb2:
	ld	(hl),a
	inc	hl
	djnz	clrfcb2
	pop	hl
	pop	bc
	ret

; parse file name pointed by HL, put name and extension in default FCB
; at fcb
parsefn:
 	ld	de,fcb
	call	clrfcb
	call	parsedr	;check for drive letter
	inc	de
	ld	b,8
	call	parsenp ;up to 8 characters for name
	ld	a,8
	add	a,e
	ld	e,a
	ld	a,0
	adc	a,d
	ld	d,a
	ld	b,3
	call	parsenp ;up to 3 characters for extension
	ret

parsedr:
	xor	a
	ld	(de),a	; default drive
	push	hl
	inc	hl
	ld	a,(hl)
	pop	hl
	cp	":"	; check if drive letter included
	ret	nz
	ld	a,(hl)
	call	ucase
	sub	"A"-1
	ld	(de),a	;if drive letter, then store explicit drive
	inc	hl
	inc	hl	;and skip the drive part
	ret

parsenp:
	push	de
np1:	ld	a,(hl)
	cp	" "
	jr	z,npe
	cp	"."
	jr	z,npe
	or	a
	jr	z,npe
	call	ucase
	ld	(de),a
	inc	de
	inc	hl
	djnz	np1
np2:	ld	a,(hl)
	cp	" "
	jr	z,npe
	cp	"."
	jr	z,npe
	or	a
	jr	z,npe
	inc	hl
	jr	np2
npe:	pop	de
	ld	a,(hl)
	or	a
	ret	z
	inc	hl
	ret

ucase:	cp	"a"
	ret	c
	cp	"z"+1
	ret	nc
	sub	0x20
	ret

printfn:
	ld	hl,fcb
	ld	a,(hl)
	or	a
	jr	z,prfn1
	add	a,"A"-1
	call	putc
	ld	a,":"
	call	putc
prfn1:	ld	b,8
prfn2:	inc	hl
	ld	a,(hl)
	cp	" "
	call	nz,putc
	djnz	prfn2
	inc	hl
	ld	a,(hl)
	cp	" "
	ret	z
	ld	a,"."
	call	putc
	ld	b,3
prfn3:	ld	a,(hl)
	cp	" "
	ret	z
	call	putc
	inc	hl
	djnz	prfn3
	ret

; about 100ms delay, cut short if receive data is ready
;
sleep100:
	push	af
	in	a,(A_AUXS)
	and	0x40
	jr	nz,slp3
	push	bc
	ld	c,100
	ld	b,64
slp1:	djnz	slp1
	in	a,(A_AUXS)
	and	0x40
	jr	nz,slp2
	dec	c
	jr	nz,slp1
slp2:	pop	bc
slp3:	pop	af
	ret

; multiply DE*BC, result in DEHL
; div/mul routines from http://z80-heaven.wikidot.com/math
;
demulbc:
	ld	hl,0
	ld	a,16
dml1:	add	hl,hl
	rl	e
	rl	d
	jr	nc,dml2
	add	hl,bc
	jr	nc,dml2
	inc	de
dml2:	dec	a
	jr 	nz,dml1
       	ret

; multiply DEHL by 10, return result in DEHL
dehlmul10:
	add	hl,hl
	push	hl
	push	hl
	ld	h,d
	ld	l,e
	adc	hl,de
	ex	de,hl
	pop	hl
	push	de	;DEHL=DEHL*2, copy in stack
	add	hl,hl
	push	hl
	ld	h,d
	ld	l,e
	adc	hl,hl
	ex	de,hl
	pop	hl	;DEHL=DEHL*4
	add	hl,hl
	push	hl
	ld	h,d
	ld	l,e
	adc	hl,hl
	ex	de,hl
	pop	hl	;DEHL=DEHL*8
	pop	ix
	pop	bc
	add	hl,bc
	push	hl
	push	ix
	pop	hl
	adc	hl,de
	ex	de,hl
	pop	hl
	ret

; divides DEHL by C, returns result in DEHL, remainder in A
;
dehldivc:
	ld 	b,32
	xor 	a
divc2:	add 	hl,hl
	rl 	e
	rl 	d
       	rla
       	cp 	c
       	jr 	c,divc1
        inc 	l
        sub 	c
divc1:	djnz 	divc2
	ret

putc:	push	bc
	push	de
	push	hl
	ld	c,2
	ld	e,a
	call	5
	pop	hl
	pop	de
	pop	bc
	ret

getc:	push	bc
	push	de
	push	hl
	ld	c,1
	call	5
	pop	hl
	pop	de
	pop	bc
	ret

; print HL as decimal number
pdec16:
	ld	a,h
	or	a
	jr	nz,pdc161
	ld	a,l
	cp	10
	jr	nc,pdc161
pdc162:	add	a,"0"
	call	putc
	ret
pdc161:	ld	de,0
	ld	c,10
	call	dehldivc
	push	af
	call	pdec16
	pop	af
	jr	pdc162


; print HL as hex
phex16:
        ld      a,h
        call    phex
        ld      a,l
; print A as hex
phex:	push	af
	push	af
        rra
        rra
        rra
        rra
        call    pdigit
        pop     af
	call	pdigit
	pop	af
	ret

pdigit: and     0x0f
        add     "0"
        cp      "9"+1
        jr      c,pd1
        add     7
pd1:    call	putc
        ret

; print zero terminated string at HL
pstr:   ld      a,(hl)
        and     a
        ret     z
	call	putc
        inc     hl
        jr      pstr

crlfprint:
        ld      a,13
	call	putc
        ld      a,10
        call	putc
; print zero terminated string following the call
sprint:  pop     hl
        call    pstr
        inc     hl
        jp      (hl)

xmcrc:	    db	0	; crc mode?
ymodem:	    db	0	; ymodem mode?
packetnum:  db	0	; received packet #
packettype: db	0	; received packet type (SOH,STX,EOT)
counter:    db	0	; generic counter
xerrno:	    db	0	; xmodem receiver error code
rrecords:   dw	0	; number of successfully received 128 byte records
;
epacket:    db	0	; expected packet #
rcount:	    db	1	; 128 byte record count in packet being received
havefile:   db	0	; nz if file has been opened
fcb:	    ds	36
	    ds	128
stack:
recbuf:	    ds	1024
