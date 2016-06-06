#!/usr/bin/env python
from struct import *
from collections import defaultdict
import numpy
import argparse

parser = argparse.ArgumentParser(description='Check a capture file.')
parser.add_argument("-e", "--exceptions", action='store_true',
        help="Print details when sample dt is more than 5%% away from the mean")
parser.add_argument("capture_filename")

args = parser.parse_args()

packets = defaultdict(list)
f = open(args.capture_filename)
header_size = 16
header_str = f.read(header_size)
while header_str != "":
  (pbytes, ptype, user, ptime) = unpack('IHHQ', header_str)
  if ptype == 1:
    ptime += 16667
  print pbytes, ptype, user, float(ptime)/1e6
  packets[ptype].append(int(ptime))
  junk = f.read(pbytes-header_size)
  header_str = f.read(header_size)

f.close()

for packet_type in packets:
  print "type:", packet_type, "number:", len(packets[packet_type])
  deltas = numpy.array(packets[packet_type][1:]) - numpy.array(packets[packet_type][:-1])
  mean_delta = numpy.mean(deltas)
  print "rate (hz):", 1/(mean_delta/1e6), "mean dt (us):", mean_delta, "std dt (us):", numpy.std(deltas)
  exceptions = numpy.flatnonzero(numpy.logical_or(deltas > mean_delta*1.05, deltas < mean_delta*0.95))
  print len(exceptions), "samples are more than 5% from mean"
  if args.exceptions:
      for e in exceptions:
          print "Exception: t t+1 delta", packets[packet_type][e], packets[packet_type][e+1], deltas[e]
  print ""
