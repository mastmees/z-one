""" The MIT License (MIT)
 
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
"""

import serial # you need py-serial installed from ports on OSX
import sys
import time
import tty

# adjust the names as needed. The FDTI device naming below is from OSX built-in driver
ser=serial.Serial("/dev/tty.usbserial-00000000",9600)

f=open(sys.argv[1],"r")
for c in f.read():
  if c=="\n":
    while ser.out_waiting:
      time.sleep(0.1)
  ser.write(c)

f.close()
while ser.out_waiting:
  time.sleep(0.1)
ser.flush()
time.sleep(1)
ser.close()
