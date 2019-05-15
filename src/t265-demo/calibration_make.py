#############################################
# Handle RGB camera calibration parameters
# for csvrwv1.py
# v2 3 April 2019 Rowland Marshall - updated xmp_principal_point to latest value
#############################################

import io
import os
import sys
import json

def make_default_json():

    #in original csvrwv1.py
    #xmp_namespace_url          = u'http://pix4d.com/camera/1.0/'
    #xmp_model_type             = u"perspective"
    #xmp_principal_point        = u"2083.811, 1169.033"
    #xmp_perspective_distortion = u"0.1976732, -0.5061321, 0.3403559"

    #in csvrwv3.py
    #xmp_namespace_url          = u'http://pix4d.com/camera/1.0/'
    #xmp_model_type             = u"perspective"
    #xmp_principal_point        = u"12.9221, 7.24935"
    #xmp_perspective_focal      = u"18.5779"
    #xmp_perspective_distortion = u"0.1976732, -0.5061321, 0.3403559, 0, 0"
    #exif_focalplane_x_resolution = (16126,100)
    #exif_focalplane_y_resolution = (16126,100)
    #exif_focal_length            = (185779,10000)

    # Blackfly tamron
    #xmp_namespace_url           = u'http://pix4d.com/camera/1.0/'
    #xmp_model_type              = u"perspective"
    #xmp_principal_point         = u"6.6517344, 4.5154752"
    #xmp_perspective_focal       = u"7.9711776"
    #xmp_perspective_distortion  = u"-0.01549026,-0.03786489, 0.00995892, 0., 0."

    #exif_make                       = u"Intel InSense"
    #exif_model                      = u"InSense Blackfly S" #u"InSense Logitech"  
    #exif_focalplane_x_resolution    = (416666667,1000000)
    #exif_focalplane_y_resolution    = (416666667,1000000)
    #exif_focal_length               = (79711776,10000000)
    #exif_focalplane_resolution_unit = 4  #2 = inch  3 = cm  4 = mm

    # Blackfly Computar
    xmp_namespace_url           = u'http://pix4d.com/camera/1.0/'
    xmp_model_type              = u"perspective"
    xmp_principal_point         = u"6.5483904, 4.3386432"
    xmp_perspective_focal       = u"12.397925999999998"
    xmp_perspective_distortion  = u"-0.1002249, 0.1182915, 0.09997253, 0., 0."
    xmp_cam_gps_xy_accuracy     = u"0.2"
    xmp_cam_gps_z_accuracy      = u"0.2"
    xmp_cam_gyro_rate           = u"0.005"
    xmp_cam_imu_pitch_accuracy  = u"0.2"
    xmp_cam_imu_roll_accuracy   = u"0.2"
    xmp_cam_imu_yaw_accuracy    = u"0.2"

    exif_make                       = u"Intel InSense"
    exif_model                      = u"InSense Blackfly S" #u"InSense Logitech"  
    exif_focalplane_x_resolution    = (416666667,1000000)
    exif_focalplane_y_resolution    = (416666667,1000000)
    exif_focal_length               = (12397926,1000000)
    exif_focalplane_resolution_unit = 4  #2 = inch  3 = cm  4 = mm


    json_obj = { 
        "xmp" : {
            "namespace_url"          : xmp_namespace_url,
            "model_type"             : xmp_model_type,
            "principal_point"        : xmp_principal_point,
            "perspective_focal"      : xmp_perspective_focal,
            "perspective_distortion" : xmp_perspective_distortion,
            "gps_xy_accuracy"        : xmp_cam_gps_xy_accuracy,
            "gps_z_accuracy"         : xmp_cam_gps_z_accuracy,
            "gyro_rate"              : xmp_cam_gyro_rate,
            "imu_pitch_accuracy"     : xmp_cam_imu_pitch_accuracy,
            "imu_roll_accuracy"      : xmp_cam_imu_roll_accuracy,
            "imu_yaw_accuracy"       : xmp_cam_imu_yaw_accuracy,
        },
        "exif" : {
            "make"                       : exif_make,
            "model"                      : exif_model,
            "focalplane_resolution_unit" : exif_focalplane_resolution_unit,
            "focalplane_x_resolution"    : exif_focalplane_x_resolution,
            "focalplane_y_resolution"    : exif_focalplane_y_resolution,
            "focal_length"               : exif_focal_length,
        }
    }
    return json.dumps(json_obj, indent=4, separators=(", "," : "))

def read_json_calibration(path):
    filepath = os.path.join(path, "calibration.txt")
    try:
        f = open(filepath,"rt")
        json_str = f.read()
        f.close()
    except:
        print("calibration.txt not found, a default " + filepath + " will be created.")
        f = open(filepath,"wt")
        json_str = make_default_json()
        f.write(json_str)
        f.close()
    finally:
        return json.loads(json_str)

if __name__ == '__main__':
    if len(sys.argv) < 2: 
        path = ".\capture"
        try:
            os.mkdir(path)
        except:
            print(path + " exists.")
    else:
        path = sys.argv[1]
    read_json_calibration(path)
