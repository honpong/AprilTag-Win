#!/usr/bin/env python
from struct import pack
import csv
from collections import defaultdict
import math
import sys
import getopt

camera_type = 1
accel_type = 20
gyro_type = 21
image_with_depth_type = 28

use_depth = True
offset = 0
try:
    opts, (path, output_filename) = getopt.gnu_getopt(sys.argv[1:], "Do:", ["no-depth", "depth-offset="])
    for o,v in opts:
        if o in ("-D", "--no-depth"):
            use_depth = False;
        elif o in ("-o", "--depth-offset"):
            offset = float(v)
except Exception as e:
    print e
    print sys.argv[0], "[--no-depth] [--depth-offset=<N>] <intel_folder> <output_filename>"
    sys.exit(1)

def read_image_timestamps(filename):
    #image is filename timestamp
    rows = []
    with open(filename, 'rb') as csvfile:
        reader = csv.reader(csvfile, delimiter=' ')
        for row in reader:
            (filename, timestamp) = row
            rows.append([float(timestamp), image_with_depth_type, filename])
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

def read_pgm(filename):
    with open(filename, 'rb') as fi:
        assert fi.read(1) == 'P', '%s is a pgm' % filename
        P = fi.readline()
        while len(P.split()) < 4:
            P += fi.readline()
        P = map(int,P.split())
        w, h, b, d = P[1], P[2], int(math.log(P[3]+1,2)/8), fi.read()
        assert h * w * b == len(d), "%d x %d %d bytes/pixel == %d bytes" % (h , w, b, len(d))
        return (w,h,b,d)

raw = {
   'gyro':  read_csv_timestamps(path + 'gyro.txt', gyro_type),
   'accel': read_csv_timestamps(path + 'accel.txt', accel_type),
   'fish':  read_image_timestamps(path + 'fisheye_timestamps.txt'),
   'depth': read_image_timestamps(path + 'depth_timestamps.txt') if use_depth else [] ,
   'color': read_image_timestamps(path + 'color_timestamps.txt') if False else [],
}

data = [];
for t in ['gyro','accel', 'fish']:
    data.extend(raw[t])
data.sort()

if use_depth:
    # The depth camera is not on the same clock as the gyro, accel and fisheye camera
    offset += raw['depth'][0][0] - raw['fish'][0][0]
    def depth_for_image(t):
        t += offset
        best = raw['depth'][0]
        for r in raw['depth']:
            if abs(r[0] - t) < abs(r[0] - best[0]):
                best = r
        return best

wrote_packets = defaultdict(int)
wrote_bytes = 0
got_image = False
with open(output_filename, "wb") as f:
    for line in data:
        microseconds = int(line[0]*1e3)
        ptype = line[1]
        if ptype == image_with_depth_type or ptype == camera_type:
            got_image = True
        if not got_image:
            continue
        data = ""
        if ptype == camera_type:
            with open(path + line[2]) as fi:
                data = fi.read()
        elif ptype == image_with_depth_type:
            w, h, b, d = read_pgm(path + line[2])
            assert b == 1, "image should be 1 byte, not %d" % b
            dw, dh, db, dd = read_pgm(path + depth_for_image(line[0])[2]) if use_depth else (0, 0, 0, '')
            assert db == 2 or not use_depth, "depth should be 2 bytes, not %d" % db
            data = pack('LHHHH', 0*33333333, w, h, dw, dh) + d + dd
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

print "Wrote", wrote_bytes/1e6, "Mbytes"
for key in wrote_packets:
    print "Type", key, "-", wrote_packets[key], "packets"

with open(path + "calibration.json", 'r') as f:
    with open(output_filename + ".json", 'w') as t:
        t.write(f.read())
