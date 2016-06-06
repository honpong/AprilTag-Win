#!/usr/bin/env python
from struct import *
from collections import defaultdict
import numpy

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
  #for e in exceptions:
  #    print "Exception: t t+1 delta", packets[packet_type][e], packets[packet_type][e+1], deltas[e]
  print ""
