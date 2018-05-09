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
wait_for_image = False
scale_units = False
try:
    opts, (path, output_filename) = getopt.gnu_getopt(sys.argv[1:], "Do:", ["no-depth", "wait-for-image", "scale-units"])
    for o,v in opts:
        if o in ("-D", "--no-depth"):
            use_depth = False;
        elif o in ("-w", "--wait-for-image"):
            wait_for_image = True
        elif o in ("-s", "--scale-units"):
            scale_units = True
except Exception as e:
    print e
    print sys.argv[0], "[--no-depth] [--wait-for-image] [--scale-units] <intel_folder> <output_filename>"
    sys.exit(1)

def read_image_timestamps(filename, image_type, sensor_id):
    rows = []
    with open(filename, 'rb') as f:
        has_header = csv.Sniffer().has_header(f.read(1024))
        f.seek(0)
        for line in f.xreadlines():
            if has_header:
                has_header = False
                continue
            row = line.split()
            if len(row) == 2:
                (filename, timestamp) = row
                offset, length = 0, None
            else:
                (filename, offset, length, timestamp) = row
                offset, length = int(offset), int(length)

            rows.append({'time_ms': float(timestamp), 'user': sensor_id,
                         'ptype': image_raw_type, 'image_type': image_type,
                         'filename': filename, 'offset': offset, 'length': length})
    return rows

def read_csv_timestamps(filename, ptype, sensor_id):
    #accel is timestamp, x, y, z
    #gyro is timestamp, wx, wy, wz
    rows = []
    with open(filename, 'rb') as csvfile:
        has_header = csv.Sniffer().has_header(csvfile.read(1024))
        csvfile.seek(0)
        reader = csv.reader(csvfile, delimiter=',')
        for row in reader:
            if has_header:
                has_header = False
                continue
            (timestamp, x, y, z) = row
            rows.append({'time_ms': float(timestamp), 'user': sensor_id, 'ptype': ptype, 'vector': [float(x), float(y), float(z)]})
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

fisheye_path = path + ('fisheye_offsets_timestamps.txt')
if not os.path.exists(fisheye_path):
    fisheye_path = path + ('fisheye_timestamps.txt')

depth_path   = path + (  'depth_offsets_timestamps.txt')
if not os.path.exists(depth_path):
    depth_path = path + (  'depth_timestamps.txt')

raw = {
   'gyro':  read_csv_timestamps(path + 'gyro.txt', gyro_type, 0),
   'accel': read_csv_timestamps(path + 'accel.txt', accel_type, 0),
   'fish':  read_image_timestamps(fisheye_path, rc_IMAGE_GRAY8, 0),
   'depth': read_image_timestamps(depth_path, rc_IMAGE_DEPTH16, 0) if use_depth else [],
   'color': read_image_timestamps(path + 'color_timestamps.txt', 0) if False else [],
}

data = [];
for t in ['gyro','accel', 'fish', 'depth']:
    data.extend(raw[t])
data.sort(key=lambda d: (d['time_ms'],d['ptype']))

wrote_packets = defaultdict(int)
wrote_bytes = 0
got_image = False
with open(output_filename, "wb") as f:
    for d in data:
        microseconds = int(d['time_ms']*1e3)
        ptype, user = d['ptype'], d['user']
        if ptype == image_raw_type:
            got_image = True
        if wait_for_image and not got_image:
            continue
        data = ""
        if ptype == image_raw_type:
            image_type = d['image_type']
            w, h, b, d = read_pgm(path + d['filename'], d['offset'], d['length'])
            if image_type == rc_IMAGE_GRAY8:
                assert b == 1, "image should be 1 byte, not %d" % b
            if image_type == rc_IMAGE_DEPTH16:
                assert b == 2, "depth should be 2 bytes, not %d" % b
            stride = b*w
            data = pack('QHHHH', 0*33333333, w, h, stride, image_type) + d
        elif ptype == gyro_type:
            data = pack('fff', *map(lambda x: x * (math.pi / 1280 if scale_units else 1), d['vector']))
        elif ptype == accel_type:
            data = pack('fff', *map(lambda x: x * (9.8065         if scale_units else 1), d['vector']))
        else:
            print "Unexpected data type", ptype
        pbytes = len(data) + 16
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
                    if camera.attrib["type"] == "calibu_fu_fv_u0_v0_k1_k2_k3" and camera.find("height").text == "720":
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
                        # should be about [0.005 0 0] on e6t
                        print "Fisheye tc", tf[3], tf[7], tf[11]
                    if extrinsic.attrib["frame_B_id"] == ds4_color_index:
                        atb = extrinsic.find("A_T_B").text
                        tf = [float(a) for a in atb.translate(None, '[];,').split()]
                        imu_to_ds4_color = [tf[3], tf[7], tf[11]]
                        # should be about [0.095 0 0] on e6t
                        print "Color camera Tc", tf[3], tf[7], tf[11]

            #print "d_e_c", d_e_c
            # should be about [-0.058 0 0] on e6t
            ds4_color_to_depth = map(lambda x: x/1000., map(float,d_e_c[6:9]))
            depth_Wc = [0, 0, 0]
            depth_Tc = transform(depth_Wc, imu_to_ds4_color, ds4_color_to_depth)
            if not 'depth' in cal or not 'Tc0' in cal['depth']:
                cal['depth'] = dict(zip("imageWidth imageHeight Fx Fy Cx Cy Wc0 Wc1 Wc2 Tc0 Tc1 Tc2".split(), map(int,d_e_c[:2]) + map(float,d_e_c[2:6]) + depth_Wc + depth_Tc))
                print "Added depth intrinsics and extrinsics to " + output_filename + ".json"
                print cal['depth']
            with open(output_filename + ".json", 'w') as t:
                 t.write(json.dumps(cal, sort_keys=True, indent=4,separators=(',', ': ')))
else:
    with open(path + "calibration.json", 'r') as f:
        with open(output_filename + ".json", 'w') as t:
            t.write(f.read())