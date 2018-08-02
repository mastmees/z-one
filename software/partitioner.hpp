#ifndef __partitioner_hpp__
#define __partitioner_hpp__

#include "sdcard.hpp"

extern SDCard sdcard;

typedef struct {
  uint8_t status;
  uint8_t firstchs[3];
  uint8_t type;
  uint8_t lastchs[3];
  uint8_t firstlba[4];
  uint8_t lbacount[4];
} PARTITION;

typedef struct {
  uint8_t status; // 0-16 user number, 0xe5 unused
  uint8_t filename[8];
  uint8_t extension[8];
  uint8_t xl; // extent number low bits
  uint8_t bc;
  uint8_t xh; // extent number high bits
  uint8_t rc; // records used in last used extent
  uint8_t allocations; // block pointers
} DIRENTRY;

void createpartitionentry(PARTITION* p,uint8_t type,uint32_t firstlba,uint32_t count);
void initialize_directory_sectors(uint32_t firstsector,uint16_t count);
void checkdisk(void);

#endif
