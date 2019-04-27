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
    xmp_namespace_url          = u'http://pix4d.com/camera/1.0/'
    xmp_model_type             = u"perspective"
    xmp_principal_point        = u"12.9221,7.24935"
    xmp_perspective_focal      = u"18.5779"
    xmp_perspective_distortion = u"0.1976732, -0.5061321, 0.3403559, 0, 0"

    #new for exif
    exif_make    = u"Intel InSense"
    exif_model   = u"InSense Blackfly S" #u"InSense Logitech"  
    
    json_obj = { 
        "xmp" : {
            "namespace_url"   : xmp_namespace_url,
            "model_type"      : xmp_model_type,
            "principal_point" : xmp_principal_point,
            "perspective_focal"      : xmp_perspective_focal,
            "perspective_distortion" : xmp_perspective_distortion,
        },
        "exif" : {
            "make"  :  exif_make,
            "model" :  exif_model,
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
