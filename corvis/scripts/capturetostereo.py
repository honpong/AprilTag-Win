#!/usr/bin/env python
from struct import unpack, pack
import sys
import os, shutil
import errno

def ensure_path(path):
    try:
        os.makedirs(path)
    except OSError as exception:
        if exception.errno != errno.EEXIST:
            raise

image_raw_type = 29
stereo_raw_type = 40

capture_filename = sys.argv[1]
output_filename = sys.argv[2]

# defined in rc_tracker.h
rc_IMAGE_GRAY8 = 0
rc_IMAGE_DEPTH16 = 1

class image_frame:
    image_data = None
    (sensor_id, time_us) = (0,0)
    (exposure_us, width_px, height_px, stride, camera_format) = (0,0,0,0,0)
    def __init__(self, sensor_id, time_us, data):
        self.sensor_id = sensor_id
        self.time_us = time_us
        (self.exposure_us, self.width_px, self.height_px, self.stride,
                self.camera_format) = unpack('QHHHH', data[:16])
        self.image_data = data[16:]

def write_stereo_frame(f_out, frame1, frame2):
    if frame2.sensor_id < frame1.sensor_id:
        return write_stereo_frame(f_out, frame2, frame1)

    time_us = min(frame1.time_us, frame2.time_us)
    #print "write stereo frame", time_us, frame1.time_us, frame2.time_us
    data_header = pack('QHHHHH', frame1.exposure_us, frame1.width_px,
            frame1.height_px, frame1.stride, frame2.stride,
            frame1.camera_format)
    #print len(data_header)
    packet_bytes = len(data_header) + 16 + len(frame1.image_data) + len(frame2.image_data)
    header = pack('IHHQ', packet_bytes, stereo_raw_type, 0, time_us)
    f_out.write(header)
    f_out.write(data_header)
    f_out.write(frame1.image_data)
    f_out.write(frame2.image_data)

f = open(capture_filename, "rb")
f_out = open(output_filename, "wb")
header_size = 16
header_str = f.read(header_size)
last_frame = None
while header_str != "":
    (pbytes, ptype, sensor_id, ptime) = unpack('IHHQ', header_str)
    data = f.read(pbytes-header_size)
    if not ptype == image_raw_type:
        f_out.write(header_str)
        f_out.write(data)
        
    if ptype == image_raw_type:
        this_frame = image_frame(sensor_id, ptime, data)

        if last_frame and abs(last_frame.time_us - this_frame.time_us) < 1000:
            write_stereo_frame(f_out, last_frame, this_frame)

        last_frame = this_frame

    header_str = f.read(header_size)

f.close()
f_out.close()

if os.path.isfile(capture_filename + ".json"):
    shutil.copy(capture_filename + ".json", output_filename + ".json")

if os.path.isfile(capture_filename + ".tum"):
    shutil.copy(capture_filename + ".tum", output_filename + ".tum")
