#!/usr/bin/env python
from struct import pack
import csv
from collections import defaultdict
import sys

camera_type = 1
accel_type = 20
gyro_type = 21
if len(sys.argv) != 3:
    print sys.argv[0], "<intel folder> <output filename>"
    sys.exit(1)

path = sys.argv[1]
output_filename = sys.argv[2]

def read_image_timestamps(filename):
    #image is filename timestamp
    rows = []
    with open(filename, 'rb') as csvfile:
        reader = csv.reader(csvfile, delimiter=' ')
        for row in reader:
            (filename, timestamp) = row
            rows.append([float(timestamp), camera_type, filename])
    return rows

def read_csv_timestamps(filename, ptype):
    #accel is timestamp, x, y, z
    #gyro is timestamp, wx, wy, wz
    rows = []
    with open(filename, 'rb') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        for row in reader:
            (timestamp, x, y, z) = row
            rows.append([float(timestamp), ptype, float(x), float(y), float(z)])
    return rows

data = []
data.extend(read_csv_timestamps(path + 'gyro.txt', gyro_type))
data.extend(read_csv_timestamps(path + 'accel.txt', accel_type))
data.extend(read_image_timestamps(path + 'fisheye_timestamps.txt'))
data.sort()

wrote_packets = defaultdict(int)
wrote_bytes = 0
f = open(output_filename, "wb")
for line in data:
    microseconds = int(line[0]*1e6)
    ptype = line[1]
    data = ""
    if ptype == camera_type:
        fi = open(path + line[2])
        data = fi.read()
        fi.close()
    elif ptype == gyro_type:
        data = pack('fff', line[2], line[3], line[4])
    elif ptype == accel_type:
        data = pack('fff', line[2], line[3], line[4])
    else:
        print "Unexpected data type", ptype
    pbytes = len(data) + 16
    user = 0
    header_str = pack('IHHQ', pbytes, ptype, user, microseconds)
    f.write(header_str)
    f.write(data)
    wrote_packets[ptype] += 1
    wrote_bytes += len(header_str) + len(data)

f.close()

print "Wrote", wrote_bytes/1e6, "Mbytes"
for key in wrote_packets:
    print "Type", key, "-", wrote_packets[key], "packets"
