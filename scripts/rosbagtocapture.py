#!/usr/bin/python

from __future__ import print_function

import packet


def read_all(file,d_topics):
    data = []
    bag = rosbag.Bag(file)
    for topic, msg, t in bag.read_messages(topics=list(d_topics.values())):
        if topic == d_topics['image0']:
            data.append({'time_us': msg.header.stamp.to_nsec()/1e3, 'type': 'image', 'id': 0, 'values': msg.data, 'exposure_us': 0, 'width': msg.width, 'height': msg.height})  # fe0, no exposure time
        elif topic == d_topics['image1']:
            data.append({'time_us': msg.header.stamp.to_nsec()/1e3, 'type': 'image', 'id': 1, 'values': msg.data, 'exposure_us': 0, 'width': msg.width, 'height': msg.height})  # fe1, no exposure time
        elif topic == d_topics['accel']:
            data.append({'time_us': msg.header.stamp.to_nsec()/1e3, 'type': 'accel', 'id': 0, 'value': [msg.linear_acceleration.x, msg.linear_acceleration.y, msg.linear_acceleration.z]})  # dummy id
        elif topic == d_topics['gyro']:
            data.append({ 'time_us': msg.header.stamp.to_nsec()/1e3, 'type': 'gyro', 'id': 0, 'value': [msg.angular_velocity.x, msg.angular_velocity.y, msg.angular_velocity.z]})  # dummy id
        elif topic == d_topics['odom']:
            data.append({'time_us': msg.header.stamp.to_nsec()/1e3, 'type': 'odom', 'id': 0, 'value': [msg.twist.twist.linear.x, msg.twist.twist.linear.y, msg.twist.twist.linear.z, msg.twist.twist.angular.x, msg.twist.twist.linear.y, msg.twist.twist.linear.z]})  # dummy id
    bag.close()
    return data


accel_type = packet.PacketType.accelerometer
gyro_type = packet.PacketType.gyroscope
image_raw_type = packet.PacketType.image_raw
odom_type = packet.PacketType.velocimeter

# see rc_tracker.h
rc_IMAGE_GRAY8 = 0
rc_IMAGE_DEPTH16 = 1

def packets_write(data, output_filename):
    from struct import pack
    with open(output_filename, "wb") as f:
      for d in data:

        if d['type'] == 'image':
            ptype, image_type = image_raw_type, rc_IMAGE_GRAY8
            w = d['width']
            h = d['height']
            bytes = d['values']
            assert w*h == len(bytes), "image {}x{} has wrong number of bytes {}".format(w,h,len(bytes))
            stride = w
            data = pack('QHHHH', d['exposure_us'], w, h, stride, image_type) + bytes

        elif d['type'] == 'gyro':
            ptype = gyro_type
            data = pack('fff', *d['value'])

        elif d['type'] == 'accel':
            ptype = accel_type
            data = pack('fff', *d['value'])

        elif d['type'] == 'odom':
            ptype = odom_type
            data = pack('ffffff', *d['value'])

        else:
            print("Unexpected data type", type)

        pbytes = len(data) + 16
        header_str = pack('IHHQ', pbytes, ptype, int(d['id']), int(d['time_us']))

        f.write(header_str)
        f.write(data)


if __name__ == "__main__":
    import sys,os
    import rosbag
    import argparse
    parser = argparse.ArgumentParser(description='''
    Converts from rosbag to rc format.
    ''')
    parser.add_argument('in_file', help='input rosbag (.bag)')
    parser.add_argument('out_file', help='output capture file (.rc)')
    parser.add_argument('--t_accel', nargs='?', default='/imu/ACCLEROMETER/imu_raw/0', 
    	help='name of accel topic (default: /imu/ACCLEROMETER/imu_raw/0)')
    parser.add_argument('--t_gyro', nargs='?', default='/imu/GYROMETER/imu_raw/0', 
    	help='name of gyro topic (default: /imu/GYROMETER/imu_raw/0)')
    parser.add_argument('--t_image0', nargs='?', default='/camera/FISHEYE/image_raw/0', 
    	help='name of image0 topic (default: /camera/FISHEYE/image_raw/0)')
    parser.add_argument('--t_image1', nargs='?', default='/camera/FISHEYE2/image_raw/0', 
    	help='name of image1 topic (default: /camera/FISHEYE2/image_raw/0)')
    parser.add_argument('--t_odom', nargs='?', default='/odom', 
    	help='name of odometry topic (default: /odom)')
    args = parser.parse_args()
    
    d_topics = {'image0': args.t_image0, 'image1': args.t_image1, 'accel': args.t_accel, 'gyro': args.t_gyro, 'odom': args.t_odom}
    data = []
    data += read_all(args.in_file, d_topics)
    packets_write(data, args.out_file)
