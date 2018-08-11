#!/usr/bin/env python
from __future__ import print_function
import sys
import csv
import os.path
import numpy
if len(sys.argv) != 4:
    print("Usage:", sys.argv[0], "<vicon.log> <capture_folder> <shifted_vicon.log>")
    sys.exit(1)

vicon_file = sys.argv[1]
accel_file = os.path.join(sys.argv[2], "accel.txt")
gyro_file = os.path.join(sys.argv[2], "gyro.txt")
vicon_output_file = sys.argv[3]
vicon_to_camera__m = numpy.array([0.065, 0.035, -0.095])

def quaternion_to_rotation_matrix(x, y, z, w):
    R = numpy.zeros([3,3])
    R[0][0] = 1 - 2 * (y*y + z*z)
    R[0][1] = 2 * (x*y - w*z)
    R[0][2] = 2 * (x*z + w*y)
    R[1][0] = 2 * (x*y + w*z)
    R[1][1] = 1 - 2 * (x*x + z*z)
    R[1][2] = 2 * (y*z - w*x)
    R[2][0] = 2 * (x*z - w*y)
    R[2][1] = 2 * (y*z + w*x)
    R[2][2] = 1 - 2 * (x*x + y*y)
    return R

def read_csv_floats(filename, header=False):
    #accel is timestamp_us, x, y, z
    #gyro is timestamp_us, wx, wy, wz
    #time_sec,time_nsec,seq,x,y,z,qx,qy,qz,qw
    #1460549401,54643481,546125,-0.141443916478,0.705408679784,0.230390428816,0.0103142744375,-0.0107986784063,-0.0400554246202,0.999085865802
    rows = []
    with open(filename, 'rb') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        if header: reader.next() # skip header
        for row in reader:
            float_row = [float(r) for r in row]
            rows.append(float_row)
    return rows

gyro_data = read_csv_floats(gyro_file)
accel_data = read_csv_floats(accel_file)
vicon_data = read_csv_floats(vicon_file, header=True)

first_timestamp_ms = min(gyro_data[0][0], accel_data[0][0])
last_timestamp_ms = max(gyro_data[-1][0], accel_data[-1][0])
vicon_start_s = vicon_data[0][0]
vicon_start_ns = vicon_data[0][1]

print "IMU data had", (last_timestamp_ms - first_timestamp_ms)/1e3, "seconds"
print "Vicon had", vicon_data[-1][0] - vicon_data[0][0], "seconds"

def gyro_offset(gyro_data, threshold):
    for meas in gyro_data:
        if (meas[1]*meas[1] + meas[2]*meas[2] + meas[3]*meas[3]) > threshold*threshold:
            print "Gyro", meas
            return meas[0] - first_timestamp_ms
    print "ERROR, didn't find an offset"

import numpy, math
def accel_offset(accel_data, low_pass, threshold):
    acc = numpy.array(accel_data[0][1:])
    n_meas = 0
    for meas in accel_data:
        acc = acc * (1-low_pass) + numpy.array(meas[1:])*low_pass
        delta = acc - meas[1:]
        if delta.dot(delta) > threshold*threshold:
            print "Accel delta", delta
            return meas[0] - first_timestamp_ms
    print "ERROR, didn't find an offset"

def vicon_offset(vicon_data, rotation_thresh, translation_m):
    T_start = numpy.array(vicon_data[0][3:6])
    w_start = numpy.array(vicon_data[0][6:10])
    for meas in vicon_data:
        T_now = numpy.array(meas[3:6])
        w_now = numpy.array(meas[6:10])
        dT = T_now - T_start
        dR = w_now - w_start
        if dT.dot(dT) > translation_m*translation_m or dR.dot(dR) > rotation_thresh*rotation_thresh:
            print "Vicon", dT, dR
            return meas[0] - vicon_start_s, meas[1] - vicon_start_ns
    print "ERROR, didn't find an offset"

gyro_time_offset_ms = gyro_offset(gyro_data, 0.5)
accel_time_offset_ms = accel_offset(accel_data, 0.1, 3)
translation_m = 0.01
rotation_thresh = 2 * math.pi / 180 # for small angles norm is similar to angle between rotations
(vicon_time_offset_s, vicon_time_offset_ns) = vicon_offset(vicon_data, rotation_thresh, translation_m)
print "Offsets: ", gyro_time_offset_ms/1e3, accel_time_offset_ms/1e3
print "Vicon offsets:", vicon_time_offset_s, vicon_time_offset_ns

print "First was:", first_timestamp_ms/1e3
imu_offset_ms = first_timestamp_ms + min(gyro_time_offset_ms, accel_time_offset_ms)
imu_offset_s = math.floor(imu_offset_ms/1e3)
imu_offset_ns = (imu_offset_ms/1e3 - imu_offset_s)*1e9
vicon_to_imu_s = imu_offset_s - vicon_start_s - vicon_time_offset_s
vicon_to_imu_ns = imu_offset_ns - vicon_start_ns - vicon_time_offset_ns
print "S, ns", vicon_to_imu_s, vicon_to_imu_ns, imu_offset_ms, imu_offset_ns

def write_offset_vicon(original_filename, output_filename, offset_s, offset_ns):
    #time_sec,time_nsec,seq,x,y,z,qx,qy,qz,qw
    #1460549401,54643481,546125,-0.141443916478,0.705408679784,0.230390428816,0.0103142744375,-0.0107986784063,-0.0400554246202,0.999085865802
    rows = []
    with open(original_filename, 'rb') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        with open(output_filename, 'wb') as csvout:
            writer = csv.writer(csvout, delimiter=',')
            writer.writerow(reader.next()) # write header
            for row in reader:
                row_s = int(row[0]) + offset_s
                row_ns = int(row[1]) + offset_ns
                if row_ns < 0:
                    row_s -= 1
                    row_ns += 1e9
                row[0] = str(int(row_s))
                row[1] = str(int(row_ns))
                T = numpy.array([float(row[3]), float(row[4]), float(row[5])])
                R = quaternion_to_rotation_matrix(float(row[6]), float(row[7]),
                        float(row[8]), float(row[9]))
                T = T + numpy.dot(R, vicon_to_camera__m)
                row[3] = str(T[0])
                row[4] = str(T[1])
                row[5] = str(T[2])
                writer.writerow(row)

write_offset_vicon(vicon_file, vicon_output_file, vicon_to_imu_s, vicon_to_imu_ns)
