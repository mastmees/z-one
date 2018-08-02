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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "queue.hpp"
#include "softwareuart.hpp"

// adjust these for actual I/O pins
#define rx_pin_low() (!(PINC&2))
#define set_tx_low() PORTC&=~1
#define set_tx_high() PORTC|=1
#define make_tx_output() PORTC|=1; DDRC|=1
#define make_tx_input() PORTC|=1; DDRC&=~1
#define make_rx_input() PORTC|=2; DDRC&=~2
// pin change interrupts on Rx
#define configure_pci() PCMSK2|=2
#define disable_pci() PCICR&=~4
#define enable_pci()  PCICR|=4
#define PCIVECTOR PCINT2_vect

// software uart is using 2 timers, one for transmitter,
// another for receiver. timer 1 is used for transmit,
// timer 3 is used for receive.
//
// receiver starts on pin change interrupt, waits for half
// bit length, then samples the input in a middle of each
// bit to collect the byte
//
// transmitter starts the bit timer when a byte is in queue,
// clocks out the bits and if the queue is empty stops the timer
// again
//
enum TXSTATE  { TXIDLE,STARTSENT,SENDBITS,SENDSTOP,FINISH };
enum RXSTATE  { RXIDLE,RXINIT,STARTRECEIVED,RECEIVEBITS,RECEIVESTOP };

static Queue<uint8_t,32> rxbuf,txbuf;
static TXSTATE txstate;
static RXSTATE rxstate;
static uint16_t clockrate,halfclock;
static volatile bool enabled;

// when tx goes low, start timer for half bit length
ISR(PCIVECTOR)
{
  if (rx_pin_low() && rxstate==RXIDLE) {
    PCICR=0;
    TCNT3=halfclock;
    TCCR3B=2; // start timer, clock prescaler 8
    rxstate=RXINIT;
  }
}

// timer3 overflow interrupts run receiver state machine
ISR(TIMER3_OVF_vect)
{
static uint8_t byte,bits;
  TCNT3=clockrate;
  switch (rxstate) {
    case RXINIT:
      if (rx_pin_low()) {
        rxstate=STARTRECEIVED;
      }
      else {
        TCCR3B=0;
        PCICR=4; // enable pin change interrupt again
        rxstate=RXIDLE;
      }
      break;
    case STARTRECEIVED:
      if (rx_pin_low()) {
        byte=0;
      }
      else {
        byte=0x80;
      }
      bits=7;
      rxstate=RECEIVEBITS;
      break;
    case RECEIVEBITS:
      byte>>=1;
      if (!rx_pin_low()) {
        byte|=0x80;
      }
      bits--;
      if (!bits)
        rxstate=RECEIVESTOP;
      break;
    case RECEIVESTOP:
      rxbuf.Push(byte);
      TCCR3B=0;
      PCICR=4; // enable pin change interrupt on rx again
      rxstate=RXIDLE;
      break;
    default:
      rxstate=RXIDLE;
      break;
  }
}

// timer1 interrupts run transmitter state machine
ISR(TIMER1_OVF_vect)
{
static uint8_t byte,bits;
  TCNT1=clockrate;
  switch (txstate) {
    case STARTSENT:
      byte=txbuf.Pop();
      if (byte&1) {
        set_tx_high();
      }
      else {
        set_tx_low();
      }
      byte>>=1;
      bits=7;
      txstate=SENDBITS;
      break;
    case SENDBITS:
      if (byte&1) {
        set_tx_high();
      }
      else {
        set_tx_low();
      }
      byte>>=1;
      bits--;
      if (!bits)
        txstate=SENDSTOP;
      break;
    case SENDSTOP:
      set_tx_high();
      txstate=FINISH;
      break;
    case FINISH: // check queue in case something was inserted
      if (txbuf.Count()) {
        set_tx_low(); // start bit
        txstate=STARTSENT;
      }
      else {
        TCCR1B=0; // stop transmit timer
        txstate=TXIDLE;
      }
      break;
    default:
      txstate=TXIDLE;
      break;
  }
}

void SUART_Disable()
{
  enabled=false;
  disable_pci();
  make_tx_input();
  txbuf.Purge();
  rxbuf.Purge();
  txstate=TXIDLE;
  rxstate=RXIDLE;
  TCCR1B=0;
  TCCR3B=0;
  TIMSK1=0;
  TIMSK3=0;
}

void SUART_Enable(uint32_t baud)
{
  make_tx_output();
  make_rx_input();
  txstate=TXIDLE;
  rxstate=RXIDLE;
  // number of clock periods per bit
  clockrate=(F_CPU/8)/baud;
  halfclock=clockrate>>1;
  clockrate=(0xffff-clockrate)+8;
  halfclock=(0xffff-halfclock)+8;  
  // initialize both timers, but disable clock
  // so that they are stopped, to start
  // the timer, just TCNTx value needs to be
  // set and TCCRxB needs to be set to 1
  TCNT1=0;  // ensure no interrupt occurs
  TCCR1A=0; // outputs disconnected, mode0
  TCCR1B=0; // mode0, clock disconnected
  TIFR1=1;  // clear any pending interrupt
  TIMSK1=1; // enable overflow interrupts
  TCNT3=0;  // ensure no interrupt occurs
  TCCR3A=0; // outputs disconnected, mode0
  TCCR3B=0; // mode0, clock disconnected
  TIFR3=1;  // clear any pending interrupt
  TIMSK3=1; // enable overflow interrupts
  
  // configure pin change interrupts on PC1
  configure_pci();
  enable_pci();
  enabled=true;
}

void SUART_Send(uint8_t c)
{
  txbuf.Push(c);
  switch (txstate) {
    case TXIDLE:
      set_tx_low(); // start bit
      TCNT1=clockrate;
      TCCR1B=2; // start timer, clock prescaler 8
      txstate=STARTSENT;
      break;
    default:
      break;
  }
}

uint8_t SUART_Receive()
{
  return rxbuf.Pop();
}

uint8_t SUART_RxCount()
{
  return rxbuf.Count();
}

uint8_t SUART_TxCount()
{
  return txbuf.Count();
}

bool SUART_TxFull()
{
  return txbuf.IsFull();
}

bool SUART_IsEnabled()
{
  return enabled;
}
