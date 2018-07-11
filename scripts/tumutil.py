#!/usr/bin/python

from __future__ import print_function
import sys

def write_tum(tum_filename, poses):
    f = open(tum_filename, "w")
    for l in poses:
        if len(l) == 8:
            f.write("%.9f %.9f %.9f %.9f %.9f %.9f %.9f %.9f\n" % (l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]))
        elif len(l) == 10: #tumx
            f.write("%.9f %.9f %.9f %.9f %.9f %.9f %.9f %.9f %d %d\n" % (l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7], l[8], l[9]))
        else:
            print("Error: could not write", l, "to", tum_filename)
            sys.exit(1)

def read_tum(tum_filename):
    tum = []
    for line in open(tum_filename, "r"):
        fields = line.split()
        if len(fields) == 8:
            fields += ["0", "3"] # device_id 0, confidence 3
        elif len(fields) != 10:
            print("Error: could not read tum line", line, "from", tum_filename)
            sys.exit(1)
        tum.append([float(f) for f in fields])
    print("Read", len(tum), "poses from", tum_filename)
    return tum

import csv
def extract_csv_poses(filename, device_id):
    tum = []
    for fields in csv.DictReader(open(filename, "r")):
        if int(fields['Pose Index']) != device_id:
            continue
        time = float(fields['FW Timestamp (NanoSec)'])/1.e9
        (x,y,z) = (fields['Translation X (Meter)'], fields['Translation Y (Meter)'], fields['Translation Z (Meter)'])
        (i,j,k,r) = (fields['Rotation I'], fields['Rotation J'], fields['Rotation K'], fields['Rotation R'])
        confidence = fields['Tracker Confidence']
        line = [str(time), x, y, z, i, j, k, r, str(device_id), confidence]
        tum.append([float(f) for f in line])
    print("Read", len(tum), "poses from", filename, device_id)
    return tum

def extract_vive_csv_poses(filename, device_id):
    tum = []
    for fields in csv.DictReader(open(filename, "r")):
        if int(fields['Pose Index']) != device_id:
            continue
        time = float(fields[' Vive Timestamp(NanoSec)'])/1.e9
        (x,y,z) = (fields[' Translation X(Meter)'], fields[' Translation Y(Meter)'], fields[' Translation Z(Meter)'])
        (i,j,k,r) = (fields[' Rotation I'], fields[' Rotation J'], fields[' Rotation K'], fields[' Rotation R'])
        confidence = "3"
        line = [str(time), x, y, z, i, j, k, r, str(device_id), confidence]
        tum.append([float(f) for f in line])
    print("Read", len(tum), "poses from", filename, device_id)
    return tum

def extract_pose_offset(gt_filename, live_filename, live_device_id):
    (gt_vive_time_s, gt_host_time_s) = (None, None)
    for fields in csv.DictReader(open(gt_filename, "r")):
        gt_host_time_s = float(fields[' Host Timestamp(NanoSec)'])/1.e9
        gt_vive_time_s = float(fields[' Vive Timestamp(NanoSec)'])/1.e9
    if gt_vive_time_s is None:
        print("Error: No valid timestamp in", gt_filename)
        sys.exit(1)

    (live_fw_time_s, live_host_time_s) = (None, None)
    for fields in csv.DictReader(open(live_filename, "r")):
        if int(fields['Pose Index']) != live_device_id:
            continue
        live_fw_time_s   = float(fields['FW Timestamp (NanoSec)'])/1.e9
        #live_host_time_s = float(fields['Host Correlated Timestamp (NanoSec)'])/1.e9
        live_host_time_s = float(fields['Host Correlated to FW Timestamp (NanoSec)'])/1.e9
        break
    if live_fw_time_s is None:
        print("Error: No valid timestamp in", live_filename, live_device_id)
        sys.exit(1)

    host_to_fw = -live_host_time_s + live_fw_time_s
    gt_to_host = -gt_vive_time_s + gt_host_time_s
    gt_to_fw = gt_to_host + host_to_fw
    return gt_to_fw

def offset_poses(poses, offset):
    poses_out = []
    for p in poses:
        p[0] += offset
        poses_out.append(p)
    return poses_out

import transformations as tm
import numpy
def apply_handeye(poses, handeye):
    handeye_i = tm.inverse_matrix(handeye)
    poses_out = []
    for p in poses:
        # note p[7] is q.w()
        G = numpy.matmul(tm.translation_matrix([p[1], p[2], p[3]]), tm.quaternion_matrix([p[7], p[4], p[5], p[6]]))
        G = numpy.matmul(handeye, numpy.matmul(G, handeye_i))
        q = tm.quaternion_from_matrix(G)
        t = tm.translation_from_matrix(G)
        p[1:4] = t
        # note q[0] is q.w()
        p[4:8] = [q[1], q[2], q[3], q[0]] 
        poses_out.append(p)
    return poses_out

def read_handeye(filename):
    array = numpy.loadtxt(filename)
    if array.shape[0] != 4 or array.shape[1] != 4:
        print("Error: Handeye calibration", filename, "was the wrong size", array.shape)
    return array
