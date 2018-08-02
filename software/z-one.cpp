/* The MIT License (MIT)
 
  Copyright (c) 2018 Madis Kaal <mast@nomad.ee>
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#include "sdcard.hpp"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "z-one.hpp"
#include "console.hpp"
#include "softwareuart.hpp"
#include "queue.hpp"
#include "partitioner.hpp"

Console console;
SDCard sdcard;

enum IOOPERATION { IONONE,IOWRITE,IOREAD };
enum MSTATE { IDLE, BIOS, CPM, TIMECOUNTER };

static uint32_t timecounter,timecountersnapshot; // counts 10ms ticks
static uint8_t auxbaud;
static uint8_t sd0,sd1,sd2,sd3,sds;

// this awfulness is for helping compiler to produce better interrupt handler
// for handling Z80 I/O requests
//
register IOOPERATION iotype asm("r2");
register uint8_t sdc asm("r3");
register uint8_t rega asm("r4");
register uint8_t regd asm("r5");
register uint16_t dataofs asm("r6");

static MSTATE state;
static uint16_t count;

static uint16_t baudrates[8]= {
 50, 300,1200,2400,4800,9600,19200,38400 
};

#define Z80CLOCK 1
// for 18.432MHz AVR clock
// 0 = 9.216MHz (I/O through AVR will be 3x slower than at 4.6MHz!) 
// 1 = 4.608MHz
// 2 = 3.072MHz

#define z80_writing() ((PINB&0x02)==0)
#define z80_reading() ((PIND&0x80)==0)

// the quick version is for z80 writing
#define z80_quickrelease() PORTD&=~(0x40);PORTD&=~(0x40);PORTD|=0x40;

// the slow version is for z80 reading where the data bus drivers need to be
// disabled once z80 removes the /RD signal. At 9MHz Z80 clock the clock speed needs
// to be reduced for bus timing. this impacts I/O performance a lot
#if Z80CLOCK==0
  #define z80_release() OCR0A=1;PORTD&=~(0x40);while ((PIND&0x80)==0);DDRA=0;PORTD|=0x40;TCNT0=0;OCR0A=0;
#else
 #define z80_release() PORTD&=~(0x40); while ((PIND&0x80)==0); DDRA=0; PORTD|=0x40;
#endif

// z80 clock is created by timer0, with its output connected to OC0A
// the clock frequency is AVR_clock/2/(OCR0A+1)
//
void enable_z80_clock()
{
  TCCR0A=0x42; // CTC mode, toggle OC0A on match
  OCR0A=Z80CLOCK;
  TCCR0B=0x01; // internal clock with no prescaler
  DDRB|=8;     // enable clock output pin
}

void aquire_bus()
{
  PORTD&=0xef;   // BUSRQ low
  DDRD|=0x10;    // make pin output
  PORTD|=0x20;   // z80 reset high
  PORTD&=~(0x40);
  PORTD|=0x40;   // ensure z80 is not in wait state
  while (PIND&0x08) // wait for BUSAK to go low
    ;
  // z80 bus is now floating, enable adr,data,and control outputs
  // to prepare for writing bootloader to ram
  PORTA=0xff;
  DDRA=0xff; // data bus
  PORTC=0xff;
  DDRC=0xfc; // a0..a5
  PORTB|=7;
  DDRB|=7; // a6, /wr, /mreq
}

void release_bus()
{
  // release buses
  PORTA=0xff;
  DDRA=0; // data bus
  PORTC=0xff;
  DDRC=0; // a0..a5
  PORTB|=7;
  DDRB&=~7; // a6, /wr, /mreq
  // now give bus back to z80
  // and reset z80 to restart it
  PORTD&=~(0x20); // z80 reset low
  PORTD|=0x10;    // drive BUSRQ high
  DDRD&=0xef;     // make input
  _delay_us(5);
  PORTD|=0x20;    // z80 reset high, and it starts running from 0x0000
}


void copy_bootloader(void)
{
  console.print("Bootstrap loader");
  console.print("\r\n");
  aquire_bus();
  for (uint8_t a=0;a<sizeof(bootstrap_bin);a++) {
    PORTA=pgm_read_byte(&bootstrap_bin[a]); // d0..7
    PORTC=(a<<2)|3; // a0..a5
    if (a&0x40)
      PORTB|=1; // a6
    else
      PORTB&=0xfe;
    PORTB&=0xfb; // /mreq low
    _delay_us(1);
    PORTB&=0xf9; // /mreq + /wr low
    _delay_us(1);
    PORTB|=0x02; // /wr high
    _delay_us(1);
    PORTB|=0x04; // /mreq high
    _delay_us(1);
  }  
  console.print("Starting Z80");
  release_bus();
}


ISR(WDT_vect)
{
}

/*
INT0 is triggered on each access to AVR I/O address 0xe0. the trigger happens
on falling front of address decode, so when the interrupt handler executes
the /wait signal is already asserted to z80.

interrupt handler reads address and data to pass on to main
loop, and returns while leaving the Z80 in wait state.

For better sdcard read performance buffer transfer is handled
directly in interrupt handler, this gives about 3X improvement
in data read rate
*/
ISR(INT0_vect)
{
  rega=PINC&0x7c;
  if (z80_reading() && rega==(0x09<<2)) {
    if ((sdc&0xc0)==0x80) { // still reading sector data
      PORTA=sdcard.GetBuf()[dataofs];
      DDRA=0xff;
      z80_release();
      dataofs++;
      if (dataofs>511)
        sdc|=0x40;
    }
    iotype=IONONE;
    return;
  }
  rega>>=2;
  regd=PINA;
  if (z80_writing())
    iotype=IOWRITE;
  else
    iotype=IOREAD;
}

/*
main loop then calls command processor, with interrupts enabled,
to process request that z80 made. once the request is processed
z80 is released from wait state.
*/
void ProcessCommand(void)
{
uint32_t s;
  switch (rega) {
    case 0x08: // A_SDC
      if (iotype==IOWRITE) {
        sdc=regd;
        switch (regd) {
          case 0: // read
            sds=sdcard.ReadSector((uint32_t)sd3<<24|(uint32_t)sd2<<16|(uint32_t)sd1<<8|sd0,
              sdcard.GetBuf());
            dataofs=0;
            sdc|=0x80;
            break;
          case 1: // write, this will need data from data register first
            dataofs=0;
            break;
          case 2: // get card type
            sds=sdcard.GetType();          
            sdc|=0x80;
            break;
          case 3: // get card size
            s=sdcard.GetTotalSectors();
            sd0=s;
            sd1=s>>8;
            sd2=s>>16;
            sd3=s>>24;
            sdc|=0x80;
            break;
          case 4: // reset
            sds=sdcard.Init(true); // silent initialize
            sdc|=0x80;
            break;
          default: // unknowns
            sds=0xff;
            sdc|=0x80;
            break;
        }
      }
      else {
        PORTA=sdc;
        DDRA=0xff;
      }
      break;
    case 0x09: // A_SDD
      if (iotype==IOWRITE && sdc==1 && dataofs<512) {
        sdcard.GetBuf()[dataofs++]=regd;
        if (dataofs>511) {
          sds=sdcard.WriteSector((uint32_t)sd3<<24|(uint32_t)sd2<<16|(uint32_t)sd1<<8|sd0,
             sdcard.GetBuf());
          sdc|=0x80;
        }
        break;
      }
      break;
    case 0x0a: // A_SDS
      if (!(iotype==IOWRITE)) {
        PORTA=sds;
        DDRA=0xff;
      }
      break;
    case 0x0b:
      if (iotype==IOWRITE)
        sd0=regd;
      else {
        PORTA=sd0;
        DDRA=0xff;
      }
      break;
    case 0x0c:
      if (iotype==IOWRITE)
        sd1=regd;
      else {
        PORTA=sd1;
        DDRA=0xff;
      }
      break;
    case 0x0d:
      if (iotype==IOWRITE)
        sd2=regd;
      else {
        PORTA=sd2;
        DDRA=0xff;
      }
      break;
    case 0x0e:
      if (iotype==IOWRITE)
        sd3=regd;
      else {
        PORTA=sd3;
        DDRA=0xff;
      }
      break;
    // console I/O
    //
    case 2: // console data
      if (iotype==IOWRITE) {
        console.send(regd);
      }
      else {
        if (console.rxready()) {
          PORTA=console.receive();
        }
        else
          PORTA=0;
        DDRA=0xff;
      }
      break;
    case 3: // console status
      if (!(iotype==IOWRITE)) {
        regd=console.rxready()?1:0;
        regd|=console.txempty()?2:0;
        PORTA=regd;
        DDRA=0xff;
      }
      break;
    case 4: // aux data
      if (SUART_IsEnabled()) {
        if (iotype==IOWRITE) {
          SUART_Send(regd);
        }
        else {
          PORTA=SUART_Receive();
          DDRA=0xff;
        }
      }
      break;
    case 5: // aux control/status
      if (iotype==IOWRITE) {
        auxbaud=regd&7;
        if (regd&0x80)
          SUART_Enable(baudrates[auxbaud]);
        else
          SUART_Disable();
      }
      else {
        regd=SUART_IsEnabled()?0x80:0;
        regd|=SUART_RxCount()?0x40:0;
        regd|=SUART_TxCount()?0x20:0;
        regd|=SUART_TxFull()?0x10:0;
        regd|=auxbaud;
        PORTA=regd;
        DDRA=0xff;
      }
      break;
    
    //
    // misc functions
    //
    case 0: // command
      if (iotype==IOWRITE) {
        switch (regd) {
          case 0: // load bios
            state=BIOS;
            count=0;
            break;
          case 1: // reboot
            copy_bootloader();
            state=IDLE;
            break;
          case 2: // load CP/M
            state=CPM;
            count=0;
            break;
          case 3: // reset timecounter
            timecounter=0;
            break;
          case 4: // read timecounter
            state=TIMECOUNTER;
            timecountersnapshot=timecounter;
            break;
          default:
            state=IDLE;
            break;
        }
      }
      break;
    case 1: // data
      switch (state) {
        case BIOS: // reading bios code
          if (!(iotype==IOWRITE)) {
            if (count>=sizeof(monitor_bin))
              PORTA=0x55;
            else
              PORTA=pgm_read_byte(&monitor_bin[count]);
            DDRA=0xff; // put the byte on data bus
            count++;
          }
          break;
        case CPM:
          if (!(iotype==IOWRITE)) {
            if (count>=sizeof(cpm_bin))
              PORTA=0x55;
            else
              PORTA=pgm_read_byte(&cpm_bin[count]);
            DDRA=0xff; // put the byte on data bus
            count++;
          }
          break;
        case TIMECOUNTER:
          if (!(iotype==IOWRITE)) {
            PORTA=timecountersnapshot&255;
            timecountersnapshot>>=8;
            DDRA=0xff;
          }
          break;
        default:
          state=IDLE;
          break;
      }
      break;
  }
  if (DDRA==0xff) {
    // if data was placed on z80 data bus then release Z80 from wait and
    // immediately release data bus after that
    cli();
    z80_release();
    sei();
  } else {
    cli();
    z80_quickrelease();
    sei();    
  }
  iotype=IONONE;
}

ISR(TIMER2_OVF_vect)
{
  TCNT2=255-180;
  timecounter++;
  console.tick();
}


/*
I/O configuration
-----------------
I/O pin                               direction    DDR  PORT
PA0 D0                                input        0    1
PA1 D1                                input        0    1
PA2 D2                                input        0    1
PA3 D3                                input        0    1
PA4 D4                                input        0    1
PA5 D5                                input        0    1
PA6 D6                                input        0    1
PA7 D7                                input        0    1

PB0 A6                                input        0    1
PB1 /WR                               input        0    1
PB2 /MREQ                             input        0    1
PB3 Z80CLK                            input        0    1
PB4 /SS                               input        0    1
PB5 MOSI                              input        0    1
PB6 MISO                              input        0    1
PB7 SCK                               input        0    1

PC0 SCL / sw uart tx                  input        0    0
PC1 SDA / sw uart rx                  input        0    0
PC2 A0                                input        0    1
PC3 A1                                input        0    1
PC4 A2                                input        0    1
PC5 A3                                input        0    1
PC6 A4                                input        0    1
PC7 A5                                input        0    1

PD0 RxD                               input        0    1
PD1 TxD                               output       1    1
PD2 /IOSELE0                          input        0    0
PD3 /BUSAK                            input        0    1
PD4 /BUSRQ                            input        0    1
PD5 /RESET                            output       1    0
PD6 /RESUME                           output       1    1
PD7 /RD                               input        0    1
*/
int main(void)
{
  MCUSR=0;
  MCUCR=(1<<JTD); // this does not actually seem to help, fuse needs to be blown too
  // I/O directions and initial state
  DDRA=0x00;
  PORTA=0xff;
  DDRB=0x00;
  PORTB=0xff;
  DDRC=0x00;
  PORTC=0xfc;
  DDRD=0x62;
  PORTD=0xdb;
  //
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  // configure watchdog to interrupt&reset, 4 sec timeout
  WDTCSR|=0x18;
  WDTCSR=0xe8;
  // configure timer2 for generic timekeeping, triggering
  // overflow interrupts every 10ms
  TCCR2A=0;
  TCCR2B=7;
  TCNT2=255-180;
  TIMSK2=1;
  // set up external interrupt for z80 interface
  EICRA=2; // falling edge INT0 interrupts
  EIFR=3;
  EIMSK=1;
  sei();
  _delay_ms(100);
  console.init();
  console.print("\ec\x0f\e[H\e[2J");
  sdcard.Init();
  checkdisk();
  enable_z80_clock();
  copy_bootloader();

  // aux serial enabled at 9600 by default
  auxbaud=5;
  SUART_Enable(baudrates[auxbaud]);
  
  while (1) {
    sleep_cpu(); // any interrupt, including watchdog, wakes up
    wdt_reset();
    WDTCSR|=0x40;
    if (iotype!=IONONE)
      ProcessCommand();
  }
}
