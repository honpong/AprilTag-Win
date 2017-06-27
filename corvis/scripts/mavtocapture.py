#!/usr/bin/env python
from PIL import Image
from struct import pack
from collections import defaultdict
import itertools
import json
import yaml
import csv
import sys
import math
import re
from os import path

accel_type = 20
gyro_type = 21
image_raw_type = 29

# defined in rc_tracker.h
rc_IMAGE_GRAY8 = 0
rc_IMAGE_DEPTH16 = 1

try:
    input_dir, output_filename = sys.argv[1:3]
except Exception as e:
    print(e)
    print(sys.argv[0], "<MAV_folder> <output_filename>")
    sys.exit(1)


def load_body(base):
    with open(base + "/body.yaml") as f:
        return yaml.safe_load(f)

class mav_sensor(object):
    def __init__(self,base):
        with open(base + "/sensor.yaml") as f:
            self.meta = yaml.safe_load(f)

        self.data = []
        with open(base + "/data.csv") as f:
            header = f.readline().rstrip()
            assert header[0] == "#", "data not csv with a header starting with #"
            keys = re.split(r"\s*,\s*", header[1:])
            for values in csv.reader(f):
                self.data.append(dict(zip(keys,values)))

def compute_extrinsics(m):
    assert m['rows'] == 4 and m['cols'] == 4
    import numpy as np
    from scipy import linalg
    G = np.array(m['data'])
    G.resize(m['rows'], m['cols'])
    W = linalg.logm(G[0:3,0:3])
    return {
        'T': list(G[0:3,3]),
        'W': [(W[2, 1] - W[1, 2])/2,
              (W[0, 2] - W[2, 0])/2,
              (W[1, 0] - W[0, 1])/2],
        'T_variance': [0,0,0],
        'W_variance': [0,0,0],
    }

s = load_body(input_dir)
cal = {'calibration_version': 10, 'device_id':"", 'device_type':s['comment'], 'cameras':[], 'imus':[], 'depths':[]}
data = []
for i in itertools.count():
    imu = input_dir + "/imu"+str(i)
    if not path.exists(imu):
        break
    sensor = mav_sensor(imu)
    s = sensor.meta
    gyro_bias_variance = s['gyroscope_random_walk'] * s['gyroscope_random_walk'] * s['rate_hz']
    acc_bias_variance = s['accelerometer_random_walk'] * s['accelerometer_random_walk'] * s['rate_hz']
    cal['imus'].append({
        'accelerometer': {
            'noise_variance': s['accelerometer_noise_density'] * s['accelerometer_noise_density'] * s['rate_hz'],
            'bias': [0,0,0],
            'bias_variance': [acc_bias_variance, acc_bias_variance, acc_bias_variance],
            'scale_and_alignment': [ 1,0,0, 0,1,0, 0,0,1 ],
        },
        'gyroscope':     {
            'noise_variance': s['gyroscope_noise_density'] * s['gyroscope_noise_density'] * s['rate_hz'],
            'bias': [0,0,0],
            'bias_variance': [gyro_bias_variance, gyro_bias_variance, gyro_bias_variance],
            'scale_and_alignment': [ 1,0,0, 0,1,0, 0,0,1 ],
        },
        'extrinsics': compute_extrinsics(s['T_BS']),
    })
    for d in sensor.data:
        data.append({ 'ptype':gyro_type,  'id':i, 'time_ns':int(d['timestamp [ns]']), 'w':tuple(map(float,(d['w_RS_S_x [rad s^-1]'], d['w_RS_S_y [rad s^-1]'], d['w_RS_S_z [rad s^-1]']))) })
        data.append({ 'ptype':accel_type, 'id':i, 'time_ns':int(d['timestamp [ns]']), 'a':tuple(map(float,(d['a_RS_S_x [m s^-2]'],   d['a_RS_S_y [m s^-2]'],   d['a_RS_S_z [m s^-2]']  ))) })

for i in itertools.count():
    cam = input_dir + "/cam"+str(i)
    if not path.exists(cam):
        break
    sensor = mav_sensor(cam)
    s = sensor.meta
    cal['cameras'].append({
        'focal_length_px': [s['intrinsics'][0],s['intrinsics'][1]],
        'center_px': [s['intrinsics'][2],s['intrinsics'][3]],
        'size_px': s['resolution'],
        'distortion': { 'pinhole': { 'type': 'polynomial', 'k': s['distortion_coefficients'][0:3]  } }[s['camera_model']],
        'extrinsics': compute_extrinsics(s['T_BS']),
    })
    for d in sensor.data:
        data.append({ 'ptype':image_raw_type,'id':i, 'time_ns':int(d['timestamp [ns]']), 'name': cam + '/data/' + d['filename'], 'rate_hz': float(s['rate_hz']) })

groundtruth = []
for i in itertools.count():
    gte = input_dir + "/state_groundtruth_estimate"+str(i)
    if not path.exists(gte):
        break
    sensor = mav_sensor(gte)

    if False: # These optimized biases are per-sequence
        cal['imus'][i]['accelerometer']['bias'] = list(float(sensor.data[0][x]) for x in ('b_a_RS_S_x [m s^-2]',  'b_a_RS_S_y [m s^-2]',  'b_a_RS_S_z [m s^-2]'  ))
        cal['imus'][i]['gyroscope'    ]['bias'] = list(float(sensor.data[0][x]) for x in ('b_w_RS_S_x [rad s^-1]','b_w_RS_S_y [rad s^-1]','b_w_RS_S_z [rad s^-1]'))
    else: # Instead we use averaged values above so they are per-device not per-run
        cal['imus'][i]['accelerometer']['bias'] = [-0.0207691, 0.119103, 0.0737784]
        cal['imus'][i]['gyroscope'    ]['bias'] = [-0.00236, 0.02221, 0.077807]

    #FIXME transform by the given extrinsics
    for d in sensor.data:
        groundtruth.append({
            'time_us': int(int(d['timestamp'])/1000),
            'Q': list(float(d[x]) for x in ('q_RS_w []', 'q_RS_x []', 'q_RS_y []', 'q_RS_z []')),
            'T_m': list(float(d[x]) for x in ('p_RS_R_x [m]', 'p_RS_R_y [m]', 'p_RS_R_z [m]')),
        })

vicon = []
for i in itertools.count():
    vic = input_dir + "/vicon"+str(i)
    if not path.exists(vic):
        break
    sensor = mav_sensor(vic)
    #FIXME transform by the given extrinsics
    for d in sensor.data:
        vicon.append({
            'time_us': int(int(d['timestamp [ns]'])/1000),
            'Q': list(float(d[x]) for x in ('q_RS_w []', 'q_RS_x []', 'q_RS_y []', 'q_RS_z []')),
            'T_m': list(float(d[x]) for x in ('p_RS_R_x [m]', 'p_RS_R_y [m]', 'p_RS_R_z [m]')),
        })

leica = []
for i in itertools.count():
    lei = input_dir + "/leica"+str(i)
    if not path.exists(lei):
        break
    sensor = mav_sensor(lei)
    #FIXME transform by the given extrinsics
    for d in sensor.data:
        leica.append({
            'time_us': int(int(d['timestamp [ns]'])/1000),
            'Q': [1,0,0,0],
            'T_m': list(float(d[x]) for x in ('p_RS_R_x [m]', 'p_RS_R_y [m]', 'p_RS_R_z [m]')),
        })

with open(output_filename + ".json", "w") as f:
  json.dump(cal, f, sort_keys=True,indent=4)

if len(vicon) or len(groundtruth) or len(leica):
    with open(output_filename + ".tum", "w") as f:
        for p in groundtruth if groundtruth else vicon if vicon else leica:
            time_s = "%.9f" % (p['time_us']/1.e6)
            f.write(' '.join(map(str,(time_s, p['T_m'][0], p['T_m'][1], p['T_m'][2], p['Q'][1], p['Q'][2], p['Q'][3], p['Q'][0]))) + "\n")

wrote_packets = defaultdict(int)
wrote_bytes = 0

with open(output_filename, "wb") as f:
  for p in sorted(data, key=lambda x: x['time_ns']):
    ptype, time_us, sensor_id = p['ptype'], int(p['time_ns']/1000), p['id']

    if ptype == image_raw_type:
        im = Image.open(p['name'])
        (w,h) = im.size
        b = 1
        stride = w*b
        d = im.tobytes()
        assert len(d) == stride*h
        data = pack('QHHHH', 0, w, h, stride, rc_IMAGE_GRAY8) + d
    elif ptype == gyro_type:
        data = pack('fff', *p['w'])
    elif ptype == accel_type:
        data = pack('fff', *p['a'])
    else:
        print("Unexpected data type", ptype)

    pbytes = len(data) + 16
    header_str = pack('IHHQ', pbytes, ptype, sensor_id, time_us)
    f.write(header_str)
    f.write(data)
    wrote_packets[ptype] += 1
    wrote_bytes += len(header_str) + len(data)
