#!/usr/bin/env python
from struct import *
from collections import defaultdict
import numpy
import argparse
from math import sqrt
import sys

packet_types = defaultdict(str, {1:"camera", 20:"accelerometer", 21:"gyro", 28:"image_with_depth", 29:"image_raw", 30:"odometry", 31:"thermometer", 40:"stereo_raw", 42:"pose", 43:"calibration_json", 44:"arrival_time", 45:"velocimeter", 48:"calibration_bin", 49:"exposure", 50:"controller_physical_info"})
format_types = defaultdict(str, {0:"Y8", 1:"Z16_mm"})

parser = argparse.ArgumentParser(description='Check a capture file.')
parser.add_argument("-v", "--verbose", action='store_true',
        help="Print more information about every packet")
parser.add_argument("-p", "--progress", action='store_true',
        help="Report file read progress every 100MB")
parser.add_argument("-e", "--exceptions", action='store_true',
        help="Print details when sample dt is more than 5%% away from the median")
parser.add_argument("-w", "--warnings", action='store_true',
        help="Print details when multiple image frames arrive without an imu frame in between")
parser.add_argument("-l", "--latency_warnings", action='store_true',
        help="Print details when latency exceeds 33ms for any sensor")
parser.add_argument("-x", "--exposure_warnings", action='store_true',
        help="Print details when exposure times are less than 1ms or greater than 50ms")
parser.add_argument("-i", "--imu_warnings", action='store_true',
        help="Print details when accelerometer deltas are > 9m/s^2 or gyro delta is > 2 radians/sec")
parser.add_argument("-n", "--max-packets", type=int, default=-1,
        help="number of packets to process")
parser.add_argument("capture_filename")

args = parser.parse_args()
if args.max_packets > 0:
    print "Inspecting the first ", args.max_packets, " packets"

accel_type = 20
gyro_type = 21
image_with_depth = 28
image_raw_type = 29
odometry_type = 30
thermometer_type = 31
stereo_raw_type = 40
arrival_time_type = 44
calibration_type = 43
calibration_bin_type = 48
velocimeter_type = 45
got_types = defaultdict(int)

packets = defaultdict(list)
latencies = defaultdict(list)
arrivals = defaultdict(list)
latest_received = None
f = open(args.capture_filename,'rb')
header_size = 16
header_str = f.read(header_size)
prev_packet_str = ""
warnings = defaultdict(list)
exposure_warnings = defaultdict(list)
imu_warnings = defaultdict(list)
out_of_order_warnings = []
missing_arrival_time_warnings = []
unconsumed_arrival_time_warnings = []
last_data = {}
last_arrival_time = 0
last_ptype = None
packet_count = 0;
bytes_read = 0
total_bytes_read = 0
while header_str != "":
  (pbytes, ptype, sensor_id, ptime) = unpack('IHHQ', header_str)
  if pbytes < header_size:
    print "Error: packet at", total_bytes_read, "bytes, (time,type,id) (", ptime, ",", ptype, ",", sensor_id,") has size ", pbytes, "bytes but minimum packet size is", header_size
    sys.exit(1)
  got_types[ptype] += 1
  packet_str = packet_types[ptype] + "_" + str(sensor_id)
  if ptype == 1:
    ptime += 16667
  if args.verbose:
      print packet_str, pbytes, ptype, sensor_id, ptime,
  data = f.read(pbytes-header_size)
  packet_count += 1
  bytes_read += pbytes;
  total_bytes_read += pbytes
  if bytes_read > 1024*1024*100:
    bytes_read -= 1024*1024*100
    if args.progress: 
        sys.stdout.write('.')
        sys.stdout.flush()

  if args.max_packets > 0  and packet_count > args.max_packets:
      break

  if ptype == arrival_time_type and last_ptype == arrival_time_type:
      if args.verbose: print "Warning: Unconsumed arrival time"
      unconsumed_arrival_time_warnings.append((ptime))
  if last_ptype is not None and ptype != arrival_time_type and last_ptype != arrival_time_type:
      missing_arrival_time_warnings.append((ptime))
      if args.verbose: print "warning: missing data packet arrival_time"
  last_ptype = ptype

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

  elif ptype == image_raw_type or ptype == stereo_raw_type:
      if ptype == image_raw_type:
          (exposure, width, height, stride, camera_format) = unpack('QHHHH', data[:16])
      else:
          (exposure, width, height, stride, stride2, camera_format) = unpack('QHHHHH', data[:18])
      type_str = format_types[camera_format]
      packet_str += "_" + type_str
      if exposure < 10 or exposure > 30000:
          exposure_warnings[packet_str].append((ptime, exposure))
      ptime += exposure/2
      if args.verbose:
          camera_str = "%s %d (%d) %dx%d, %d stride, %d exposure, %d adjusted time" % (type_str, sensor_id, camera_format, width, height, stride, exposure, ptime)
          print "\t", camera_str

  elif ptype == thermometer_type:
      if args.verbose:
          (temp_C) = unpack('f', data[:4])
          print "\t%.2fC" % (temp_C)

  elif ptype == arrival_time_type:
      if last_arrival_time > ptime:
          out_of_order_warnings.append((ptime, last_arrival_time))
      last_arrival_time = ptime
      if args.verbose:
          print "\t %d" % ptime

  elif ptype == odometry_type:
      (x, y) = unpack('ff', data[:8])
      if args.verbose:
          print "\t", x, y

  elif ptype == velocimeter_type:
      (x, y, z) = unpack('fff', data[:12])
      if args.verbose:
          print "\t", sensor_id, x, y, z

  elif ptype == calibration_type:
      if args.verbose:
          print "\t", data

  else:
      if args.verbose:
          print ""

  if packet_str == "":
      packet_str = str(ptype)
  if not latest_received or latest_received < ptime:
      latest_received = ptime
  if ptype not in (arrival_time_type, calibration_type) :
    latencies[packet_str].append(latest_received - ptime)
    packets[packet_str].append(ptime)
    arrivals[packet_str].append(last_arrival_time)
  if not (ptype == accel_type or ptype == gyro_type) and prev_packet_str == packet_str:
      warnings[packet_str].append((last_time, ptime))
  last_time = ptime
  prev_packet_str = packet_str
  header_str = f.read(header_size)

f.close()
if args.progress:
    print ''

error_text = ""
total_warnings = 0

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

start_s = {}
end_s = {}
for packet_type in sorted(packets.keys()):
  if packet_type == arrival_time_type: continue
  timestamps = numpy.array(packets[packet_type])
  platencies = numpy.array(latencies[packet_type])
  print packet_type, len(packets[packet_type]), "packets"
  if len(packets[packet_type]) == 1:
      print ""
      continue
  deltas = timestamps[1:] - timestamps[:-1]
  if len(arrivals[packet_type]) > 0:
    parrivals = numpy.array(arrivals[packet_type])
    arrivals_deltas = parrivals[1:] - parrivals[:-1]
  median_delta = numpy.median(deltas)
  start_s[packet_type] = numpy.min(timestamps)/1e6
  end_s[packet_type] = numpy.max(timestamps)/1e6
  print "\tRate:", numpy.float64(1)/(median_delta/1e6), "Hz"
  print "\tmedian dt (us):", median_delta
  print "\tstd dt (us):", numpy.std(deltas)
  print "\trelative latency (us): %.3f min, %.3f median, %.3f max, %.3f std" % (numpy.min(platencies), numpy.median(platencies), numpy.max(platencies), numpy.std(platencies))
  print "\tstart (s) finish (s):", start_s[packet_type], end_s[packet_type]
  print "\tlength (s):", end_s[packet_type] - start_s[packet_type]
  if len(arrivals[packet_type]) > 0:
    print "\tarrival (us): min: %.0f median: %.0f max: %.0f std: %.0f" % (numpy.min(arrivals_deltas), numpy.median(arrivals_deltas), numpy.max(arrivals_deltas), numpy.std(arrivals_deltas))
  else:
    print "\tNo arrival time data"

  if packet_type.startswith("thermometer"):
      print "Skipping packet frequency analysis for this sensor"
      continue

  exceptions = numpy.flatnonzero(numpy.logical_or(deltas > median_delta*1.05, deltas < median_delta*0.95))
  latency_warnings = numpy.flatnonzero(platencies > 33333)

  print len(exceptions), "samples are more than 5% from median"
  if len(latency_warnings):
      print len(latency_warnings), "packet latency warnings"
  if len(warnings[packet_type]):
      print len(warnings[packet_type]), "latency warnings"
  if len(exposure_warnings[packet_type]):
      print len(exposure_warnings[packet_type]), "exposure warnings"
  if len(imu_warnings[packet_type]):
      print len(imu_warnings[packet_type]), "IMU warnings"

  total_warnings += len(exceptions) + len(latency_warnings) + len(warnings[packet_type]) + len(exposure_warnings[packet_type]) + len(imu_warnings[packet_type])

  for l in latency_warnings:
      if platencies[l] > 50000:
          error_text += "Latency higher than 50ms for " + packet_type + "\n"
          break;
  for l in latency_warnings:
      if args.latency_warnings or platencies[l] > 50000:
          print "High latency:", platencies[l], "us at", timestamps[l]
  for e in exceptions:
      if deltas[e] > median_delta * 2:
          error_text += "Missing " + packet_type + " data (deltas too large)\n"
          break
  for e in exceptions:
      if args.exceptions or deltas[e] > median_delta * 2:
          print "Exception: t t+1 delta", timestamps[e], timestamps[e+1], deltas[e]
  if args.warnings:
      compressed_warnings = compress_warnings(warnings[packet_type])
      for w in compressed_warnings:
          print "Warning: ", len(w), "images at timestamps:", w
  if len(exposure_warnings[packet_type]):
      error_text += "Error: Exposure problems with " + packet_type + "\n"
  if args.exposure_warnings:
      for w in exposure_warnings[packet_type]:
          print "Warning: Image at", w[0], "had exposure of", w[1], "microseconds"
  if args.imu_warnings:
      numpy.set_printoptions(precision=2)
      for w in imu_warnings[packet_type]:
          print "Warning:", packet_type, "at", w[0], "changed by ", w[1], "current: ", w[2], "last:", w[3]
  print ""

start_times_s = numpy.array([start_s[key] for key in start_s])
if numpy.max(start_times_s) - numpy.min(start_times_s) > 5:
    error_text += "Error: Sensor start times differed by more than 5 seconds"

end_times_s = numpy.array([end_s[key] for key in end_s])
if numpy.max(end_times_s) - numpy.min(end_times_s) > 5:
    error_text += "Error: Sensor end times differed by more than 5 seconds"

total_time_s = numpy.max(end_times_s) - numpy.min(start_times_s)
print "Total capture time: %.2fs" % total_time_s

if got_types[calibration_type] == 0 and got_types[calibration_bin_type] == 0:
    print "Warning: Never received calibration packet"
    total_warnings += 1
elif got_types[calibration_type] + got_types[calibration_bin_type] > 1:
    error_text += "Too many calibration packets\n"
if got_types[arrival_time_type] == 0:
    print "Warning: Never received arrival_time packet"
    total_warnings += 1
else:
    if len(out_of_order_warnings) > 0 :
        error_text += "Error: %d packets arrival_time is out of order\n" % len(out_of_order_warnings)
    if len(missing_arrival_time_warnings) > 0 :
        error_text += "Error: %d packets missing arrival time data\n" % len(missing_arrival_time_warnings)
    if len(unconsumed_arrival_time_warnings) > 0 :
        error_text += "Error: %d unconsumed arrival-time packets\n" % len(unconsumed_arrival_time_warnings)

if got_types[thermometer_type] == 0:
    print "Warning: Never received any thermometer data\n"
    total_warnings += 1

if got_types[accel_type] == 0:
    error_text += "Error: Never received any accelerometer data\n"
if got_types[gyro_type] == 0:
    error_text += "Error: Never received any gyro data\n"
if got_types[image_raw_type] == 0 and \
   got_types[image_with_depth] == 0 and \
   got_types[stereo_raw_type] == 0:
    error_text += "Error: Never received any image data\n"

max_warnings = max(10*total_time_s/60, 10) # 10 warnings per minute
exit_code=0
if len(error_text) > 0:
    print "Failing errors:\n" + error_text
    print "capturecheck result: Failed!"
    exit_code = 1
elif total_warnings > max_warnings:
    print "capturecheck result: Failed, with", total_warnings, "warnings!"
    exit_code = 2
else:
    print "capturecheck result: Passed, with", total_warnings, "warnings"
sys.exit(exit_code)

