#!/usr/bin/env python
from struct import *

import sys

if len(sys.argv) < 2:
  print "Usage:", sys.argv[0], "<capture file>"
  sys.exit(1)

filename = sys.argv[1]

f = open(filename, "rb")

header_size = 16
header_str = f.read(header_size)
while header_str != "":
  (pbytes, ptype, user, ptime) = unpack('IHHQ', header_str)
  data = f.read(pbytes-header_size)
  if ptype == 43: # packet_calibration_json
     print data 
     sys.exit(0)
  header_str = f.read(header_size)

f.close()

print "Error: No packet_calibration_json found!"
sys.exit(1)
