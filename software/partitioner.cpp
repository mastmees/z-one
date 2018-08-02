#include <string.h>
#include "partitioner.hpp"


void createpartitionentry(PARTITION* p,uint8_t type,uint32_t firstlba,uint32_t count)
{
  p->status=0;
  p->firstchs[0]=0xfe;
  p->firstchs[1]=0xff;
  p->firstchs[2]=0xff;
  p->type=type;
  p->lastchs[0]=0xfe;
  p->lastchs[1]=0xff;
  p->lastchs[2]=0xff;
  p->firstlba[0]=firstlba&255;
  p->firstlba[1]=firstlba>>8;
  p->firstlba[2]=firstlba>>16;
  p->firstlba[3]=firstlba>>24;
  p->lbacount[0]=count&255;
  p->lbacount[1]=count>>8;
  p->lbacount[2]=count>>16;
  p->lbacount[3]=count>>24;
}

void initialize_directory_sectors(uint32_t firstsector,uint16_t count)
{
  memset(sdcard.GetBuf(),0xe5,512);
  while (count--) {
    if (sdcard.WriteSector(firstsector,sdcard.GetBuf())) {
      console.print("Write error\r\n");
      return;
    }
    firstsector++;
  }
}

/*
see if the card is correctly partitioned for the machine, and if not then
offer to partition it. If partitioned here, then the result is disk with two
partitions. First has space for up to 16 max size cp/m drives, with 64KB extra
for reserved boot track (65,536*128=8,388,608 bytes or 16384 usable LBA sectors
per drive, plus 128 LBA sectors for boot track) totalling 264192 sectors
(135,266,304 bytes). Second partition takes up the rest of the card and can be used as
unstructured block storage (think of it as tape drive with 512 byte blocks).

As even the drives are probably going to by a little bit more than a
128M card has usable room for, 256MB SD card is the minimum requirement.
*/
void checkdisk(void)
{
PARTITION *p=(PARTITION*)(&sdcard.GetBuf()[446]);
uint8_t i;
uint32_t sector;
uint32_t dsk_offset;
uint32_t dsk_sectors;
uint32_t tape_offset;
uint32_t tape_sectors;

  dsk_offset=dsk_sectors=tape_offset=tape_sectors=0;
  if (sdcard.GetType()==SDCard::NONE || sdcard.ReadSector(0,sdcard.GetBuf())) {
    console.print("Unreadable card\r\n");
    sdcard.Invalidate();
    return;
  }
  if (sdcard.GetTotalSectors()<265000U) {
    console.print("Card too small to be used (at least 256MB card required)\r\n");
    sdcard.Invalidate();
    return;
  }
  if (sdcard.GetBuf()[510]!=0x55 || sdcard.GetBuf()[511]!=0xaa) {
    console.print("Card has no partitions,");
  }
  else {
    for (i=0;i<4;i++) {
      switch (p->type) {
        case 0x58: // cp/m disks
          dsk_offset=(uint32_t)p->firstlba[0]|(uint32_t)p->firstlba[1]<<8|(uint32_t)p->firstlba[2]<<16|(uint32_t)p->firstlba[3]<<24;
          dsk_sectors=(uint32_t)p->lbacount[0]|(uint32_t)p->lbacount[1]<<8|(uint32_t)p->lbacount[2]<<16|(uint32_t)p->lbacount[3]<<24;
          break;
        case 0x59: // cp/m tape
          tape_offset=(uint32_t)p->firstlba[0]|(uint32_t)p->firstlba[1]<<8|(uint32_t)p->firstlba[2]<<16|(uint32_t)p->firstlba[3]<<24;
          tape_sectors=(uint32_t)p->lbacount[0]|(uint32_t)p->lbacount[1]<<8|(uint32_t)p->lbacount[2]<<16|(uint32_t)p->lbacount[3]<<24;
          break;
      }
      i++;
      p++;
    }
    // as sector 0 is MBR, the partitions cannot start from sector 0
    if (dsk_offset && tape_offset)
      return;
    if (dsk_offset) {
      console.print("Card has drive space but no tape space,");
    }
    else {
      console.print("Card has no CP/M partitions,");
    }
  }
  console.print(" create new partition table?");
  while (!console.rxready()) {
    wdt_reset();
    WDTCSR|=0x40;
  }
  uint8_t c=console.receive();
  console.send(c);
  console.print("\r\n");
  if (c=='y' || c=='Y') {
    memset(sdcard.GetBuf(),0,512);
    p=(PARTITION*)(&sdcard.GetBuf()[446]);
    createpartitionentry(p++,0x58,1,264192U);
    createpartitionentry(p,0x59,264193U,sdcard.GetTotalSectors()-264193U);
    sdcard.GetBuf()[510]=0x55;
    sdcard.GetBuf()[511]=0xaa;
    if (sdcard.WriteSector(0,sdcard.GetBuf())) {
      console.print("Failed to write partition table\r\n");
      sdcard.Invalidate();
    }
    wdt_reset();
    WDTCSR|=0x40;
    // now create file systems on first partition by initializing directory blocks
    // for all 16 drives
    for (i=0;i<16;i++) {
      sector=1+128+(uint32_t)i*16512U; // starting from sector 1, 128 sectors for boot track
                                       // 16384+128 total sectors per drive
      initialize_directory_sectors(sector,16384/512); // one 16K blocks for directory
      wdt_reset();
      WDTCSR|=0x40;
    }
  }
  else {
    sdcard.Invalidate();
  }
}
