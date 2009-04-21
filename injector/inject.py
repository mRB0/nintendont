#!/usr/bin/env python

SERDEV = "COM13"
BAUDRATE = 115200

BOTRDY = '\x51'
INJRDY = '\x52'
INJABT = '\x53'
BOTRDY_I = '\x61'
INJRDY_I = '\x62'
INJABT_I = '\x63'
ACK = '\x53'
BLKSIZE = 64

import serial
import os
import struct
import time
import sys

def dump_serial(ser):
  while 1:
    sys.stdout.write(ser.read(1).decode('ascii', 'replace').encode('ascii', 'replace'))
    sys.stdout.flush()
  
  
def go(ser, is_internal, filespec, skip=False):
  
  if is_internal:
    injrdy = INJRDY_I
    botrdy = BOTRDY_I
    injabt = INJABT_I
  else:
    injrdy = INJRDY
    botrdy = BOTRDY
    injabt = INJABT
  
  print "waiting for bot"
  
  ser.flushInput()
  ser.flushOutput()
    
  # wait for ready
  indata = 0x0
  while(indata != botrdy):
    indata = ser.read(1)
    #print "%02x" % (struct.unpack("0xB", indata[0])[0],)
  
  print "bot rdy"
  
  if (skip):
    print "skipping"
    ser.write(injabt)
    return
  
  # send ready
  ser.write(injrdy)
  
  # wait for ready ack
  
  indata = ser.read(1)
  
  while (indata == botrdy):
    indata = ser.read(1)
   
  if indata != ACK:
    raise ValueError('Received %c instead of %c' % (indata,ACK))
  
  print "acked"
  
  flen = os.path.getsize(filespec)
  packedlen = struct.pack(">L", flen)
  
  #print "sending length"
  print "sending length (%d = %s)\n" % (flen, repr(packedlen))
  
  for i in xrange(3):
    ser.write(packedlen[i+1]) # 24-bit
  
  if ser.read(1) != ACK:
    raise ValueError('Received something else instead of %02x' % (ACK,))
  
  # sending some data
  
  inf = file(filespec, "rb")
  
  fmt = "%-" + str(BLKSIZE) + "s"
  
  somedata = inf.read(BLKSIZE)
  while len(somedata) > 0:
    if inf.tell() % 0x1000 == 0:
      print "sending: at offs %d" % (inf.tell(),)
    
    
    ser.write(fmt % (somedata,))
    if ser.read(1) != ACK:
      raise ValueError('Received something else instead of %02x at offs=%d' % (ACK, inf.tell()))
    #print "(acked)"
    
    somedata = inf.read(BLKSIZE)
    #time.sleep(0.1)
  
  return ser
  

if __name__ == '__main__':
  #dump_serial(serial.Serial(SERDEV, BAUDRATE))
  
  assert(len(sys.argv) > 2)
  
  ser = serial.Serial(SERDEV, BAUDRATE)
  print "=> sending data for %s" % (sys.argv[1],)
  go(ser, False, sys.argv[1], False)
  print "=> sending data for %s" % (sys.argv[2],)
  go(ser, True, sys.argv[2], False)
  print "=> starting serial dump!"
  dump_serial(ser)
