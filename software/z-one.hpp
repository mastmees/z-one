#ifndef __z80_tiny_hpp__
#define __z80_tiny_hpp__

#include <avr/pgmspace.h>

extern const char copyright[] PROGMEM;

extern const uint8_t monitor_bin[8192] PROGMEM;
extern const uint8_t bootstrap_bin[128] PROGMEM;
extern const uint8_t cpm_bin[8192] PROGMEM;

#endif
