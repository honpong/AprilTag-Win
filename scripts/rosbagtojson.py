#!/usr/bin/python

from __future__ import print_function


def read_imu_accel(file):
    import rosbag
    data = []
    bag = rosbag.Bag(file)
    for topic, msg, t in bag.read_messages(topics=['/camera/accel/sample']):
        data.append({'time_us': t.to_nsec()/1000, 'type': 'accel', 'id': 0, 'value': [msg.linear_acceleration.x, msg.linear_acceleration.y, msg.linear_acceleration.z]})  # dummy id?
    bag.close()
    return data


def read_imu_gyro(file):
    import rosbag
    data = []
    bag = rosbag.Bag(file)
    for topic, msg, t in bag.read_messages(topics=['/camera/gyro/sample']):
        data.append({ 'time_us': t.to_nsec()/1000, 'type': 'gyro', 'id': 0, 'value': [msg.angular_velocity.x, msg.angular_velocity.y, msg.angular_velocity.z]})  # dummy id?
    bag.close()
    return data


def read_images(file):
    import rosbag
    data = []
    bag = rosbag.Bag(file)
    for topic, msg, t in bag.read_messages(topics='/camera/fisheye/image_raw'):
        data.append({'time_us': t.to_nsec()/1000, 'type': 'image', 'id': 0, 'values': msg.data, 'exposure_us': 0, 'width': msg.width, 'height': msg.height})
    bag.close()
    return data


accel_type = 20
gyro_type = 21
image_raw_type = 29

# defined in rc_tracker.h
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

        else:
            print("Unexpected data type", type)

        pbytes = len(data) + 16
        header_str = pack('IHHQ', pbytes, ptype, int(d['id']), int(d['time_us']))

        f.write(header_str)
        f.write(data)


def read_param_cam(file):
    import rosbag
    import numpy as np

    data = OrderedDict()  # {}
    bag = rosbag.Bag(file)

    topics = bag.read_messages('/camera/fisheye/camera_info')
    topic0 = next(topics)
    msg = topic0.message

    data["size_px"] = [ msg.width, msg.height ]
    K = np.array( msg.K )
    K = K.reshape(3,3)
    data["center_px"] = [ K[0,2], K[1,2] ]
    data["focal_length_px"] = [ K[0,0], K[1,1] ]
    #data["distortion"] = {'w':msg.D[0], 'type':'fisheye'}
    data["distortion"] = OrderedDict({'w':msg.D[0], 'type':'fisheye'})

    # todo: read from /camera/extrinsics/fisheye2imu
    data["extrinsics"] = OrderedDict()  # {}
    data["extrinsics"]["T"] = [0.0,0.0,0.0]
    data["extrinsics"]["T_variance"] = [1e-6,1e-6,1e-6]
    data["extrinsics"]["W"] = [0.0,0.0,0.0]
    data["extrinsics"]["W_variance"] = [1e-6,1e-6,1e-6]

    bag.close()
    return data


def read_param_depth(file):
    import rosbag

    data = OrderedDict()  # {}
    bag = rosbag.Bag(file)

    #topics = bag.read_messages() #'/camera/fisheye/camera_info')
    #topic0 = topics.next()
    #msg = topic0.message

    # dummy
    data["size_px"] = [ 0,0 ]
    data["center_px"] = [ 0.0,0.0 ]
    data["focal_length_px"] = [ 0.0,0.0 ]
    data["distortion"] = {'type':'undistorted'}
    data["extrinsics"] = OrderedDict()  # {}
    data["extrinsics"]["T"] = [0.0,0.0,0.0]
    data["extrinsics"]["T_variance"] = [1e-6,1e-6,1e-6]
    data["extrinsics"]["W"] = [0.0,0.0,0.0]
    data["extrinsics"]["W_variance"] = [1e-6,1e-6,1e-6]

    bag.close()
    return data


def read_param_imu( file, topicname ):
    import rosbag

    data = OrderedDict()  # {}
    bag = rosbag.Bag(file)
    topics = bag.read_messages(topics=topicname)
    topic0 = next(topics)
    msg = topic0.message

    data["scale_and_alignment"] = [1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0]  # dummy
    data["bias"] = [0.0,0.0,0.0]  # dummy
    data["bias_variance"] = msg.bias_variances
    data["noise_variance"] = msg.noise_variances[0]

    bag.close()
    return data


def read_extrinsics( file ):
    import rosbag

    data = OrderedDict()  # {}
    bag = rosbag.Bag(file)
    topics = bag.read_messages()  # todo: check where to find #topics=topicname)
    topic0 = next(topics)
    msg = topic0.message

    # todo: read from /camera/extrinsics/fisheye2imu
    data = OrderedDict()  # {}
    data["T"] = [0.0,0.0,0.0]
    data["T_variance"] = [1e-6,1e-6,1e-6]
    data["W"] = [0.0,0.0,0.0]
    data["W_variance"] = [1e-6,1e-6,1e-6]

    bag.close()
    return data


if __name__ == "__main__":
    import sys,os
    import json
    from collections import OrderedDict

    if len(sys.argv) == 1:
        print("rosbagtocapture <input_rosbag> <output_json>")
        sys.exit(1)

    in_file,out_file = sys.argv[1:]

    #data = []
    #data += read_imu_accel(os.path.join(in_file))
    #data += read_imu_gyro(os.path.join(in_file))
    #data += read_images(os.path.join(in_file))

    calib = OrderedDict()  # {}

    calib['device_id'] = ""
    calib['device_type'] = ""
    calib['calibration_version'] = 10

    calib['cameras'] = []
    calib['cameras'].append( read_param_cam( os.path.join(in_file) ) )

    calib['imus'] = []
    imus = OrderedDict()  # {}
    imus["accelerometer"] = read_param_imu( os.path.join(in_file), "/camera/accel/imu_info" )
    imus["gyroscope"] = read_param_imu( os.path.join(in_file), "/camera/gyro/imu_info" )
    imus["extrinsics"] = read_extrinsics( os.path.join(in_file) )
    calib['imus'].append( imus )  # why is this dictionary inside a list?

    #data.sort(key=lambda x: x['time_us'])

    #packets_write(data, out_file)

    with open(out_file, 'w') as outfile:
        json.dump(calib, outfile, indent=4)

    #print("parsing of rosbag \"{}\" finished.\noutput written to capture file \"{}\".".format(in_file, out_file))
