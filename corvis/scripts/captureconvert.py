#!/usr/bin/env python
from struct import pack
import csv
from collections import defaultdict
import math
import sys
import getopt
import os.path
import json

accel_type = 20
gyro_type = 21
image_raw_type = 29

# defined in rc_tracker.h
rc_IMAGE_GRAY8 = 0
rc_IMAGE_DEPTH16 = 1

use_depth = True
depth_time_offset = 0
wait_for_image = False
try:
    opts, (path, output_filename) = getopt.gnu_getopt(sys.argv[1:], "Do:", ["no-depth", "depth-offset=", "wait-for-image"])
    for o,v in opts:
        if o in ("-D", "--no-depth"):
            use_depth = False;
        elif o in ("-o", "--depth-offset"):
            depth_time_offset = float(v)
        elif o in ("-w", "--wait-for-image"):
            wait_for_image = True
except Exception as e:
    print e
    print sys.argv[0], "[--no-depth] [--depth-offset=<N>] [--wait-for-image] <intel_folder> <output_filename>"
    sys.exit(1)

def read_chunk_info(filename):
    with open(filename, 'rb') as f:
        line = f.readline()
        (filename, offset, length, timestamp) = line.split()
        return (filename, int(length))

def read_image_timestamps(filename, image_type, fixed_filename = None, data_length = None):
    #image is filename timestamp
    rows = []
    z = 0
    with open(filename, 'rb') as f:
        for line in f.xreadlines():
            row = line.split()
            if len(row) == 2:
                (filename, timestamp) = row
                (offset, length) = (0, None)
                if fixed_filename and data_length:
                    filename = fixed_filename
                    length = data_length
                    offset = data_length*z
                    z += 1
            else:
                (filename, offset, length, timestamp) = row

            row = [float(timestamp), image_raw_type, filename]
            row.append(int(offset) if offset else 0)
            row.append(int(length) if length else None)
            row.append(image_type)

            rows.append(row)
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

import StringIO
def read_pgm(filename, offset=0, length=None):
    with open(filename, 'rb') as fi:
        fi.seek(offset) if offset else None
        return parse_pgm(fi if length is None else StringIO.StringIO(fi.read(length)))

def parse_pgm(f):
        assert f.read(1) == 'P', '%s is a pgm' % filename
        P = f.readline()
        while len(P.split()) < 4:
            P += f.readline()
        P = map(int,P.split())
        w, h, b, d = P[1], P[2], int(math.log(P[3]+1,2)/8), f.read()
        assert h * w * b == len(d), "%d x %d %d bytes/pixel == %d bytes" % (h , w, b, len(d))
        return (w,h,b,d)

(fisheye_data, fisheye_data_length) = (None, None)
if os.path.exists(path+'fisheye_offsets_timestamps.txt'):
    (fisheye_data, fisheye_data_length) = read_chunk_info(path+'fisheye_offsets_timestamps.txt')

(depth_data, depth_data_length) = (None, None)
if os.path.exists(path+'depth_offsets_timestamps.txt'):
    (depth_data, depth_data_length) = read_chunk_info(path+'depth_offsets_timestamps.txt')

fisheye_path = path + ('fisheye_timestamps.txt')
depth_path = path + ('depth_timestamps.txt')
raw = {
   'gyro':  read_csv_timestamps(path + 'gyro.txt', gyro_type),
   'accel': read_csv_timestamps(path + 'accel.txt', accel_type),
   'fish':  read_image_timestamps(fisheye_path, rc_IMAGE_GRAY8, fisheye_data, fisheye_data_length),
   'depth': read_image_timestamps(depth_path, rc_IMAGE_DEPTH16, depth_data, depth_data_length) if use_depth else [],
   'color': read_image_timestamps(path + 'color_timestamps.txt') if False else [],
}

data = [];
for t in ['gyro','accel', 'fish', 'depth']:
    data.extend(raw[t])
data.sort()

if use_depth:
    if abs(raw['depth'][0][0] - raw['fish'][0][0]) > 100:
        depth_time_offset += raw['depth'][0][0] - raw['fish'][0][0]
        print "correcting for depth camera and fisheye not being on the same clock"

wrote_packets = defaultdict(int)
wrote_bytes = 0
got_image = False
with open(output_filename, "wb") as f:
    for line in data:
        microseconds = int(line[0]*1e3)
        ptype = line[1]
        if ptype == image_raw_type:
            got_image = True
        if wait_for_image and not got_image:
            continue
        data = ""
        if ptype == image_raw_type:
            (time, ptype, filename, offset, length, image_type) = line
            w, h, b, d = read_pgm(path + filename, offset, length)
            if image_type == rc_IMAGE_GRAY8:
                assert b == 1, "image should be 1 byte, not %d" % b
            if image_type == rc_IMAGE_DEPTH16:
                assert b == 2, "depth should be 2 bytes, not %d" % b
                time += depth_time_offset
            stride = b*w
            data = pack('QHHHH', 0*33333333, w, h, stride, image_type) + d
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



def transform(W, T, v):
    from numpy import asarray, cross, eye, matrix
    from scipy.linalg import expm3
    return list(expm3(cross(eye(3), asarray(W))).dot(asarray(v)) + asarray(T))

if os.path.isfile(path + "CameraParameters.txt"):
    with open(path + "CameraParameters.txt", 'r') as p:
        d_e_c = p.readline().split()
        with open(path + "calibration.json", 'r') as f:
            cal = defaultdict(int, json.loads(f.read()))
            # ignored and often uninitialized
            if 'px' in cal:
                del cal['px']
            if 'py' in cal:
                del cal['py']

            imu_to_ds4_color = [0.095, 0, 0]
            if os.path.isfile(path + "calibration.xml"):
                import xml.etree.ElementTree as ET
                tree = ET.parse(path + "calibration.xml")
                root = tree.getroot()
                fisheye_index = "0"
                ds4_color_index = "3"
                for camera in root.findall("camera/camera_model"):
                    if camera.attrib["type"] == "calibu_fu_fv_u0_v0_k1_k2_k3" and camera.find("height").text == "180":
                        ds4_color_index = camera.attrib["index"]
                        print "Found ds4 color at", ds4_color_index
                    if camera.attrib["type"] == "calibu_fu_fv_u0_v0_w":
                        fisheye_index = camera.attrib["index"]
                        print "Found fisheye at", fisheye_index
                        params = camera.find("params").text
                        width = camera.find("width").text
                        height = camera.find("height").text
                        cal['imageWidth'] = int(width)
                        cal['imageHeight'] = int(height)
                        params = [float(p) for p in params.translate(None, '[];').split()]
                        cal['Fx'] = params[0]
                        cal['Fy'] = params[1]
                        cal['Cx'] = params[2]
                        cal['Cy'] = params[3]
                        cal['Kw'] = params[4]
                for extrinsic in root.findall("extrinsic_calibration"):
                    if extrinsic.attrib["frame_B_id"] == fisheye_index:
                        atb = extrinsic.find("A_T_B").text
                        tf = [float(a) for a in atb.translate(None, '[];,').split()]
                        (cal['Tc0'], cal['Tc1'], cal['Tc2']) = (tf[3], tf[7], tf[11])
                        (cal['Wc0'], cal['Wc1'], cal['Wc2']) = (0, 0, 0)
                        print "Fisheye tc", tf[3], tf[7], tf[11]
                    if extrinsic.attrib["frame_B_id"] == ds4_color_index:
                        atb = extrinsic.find("A_T_B").text
                        tf = [float(a) for a in atb.translate(None, '[];,').split()]
                        imu_to_ds4_color = [tf[3], tf[7], tf[11]]
                        print "Color camera Tc", tf[3], tf[7], tf[11]

            #print "d_e_c", d_e_c
            # g_accel_depth = g_accel_color * (g_color_depth)^-1
            ds4_color_to_depth = map(lambda x: -x/1000., map(float,d_e_c[6:9]))
            depth_Wc = [0, 0, 0]
            depth_Tc = transform(depth_Wc, imu_to_ds4_color, ds4_color_to_depth)
            if not 'depth' in cal:
                cal['depth'] = dict(zip("imageWidth imageHeight Fx Fy Cx Cy Wc0 Wc1 Wc2 Tc0 Tc1 Tc2".split(), map(int,d_e_c[:2]) + map(float,d_e_c[2:6]) + depth_Wc + depth_Tc))
                print "Added depth intrinsics and extrinsics to " + output_filename + ".json"
            with open(output_filename + ".json", 'w') as t:
                 t.write(json.dumps(cal, sort_keys=True, indent=4,separators=(',', ': ')))
else:
    with open(path + "calibration.json", 'r') as f:
        with open(output_filename + ".json", 'w') as t:
            t.write(f.read())
