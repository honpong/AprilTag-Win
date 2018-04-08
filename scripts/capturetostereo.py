#!/usr/bin/env python
from struct import unpack, pack
import sys
import os, shutil
import errno
from packet import Packet, PacketType

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

def stereo_frame(f_out, frame1, frame2):
    if frame2.sensor_id < frame1.sensor_id:
        return stereo_frame(f_out, frame2, frame1)

    time_us = min(frame1.time_us, frame2.time_us)
    #print "write stereo frame", time_us, frame1.time_us, frame2.time_us
    data_header = pack('QHHHHH', frame1.exposure_us, frame1.width_px,
            frame1.height_px, frame1.stride, frame2.stride,
            frame1.camera_format)
    #print len(data_header)
    packet_bytes = len(data_header) + 16 + len(frame1.image_data) + len(frame2.image_data)
    header = pack('IHHQ', packet_bytes, stereo_raw_type, 0, time_us)
    return Packet(Packet.Header(header), data_header+frame1.image_data+frame2.image_data)

f = open(capture_filename, "rb")
f_out = open(output_filename, "wb")
p = Packet.from_file(f)
last_frame = None
while p is not None:
    if (not p.header.type == PacketType.image_raw) or \
       (p.header.type == PacketType.image_raw and p.header.sensor_id >= 2):
        p.to_file(f_out)
    else:
        this_frame = image_frame(p.header.sensor_id, p.header.time, p.data)

        if last_frame and last_frame.sensor_id != this_frame.sensor_id and abs(last_frame.time_us - this_frame.time_us) < 1000:
            stereo_packet = stereo_frame(f_out, last_frame, this_frame)
            stereo_packet.arrival_time = p.arrival_time
            stereo_packet.to_file(f_out)

        last_frame = this_frame

    p = Packet.from_file(f)

f.close()
f_out.close()

for ext in [".json", ".tum", ".loop"]:
    if os.path.isfile(capture_filename + ext):
        shutil.copy(capture_filename + ext, output_filename + ext)
