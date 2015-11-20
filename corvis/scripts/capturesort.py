#!/usr/bin/env python
from __future__ import print_function
from struct import *
import sys

if not len(sys.argv) in (2,3):
  print("Usage:", sys.argv[0], "<source capture file> [<destination sorted capture file>]")
  sys.exit(1)

src = sys.argv[1]
dst = sys.argv[2] if len(sys.argv) > 2 else None

def capture_packets(filename):
    with open(filename) as f:
        while True:
          header = f.read(16)
          if header == "": break
          (pbytes, ptype, user, ptime) = unpack('IHHQ', header)
          yield { 'type': ptype, 'user': user, 'time': ptime, 'header': header, 'data': f.read(pbytes - 16) }

packets = list(capture_packets(src))

if dst is not None:
  with open(dst, "w") as d:
    for p in sorted(packets, None, lambda p: p['time']):
        d.write(p['header'])
        d.write(p['data'])
else:
  for p in packets:
    print((p['time'], p['type']))
