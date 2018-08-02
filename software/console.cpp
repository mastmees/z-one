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

#include "console.hpp"

#define isdigit(c) (c>='0' && c<='9')
#define BAUDRATE 115200L
#define UBRR (F_CPU/(16L*BAUDRATE)-1)

// low-level serial port tx and rx queues
Queue<uint8_t,64> txqueue,rxqueue;

ISR(USART0_RX_vect) // Rx complete
{
  rxqueue.Push(UDR0);
}

ISR(USART0_UDRE_vect) // Data register empty
{
  if (txqueue.Count())
    UDR0=txqueue.Pop();
  else { // disable tx interrupts if no data
    UCSR0B&=~(1<<UDRIE0);
  }
}

static void _init()
{
  UBRR0H=(unsigned char)(UBRR>>8);
  UBRR0L=(unsigned char)UBRR;
  UCSR0A=0;
  UCSR0B=(1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
  UCSR0C=(1<<USBS0)|(3<<UCSZ00);
}

static bool _rxready()
{
  return rxqueue.Count()!=0;
}  
  
static uint8_t _receive()
{
  while (!_rxready()); // blocks caller until data is received
  return rxqueue.Pop();
}

static bool _txempty()
{
  return txqueue.Count()==0;
}

static bool _txfull()
{
  return txqueue.IsFull();
}

static void _send(uint8_t c)  
{
  while (_txfull());
  cli();
  txqueue.Push(c);
  UCSR0B|=(1<<UDRIE0);
  sei();
}

void Console::init()
{
  _init();
  sstate=PASSTHROUGH;
  kstate=NO_KEY;
  tickcount=0;
  altkeypad=NORMAL;
}

/*
  VT-52 key sequences
  
  left blank key            ESC P
  mid blank key             ESC Q
  right blank key           ESC R
  UP arrow                  ESC A
  DOWN arrow                ESC B
  RIGHT arrow               ESC C
  LEFT arrow                ESC D
  
  numeric keypad   normal      alternate
  0                0           ESC ? p
  1                1           ESC ? q
  2                2           ESC ? r
  3                3           ESC ? s
  4                4           ESC ? t
  5                5           ESC ? u
  6                6           ESC ? v
  7                7           ESC ? w
  8                8           ESC ? x
  9                9           ESC ? y
  .                .           ESC ? n
  ENTER            CR          ESC ? M
*/
void Console::kpushn(int16_t n)
{
  if (n>9)
    kpushn(n/10);
  kqueue.Push((n%10)+'0');
}

bool Console::rxready()
{
uint8_t c;
static int16_t arg1,arg2;
  while (_rxready()) {
    c=_receive();
    switch (kstate) {
      default:
        if (c==0x1b) {
          kstate=ESC_KEY;
          tickcount=5;
        }
        else
          kqueue.Push(c);
        break;
      case ESC_KEY: // have received ESC from terminal
        switch (c) {
          default:
            kqueue.Push(0x1b);
            kqueue.Push(c);      
            kstate=NO_KEY;
          case '[':
            kstate=ANSI_KEY;
            arg1=arg2=-1;
            break;
          case 'O':
            kstate=O_KEY;
            break;
        }
        break;
      case COLLECT1: // collect first numeric value after Esc [
        if (isdigit(c)) {
          arg1=arg1*10+(c-'0');
        }
        else {
          switch (c) {
            case ';':
              kstate=COLLECT2;
              break;
            case '~':
              // ins  Esc [ 2 ~
              // del  Esc [ 3 ~
              // home Esc [ 1 ~
              // end  Esc [ 4 ~
              // pgup Esc [ 5 ~
              // pgdn Esc [ 6 ~
              if (altkeypad==WORDSTAR) {
                switch (arg1) {
                  case 1: // CtrlQ-S - start of row in WS
                    kqueue.Push(0x11);
                    kqueue.Push('S');
                    break;
                  case 4: // CtrlQ-D - end of row in WS
                    kqueue.Push(0x11);
                    kqueue.Push('D');
                    break;
                  case 3: // Ctrl-G - delete character
                    kqueue.Push(0x07);
                    break;
                  case 2: // Ctrl-V - toggle insert/overwrite
                    kqueue.Push(0x16);
                    break;
                  case 5: // CtrlR - page up in WS
                    kqueue.Push(0x12);
                    break;
                  case 6: // CtrlC - page down in WS
                    kqueue.Push(0x03);
                    break;
                  default:
                    break;
                }
                kstate=NO_KEY;
                break;
              } // if not in wordstar mode then fall through to default handling
            default:
              // must forward the Esc [ <num> c
              // or Esc [ c if arg1 is still -1
              kqueue.Push(0x1b);
              kqueue.Push('[');
              if (arg1>=0)
                kpushn(arg1);
              kqueue.Push(c);
              kstate=NO_KEY;
          }
        }
        break;
      case COLLECT2: // collect second numeric value and watch for letter
        if (isdigit(c)) {
          if (arg2<0)
            arg2=0;
          arg2=arg2*10+(c-'0');
        }
        else {
          if (c=='R') { // cursor position report
            kqueue.Push(0x1b);
            kqueue.Push('y');
            kqueue.Push(arg1+31);
            kqueue.Push(arg2+31);
            kstate=NO_KEY;
          }
          else {
            // must forward the Esc [ <num1> ; <num2> c
            // or Esc [ <num1> c if arg2 is still -1
            kqueue.Push(0x1b);
            kqueue.Push('[');
            kpushn(arg1);
            if (arg2>=0) {
              kqueue.Push(';');
              kpushn(arg2);
            }
            kqueue.Push(c);
            kstate=NO_KEY;
          }
        }
        break;
      case ANSI_KEY:
        switch (c) {
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            arg1=c-'0';
            kstate=COLLECT1;
            break;
          case 'A':
          case 'B':
          case 'C':
          case 'D':
            if (altkeypad==WORDSTAR) {
              switch (c) {
                case 'A':
                  kqueue.Push(0x05);
                  break;
                case 'B':
                  kqueue.Push(0x18);
                  break;
                case 'C':
                  kqueue.Push(0x04);
                  break;
                case 'D':
                  kqueue.Push(0x13);
                  break;
              }
            }
            else {
              kqueue.Push(0x1b);
              kqueue.Push(c);
            }
            kstate=NO_KEY;
            break;
          default:
            kqueue.Push(0x1b);
            kqueue.Push('[');
            kqueue.Push(c);
            kstate=NO_KEY;
            break;
        }
        break;
      case O_KEY:
        switch (c) {
          case 'A':
          case 'B':
          case 'C':
          case 'D':
            if (altkeypad==WORDSTAR) {
              switch (c) {
                case 'A':
                  kqueue.Push(0x05);
                  break;
                case 'B':
                  kqueue.Push(0x18);
                  break;
                case 'C':
                  kqueue.Push(0x04);
                  break;
                case 'D':
                  kqueue.Push(0x13);
                  break;
              }
            }
            else {
              kqueue.Push(0x1b);
              kqueue.Push(c);
            }
            kstate=NO_KEY;
            break;
          case 'P':
          case 'Q':
          case 'R':
          case 'S':
            kqueue.Push(0x1b);
            kqueue.Push(c);
            kstate=NO_KEY;
            break;
          default:
            kqueue.Push(0x1b);
            kqueue.Push('O');
            kqueue.Push(c);
            kstate=NO_KEY;
            break;
        }
        break;
    }
  }
  return kqueue.Count();
}  

uint8_t Console::receive()
{
  while (!rxready())
    ;
  return kqueue.Pop();
}

bool Console::txempty()
{
  return _txempty();
}

bool Console::txfull()
{
  return _txfull();
}

void Console::output(uint8_t c)
{
  _send(c);
}

void Console::output(const char *s)
{
  while (s && *s) {
    output((uint8_t)*s);
    s++;
  }
}

void Console::outputint(int32_t n)
{
    if (n<0) {
      output((uint8_t)'-');
      n=0-n;
    }
    if (n>9)
      outputint(n/10);
    output((uint8_t)((n%10)+'0'));
}

/*
VT52 Escape Sequences

  output sequences
  LF      line feed
  ESC B   cursor down, limited to bottom of screen
  ESC I   reverese index - move up, scroll down at top
  ESC A   cursor up, limited to top of screen
  ESC C   cursor forward, limited to right side
  BS      cursor left, limited to left side
  CR	  move cursor to start of line
  ESC H   cursor home
  TAB	  tab stops at 9,17,25,33,41,49,57,65,73. If the cursor was at a TAB stop to begin with, it moves rightward to the next TAB stop. If the cursor was in columns 73 - 79, it simply moves rightward one column. If the cursor was in column 80, it does not move.
  ESY Y <r><c>  move cursor, row and column are chars
          with ascii code of row or col value +31 where
          first row and column are 1
  ESC K   erase to end of line
  ESC J   erase end of screen
  ESC Z   identify. VT-52 without copier responds with ESC / K
  ESC [   enter hold screen (not implemented)
  ESC \   exit hold screen (not implemented)
  ESC =	  enter alternate keypad mode
  ESC >	  exit alternate keypad mode
  ESC F   enter graphics mode
  ESC G   exit graphics mode

  //
  // these are heathkit extensions
  //
  FF	Formfeed. clear screen and home cursor
  ESC D	Cursor left.
  ESC E Clear display. This control sequence clears the entire screen and positions the cursor into the upper left screen corner (Home Position).
  ESC <	Enter ANSI mode.
  ESC d Clear Screen up to Cursor Position. This sequence clears the screen starting at and including the current cursor position. The position of the cursor remains unchanged.
  ESC e (Cursor On) This escape sequence makes the cursor become visible.
  ESC f (Cursor Off) The cursor is deactivated again.
  ESC j (Save Cursor Position) This sequence is used to save the current cursor position.
  ESC k (Restore Cursor to Saved Position) This is the counterpart to the above function. The cursor is returned to the position previously stored with ESC j.
  ESC l Clear line. The content of the line currently containing the cursor is deleted. All remaining lines are unaffected. After the deletion, the cursor is located in the first column of the deleted line.
  ESC o Clear to start of line. Deletes the beginning of the cursor line up to and including the cursor position. The position of the cursor remains unchanged.
  ESC M	Remove line.
  ESC L	Insert line
  ESC q	Normal video, switch off inverse video text
  ESC p	Reverse video, switch on inverse video text
  ESC j	Save cursor position
  ESC k	Restore cursor position
  ESC w	wrap off
  ESC v	wrap on
  ESC e	Cur_on	Show cursor.
  ESC f	Cur_off	Hide cursor.
  ESC n   cursor pos report
  ESC b   erase beginning of display
  ESC N   delete character
  ESC @   enter insert character mode
  ESC O   exit insert character mode
  ESC z   reset to power-up configuration
  ESC x <P> set mode(s)
          1 enable 25th line
          2 no key click
          3 hold screen mode
          4 block cursor
          5 cursor off
          6 keypad shifted
          7 alternate keypad mode
          8 auto line feed
          9 auto cr
  ESC y <P> reset modes
  ESC <   enter ANSI mode
  ESC b	Foreground color	Set text colour.
  ESC c	Background color	Set background colour.
*/

void Console::send(uint8_t c)  
{
  switch (sstate) {
    default:
      if (c==0x1b) {
        sstate=ESCRECEIVED;
      }
      else {
        output(c);
      }
      break;
    case ANSIRECEIVED:
      switch (c) {
        case '=':
          altkeypad=WORDSTAR;
          break;
        case '>':
          altkeypad=NORMAL;
          break;
        default:
          output(0x1b);
          output('[');
          output(c);
      }
      sstate=PASSTHROUGH;
      break;
    case ESCRECEIVED:
      switch (c) {
        default:
          output(0x1b);
          output(c);
          sstate=PASSTHROUGH;
          break;
        case '[':
          sstate=ANSIRECEIVED;
          break;
        case 'E': //Clear display. This control sequence clears the entire screen 
          //and positions the cursor into the upper left screen corner (Home Position).
          output(0x1b);
          output('[');
          output('2');
          output('J');
          output(0x1b);
          output('[');
          output('H');
          sstate=PASSTHROUGH;
          break;
        case 'b': // Clear Screen up to Cursor Position. This sequence clears the screen starting at
         //and including the current cursor position. The position of the cursor remains unchanged.
          output(0x1b);
          output('[');
          output('1');
          output('J');
          sstate=PASSTHROUGH;
          break;
        case 'l': //Clear line. The content of the line currently containing the cursor is deleted. 
          //All remaining lines are unaffected. After the deletion, the cursor is located in the first column of the deleted line.
          output(0x1b);
          output('[');
          output('2');
          output('K');
          output(13);
          sstate=PASSTHROUGH;
          break;
        case 'o': // Clear to start of line. Deletes the beginning of the cursor line up to and including the 
         //cursor position. The position of the cursor remains unchanged.
          output(0x1b);
          output('[');
          output('1');
          output('K');
          sstate=PASSTHROUGH;
          break;
        case 'j': //Save cursor position
          output(0x1b);
          output('7');
          sstate=PASSTHROUGH;
          break;
        case 'k': //Restore cursor position
          output(0x1b);
          output('8');
          sstate=PASSTHROUGH;
          break;
        case 'q': // Normal video, switch off inverse video text
          output(0x1b);
          output('[');
          output('2');
          output('7');
          output('m');
          sstate=PASSTHROUGH;
          break;      
        case 'p': // Reverse video, switch on inverse video text
          output(0x1b);
          output('[');
          output('7');
          output('m');
          sstate=PASSTHROUGH;
          break;
        case 'N': // delete character
          output(0x1b);
          output('[');
          output('P');
          sstate=PASSTHROUGH;
          break;
        case 'w': // wrap off
          output(0x1b);
          output('[');
          output('?');
          output('7');
          output('l');
          sstate=PASSTHROUGH;
          break; 
        case 'v': // wrap on
          output(0x1b);
          output('[');
          output('?');
          output('7');
          output('h');
          sstate=PASSTHROUGH;
          break;
        case 'e': // cursor on
          output(0x1b);
          output('[');
          output('?');
          output('2');
          output('5');
          output('h');
          sstate=PASSTHROUGH;
          break;
        case 'f': // cursor off
          output(0x1b);
          output('[');
          output('?');
          output('2');
          output('5');
          output('l');
          sstate=PASSTHROUGH;
          break;
        case 'n': // cursor pos report, H19 should respond Esc Y row col
          output(0x1b);
          output('[');
          output('6');
          output('n');
          sstate=PASSTHROUGH;
          break;
        case 'D': // cursor left
        case 'C': // cursor forward, limited to right side
        case 'B': // cursor down, limited to bottom of screen
        case 'A': // cursor up, limited to top of screen
        case 'K': // erase to end of line
        case 'J': // erase end of screen
        case 'M': // remove line.
        case 'L': // insert line
        case 'H': // cursor home
          output(0x1b);
          output('[');
          output(c);
          sstate=PASSTHROUGH;
          break;
        case 'I': // reverese index - move up, scroll down at top
          output(0x1b);
          output('M');
          sstate=PASSTHROUGH;
          break;
        case 'Z': // identify. VT-52 without copier responds with ESC / K
          kqueue.Push(0x1b);
          kqueue.Push('/');
          kqueue.Push('K');
          sstate=PASSTHROUGH;
          break;
        case '=': // enter alternate keypad mode
          altkeypad=ALTERNATE;
          sstate=PASSTHROUGH;
          break;
        case '>': // exit alternate keypad mode
          altkeypad=NORMAL;
          sstate=PASSTHROUGH;
          break;
        case '<': // enter ANSI mode
        case 'F': // enter graphics mode
        case 'G': // exit graphics mode
          // silently ignore these
          sstate=PASSTHROUGH;
          break;
        case 'Y':
          sstate=YRECEIVED;
          break;
      }
      break;
    case YRECEIVED:
      row=c-31;
      sstate=ROWRECEIVED;
      break;
    case ROWRECEIVED:
      output(0x1b);
      output('[');
      outputint(row);
      output(';');
      outputint(c-31);
      output('H');                    
      sstate=PASSTHROUGH;
      break;
  }
}

void Console::print(const char *s)
{
  while (s && *s) {
    send(*s++);
  }
}

void Console::print(int32_t n)
{
    if (n<0) {
      send('-');
      n=0-n;
    }
    if (n>9)
      print(n/10);
    send((n%10)+'0');
}

void Console::pxdigit(uint8_t c)
{
  c=(c&0x0f)+'0';
  if (c>'9')
    c+=7;
  send(c);
}

void Console::phex(uint8_t c)
{
  pxdigit(c>>4);
  pxdigit(c);
}

