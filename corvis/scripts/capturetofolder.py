#!/usr/bin/env python
from struct import unpack
import sys
import os, os.path
import errno

def ensure_path(path):
    try:
        os.makedirs(path)
    except OSError as exception:
        if exception.errno != errno.EEXIST:
            raise

accel_type = 20
gyro_type = 21
image_raw_type = 29

capture_filename = sys.argv[1]
output_folder = sys.argv[2]
fisheye_folder = os.path.join(output_folder, "fisheye")
depth_folder = os.path.join(output_folder, "depth")

ensure_path(output_folder)
ensure_path(fisheye_folder)
fisheye_file = open(os.path.join(output_folder, "fisheye_timestamps.txt"), "w")
accel_file = open(os.path.join(output_folder, "accel.txt"), "w")
gyro_file = open(os.path.join(output_folder, "gyro.txt"), "w")

def imu_to_str(t_us, x, y, z):
    return "%f, %.16f, %.16f, %.16f\n" % (float(t_us)/1e3, x, y, z)

def camera_to_str(t_us, path):
    return  "%s %f\n" % (path, float(t_us)/1e3)

def camera_to_pgm_header(width, height, max_val):
    return "P5\n%d %d\n%d\n" % (width, height, max_val)

# defined in rc_tracker.h
rc_IMAGE_GRAY8 = 0
rc_IMAGE_DEPTH16 = 1

f = open(capture_filename, "rb")
header_size = 16
header_str = f.read(header_size)
frame_number = 0
depth_frame_number = 0
while header_str != "":
    (pbytes, ptype, user, ptime) = unpack('IHHQ', header_str)
    data = f.read(pbytes-header_size)
    if ptype == accel_type or ptype == gyro_type:
        # packets are padded to 8 byte boundary
        (x, y, z) = unpack('fff', data[:12])
        line = imu_to_str(ptime,x,y,z)
        if ptype == accel_type: accel_file.write(line)
        if ptype == gyro_type: gyro_file.write(line)
        
    if ptype == image_raw_type:
        (exposure, width, height, stride, camera_format) = unpack('QHHHH', data[:16])
        if camera_format == rc_IMAGE_GRAY8:
            image_filename = "fisheye/image_%06d.pgm" % frame_number
            line = camera_to_str(ptime, image_filename)
            pgm_header = camera_to_pgm_header(width, height, 255)
            frame_data = data[16:]
            fisheye_file.write(line)
            frame_number += 1
        elif camera_format == rc_IMAGE_DEPTH16:
            if depth_frame_number == 0:
                ensure_path(depth_folder)
                depth_file = open(os.path.join(output_folder, "depth_timestamps.txt"), "w")
            image_filename = "depth/image_%06d.pgm" % depth_frame_number
            line = camera_to_str(ptime, image_filename)
            pgm_header = camera_to_pgm_header(width, height, 65535)
            frame_data = data[16:]
            depth_file.write(line)
            depth_frame_number += 1
        frame = open(os.path.join(output_folder, image_filename), "wb")
        frame.write(pgm_header)
        frame.write(frame_data)
        frame.close()

    header_str = f.read(header_size)

f.close()

fisheye_file.close()
accel_file.close()
gyro_file.close()
if depth_frame_number:
    depth_file.close()
