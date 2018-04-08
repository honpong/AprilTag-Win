#!/usr/bin/python

import sys
import struct
import json
import numpy as np
import read_eeprom
import zlib
import ctypes

if len(sys.argv) != 3:
    print "Usage:", sys.argv[0], "/path/to/input.json" "/path/to/output_eeprom.bin"
    sys.exit(1)

def atan2c(sin, cos, sin2):
    if sin2 < np.sqrt(40/3.0 * np.finfo(float).eps) and cos > 0:
        return 1.0 + (1.0/6.0)*sin2
    else:
        return np.arctan2(sin, cos) / sin

def sinc(x, x2):
    if x2 < np.sqrt(120 * np.finfo(float).eps): 
        return 1.0 - 1.0/6.0 * x2 
    else:
        return np.sin(x)/x;

def cosc(x,x2):
    if x2 < np.sqrt(720 * np.finfo(float).eps):
        return 1.0/2.0 - 1.0/24.0 * x2
    else:
        return (1.0 - np.cos(x)) / x2

def to_rotation_vector(quaternion_in):

    q = np.array(list(quaternion_in))
    if q[3] < 0:
        q *= -1  # return [0,pi] instead of [0,2pi]
    S = np.linalg.norm(q[0:3])
    S2 = S*S
    C = q[3]
    
    scale = 2 * atan2c(S,C,S2); # robust version of 2 asin(S)/S
    return (scale * q[0:3]).tolist(); # 2 log(q)

def to_rotation_matrix(rotation_vector_in):
    x, y, z = rotation_vector_in
    v = np.array(rotation_vector_in)
    th2 = np.dot(v,v)
    th = np.sqrt(th2)
    S, C = sinc(th, th2), cosc(th, th2)
    return np.array([
        [1 - C*(y*y+z*z), C*x*y - S*z, C*x*z + S*y],
        [C*y*x + S*z, 1 - C*(x*x+z*z), C*y*z - S*x],
        [C*z*x - S*y, C*z*y + S*x, 1 - C*(x*x+y*y)]
    ])

def to_quaternion(rotation_vector_in):
    w = np.array(rotation_vector_in)/2.0
    #f_t th2, th = sqrt(th2=w.norm2()), C = cos(th), Sc = sinc(th,th2);
    th = np.linalg.norm(w)
    th2 = th*th
    C = np.cos(th)
    Sc = sinc(th,th2)
    return [Sc * w[0], Sc * w[1], Sc * w[2], C]


eeprom_struct = read_eeprom.EepromTM2()


eeprom_struct.Header.Major = 3
eeprom_struct.Header.Minor = 0
eeprom_struct.Header.TableType = 9
eeprom_struct.Header.TotalSize = ctypes.sizeof(read_eeprom.EepromTM2) - ctypes.sizeof(read_eeprom.TableHeader)

eeprom_struct.Kb4.Header.Major = 1
eeprom_struct.Kb4.Header.Minor = 0
eeprom_struct.Kb4.Header.Size = ctypes.sizeof(read_eeprom.CalibrationModel) - ctypes.sizeof(read_eeprom.CalibrationModelHeader)
eeprom_struct.Kb4.Header.NumOfCameras = 2
eeprom_struct.Kb4.Header.NumOfImus = 1


#open json 
with open(sys.argv[1], 'rb') as file:
    jsondata = json.load(file)
    #file.readinto(eeprom_struct)

distortion_type_strings = ["unknown", "fisheye", "polynomial", "undistorted", "kannalabrandt4"]

#jsondata = dict()

#jsondata["calibration_version"] = 10
#jsondata["device_type"] = "TM2"
SN = jsondata["device_id"] + "0000"
eeprom_struct.Info.SN = type(eeprom_struct.Info.SN)(*[int(SN[i:i+2],16) for i in range(0, len(SN), 2)])

#jsondata["depths"] = list()

#jsondata["cameras"]=list()
eeprom_struct.Kb4.Header.NumOfCameras = len(jsondata["cameras"])
for i in xrange(len(jsondata["cameras"])):
    camera = eeprom_struct.Kb4.Cameras[i]
    #jsondata["cameras"].append(dict())

    camera.Intrinsics.width_px, camera.Intrinsics.height_px = jsondata["cameras"][i]["size_px"]        
    camera.Intrinsics.c_x_px,   camera.Intrinsics.c_y_px = jsondata["cameras"][i]["center_px"]      
    camera.Intrinsics.f_x_px,   camera.Intrinsics.f_y_px = jsondata["cameras"][i]["focal_length_px"]

    #jsondata["cameras"][i]["distortion"] = dict()
    camera.Intrinsics._8.distortion = type(camera.Intrinsics._8.distortion)(*jsondata["cameras"][i]["distortion"]["k"])
    camera.Intrinsics.type =  distortion_type_strings.index(jsondata["cameras"][i]["distortion"]["type"])


    camera.Extrinsics.pose_m.T.v = type(camera.Extrinsics.pose_m.T.v)(*jsondata["cameras"][i]["extrinsics"]["T"])
    camera.Extrinsics.pose_m.Q.v = type(camera.Extrinsics.pose_m.Q.v)(*to_quaternion(jsondata["cameras"][i]["extrinsics"]["W"]))
    for j in 0,1,2:
        camera.Extrinsics.pose_m.R.v[j] = type(camera.Extrinsics.pose_m.R.v[j])(*to_rotation_matrix(jsondata["cameras"][i]["extrinsics"]["W"])[j,:])

    camera.Extrinsics.variance_m2.T.v = type(camera.Extrinsics.variance_m2.T.v)(*jsondata["cameras"][i]["extrinsics"]["T_variance"])
    camera.Extrinsics.variance_m2.W.v = type(camera.Extrinsics.variance_m2.W.v)(*jsondata["cameras"][i]["extrinsics"]["W_variance"])



eeprom_struct.Kb4.Header.NumOfImus = len(jsondata["imus"])
for i in xrange(eeprom_struct.Kb4.Header.NumOfImus):
    imu = eeprom_struct.Kb4.Imu[i]

    imu.Extrinsics.pose_m.T.v = type(imu.Extrinsics.pose_m.T.v)(*jsondata["imus"][i]["extrinsics"]["T"])
    imu.Extrinsics.pose_m.Q.v = type(imu.Extrinsics.pose_m.Q.v)(*to_quaternion(jsondata["imus"][i]["extrinsics"]["W"]))
    for j in 0,1,2:
        imu.Extrinsics.pose_m.R.v[j] = type(imu.Extrinsics.pose_m.R.v[j])(*to_rotation_matrix(jsondata["imus"][i]["extrinsics"]["W"])[j,:])

    imu.Extrinsics.variance_m2.T.v = type(imu.Extrinsics.variance_m2.T.v)(*jsondata["imus"][i]["extrinsics"]["T_variance"])
    imu.Extrinsics.variance_m2.W.v = type(imu.Extrinsics.variance_m2.W.v)(*jsondata["imus"][i]["extrinsics"]["W_variance"])

    
    for j in xrange(3):
        imu.AccIntrinsics.scale_and_alignment.v[j] = type(imu.AccIntrinsics.scale_and_alignment.v[j])(*jsondata["imus"][i]["accelerometer"]["scale_and_alignment"][j*3:j*3+3])

    imu.AccIntrinsics.bias_m__s2.v = type(imu.AccIntrinsics.bias_m__s2.v)(*jsondata["imus"][i]["accelerometer"]["bias"])
    imu.AccIntrinsics.bias_variance_m2__s4.v = type(imu.AccIntrinsics.bias_variance_m2__s4.v)(*jsondata["imus"][i]["accelerometer"]["bias_variance"])
    imu.AccIntrinsics.measurement_variance_m2__s4 = jsondata["imus"][i]["accelerometer"]["noise_variance"]


    for j in xrange(3):
        imu.GyroIntrinsics.scale_and_alignment.v[j] = type(imu.GyroIntrinsics.scale_and_alignment.v[j])(*jsondata["imus"][i]["gyroscope"]["scale_and_alignment"][j*3:j*3+3])

    imu.GyroIntrinsics.bias_rad__s.v = type(imu.GyroIntrinsics.bias_rad__s.v)(*jsondata["imus"][i]["gyroscope"]["bias"])
    imu.GyroIntrinsics.bias_variance_rad2__s2.v = type(imu.GyroIntrinsics.bias_variance_rad2__s2.v)(*jsondata["imus"][i]["gyroscope"]["bias_variance"])
    imu.GyroIntrinsics.measurement_variance_rad2__s2 = jsondata["imus"][i]["gyroscope"]["noise_variance"]

#update crc32
eeprom_struct.Header.Crc32 = zlib.crc32(ctypes.string_at(ctypes.addressof(eeprom_struct.Info),eeprom_struct.Header.TotalSize) ) % (1<<32)

#write bin
#buf = (ctypes.c_char * ctypes.sizeof(eeprom_struct)).from_address(eeprom_struct)
with open(sys.argv[2], 'w') as outfile:
    outfile.write(ctypes.string_at(ctypes.addressof(eeprom_struct),ctypes.sizeof(eeprom_struct)))
    #json.dump(jsondata, outfile, indent=4, ensure_ascii=False)