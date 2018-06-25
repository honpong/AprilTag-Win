#!/usr/bin/env python
from struct import pack, unpack

class PacketType:
    camera = 1 # legacy type, use image_raw instead
    image_raw = 29
    accelerometer = 20
    gyroscope = 21
    odometry = 30
    thermometer = 31
    stereo_raw = 40
    arrival_time = 44
    velocimeter = 45
    calibration_json = 43
    calibration_bin = 48
    exposure_info = 49
    controller_physical_info = 50

class Packet:
    header_size = 16
    class Header:
        def __init__(self, header_str):
            (self.bytes, self.type, self.sensor_id, self.time) = unpack('IHHQ', header_str)

        @staticmethod
        def from_file(file_handle):
            header_str = file_handle.read(Packet.header_size)
            if len(header_str) == 0: return None
            return Packet.Header(header_str)

        @staticmethod
        def from_components(pbytes, ptype, sensor_id, ptime):
            return Packet.Header(pack('IHHQ', pbytes, ptype, sensor_id, ptime))

        def to_file(self, file_handle):
            file_handle.write(pack('IHHQ', self.bytes, self.type, self.sensor_id, self.time))

    def __init__(self, header, data, arrival_time = None, file_offset = -1):
        if header.type == PacketType.camera:
            header.type = PacketType.image_raw
            data = pack('QHHHH', 33333, 640, 480, 640, rc_IMAGE_GRAY8) + data[16:]
        self.header = header
        self.data = data
        self.arrival_time = arrival_time
        self.file_offset = file_offset

    """
    Timestamp is the adjusted time of the packet which accounts for
    exposure where applicable
    """
    def timestamp(self):
        if self.header.type == PacketType.image_raw:
            (exposure_time, width, height) = unpack('QHH', self.data[:12])
            return int(self.header.time + exposure_time/2)
        else:
            return self.header.time

    @staticmethod
    def from_file(file_handle, default_arrival_time = False):
        file_offset = file_handle.tell()
        packet_header = Packet.Header.from_file(file_handle)
        if packet_header is None: return None

        arrival_time = None
        if packet_header.type == PacketType.arrival_time:
            arrival_time = packet_header
            packet_header = Packet.Header.from_file(file_handle)
            if packet_header is None: return None

        if arrival_time is None and default_arrival_time:
            arrival_time = packet_header
            arrival_time.type = PacketType.arrival_time

        data = file_handle.read(packet_header.bytes - Packet.header_size)
        return Packet(packet_header, data, arrival_time, file_offset)

    def to_file(self, file_handle):
        if self.arrival_time:
            self.arrival_time.to_file(file_handle)
        assert(self.header.bytes == len(self.data) + Packet.header_size)
        self.header.to_file(file_handle)
        file_handle.write(self.data)
