#!/usr/bin/env python
from struct import unpack
import sys
import os, os.path
import errno
from collections import defaultdict
from packet import Packet, PacketType

# from http://stackoverflow.com/a/28382515 for windows symlink supports
import os
if os.name == "nt":
    def symlink_ms(source, link_name):
        import ctypes
        csl = ctypes.windll.kernel32.CreateSymbolicLinkW
        csl.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32)
        csl.restype = ctypes.c_ubyte
        flags = 1 if os.path.isdir(source) else 0
        try:
            if csl(link_name, source.replace('/', '\\'), flags) == 0:
                raise ctypes.WinError()
        except:
            pass
    os.symlink = symlink_ms


def ensure_path(path):
    try:
        os.makedirs(path)
    except OSError as exception:
        if exception.errno != errno.EEXIST:
            raise

def convert(capture_filename, output_folder, drop_images, drop_accel, drop_gyro, drop_image_ids):
    frame_name = "fisheye"
    depth_name = "depth"
    depth_folder = os.path.join(output_folder, "depth")

    ensure_path(output_folder)
    old_fe_folder = os.path.join(output_folder, "fisheye")
    old_fe_timestamps = os.path.join(output_folder, "fisheye_timestamps.txt")
    if os.path.exists(old_fe_folder):
        try:
            os.rmdir(old_fe_folder)
        except OSError as exception:
            os.remove(old_fe_folder)
    if(os.path.exists(old_fe_timestamps)):
        os.remove(old_fe_timestamps)

    if not drop_accel:
        accel_file = open(os.path.join(output_folder, "accel.txt"), "w")

    if not drop_gyro:
        gyro_file = open(os.path.join(output_folder, "gyro.txt"), "w")

    def imu_to_str(t_us, x, y, z):
        return "%f, %.16f, %.16f, %.16f\n" % (float(t_us)/1e3, x, y, z)

    def camera_to_str(t_us, path):
        return  "%s %f\n" % (path, float(t_us)/1e3)

    def camera_to_pgm_header(width, height, max_val):
        return ("P5\n%d %d\n%d\n" % (width, height, max_val)).encode('ASCII')

    # defined in rc_tracker.h
    rc_IMAGE_GRAY8 = 0
    rc_IMAGE_DEPTH16 = 1

    f = open(capture_filename, "rb")
    frame_numbers = defaultdict(int)
    frame_file_handles = {}
    depth_frame_number = 0
    depth_frame_file_handles = {}

    p = Packet.from_file(f)
    while p is not None:
        (sensor_id, ptime) = (p.header.sensor_id, p.header.time)
        if p.header.type == PacketType.accelerometer or p.header.type == PacketType.gyroscope:
            # packets are padded to 8 byte boundary
            (x, y, z) = unpack('fff', p.data[:12])
            line = imu_to_str(ptime,x,y,z)
            if p.header.type == PacketType.accelerometer and sensor_id == 0 and not drop_accel: accel_file.write(line)
            if p.header.type == PacketType.gyroscope and sensor_id == 0 and not drop_gyro: gyro_file.write(line)

        if p.header.type == PacketType.image_raw and not drop_images and sensor_id not in drop_image_ids:
            (exposure, width, height, stride, camera_format) = unpack('QHHHH', p.data[:16])
            ptime += exposure//2
            if camera_format == rc_IMAGE_GRAY8:
                if sensor_id not in frame_file_handles:
                    frame_file_handles[sensor_id] = open(os.path.join(output_folder, "%s_%d_timestamps.txt" % (frame_name, sensor_id)), "w")
                    ensure_path(os.path.join(output_folder, "%s_%d" % (frame_name, sensor_id)))
                image_filename = "%s_%d/image_%06d.pgm" % (frame_name, sensor_id, frame_numbers[sensor_id])
                line = camera_to_str(ptime, image_filename)
                pgm_header = camera_to_pgm_header(width, height, 255)
                frame_data = p.data[16:]
                frame_file_handles[sensor_id].write(line)
                frame_numbers[sensor_id] += 1
            elif camera_format == rc_IMAGE_DEPTH16:
                if depth_frame_number == 0:
                    ensure_path(depth_folder)
                    depth_file = open(os.path.join(output_folder, "depth_timestamps.txt"), "w")
                image_filename = "depth/image_%06d.pgm" % depth_frame_number
                line = camera_to_str(ptime, image_filename)
                pgm_header = camera_to_pgm_header(width, height, 65535)
                frame_data = p.data[16:]
                depth_file.write(line)
                depth_frame_number += 1
            frame = open(os.path.join(output_folder, image_filename), "wb")
            frame.write(pgm_header)
            frame.write(frame_data)
            frame.close()

        p = Packet.from_file(f)

    f.close()

    # link old fisheye to fisheye_0
    if not drop_images and not 0 in drop_image_ids:
        os.symlink("fisheye_0", os.path.join(output_folder, "fisheye"));
        os.symlink("fisheye_0_timestamps.txt", os.path.join(output_folder, "fisheye_timestamps.txt"));

    if not drop_accel:
        accel_file.close()
    if not drop_gyro:
        gyro_file.close()
    if depth_frame_number:
        depth_file.close()

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='''
    Converts rc files to folders of images and imu txt files
    ''')

    parser.add_argument('capture_filename', help='')
    parser.add_argument('output_folder', help='')
    parser.add_argument('--drop_images',
                        help='', default=False, action="store_true")
    parser.add_argument('--drop_accel', help='', default=False, action="store_true")
    parser.add_argument('--drop_gyro', help='', default=False, action="store_true")
    parser.add_argument('--drop_image_ids', nargs='+', type=int, default=list())

    args = parser.parse_args()

    convert(args.capture_filename, args.output_folder, args.drop_images, args.drop_accel, args.drop_gyro, args.drop_image_ids)
