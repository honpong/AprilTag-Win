#!/usr/bin/env python
from packet import Packet, PacketType
import sys

if len(sys.argv) != 3:
    print "Usage:", sys.argv[0], "<capture file> <output json file>"
    sys.exit(1)

filename = sys.argv[1]
output_filename = sys.argv[2]

f = open(filename, "rb")

p = Packet.from_file(f)
while p is not None:
    if p.header.type == PacketType.calibration_json:
        with open(output_filename, "w") as fout:
            fout.write(p.data)
        f.close()
        sys.exit(0)

    p = Packet.from_file(f)

f.close()

print "Error: No packet_calibration_json found!"
sys.exit(1)
