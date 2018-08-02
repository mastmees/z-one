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
#ifndef __sdcard_hpp__
#define __sdcard_hpp__
#include <util/delay.h>
#include <avr/wdt.h>
#include "console.hpp"

#define noDEBUG

extern Console console;

#define SS (1<<PB4)
#define MOSI (1<<PB5)
#define MISO (1<<PB6)
#define SCK (1<<PB7)
#define CS_LOW() (PORTB&=~SS)
#define CS_HIGH() (PORTB|=SS)

#define no_error(c) (c!=0xff && (c&0x7e)==0)

class SDCard
{
public:
  // SD card type as detected by Init()
  enum SDType : uint8_t {
    UNKNOWN,
    NONE,
    SD1,
    SD2,
    SDHC
  };

  // only 1,8,55+41,58,59 commands are accepted in initialization phase
  enum SDCommand : uint8_t {
      GOIDLE = 0,
      SENDOPCOND = 1,
      SENDIFCOND = 8, // 1+4 byte response
      SENDCSD = 9,
      SENDCID = 10,
      STOPTRANSMISSION = 12,
      SETBLOCKLEN = 16,
      READBLOCK = 17,
      READMULTI = 18,
      SETBLOCKCOUNT = 23,
      WRITEBLOCK = 24,
      WRITEMULTI = 25,
      APPLICATIONCMD = 55,
      READOCR = 58, // 1+4 byte response
      APPSENDOPCOND = 41
  };

  // bits in response byte
  enum SDResponse : uint8_t {
    IDLE = 0x01, 
    ERASE_RESET = 0x02,
    ILLEGAL_COMMAND = 0x04,
    CRC_ERROR = 0x08,
    ERASE_SEQUENCE_ERROR = 0x10,
    ADDRESS_ERROR = 0x20,
    PARAMETER_ERROR = 0x40,
    NO_RESPONSE = 0x80
  };

private:
  SDType Type;    // Init() sets this to UNKOWN initially
                  // but ends with detected type or NONE
  bool blockmode; // true if card accepts block addressing
                  // this is handled in ReadSector()/WriteSector() that always
                  // use block addressing
  uint8_t dskbuf[512];  // reserve sector read/write buffer here, as the class wont
                  // be of much use without it anyway
  uint32_t TotalSectors; // total number of available sectors after the Type has been set
  
  uint8_t spiwrite(uint8_t ch)
  {
    SPDR = ch;
    while (!(SPSR&(1<<SPIF)));       
    return SPDR;
  }

  uint8_t readywait()
  {
    uint8_t c;
    for (uint16_t i=0;i<2000;i++) {
      if ((c=spiwrite(0xff))==0xff) {
        return c;
      }
      #ifdef DEBUG
      console.phex(c);
      #endif
      _delay_ms(1);
      wdt_reset();
      WDTCSR|=0x40;
    }
    return c;
  }

  // read responses until a byte with high bit clear appears
  uint8_t getresponse()
  {
    uint8_t c;
    for (uint16_t i=0;i<2000;i++) {
       c=spiwrite(0xff);
       #ifdef DEBUG
       console.phex(c);
       #endif
       if (!(c&0x80)) {
         return c;
       }
       _delay_us(10);
       wdt_reset();
       WDTCSR|=0x40;
    }
    return c;
  }
        
public:

  uint32_t GetTotalSectors()
  {
    return TotalSectors;
  }
  
  SDType GetType()
  {
    return Type;
  }
  
  uint8_t *GetBuf()
  {
    return dskbuf;
  }

  void Select()
  {
    _delay_us(2);
    CS_LOW();
    _delay_us(1);
  }
  
  void DeSelect()
  {
    CS_HIGH();
  }

  // send a command to SD card and read response. the caller must
  // know how many data bytes after the response code there will be
  // depending on command. if buf==NULL then the data bytes are read
  // over SPI but discarded
  //
  // returns the SD card response code
  //
  // Select() and DeSelect() must be called before and after
  //
  uint8_t Command(SDCommand cmd,uint32_t d,uint16_t count=0,uint8_t* buf=0)
  {
    uint8_t c=0xff,cc;
    uint16_t i=0;
    #ifdef DEBUG
    console.print("cmd: ");
    console.print((int)cmd);
    console.print(" ");
    #endif
    if (readywait()!=0xff) {
      #ifdef DEBUG
      console.print("not ready\r\n");
      #endif
      return 0xff;
    }
    // use known crc values for command that require correct crc, on
    // assumption that d is always 0 for these
    switch (cmd) {
      case GOIDLE:
        c=0x95;
        break;
      case SENDIFCOND:
        c=0x87;
        break;
      default:
        break;
    }
    #ifdef DEBUG
    console.print("(");
    console.phex(cmd|0x40);
    console.phex(d>>24);
    console.phex(d>>16);
    console.phex(d>>8);
    console.phex(d);
    console.phex(c);
    console.print(") ");
    #endif
    spiwrite(cmd|0x40);
    spiwrite(d>>24);
    spiwrite(d>>16);
    spiwrite(d>>8);
    spiwrite(d);
    spiwrite(c); // crc
    c=getresponse();
    if (c&0x80) {
      #ifdef DEBUG
      console.phex(c);
      console.print("\r\n");
      #endif
      return c;
    }
    #ifdef DEBUG
    console.phex(c);
    #endif
    while (count--) {
      cc=spiwrite(0xff);
      #ifdef DEBUG
      console.phex(cc);
      #endif
      if (buf)
        buf[i++]=cc;
    }
    #ifdef DEBUG
    console.print("\r\n");
    #endif
    return c;
  }

  void Invalidate()
  {
    blockmode=false;
    Type=SDType::NONE;
    TotalSectors=0;
  }
  
  // returns type of the card detected
  uint8_t Init(bool quiet=false)
  {
    uint8_t rbuf[4],c;
    uint16_t i;
    uint8_t j;
    DDRB |= SS + MOSI + SCK; // SS, MOSI and SCK as outputs
    SPCR=(1<<SPE)|(1<<MSTR); // normal speed clock/4
    Invalidate();
    Type=SDType::UNKNOWN;
    _delay_ms(10);
    // send 80 clocks without CS asserted
    DeSelect();
    for (j=0;j<10;j++)
      spiwrite(0xff);
    Select();
    c=Command(SDCard::GOIDLE,0); // switch to SPI mode
    if (c!=0x01) {
      Type=NONE;
      DeSelect();
      return Type;
    }
/*
    for (j=0;j<10;j++) {
      c=spiwrite(0xff);
      #ifdef DEBUG
      console.phex(c);
      #endif
    }
*/
    #ifdef DEBUG
    console.print("\r\n");
    #endif
    c=Command(SDCard::SENDIFCOND,0x1aa,4,rbuf);
    // initialize card, this may take few hundreds ms on large cards
    c=Command(APPLICATIONCMD,0);
    c=Command(APPSENDOPCOND,0x40000000);
    if (no_error(c)) { //responded, and not an error    
      for (i=0;i<100;i++) {
        Command(APPLICATIONCMD,0);
        c=Command(APPSENDOPCOND,0x40000000);
        if (no_error(c) && (!(c&1)))
          break;
        _delay_ms(100);
        wdt_reset();
        WDTCSR|=0x40;
      }
    }      
    else { // did not respond to app cmd, try plain
      for (i=0;i<100;i++) {
        c=Command(SDCard::SENDOPCOND,0x00000000);
        if (no_error(c) && (!(c&1)))
          break;
        _delay_ms(100);
        wdt_reset();
        WDTCSR|=0x40;
      }
    }
    DeSelect();
    if (c!=0) {
      if (!quiet)
        console.print("SD card init failed\r\n");
      Type=NONE;
      return Type;
    }
    else {
      if (!quiet)
        console.print("SD card found\r\n");
    }
    SPSR|=(1<<SPI2X); // double speed, clock/2
    // try SENDIFCOND, only supported on v2+ cards
    Select();
    c=Command(SDCard::SENDIFCOND,0x01aa,4,rbuf);
    if (!quiet)
      console.print(" Type: ");
    if ((c&SDResponse::ILLEGAL_COMMAND) || rbuf[2]!=1 || rbuf[3]!=0xaa) {
      Type=SDType::SD1;
      if (!quiet)
        console.print("SD1\r\n");
    }
    else {
      Type=SDType::SD2;
      // check if it is SDHC
      c=Command(SDCard::READOCR,0,4,rbuf);
      if (no_error(c) && (rbuf[0]&0x40)==0x40) {
        Type=SDType::SDHC;
        if (!quiet)
          console.print("SDHC");
      }
      else {
        if (!quiet)
          console.print("SD2");
      }
      if (!quiet)
        console.print("\r\n");
      c=Command(SDCard::SETBLOCKLEN,512L);
      if (no_error(c)) {
        blockmode=true;
        if (!quiet)
          console.print(" Using block addressing\r\n");
      }
    }
    DeSelect();
    //
    c=ReadSector(0,dskbuf,16,SENDCSD);
    if (!c) {
      #ifdef DEBUG
      console.print("CSD: ");
      for (c=0;c<16;c++) {
        console.print(" ");
        console.phex(dskbuf[c]);
      }
      console.print("\r\n");
      #endif
      uint8_t read_bl_len=dskbuf[5]&15;
      if ((dskbuf[0]&0xc0)==0) { // v1.0
        // in version 1 structure the available space can be calculated in physical blocks
        // and addressing is in bytes, but must be on read_bl_len boundaries (0,512,1024...)
        // to make the sizing of different versions the same, we'll recalculate the total into
        // number of 512 byte sectors, as partitioning and reading/writing will be using that
        // unit anyway
        uint16_t c_size=(((uint16_t)dskbuf[6]<<10) | ((uint16_t)dskbuf[7]<<2) | (dskbuf[8]>>6))&0xfff;
        uint8_t c_size_mult=((dskbuf[10]>>7)|(dskbuf[9]<<1))&7;
        uint16_t blocksize;
        #ifdef DEBUG
        console.print(" bl_len:");
        console.print(read_bl_len);
        console.print(", c_size:");
        console.print(c_size);
        console.print(", c_size_mult:");
        console.print(c_size_mult);
        console.print("\r\n");
        #endif
        blocksize=(uint32_t)1<<read_bl_len;
        TotalSectors=(uint32_t)(c_size+1)*(1<<(c_size_mult+2));
        TotalSectors*=blocksize/512;
      }
      else {
        // assume 2.0 layout
        // the size calculation here is completely different - (csize+1)*512K
        // so here we are converting it to total number of 512 byte sectors
        uint32_t c_size=(((uint32_t)dskbuf[7]&0x3f)<<16)|((uint32_t)dskbuf[8]<<8)|dskbuf[9];
        TotalSectors=(c_size+1)*1024;
        #ifdef DEBUG
        console.print(" c_size:");
        console.print(c_size);
        console.print("\r\n");
        #endif
      }
      if (!quiet) {
        console.print(" Capacity: ");
        console.print(TotalSectors);
        console.print(" sectors (");
        console.print(((TotalSectors/1000)*512)/1000);
        console.print(" MB)\r\n");
      }
    }
    c=ReadSector(0,dskbuf,16,SENDCID);
    if (!c) {
/*
      console.print("CID: ");
      for (c=0;c<16;c++) {
        console.print(" ");
        console.phex(dskbuf[c]);
      }
      console.print("\r\n");
*/
      if (!quiet) {
        console.print(" Model MID:");
        console.phex(dskbuf[0]);
        console.print(" OID:");
        console.phex(dskbuf[1]);
        console.send(',');
        console.phex(dskbuf[2]);
        console.print(" name:");
        for (c=3;c<8;c++)
          console.send(dskbuf[c]);
        console.print("\r\n");
      }
    }
    return Type;
  }

  // read sector function, but it can also read SDC and SID registers (see Init())
  // returns 0x00 on success
  //         nonzero error code with bits
  //         xxxxxxx1 error
  //         xxxxxx1x CC error
  //         xxxxx1xx card ECC failed
  //         xxxx1xxx out of range
  //         xxx1xxxx card is locked
  //         xx1xxxxx data rejected due to crc error
  //         x1xxxxxx data rejected due to write error
  //         0xff general failure
  //
  uint8_t ReadSector(uint32_t blocknumber,uint8_t *data,
    uint16_t len=512,SDCommand cmd=READBLOCK)
  {
    uint8_t c;
    uint16_t j;
    if (cmd==READBLOCK && ((Type==SDType::UNKNOWN) || (Type==SDType::NONE) || (blocknumber>=TotalSectors)))
      return 0xff;
    Select();
    if (!blockmode)    // if card does not support block addressing directly then
      blocknumber<<=9; // multiply address with 512 to get byte offset to sector
    c=Command(cmd,blocknumber);
    if (no_error(c)) {
      // command was accepted, look for data token
      for (j=0;j<1000;j++) {
        c=spiwrite(0xff);
        if (c!=0xff)
          break;
        _delay_us(10);
      }
      #ifdef DEBUG
      console.phex(c);
      #endif
      if (c==0xff) {
        DeSelect();
        return c;
      }
      if (c==0xfe) { // data token
        for (j=0;j<len;j++) {
          data[j]=spiwrite(0xff);
        }
        spiwrite(0xff); // read and discard crc bytes
        spiwrite(0xff);
        c=0;
      }
    }
    spiwrite(0xff); // extra 8 clocks, appearantly sandisk wants them
    DeSelect();
    return c;
  }

  // write sector
  // returns 0x00 on success
  //         nonzero error code with bits
  //         xxxxxxx1 error
  //         xxxxxx1x CC error
  //         xxxxx1xx card ECC failed
  //         xxxx1xxx out of range
  //         xxx1xxxx card is locked
  //         xx1xxxxx data rejected due to crc error
  //         x1xxxxxx data rejected due to write error
  //         0xff general failure  
  //
  uint8_t WriteSector(uint32_t blocknumber,uint8_t *data,
                      SDCommand cmd=WRITEBLOCK,uint16_t len=512)
  {
    uint8_t c=0xff;
    if (cmd==WRITEBLOCK && ((Type==SDType::UNKNOWN) || (Type==SDType::NONE) || (blocknumber>=TotalSectors)))
      return 0xff;
    Select();
    if (!blockmode)
      blocknumber<<=9; // multiply address with 512 to get sector offset
    if (readywait()!=0xff) {
      DeSelect();
      return 0xff;
    }
    c=Command(cmd,blocknumber);
    #ifdef DEBUG
    console.phex(c);
    #endif
    if (no_error(c)) {
      spiwrite(0xff);
      spiwrite(0xff);
      spiwrite(0xfe);  // data token
      for (uint16_t i=0;i<512;i++) {
        spiwrite(*data++);
      }
      spiwrite(0xff);
      spiwrite(0xff);
      c=spiwrite(0xff); // get response
      spiwrite(0xff);
      #ifdef DEBUG
      console.phex(c);
      #endif
      if ((c&0x1f)==5) {
        c=readywait();
        #ifdef DEBUG
        console.phex(c);
        #endif
        if (c==0xff)
          c=0;
      }
      else {
        if (c!=0xff) {
          if ((c&0x1f)==0x0b) {
            c=0x20;
          }
          else {
            if ((c&0x1f)==0x0d)
              c=0x40;
          }
        }
      }
    }
    DeSelect();
    return c;
  }
  
};

#endif
