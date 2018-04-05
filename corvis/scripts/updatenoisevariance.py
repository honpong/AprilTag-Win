#!/usr/bin/env python
from packet import Packet, PacketType
import sys
import json

if len(sys.argv) != 2:
  print "Usage:", sys.argv[0], "<capture file>"
  sys.exit(1)

f = open(sys.argv[1], "r+b")

p = Packet.from_file(f)
while p is not None:
    if p.header.type == PacketType.calibration_json:
        json_cal = json.loads(p.data)
        print "Accelerometer noise variance was", json_cal["imus"][0]["accelerometer"]["noise_variance"]
        print "Gyroscope noise variance was", json_cal["imus"][0]["gyroscope"]["noise_variance"]

        json_cal["imus"][0]["accelerometer"]["noise_variance"] = 6.695245e-05
        json_cal["imus"][0]["gyroscope"]["noise_variance"] = 5.14803e-6
        calibration = json.dumps(json_cal, indent=2)

        if len(calibration) > len(p.data):
            print "Error: Calibration is", len(calibration), "but there is only", len(p.data), "space"
            sys.exit(1)

        padding = len(p.data) - len(calibration)
        p.data = calibration + " "*padding
        p.header.bytes = Packet.header_size + len(p.data)
        f.seek(p.file_offset, 0)
        p.to_file(f)
        f.close()
        sys.exit(0)
    p = Packet.from_file(f)

f.close()
print "Error: calibration packet not found"
sys.exit(1)
