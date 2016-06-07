#!/usr/bin/env python
from struct import *
from collections import defaultdict
import numpy
import argparse

packet_types = defaultdict(str, {1:"camera", 20:"accelerometer", 21:"gyro", 29:"image_raw"})
format_types = defaultdict(str, {0:"Y8", 1:"Z16_mm"})

parser = argparse.ArgumentParser(description='Check a capture file.')
parser.add_argument("-v", "--verbose", action='store_true',
        help="Print more information about every packet")
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
  if args.verbose:
      print pbytes, ptype, user, float(ptime)/1e6,
  packets[ptype].append(int(ptime))
  data = f.read(pbytes-header_size)
  if ptype == 20 or ptype == 21:
      # packets are padded to 8 byte boundary
      (x, y, z) = unpack('fff', data[:12])
      if args.verbose:
          print "\t", x, y, z
  if ptype == 29:
      (exposure, width, height, stride, camera_format) = unpack('QHHHH', data[:16])
      type_str = format_types[camera_format]
      if args.verbose:
          camera_str = "%s (%d) %dx%d, %d stride, %d exposure" % (type_str, camera_format, width, height, stride, exposure)
          print "\t", camera_str
  header_str = f.read(header_size)

f.close()

for packet_type in packets:
  print "type:", packet_types[packet_type], "number:", len(packets[packet_type])
  deltas = numpy.array(packets[packet_type][1:]) - numpy.array(packets[packet_type][:-1])
  mean_delta = numpy.mean(deltas)
  print "rate (hz):", 1/(mean_delta/1e6), "mean dt (us):", mean_delta, "std dt (us):", numpy.std(deltas)
  exceptions = numpy.flatnonzero(numpy.logical_or(deltas > mean_delta*1.05, deltas < mean_delta*0.95))
  print len(exceptions), "samples are more than 5% from mean"
  if args.exceptions:
      for e in exceptions:
          print "Exception: t t+1 delta", packets[packet_type][e], packets[packet_type][e+1], deltas[e]
  print ""
