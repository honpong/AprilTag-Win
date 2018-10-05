#!/usr/bin/env python
from struct import unpack, pack, calcsize
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

capture_filename = sys.argv[1]
output_filename = sys.argv[2]

class image_frame:
    image_data = None
    (sensor_id, time_us) = (0,0)
    (exposure_us, width_px, height_px, stride, camera_format) = (0,0,0,0,0)
    def __init__(self, sensor_id, time_us, header, data, arrival_time):
        self.sensor_id = sensor_id
        self.time_us = time_us
        (self.exposure_us, self.width_px, self.height_px, self.stride, self.camera_format) = header
        self.image_data = data
        self.arrival_time = arrival_time
    def to_file(self, f_out):
        data_header = pack('QHHHH', self.exposure_us, self.width_px, self.height_px, self.stride, self.camera_format)
        packet_bytes = 16 + len(data_header) + len(self.image_data)
        header = pack('IHHQ', packet_bytes, PacketType.image_raw, self.sensor_id, self.time_us)
        Packet(Packet.Header(header), data_header+self.image_data, self.arrival_time).to_file(f_out)

class stereo_frame:
    image_data = None
    (exposure_us, width_px, height_px, stride1, stride2, camera_format) = (0,0,0,0,0,0)
    def __init__(self, packet_data):
        header_len = calcsize('QHHHHH')
        (self.exposure_us, self.width_px, self.height_px, self.stride1, self.stride2, self.camera_format) = unpack('QHHHHH', packet_data[:header_len])
        self.image_data = packet_data[header_len:]
    def mono_header(self, idx):
        return (self.exposure_us, self.width_px, self.height_px, self.stride1 if idx == 0 else self.stride2, self.camera_format)
    def mono_data(self, idx):
        if idx == 0:
            offset = 0
            sz = self.height_px * self.stride1
        else:
            offset = self.height_px * self.stride1
            sz = self.height_px * self.stride2
        return self.image_data[offset:offset+sz]

f = open(capture_filename, "rb")
f_out = open(output_filename, "wb")
p = Packet.from_file(f)
last_frame = None
while p is not None:
    if (not p.header.type == PacketType.stereo_raw):
        p.to_file(f_out)
    else:
        st = stereo_frame(p.data) 
        frame0 = image_frame(p.header.sensor_id * 2 + 0, p.header.time, st.mono_header(0), st.mono_data(0), p.arrival_time)
        frame1 = image_frame(p.header.sensor_id * 2 + 1, p.header.time, st.mono_header(1), st.mono_data(1), p.arrival_time)
        frame0.to_file(f_out)
        frame1.to_file(f_out) 

    p = Packet.from_file(f)

f.close()
f_out.close()

for ext in [".json", ".tum", ".xml"]:
    if os.path.isfile(capture_filename + ext):
        shutil.copy(capture_filename + ext, output_filename + ext)
