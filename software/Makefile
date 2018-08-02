# The MIT License (MIT)
# 
# Copyright (c) 2018 Madis Kaal <mast@nomad.ee>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# project name, resulting binaries will get that name
PROJECT=z-one

# mcu options, clock speed and device
F_CPU=18432000UL
GCCDEVICE=atmega1284p

# object files going into project
OBJECTS=z-one.o images.o softwareuart.o console.o partitioner.o
IMAGES=bootstrap.ccc monitor.ccc cpm.ccc bootstrap.bin monitor.bin cpm.bin

#avrdude options
FUSES=-U lfuse:w:0xE7:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m -U lock:w:0x3F:m
DEVICE=m1284

# additional include directories
INCLUDEDIRS=-I..
# additional libraries to link in
LIBRARIES=

vpath %.cpp ..

#--------------------------------------------------------------
CC=avr-gcc
CXX=avr-g++
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
SIZE=avr-size
AVRDUDE=avrdude
REMOVE=rm -f
LD=avr-g++

#generic compiler options
CFLAGS=-I. $(INCLUDEDIRS) -g -mmcu=$(GCCDEVICE) -Os \
	-fpack-struct -fshort-enums             \
	-funsigned-bitfields -funsigned-char -Wall \

CXXFLAGS=$(CFLAGS) -fno-exceptions -DF_CPU=$(F_CPU)

LDFLAGS=-Wl,-Map,$(PROJECT).map -mmcu=$(GCCDEVICE) $(LIBRARIES)

.PHONY: erase clean

#------------------------------------------------------------

all: clean $(IMAGES) hex tools ymodem.com
# test
	egrep "MONITORTOP|MONITORSTART|CPMTOP|CCPSTART|BDOSSTART|BIOSSTART" *.asm.map
		
hex: $(PROJECT).hex $(PROJECT).eep

#test:
#	srec_cat -Output test.hex  -Intel -address-length=2  avr.inc -Binary -Offset=256 

tools: ymodem.hex
	
$(PROJECT).elf: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $?
	@avr-size $(PROJECT).elf
	@avr-objdump -S $@ > $(PROJECT).lst
		
$(PROJECT).hex: $(PROJECT).elf
	@$(OBJCOPY) -j .text -j .data -O ihex $< $@

$(PROJECT).eep: $(PROJECT).elf
	@-$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

flash: all $(PROJECT).hex $(PROJECT).eep
	$(AVRDUDE) -F -P usb -B 5 -c usbtiny -p $(DEVICE) $(FUSES) -U flash:w:$(PROJECT).hex -U eeprom:w:$(PROJECT).eep

erase:
	$(AVRDUDE) -P usb -c usbtiny -p $(DEVICE) -e

clean:
	@rm -f $(PROJECT).hex $(PROJECT).eep $(PROJECT).elf *.o *~ *.lst *.map *.bin *.ccc *.HEX ymodem.COM *.pyc ymodem.HEX

%.hex : %.com
	srec_cat -Output $@  -Intel -address-length=2 $< -Binary -Offset=256 						 	 		

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDEDIRS) -c $< -o $@

%.bin : %.asm
	pyz80 --lstfile $<.lst --mapfile $<.map --obj $@ $<

%.com : %.asm
	pyz80 --lstfile $<.lst --mapfile $<.map --obj $@ $<

%.ccc : %.bin
	ls -l $<
	python bin2inc.py $< >$@
	
%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $< -o $@
