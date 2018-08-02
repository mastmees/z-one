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
#ifndef __console_hpp__
#define __console_hpp__
#include <avr/io.h>
#include <avr/interrupt.h>
#include "queue.hpp"

class Console
{
enum SSTATE { PASSTHROUGH, ESCRECEIVED,YRECEIVED,ROWRECEIVED, ANSIRECEIVED };
enum KSTATE { NO_KEY, ESC_KEY, ANSI_KEY, O_KEY, COLLECT1, COLLECT2 };
enum KMODE { NORMAL, ALTERNATE, WORDSTAR };
Queue<uint8_t,32> kqueue;
SSTATE sstate;
KSTATE kstate;
KMODE altkeypad;
uint8_t row,tickcount;
  void output(uint8_t c);
  void output(const char *s);
  void outputint(int32_t n);
  void kpushn(int16_t n);
public:
  void init();
  bool rxready();
  uint8_t receive();
  bool txempty();
  bool txfull();
  void send(uint8_t c);
  void print(const char *s);
  void print(int32_t n);
  void phex(uint8_t c);
  void pxdigit(uint8_t c);

  inline void tick()
  {
    if (tickcount) {
      tickcount--;
      if (!tickcount && kstate==ESC_KEY)
      {
        kqueue.Push(0x1b);
        kstate=NO_KEY;
      }
    }
  }

};

#endif
