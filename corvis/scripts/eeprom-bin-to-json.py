#!/usr/bin/python

import sys
import struct
import json
import numpy as np
import read_eeprom

if len(sys.argv) != 3:
    print "Usage:", sys.argv[0], "/path/to/eeprom.bin" "/path/to/output.json"
    sys.exit(1)

def atan2c(sin, cos, sin2):
    if sin2 < np.sqrt(40/3.0 * np.finfo(float).eps) and cos > 0:
        return 1.0 + (1.0/6.0)*sin2
    else:
        return np.arctan2(sin, cos) / sin

def to_rotation_vector(quaternion_in):

    q = np.array(list(quaternion_in))
    #print q
    if q[3] < 0:
        q *= -1  # return [0,pi] instead of [0,2pi]
    S = np.linalg.norm(q[0:3])
    S2 = S*S
    C = q[3]
    
    scale = 2 * atan2c(S,C,S2); # robust version of 2 asin(S)/S
    return (scale * q[0:3]).tolist(); # 2 log(q)

eeprom_struct = read_eeprom.EepromTM2()

with open(sys.argv[1], 'rb') as file:
    file.readinto(eeprom_struct)

distortion_type_strings = ["unknown", "fisheye", "polynomial", "undistorted", "kannalabrandt4"]

jsondata = dict()

jsondata["calibration_version"] = 10
jsondata["device_type"] = "TM2"
jsondata["device_id"] = '%x' % struct.unpack('Q', eeprom_struct.Info.SN)[0]

jsondata["depths"] = list()

jsondata["cameras"]=list()
for i in xrange(eeprom_struct.Kb4.Header.NumOfCameras):
    camera = eeprom_struct.Kb4.Cameras[i]
    jsondata["cameras"].append(dict())

    jsondata["cameras"][i]["size_px"]         = [camera.Intrinsics.width_px, camera.Intrinsics.height_px]
    jsondata["cameras"][i]["center_px"]       = [camera.Intrinsics.c_x_px,   camera.Intrinsics.c_y_px]
    jsondata["cameras"][i]["focal_length_px"] = [camera.Intrinsics.f_x_px,   camera.Intrinsics.f_y_px]

    jsondata["cameras"][i]["distortion"] = dict()
    jsondata["cameras"][i]["distortion"]["k"] = list(camera.Intrinsics._8.distortion)
    jsondata["cameras"][i]["distortion"]["type"] = distortion_type_strings[camera.Intrinsics.type]

    jsondata["cameras"][i]["extrinsics"] = dict()
    jsondata["cameras"][i]["extrinsics"]["T"] = list(camera.Extrinsics.pose_m.T.v)
    jsondata["cameras"][i]["extrinsics"]["W"] = to_rotation_vector(camera.Extrinsics.pose_m.Q.v)

    jsondata["cameras"][i]["extrinsics"]["T_variance"] = list(camera.Extrinsics.variance_m2.T.v)
    jsondata["cameras"][i]["extrinsics"]["W_variance"] = list(camera.Extrinsics.variance_m2.W.v)


jsondata["imus"]=list()
for i in xrange(eeprom_struct.Kb4.Header.NumOfImus):
    imu = eeprom_struct.Kb4.Imu[i]

    jsondata["imus"].append(dict())

    jsondata["imus"][i]["extrinsics"] = dict()
    jsondata["imus"][i]["extrinsics"]["T"] = list(imu.Extrinsics.pose_m.T.v)
    jsondata["imus"][i]["extrinsics"]["W"] = to_rotation_vector(imu.Extrinsics.pose_m.Q.v)
    jsondata["imus"][i]["extrinsics"]["T_variance"] = list(imu.Extrinsics.variance_m2.T.v)
    jsondata["imus"][i]["extrinsics"]["W_variance"] = list(imu.Extrinsics.variance_m2.W.v)

    jsondata["imus"][i]["accelerometer"] = dict()
    jsondata["imus"][i]["accelerometer"]["scale_and_alignment"] = [item for sublist in imu.AccIntrinsics.scale_and_alignment.v for item in sublist]
    jsondata["imus"][i]["accelerometer"]["bias"] = list(imu.AccIntrinsics.bias_m__s2.v)
    jsondata["imus"][i]["accelerometer"]["bias_variance"] = list(imu.AccIntrinsics.bias_variance_m2__s4.v)
    jsondata["imus"][i]["accelerometer"]["noise_variance"] = imu.AccIntrinsics.measurement_variance_m2__s4

    jsondata["imus"][i]["gyroscope"] = dict()
    jsondata["imus"][i]["gyroscope"]["scale_and_alignment"] = [item for sublist in imu.GyroIntrinsics.scale_and_alignment.v for item in sublist]
    jsondata["imus"][i]["gyroscope"]["bias"] = list(imu.GyroIntrinsics.bias_rad__s.v)
    jsondata["imus"][i]["gyroscope"]["bias_variance"] = list(imu.GyroIntrinsics.bias_variance_rad2__s2.v)
    jsondata["imus"][i]["gyroscope"]["noise_variance"] = imu.GyroIntrinsics.measurement_variance_rad2__s2


with open(sys.argv[2], 'w') as outfile:
    json.dump(jsondata, outfile, indent=4, ensure_ascii=False)