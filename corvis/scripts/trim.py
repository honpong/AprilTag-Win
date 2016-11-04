#!/usr/bin/env python
from struct import *
from collections import defaultdict

packets = defaultdict(list)

import sys

if len(sys.argv) < 4:
  print "Usage:", sys.argv[0], "<capture file> <starttime> <endtime> <output file>"
  sys.exit(1)

filename = sys.argv[1]
starttime = sys.argv[2]
endtime = sys.argv[3]
outfilename = sys.argv[4]

f = open(filename, "rb")
fout = open(outfilename, "wb")

header_size = 16
header_str = f.read(header_size)
while header_str != "":
  (pbytes, ptype, user, ptime) = unpack('IHHQ', header_str)
  if ptype == 1:
    ptime += 16667
  print pbytes, ptype, user, float(ptime)/1e6
  packets[ptype].append(ptime)
  data = f.read(pbytes-header_size)
  if float(ptime) > float(starttime):
      fout.write(header_str)
      fout.write(data)
  if float(ptime) > float(endtime):
      break
  header_str = f.read(header_size)

f.close()
fout.close()

for packet_type in packets:
  print "type:", packet_type, "number:", len(packets[packet_type])
