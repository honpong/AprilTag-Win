#!/usr/bin/env python
from packet import Packet, PacketType
import sys

if len(sys.argv) < 3:
  print "Usage:", sys.argv[0], "<capture file> <new_calibration_file> <output file>"
  sys.exit(1)

filename = sys.argv[1]
calibration_filename = sys.argv[2]
outfilename = sys.argv[3]

f = open(filename, "rb")
fout = open(outfilename, "wb")

calibration = open(calibration_filename).read()

p = Packet.from_file(f)
while p is not None:
    if p.header.type == PacketType.calibration_json:
        p.data = calibration
        p.header.bytes = Packet.header_size + len(calibration)
    p.to_file(fout)
    p = Packet.from_file(f)

f.close()
fout.close()
