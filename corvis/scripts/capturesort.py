#!/usr/bin/env python
from __future__ import print_function
from struct import pack, unpack, unpack_from
import argparse

parser = argparse.ArgumentParser(description='sort a capture')
parser.add_argument('source',                     type=str,                          help='source capture')
parser.add_argument('destination',                type=str, nargs='?', default=None, help='destination capture')
args = parser.parse_args()

def capture_packets(filename):
    with open(filename, "rb") as f:
        while True:
          header = f.read(16)
          if header == "": break
          (pbytes, ptype, user, ptime) = unpack('IHHQ', header)
          data = f.read(pbytes - 16)
          yield { 'type': ptype, 'user': user, 'time': ptime, 'header': header, 'data': data, 'exposure': unpack_from('Q', data)[0] if ptype in (28,29) else 0 }

packets = list(capture_packets(args.source))

if args.destination is not None:
  with open(args.destination, "wb") as d:
    for p in sorted(packets, None, lambda p: (p['time'] + p['exposure']/2, p['type'])):
        d.write(p['header'])
        d.write(p['data'])
else:
  for p in packets:
    print((p['time'], p['type'], p['exposure']))
