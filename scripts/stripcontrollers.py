#!/usr/bin/env python
from packet import Packet, PacketType
import sys

if len(sys.argv) < 3:
  print "Usage:", sys.argv[0], "<capture file> <output file>"
  sys.exit(1)

filename = sys.argv[1]
outfilename = sys.argv[2]

f = open(filename, "rb")
fout = open(outfilename, "wb")


p = Packet.from_file(f)
while p is not None:
    skip = (p.header.type == PacketType.image_raw and p.header.sensor_id > 1) or \
           (p.header.type == PacketType.exposure_info and p.header.sensor_id > 1) or \
           (p.header.type == PacketType.thermometer and p.header.sensor_id > 0) or \
           (p.header.type == PacketType.accelerometer and p.header.sensor_id > 0) or \
           (p.header.type == PacketType.gyroscope and p.header.sensor_id > 0)
    if not skip:
        p.to_file(fout)
    p = Packet.from_file(f)

f.close()
fout.close()
