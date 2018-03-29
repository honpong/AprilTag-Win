#!/usr/bin/env python
from collections import defaultdict
from packet import Packet, PacketType

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

p = Packet.from_file(f)
while p is not None:
  packets[p.header.type].append(p.header.time)
  if p.timestamp() > int(starttime) or p.header.type == PacketType.calibration_json:
      p.to_file(fout)
  if p.timestamp() > int(endtime):
      break
  p = Packet.from_file(f)

f.close()
fout.close()

for packet_type in packets:
  print "type:", packet_type, "number:", len(packets[packet_type])
