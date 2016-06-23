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
parser.add_argument("-w", "--warnings", action='store_true',
        help="Print details when multiple image frames arrive without an imu frame in between")
parser.add_argument("capture_filename")

args = parser.parse_args()

accel_type = 20
gyro_type = 21
image_raw_type = 29

packets = defaultdict(list)
f = open(args.capture_filename)
header_size = 16
header_str = f.read(header_size)
prev_packet_str = ""
warnings = defaultdict(list)
while header_str != "":
  (pbytes, ptype, user, ptime) = unpack('IHHQ', header_str)
  packet_str = packet_types[ptype]
  if ptype == 1:
    ptime += 16667
  if args.verbose:
      print packet_str, pbytes, ptype, user, float(ptime)/1e6,
  data = f.read(pbytes-header_size)
  if ptype == accel_type or ptype == gyro_type:
      # packets are padded to 8 byte boundary
      (x, y, z) = unpack('fff', data[:12])
      if args.verbose:
          print "\t", x, y, z
  if ptype == image_raw_type:
      (exposure, width, height, stride, camera_format) = unpack('QHHHH', data[:16])
      type_str = format_types[camera_format]
      packet_str += "_" + type_str
      if args.verbose:
          camera_str = "%s (%d) %dx%d, %d stride, %d exposure" % (type_str, camera_format, width, height, stride, exposure)
          print "\t", camera_str
  if packet_str == "":
      packet_str = str(ptype)
  packets[packet_str].append(int(ptime))
  if not (ptype == accel_type or ptype == gyro_type) and prev_packet_str == packet_str:
      warnings[packet_str].append(int(ptime))
  prev_packet_str = packet_str
  header_str = f.read(header_size)

f.close()

for packet_type in packets:
  timestamps = numpy.array(packets[packet_type])
  deltas = timestamps[1:] - timestamps[:-1]
  mean_delta = numpy.mean(deltas)
  print packet_type, len(packets[packet_type]), "packets"
  print "\tRate:", 1/(mean_delta/1e6), "hz"
  print "\tmean dt (us):", mean_delta
  print "\tstd dt (us):", numpy.std(deltas)
  print "\tstart (s) finish (s):", numpy.min(timestamps)/1e6, numpy.max(timestamps)/1e6
  print "\tlength (s):", (numpy.max(timestamps) - numpy.min(timestamps))/1e6
  exceptions = numpy.flatnonzero(numpy.logical_or(deltas > mean_delta*1.05, deltas < mean_delta*0.95))
  print len(exceptions), "samples are more than 5% from mean"
  if len(warnings[packet_type]):
      print len(warnings[packet_type]), "latency warnings"
  if args.exceptions:
      for e in exceptions:
          print "Exception: t t+1 delta", timestamps[e], timestamps[e+1], deltas[e]
  if args.warnings:
      for w in warnings[packet_type]:
          print "Warning: t", w
  print ""
