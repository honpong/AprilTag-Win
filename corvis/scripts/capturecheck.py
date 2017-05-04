#!/usr/bin/env python
from struct import *
from collections import defaultdict
import numpy
import argparse
from math import sqrt

packet_types = defaultdict(str, {1:"camera", 20:"accelerometer", 21:"gyro", 28:"image_with_depth", 29:"image_raw"})
format_types = defaultdict(str, {0:"Y8", 1:"Z16_mm"})

parser = argparse.ArgumentParser(description='Check a capture file.')
parser.add_argument("-v", "--verbose", action='store_true',
        help="Print more information about every packet")
parser.add_argument("-e", "--exceptions", action='store_true',
        help="Print details when sample dt is more than 5%% away from the median")
parser.add_argument("-w", "--warnings", action='store_true',
        help="Print details when multiple image frames arrive without an imu frame in between")
parser.add_argument("-x", "--exposure_warnings", action='store_true',
        help="Print details when exposure times are less than 1ms or greater than 50ms")
parser.add_argument("-i", "--imu_warnings", action='store_true',
        help="Print details when accelerometer deltas are > 9m/s^2 or gyro delta is > 2 radians/sec")
parser.add_argument("capture_filename")

args = parser.parse_args()

accel_type = 20
gyro_type = 21
image_with_depth = 28
image_raw_type = 29
got_types = defaultdict(int)

packets = defaultdict(list)
latencies = defaultdict(list)
latest_received = None
f = open(args.capture_filename,'rb')
header_size = 16
header_str = f.read(header_size)
prev_packet_str = ""
warnings = defaultdict(list)
exposure_warnings = defaultdict(list)
imu_warnings = defaultdict(list)
last_data = {}
while header_str != "":
  (pbytes, ptype, sensor_id, ptime) = unpack('IHHQ', header_str)
  got_types[ptype] += 1
  packet_str = packet_types[ptype] + "_" + str(sensor_id)
  if ptype == 1:
    ptime += 16667
  if args.verbose:
      print packet_str, pbytes, ptype, sensor_id, ptime,
  data = f.read(pbytes-header_size)
  if ptype == accel_type or ptype == gyro_type:
      # packets are padded to 8 byte boundary
      (x, y, z) = unpack('fff', data[:12])
      current_data = numpy.array([x, y, z])
      if args.verbose:
          print "\t", sensor_id, x, y, z, sqrt(x*x + y*y + z*z)
      if packet_str in last_data:
          norm = numpy.linalg.norm(last_data[packet_str] - current_data)
          if ptype == gyro_type and norm > 2:
              imu_warnings[packet_str].append((ptime, norm, current_data, last_data[packet_str]))
          if ptype == accel_type and norm > 9:
              imu_warnings[packet_str].append((ptime, norm, current_data, last_data[packet_str]))
      last_data[packet_str] = current_data
  elif ptype == image_with_depth:
      (exposure, width, height, depth_w, depth_h) = unpack('QHHHH', data[:16])
      if exposure < 1000 or exposure > 50000:
          exposure_warnings[packet_str].append((ptime, exposure))
      ptime += exposure/2
      if args.verbose:
          camera_str = "%d %dx%d grey, %dx%d depth, %d exposure, %d adjusted time" % (sensor_id, width, height, depth_w, depth_h, exposure, ptime)
          print "\t", camera_str
  elif ptype == image_raw_type:
      (exposure, width, height, stride, camera_format) = unpack('QHHHH', data[:16])
      type_str = format_types[camera_format]
      packet_str += "_" + type_str
      if exposure < 1000 or exposure > 50000:
          exposure_warnings[packet_str].append((ptime, exposure))
      ptime += exposure/2
      if args.verbose:
          camera_str = "%s %d (%d) %dx%d, %d stride, %d exposure, %d adjusted time" % (type_str, sensor_id, camera_format, width, height, stride, exposure, ptime)
          print "\t", camera_str
  else:
      if args.verbose:
          print ""
  if packet_str == "":
      packet_str = str(ptype)
  if not latest_received or latest_received < ptime:
      latest_received = ptime
  latencies[packet_str].append(latest_received - ptime)
  packets[packet_str].append(ptime)
  if not (ptype == accel_type or ptype == gyro_type) and prev_packet_str == packet_str:
      warnings[packet_str].append((last_time, ptime))
  last_time = ptime
  prev_packet_str = packet_str
  header_str = f.read(header_size)

f.close()

def compress_warnings(warning_list):
    output_list = []
    i = 0
    current_list = []
    while i < len(warning_list)-1:
        print warning_list[i], i
        adjacent = warning_list[i][1] == warning_list[i+1][0]
        current_list.append(warning_list[i][0])
        if not adjacent:
            current_list.append(warning_list[i][1])
            output_list.append(current_list)
            current_list = []
        i += 1
    if len(current_list):
        current_list.append(warning_list[i][0])
        current_list.append(warning_list[i][1])
        output_list.append(current_list)
    return output_list

for packet_type in sorted(packets.keys()):
  timestamps = numpy.array(packets[packet_type])
  platencies = numpy.array(latencies[packet_type])
  deltas = timestamps[1:] - timestamps[:-1]
  median_delta = numpy.median(deltas)
  print packet_type, len(packets[packet_type]), "packets"
  print "\tRate:", 1/(median_delta/1e6), "hz"
  print "\tmedian dt (us):", median_delta
  print "\tstd dt (us):", numpy.std(deltas)
  print "\trelative latency (us): %.3f min, %.3f median, %.3f max, %.3f std" % (numpy.min(platencies), numpy.median(platencies), numpy.max(platencies), numpy.std(platencies))
  print "\tstart (s) finish (s):", numpy.min(timestamps)/1e6, numpy.max(timestamps)/1e6
  print "\tlength (s):", (numpy.max(timestamps) - numpy.min(timestamps))/1e6
  exceptions = numpy.flatnonzero(numpy.logical_or(deltas > median_delta*1.05, deltas < median_delta*0.95))
  print len(exceptions), "samples are more than 5% from median"
  if len(warnings[packet_type]):
      print len(warnings[packet_type]), "latency warnings"
  if len(exposure_warnings[packet_type]):
      print len(exposure_warnings[packet_type]), "exposure warnings"
  if len(imu_warnings[packet_type]):
      print len(imu_warnings[packet_type]), "IMU warnings"
  if args.exceptions:
      for e in exceptions:
          print "Exception: t t+1 delta", timestamps[e], timestamps[e+1], deltas[e]
  if args.warnings:
      compressed_warnings = compress_warnings(warnings[packet_type])
      for w in compressed_warnings:
          print "Warning: ", len(w), "images at timestamps:", w
  if args.exposure_warnings:
      for w in exposure_warnings[packet_type]:
          print "Warning: Image at", w[0], "had exposure of", w[1], "microseconds"
  if args.imu_warnings:
      numpy.set_printoptions(precision=2)
      for w in imu_warnings[packet_type]:
          print "Warning:", packet_type, "at", w[0], "changed by ", w[1], "current: ", w[2], "last:", w[3]
  print ""

if got_types[accel_type] == 0:
    print "Error: Never received any accelerometer data"
if got_types[gyro_type] == 0:
    print "Error: Never received any gyro data"
if got_types[image_raw_type] == 0 and got_types[image_with_depth] == 0:
    print "Error: Never received any image data"
