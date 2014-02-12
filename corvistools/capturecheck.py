#!/opt/local/bin/python2.7
from struct import *
from collections import defaultdict

packets = defaultdict(list)

import sys

if len(sys.argv) < 2:
  print "Usage:", sys.argv[0], "<capture file>"
  sys.exit(1)

filename = sys.argv[1]

f = open(filename)
header_size = 16
header_str = f.read(header_size)
while header_str != "":
  (pbytes, ptype, user, ptime) = unpack('IHHQ', header_str)
  if ptype == 1:
    ptime += 16667
  print pbytes, ptype, user, float(ptime)/1e6
  packets[ptype].append(ptime)
  junk = f.read(pbytes-header_size)
  header_str = f.read(header_size)

f.close()

for packet_type in packets:
  print "type:", packet_type, "number:", len(packets[packet_type])
