#!/usr/bin/python

import sys
import struct
import json
import numpy as np
import read_eeprom

if len(sys.argv) != 3:
    print "Usage:", sys.argv[0], "/path/to/eeprom.bin" "/path/to/output.xml"
    sys.exit(1)

xml_file = open(sys.argv[2], "w")

def atan2c(sin, cos, sin2):
    if sin2 < np.sqrt(40/3.0 * np.finfo(float).eps) and cos > 0:
        return 1.0 + (1.0/6.0)*sin2
    else:
        return np.arctan2(sin, cos) / sin

def to_rotation_vector(quaternion_in):
    q = np.array(list(quaternion_in))
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

xml_file.write("<rig>\n")

for i in xrange(eeprom_struct.Kb4.Header.NumOfCameras):
    camera = eeprom_struct.Kb4.Cameras[i]
    xml_file.write("    <camera>\n")
    if distortion_type_strings[camera.Intrinsics.type] == 'kannalabrandt4':
        xml_file.write("        <camera_model name=\"\" index=\"%d\" serialno=\"0\" type=\"calibu_fu_fv_u0_v0_k\" version=\"0\">\n" %i)
    else:
        print ("Only kannalabrandt4 for now...")
        exit(1)
    xml_file.write("            <width> %u </width>\n" %(camera.Intrinsics.width_px))
    xml_file.write("            <height> %u </height>\n" %(camera.Intrinsics.height_px))
    xml_file.write("            <!-- Use RDF matrix, [right down forward], to define the coordinate frame convention -->\n")
    xml_file.write("            <right> [ 1; 0; 0 ] </right>\n")
    xml_file.write("            <down> [ 0; 1; 0 ] </down>\n")
    xml_file.write("            <forward> [ 0; 0; 1 ] </forward>\n")
    xml_file.write("            <!-- Camera parameters ordered as per type name. -->\n")
    xml_file.write("            <params> [  %.10lf;  %.10lf;  %.10lf;  %.10lf;  %.10lf,  %.10lf,  %.10lf,  %.10lf ] </params>\n" %
                   (camera.Intrinsics.f_x_px, camera.Intrinsics.f_y_px,
                    camera.Intrinsics.c_x_px, camera.Intrinsics.c_y_px,
                    camera.Intrinsics._8.distortion[0], camera.Intrinsics._8.distortion[1],
                    camera.Intrinsics._8.distortion[2], camera.Intrinsics._8.distortion[3]))
    # f_x_px, f_y_px, c_x_px, c_y_px, distortion[0], distortion[1], distortion[2], distortion[3]);

    xml_file.write("        </camera_model>\n")
    xml_file.write("        <pose>\n")
    xml_file.write("            <!-- Camera pose. World from Camera point transfer. 3x4 matrix, in the RDF frame convention defined above -->\n")
    xml_file.write("            <T_wc> [ %.10f, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f, %.10f ] </T_wc>\n" %
                   (camera.Extrinsics.pose_m.R.v[0][0], camera.Extrinsics.pose_m.R.v[0][1], camera.Extrinsics.pose_m.R.v[0][2], camera.Extrinsics.pose_m.T.v[0], \
                    camera.Extrinsics.pose_m.R.v[1][0], camera.Extrinsics.pose_m.R.v[1][1], camera.Extrinsics.pose_m.R.v[1][2], camera.Extrinsics.pose_m.T.v[1], \
                    camera.Extrinsics.pose_m.R.v[2][0], camera.Extrinsics.pose_m.R.v[2][1], camera.Extrinsics.pose_m.R.v[2][2], camera.Extrinsics.pose_m.T.v[2]))
    xml_file.write("        </pose>\n")
    xml_file.write("    </camera>\n")

xml_file.write("</rig>\n")
