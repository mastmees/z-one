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
	ds	bios-$

BIOSSTART:
nsects:	equ	($-ccp)/128	;warm start sector count

	include	"avr.inc"

; our disks have 128 logical sectors per track, in 32 physical sectors
; 512 tracks+4 system tracks
; 16384 byte logical blocks, 512 blocks per drive
; each track is one logical block

blksiz:	equ	16384		;CP/M allocation size
hstsiz:	equ	512		;host disk sector size
hstspt:	equ	32		;host disk sectors/trk
hstblk:	equ	4		;CP/M sects/host buff
cpmspt:	equ	128		;CP/M sectors/track
secmsk:	equ	hstblk-1	;sector mask
diskblocks: equ	512		;disk size in logical blocks

;
;	jump vectors for individual subroutines
;
CBOOT:	jp	_boot	;cold start
WBOOT:	jp	_wboot	;warm start
CONST:	jp	_const	;console status
CONIN:	jp	_conin	;console character in
CONOUT:	jp	_conout	;console character out
LIST:	jp	_list	;list character out
PUNCH:	jp	_punch	;punch character out
READER:	jp	_reader	;reader character out
HOME:	jp	_home	;move head to home position
SELDSK:	jp	_seldsk	;select disk
SETTRK:	jp	_settrk	;set track number
SETSEC:	jp	_setsec	;set sector number
SETDMA:	jp	_setdma	;set dma address
READ:	jp	_read	;read disk
WRITE:	jp	_write	;write disk
LISTST:	jp	_listst	;return list status
SECTRN: jp	_sectran ;sector translate

;	fixed data tables for all 16 drives
;
;	disk Parameter header for disk 00
dpbase:	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all00 ; disk allocation vector
;	disk parameter header for disk 01
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all01 ; disk allocation vector
;	disk parameter header for disk 02
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all02 ; disk allocation vector
;	disk parameter header for disk 03
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all03 ; disk allocation vector
;	disk parameter header for disk 04
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all04 ; disk allocation vector
;	disk parameter header for disk 05
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all05 ; disk allocation vector
;	disk parameter header for disk 06
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all06 ; disk allocation vector
;	disk parameter header for disk 07
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all07 ; disk allocation vector
;	disk parameter header for disk 08
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all08 ; disk allocation vector
;	disk parameter header for disk 09
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all09 ; disk allocation vector
;	disk parameter header for disk 10
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all10 ; disk allocation vector
;	disk parameter header for disk 11
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all11 ; disk allocation vector
;	disk parameter header for disk 12
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all12 ; disk allocation vector
;	disk parameter header for disk 13
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all13 ; disk allocation vector
;	disk parameter header for disk 14
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all14 ; disk allocation vector
;	disk parameter header for disk 15
	dw	0x0000 ; no translation table
        dw	0x0000,0x0000,0x0000 ; CP/M workspace
	dw	dirbf ; address of 128 byte sector buffer (shared)
	dw	dpblk ; DPB address
	dw	0x0000 ; not 'removable', so no directory checksums
	dw	all15 ; disk allocation vector
ndisks:	equ	($-dpbase)/16

;disk parameter block, common to all disks

dpblk:	dw	128		;SPT number of 128 byte sectors per track
	db	7		;BSH block shift factor (128<<bsf=block size)
	db	127		;BLM block mask (blm+1)*128=block size
	db	7		;EXM extent mask EXM+1 physical extents per
				;dir entry
	dw	diskblocks-1	;disk size-1, in blocks
	dw	511		;directory max, 1 block for directory=512
				;directory entries
	db	0x80		;alloc 0
	db	0		;alloc 1
	dw	0		;check size, 0 for fixed disks
	dw	4		;track offset for boot tracks


; cold boot loader, this is only invoked once, 
; when the CP/M is initially loaded
_boot:	ld	sp,0x80
_bootx:	call	welcome		;do one-off things, later patched out
	jr	initcpm		;initialize and go to cp/m

; warm boot loader, this is called to
; reload ccp&bdos in case it was overwritten by application
_wboot:
	ld	sp, 0x80	;use space below buffer for stack
	call	writeback	;ensure host sector is written
        ld      hl,0
        ld      (buflba_l),hl	;and force read on next access
        ld      (buflba_h),hl
        ld      a,2
        out     (A_MSCC),a 	; command 2 - load cpm
        ld      hl,ccp
        ld      e,0x16        	; ccp+bdos is 0x1600 bytes
        ld	b,0        	; full pages
        ld      c,A_MSCD   	; data port
loop:   inir               	; transfer page from data port
        dec     e          	; all pages done?
        jr      nz,loop    	; no, next page

initcpm:
	ld	a, 0xc3		;c3 is a jmp instruction
	ld	(0),a		;for jmp to wboot
	ld	hl, WBOOT	;wboot entry point
	ld	(1),hl		;set address field for jmp at 0
	ld	(5),a		;for jmp to bdos
	ld	hl, bdos	;bdos entry point
	ld	(6),hl		;address field of Jump at 5 to bdos
	ld	bc, 0x0080	;default dma address is 80h
	call	_setdma
	ld	a,(sekdsk)	;get current disk number
	ld	c, a		;send to the ccp
	jp	ccp		;start command processor

; Console status
; Returns its status in A; 0 if no character is ready, 0FFh if one is.
_const:	in     	a,(A_CONS)
	and	1
	ret	z
	ld	a,0xff
	ret

; Console input
; Wait until the keyboard is ready to provide a character, and return it in A.
_conin:	in	a,(A_CONS)
	and	1
	jr	z,_conin
	in	a,(A_COND)
	and	0x7f		;strip parity bit
	ret

; Console output
; Write the character in C to the screen
_conout:
	in	a,(A_CONS)	;make sure there is room
	and	2		;so that we wont freeze AVR interrupt
	jr	z,_conout	;handler
	ld	a, c		;get to accumulator
	out	(A_COND),a
	ret

; List output
; Write the character in C to the printer. If the printer isn't ready, wait until it is.
_list:	ret			;we dont have printer yet

; List status
; Return status of current printer device.
; Returns A=0 (not ready) or A=0FFh (ready).
_listst: xor	a	 	;never ready
	ret

; Punch out
; Write the character in C to the "paper tape punch" 
; or whatever the current auxiliary device is.
; If the device isn't ready, wait until it is.
; for the Z-ONE, punch is aux serial port
; but it is not initialized by default
;
_punch:	in	a,(A_AUXS)
	and	0x10		;check for tx queue full
	jr	nz,_punch
	ld	a, c		;character to register a
	out	(A_AUXD),a
	ret

; Reader in
; Read a character from the "paper tape reader"
; or whatever the current auxiliary device is. 
; If the device isn't ready, wait until it is. 
; The character will be returned in A. If this device isn't implemented, return character 26 (^Z).
;
_reader:
	in	a,(A_AUXS)
	and	0x40		;check for receive data available
	jr	z,_reader
	in	a,(A_AUXD)
	and    	0x7f		;remember to strip parity bit
	ret

; home the selected disk, in old times that just did the seek
; to track 0. this is called relatively often, so we are
; using it to also flush unwritten host sector to SD card
;
_home:	call	writeback
	ld	bc,0
	jp	_settrk

; select disk
; C is disk number
; return pointer to DPB in HL
_seldsk:
	ld	a,c
	ld	hl,0
	cp	ndisks		;see if disk is in valid range
	ret	nc
	ld	(sekdsk),a	;store it for later
	; now calculate first LBA sector number for drive
	; by adding constant drive size to start of partition
	; (yes, it means higher number disks are slower to select)
	ld	hl,(firstlba_l)
	ld	de,(firstlba_h)
	inc	c
_seld1:	dec	c
	jr	z,_seld2
	push	bc
	ld	bc,16384+128
	add	hl,bc
	jr	nc,_seld3
	inc	de
_seld3:	pop	bc
	jr	_seld1
_seld2:	ld	(dsklba_l),hl	;store the start sector address
	ld	(dsklba_h),de	;for selected disk
	ld	h,0
	ld	a,(sekdsk)
	ld	l,a		;disk number to HL
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	de,dpbase	;base of parm block
	add	hl,de		;hl=DPB of selected disk
	ret

; set track given by BC
_settrk:
	ld	h,b
	ld	l,c
	ld	(sektrk),hl
	ret

; set sector given by register bc 
; as we have only 128 sectors per track, b can be ignored
_setsec:
	ld	a,c
	ld	(seksec),a
	ret

;set dma address given by BC
_setdma:
	ld	h,b
	ld	l,c
	ld	(dmaadr),hl
	ret

; translate sector number BC
; return translated sector number in HL, in our case unchanged
_sectran:
	ld	h,b
	ld	l,c
	ret

; read sector at sekdsk,sektrk,seksec into buffer set by setdma
_read:	call	checkbuf	;ensure correct host sector is in buffer
	or	a		;failures are actually rather fatal
	ret	nz		;but we'll let BDOS handle the errors
	ld	bc,128		;logical sector size
	ldir			;checkbuf already set up registers
	xor	a		;successful read
	ret

; on entry
; c=0 for normal sector read/write, if different from buffered then
;     new host sector must be read to buffer
; c=1 for directory sectore read/write. the host buffer will be written
;     back immediately
; c=2 for write to unallocated block, no preread needed. this is only set
;     for very first sector and DR deblocking code then tried to keep track
;     when the disk block was filled up. The implementation however only
;     worked for first host sector and then started pre-reading all sectors
;     anyway, so in my code i'm just prereading in all cases as the SD card
;     is super fast compared to floppies

_write:	call	checkbuf	;ensure correct host sector is in buffer
	or	a
	ret	nz
	push	bc
	ld	bc,128
	ex	de,hl		;change director for write
	ldir			;and transfer data to buffer
	pop	bc
	ld	a,1		;mark buffer dirty
	ld	(bufmod),a
	cp	c		;was this a directory write?
	ld	a,0
	ret	nz		;no, all done
	jp	writeback	;otherwise write to card immediately

; ensure that host buffer has wanted host sector in it
; if a new sector is needed then checks the buffer status and
; writes the contents to media if the buffer has changed

checkbuf:
	call	isvalidsec
	jr	z,cbuf2		;already have correct sector
	;host sector is changing
cbuf1:	push	bc
	call	writeback	;if current buffer is dirty, write it
	pop	bc
	or	a
	ret	nz
	call	calchstsec	;calculate new wanted host sector
	ld	(buflba_l),hl
	ld	(buflba_h),de	;store for later writeback
	push	bc
	ld	bc,hstbuf
	call	readsector	;and read in new host sector
	pop	bc
	or	a
	ret	nz
cbuf2:	call	setdmareg	;set up registers for data transfers
	xor	a
	ret

; calculate pointer into host sector buffer for seksec
; into HL, load current dmaadr to DE
setdmareg:
	ld	a,(seksec)	;0..127
	and	3		;4 logical sectors per host sector
	ld	l,a
	ld	h,0
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl		;HL now offset into host sector
	ld	de,hstbuf
	add	hl,de		;hl now points to sector data in host buf
	ld	de,(dmaadr)	;de points to user buffer
	ret

; check if the host sector in buffer is the sector
; sekdsk, sektrk, seksec refer to. return NZ if wrong
; sector in buffer, Z if correct sector

isvalidsec:
	push	bc
	call	calchstsec
	ld	b,h
	ld	c,l
	ld	hl,buflba_h
	ld	a,e
	cp	(hl)
	jr	nz,isvs1
	inc	hl
	ld	a,d
	cp	(hl)
	jr	nz,isvs1
	ld	hl,buflba_l
	ld	a,c
	cp	(hl)
	jr	nz,isvs1
	inc	hl
	ld	a,b
	cp	(hl)
isvs1:	ld	h,b
	ld	l,c
	pop	bc
	ret

; calculate host sector address for sekdsk,sektrk,seksec
; returns with LBA sector number in DEHL

calchstsec:
	push	bc
	ld	hl,(sektrk)	; we have 32 LBA sectors per track
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	b,h
	ld	c,l		;bc is now LBA sector at beginning of track
	ld	a,(seksec)
	or	a
	rra
	or	a
	rra			;cp/m sector number now divided by 4
	add	a,c
	ld	c,a
	ld	a,0
	adc	a,b
	ld	b,a		;BC is now LBA sector within drive
	ld	hl,(dsklba_l)
	ld	de,(dsklba_h)
	add	hl,bc		;add to drive start to make it absolute
	jr	nc,cbuf3
	inc	de
cbuf3:	pop	bc
	ret

; flush host sector buffer if dirty, and mark it clean

writeback:
	ld	a,(bufmod)
	or	a
	ret	z
	ld	hl,(buflba_l)
	ld	de,(buflba_h)
	ld	bc,hstbuf
	call	writesector
	ld	(bufmod),a	;if good write then also marks clean
	ret


; reading and writing is done by first setting sector number, then writing
; command (0 for read, 1 for write) to A_SDC. for write the next step is to
; write 512 bytes to the data register. After that A_SDC needs to be
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
	xor	a
	ret

setsadr:
	ld	a,d
	out	(A_SD3),a
	ld	a,e
	out	(A_SD2),a
	ld	a,h
	out	(A_SD1),a
	ld	a,l
	out	(A_SD0),a
	ret

; print zero terminated string at HL
pstr:   ld      a,(hl)
        and     a
        ret     z
	push	bc
	ld	c,a
	call	_conout
	pop	bc
        inc     hl
        jr      pstr

dsklba_l:   dw 	0 ; first LBA sector of currently selected disk
dsklba_h:   dw	0
firstlba_l: dw	0 ; first LBA sector of CP/M partition on the card
firstlba_h: dw	0
buflba_l:   dw	0 ; LBA sector that is currently in hstbuf
buflba_h:   dw	0
bufmod:	    db	0 ; nonzero means the sector as been modified
;
;*****************************************************
;*                                                   *
;*	Unitialized RAM data areas		     *
;*                                                   *
;*****************************************************
begdat: equ     $               ;beginning of data area
dirbf:  ds      128             ;scratch directory area
; disk allocation vectors, 1 bit per block. we have
; 128 logical sectors per block, so total 512 blocks
; on drive, alv needs 64 bytes per drive, one for each drive
all00:  ds      (diskblocks/8)              ;A
all01:  ds      (diskblocks/8)              ;B
all02:  ds      (diskblocks/8)              ;C
all03:  ds      (diskblocks/8)              ;D
all04:  ds      (diskblocks/8)              ;E
all05:  ds      (diskblocks/8)              ;F
all06:  ds      (diskblocks/8)              ;G
all07:  ds      (diskblocks/8)              ;H
all08:  ds      (diskblocks/8)              ;I
all09:  ds      (diskblocks/8)              ;J
all10:  ds      (diskblocks/8)              ;K
all11:  ds      (diskblocks/8)              ;L
all12:  ds      (diskblocks/8)              ;M
all13:  ds      (diskblocks/8)              ;N
all14:  ds      (diskblocks/8)              ;O
all15:  ds      (diskblocks/8)              ;P

sekdsk:	ds	1		;seek disk number
sektrk:	ds	2		;seek track number
seksec:	ds	1		;seek sector number
dmaadr:	ds	2		;last dma address

; host sector buffer has 512 bytes of space that will be overwritten
; after boot, using this for one-off stuff happening at cold boot only.
; if needed, directory buffer and allocation vector area could be used similarily.
hstbuf:
	;reset and clear console
	db	27,"c",15,27,"[","H",27,"[","2","J"
	db	0
ostitle:
	db	"6","3","k"," ","C","P","/","M"," ","v","e","r","s","i","o","n"," ","2",".","2",13,10
	db	"Z","-","O","n","e"," ","B","I","O","S"," ","v","e","r","s","i","o","n"," ","1",".","0",13,10
	db	"h","t","t","p",":","/","/","w","w","w",".","n","o","m","a","d",".","e","e","/"
	db	"m","i","c","r","o","s","/","z","-","o","n","e"
	db	13,10,0

welcome:
	ld	hl,hstbuf
	call	pstr		;first initialize console and
	ld	hl,ostitle	;display welcome  messages
	call	pstr
	xor	a		;zero in the accum
	ld	(bufmod),a	;make sector buffer clear
	ld	(iobyte),a	;clear the iobyte
	ld	(sekdsk),a	;select disk zero
	ld	hl,(0x84)       ;bootstrap loader stored
	ld	(firstlba_l),hl ;partition data at 0x80
	ld	hl,(0x86)       ;and first LBA sector number
	ld	(firstlba_h),hl	;is at 0x84
	ld	hl,0
	ld	(buflba_l),hl	;cp/m should never try accessing partition
	ld	(buflba_h),hl	;table as the first thing, so this should be
				;safe starting point
	ld	hl,0
	ld	(_bootx),hl	;ensure this code is no longer called
	ld	(_bootx+1),hl   ;by modifying cold boot function 
	ret

	ds	hstsiz-($-hstbuf)	;host buffer

CPMTOP:	ds	0xffff-$
	nop


