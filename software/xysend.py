""" The MIT License (MIT)
 
  Copyright (c) 2017 Madis Kaal <mast@nomad.ee>
 
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
"""

# a copy of xmodem source is included to ensure that the ugly ymodem name packet
# hack works down the road (see https://github.com/tehmaze/xmodem)

import serial
import sys
import time
import tty
import termios
import xmodem
import logging
import time
import os
from os import path

import serial
from serial.serialutil import SerialBase, SerialException, to_bytes, portNotOpenError, writeTimeoutError

# adjust the names as needed. The FDTI device naming below is from OSX built-in driver
ser=serial.Serial("/dev/tty.usbserial-00000000",19200,timeout=5)


def getc(size, timeout=1):
  data=to_bytes(ser.read(size))
  return data
  
def putc(data, timeout=1):
  data=to_bytes(data)
  return ser.write(data)

while ser.in_waiting:
  ser.read()

modem = xmodem.XMODEM(getc, putc)
modem.log.setLevel("INFO")
modem.log.addHandler(logging.StreamHandler(sys.stdout))

def sendname(filename):
  global modem
  crc_mode = 0
  cancel = 0
  error_count = 0
  retry = 10
  while True:
    char = getc(1)
    if char:
      if char == xmodem.NAK:
        modem.log.debug('standard checksum requested (NAK). Ignoring.')
        error_count=0
      if char == xmodem.CRC:
        modem.log.debug('16-bit CRC requested (CRC).')
        crc_mode = 1
        break
      elif char == xmodem.CAN:
        if cancel:
          modem.log.info('Transmission canceled: received 2xCAN '
                                      'at start-sequence')
          return False
        else:
          modem.log.debug('cancellation at start sequence.')
          cancel = 1
      else:
        modem.log.error('send error: expected NAK, CRC, or CAN; '
                                   'got %r', char)
        error_count += 1
        if error_count > retry:
          modem.log.info('send error: error_count reached %d, '
                              'aborting.', retry)
          modem.abort(timeout=3)
          return False

  # send data
  error_count = 0
  cancel = 0
  data = filename
  header = modem._make_send_header(128, 0)
  data = data.ljust(128, "\0")
  checksum = modem._make_send_checksum(crc_mode, data)

  # emit packet
  while error_count<3:
    modem.log.debug('send: block %d', 0)
    putc(header + data + checksum)
    char=getc(1)
    if char:
      if char == xmodem.NAK:
        error_count+=1
      elif char == xmodem.ACK:
        modem.log.info('Name accepted')
        return True
      elif char == xmodem.CAN:
         if cancel:
           modem.log.info('Transmission canceled: received 2xCAN '
                                    'at start-sequence')
           return False
         else:
           modem.log.debug('cancellation at start sequence.')
           cancel = 1
      else:
        modem.log.error('send error: expected NAK, CRC, or CAN; '
                                 'got %r', char)
        error_count += 1
  modem.abort(timeout=3)
  return False
  

for filename in sys.argv[1:]:
  f=open(filename,"r")
  print "Sending %s"%filename
  if sendname(path.basename(filename)):
    modem.send(f)
  f.close()

ser.close()
