;**************************************************************
;*
;*             C P / M   version   2 . 2
;*
;*   Reconstructed from memory image on February 27, 1981
;*
;*                by Clark A. Calkins
;*
;*   Modifications by Madis Kaal 2018 to produce identical
;*   binary image to actual CPM.SYS for some Xerox machine
;*   Also fixed some minor typos and converted to pyz80
;*   assembler syntax
;**************************************************************
;
;   Set memory limit here. This is the amount of contigeous
; ram starting from 0000. CP/M will reside at the end of this space.
;
MEM:	EQU	63		;
;
msize:	equ	MEM		;cp/m version memory size in kilobytes
		
				;this assumes that bios and buffers take
				;0x600 bytes from the end of memory
				;in our case the buffers need more space,
				;so even though we have 64K RAM, we need
				;to reduce this number to fit the bios in


;
IOBYTE:	EQU	3		;i/o definition byte.
TDRIVE:	EQU	4		;current drive name and user number.
ENTRY:	EQU	5		;entry point for the cp/m bdos.
TFCB:	EQU	0x5C		;default file control block.
TBUFF:	EQU	0x80		;i/o buffer and command line storage.
TBASE:	EQU	0x100		;transiant program storage area.
;
;	"bias" is address offset from 3400h for memory systems larger
;	than 16k (referred to as"b" throughout the text)
;
bias:	equ	(msize-20)*1024
CCP:	equ	0x3400+bias	;base of ccp
bdos:	equ	ccp+0x806	;base of bdos
bios:	equ	ccp+0x1600	;base of bios
cdisk:	equ	0x0004		;current disk number 0=a,... l5=p
iobyte:	equ	0x0003		;intel i/o byte

;
;   Set control character equates.
;
CNTRLC:	EQU	3		;control-c
CNTRLE:	EQU	0x05		;control-e
BS:	EQU	0x08		;backspace
TAB:	EQU	0x09		;tab
LF:	EQU	0x0A		;line feed
FF:	EQU	0x0C		;form feed
CR:	EQU	0x0D		;carriage return
CNTRLP:	EQU	0x10		;control-p
CNTRLR:	EQU	0x12		;control-r
CNTRLS:	EQU	0x13		;control-s
CNTRLU:	EQU	0x15		;control-u
CNTRLX:	EQU	0x18		;control-x
CNTRLZ:	EQU	0x1A		;control-z (end-of-file mark)
DEL:	EQU	0x7F		;rubout


