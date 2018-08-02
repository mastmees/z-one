; The MIT License (MIT)
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

monitor: equ	0xee00
pages:	equ	((0xffff-monitor)+1)/256

; misc operations
; command  description
; 0 	   read monitor image from data port
; 1	   reboot
; 2        read CP/M image from data port
; 3        reset timecounter
; 4        read timecounter
;
A_MSCC:	equ	0xa0 	  ; i/o port address of avr command
A_MSCD:	equ	0xa1 	  ; i/o port address of avr data

; console i/o
;
A_COND:	equ	0xa2 	  ; console output (write) / input (read)
A_CONS:	equ	0xa3 	  ; console status (read)
; console status bits -
; 0 - data ready
; 1 - transmitter empty
; 2 -
; 3 -
; 4 -
; 5 -
; 6 -
; 7 -

; aux i/o
A_AUXD:	equ	0xa4	  ; aux output (write) / input (read)
A_AUXS:	equ	0xa5	  ; aux status (read) / control (write)
;
; aux status bits
; 0 - baudrate0
; 1 - baudrate1
; 2 - baudrate2
; 3 -
; 4 - transmit queue full
; 5 - transmit queue not empty
; 6 - received data ready
; 7 - enabled
;
; aux control bits
; 0 - baudrate0
; 1 - baudrate1
; 2 - baudrate2
; 3 -
; 4 -
; 5 -
; 6 -
; 7 - enable
;
; baud rates
; 0 - 50
; 1 - 300
; 2 - 1200
; 3 - 2400
; 4 - 4800
; 5 - 9600
; 6 - 19200
; 7 - 38400
;
; SD card interface
;
; reading and writing is done by first setting sector number, then writing
; command (0 for read, 1 for write) to A_SDC. for write the next step is to
; write 512 bytes to da data register. After that A_SDC needs to be
; read until its high bit becomes set to indicate operation has completed.
; zero value read from A_SDS means that operation was successful, and for
; read operation 512 data bytes may now be read from A_SDD.
;
; cart type can be read from status register after writing command
;  0=UNKNOWN,1=NONE,2=SD1,3=SD2,4=SDHC
;
A_SDC:	equ	0xa8      ; command (0=read,1=write,2=get type,3=get
			  ; size,4=reinitialize)
A_SDD:	equ	0xa9      ; data
A_SDS:  equ	0xaa      ; status/type
A_SD0:	equ	0xab	  ; byte0 of LBA sector number
A_SD1:	equ	0xac      ; byte1 of LBA sector number
A_SD2:	equ	0xad      ; byte2 of LBA sector number
A_SD3:	equ	0xae      ; byte3 of LBA sector number
;
