#!/usr/bin/env python
from packet import Packet, PacketType
import sys
import json

if len(sys.argv) != 3:
  print "Usage:", sys.argv[0], "<capture file> <new_calibration_file>"
  sys.exit(1)

filename = sys.argv[1]
calibration_filename = sys.argv[2]
try:
    calibration = open(calibration_filename).read()
    calibration_no_whitespace = json.dumps(json.loads(calibration), separators=(',', ':'))
except ValueError:
    print calibration_filename, "was not valid json"
    sys.exit(1)

f = open(filename, "r+b")

p = Packet.from_file(f)
while p is not None:
    if p.header.type == PacketType.calibration_json:
        if len(calibration) > len(p.data):
            print "Warning: new calibration is", len(calibration), "bytes, but existing calibration is", len(p.data), "bytes. Trying to compress..."
            if len(calibration_no_whitespace) <= len(p.data):
                calibration = calibration_no_whitespace
            else:
                print "Error: Compressed calibration is still", len(calibration), "bytes"
                sys.exit(1)
        padding = len(p.data) - len(calibration)
        print "Found calibration packet, adding", padding, "characters padding"
        p.data = calibration + " "*padding
        p.header.bytes = Packet.header_size + len(p.data)
        f.seek(p.file_offset, 0)
        p.to_file(f)
        f.close()
        sys.exit(0)
    p = Packet.from_file(f)

f.close()
