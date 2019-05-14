##########
# csvrwv1.py 
# Rowland Marshall 3 Apr 2019
# importing data from a CSV and write it into the exif
# v4 corrected the principle point
# v3 make it sys callable, include xmp
##########

###### imports
import io
from PIL import Image # https://pillow.readthedocs.io/en/stable/
import piexif # https://piexif.readthedocs.io
import csv
import os
import sys
import shutil
import calibration_make as calibration_io
from libxmp import XMPFiles, consts

###### DEFINITIONS

def singleFileEXIFWrite(src_dir, des_dir_name, dataToWrite):
    
    # define file sources and destinations
    src_image_path = os.path.join(src_dir, str(dataToWrite[0]))
    dst_image_path = os.path.join(des_dir_name, "tagged_" + str(dataToWrite[0]))
    src_geojson_path = os.path.join(src_dir, str(dataToWrite[0]).split(".")[0] + ".geojson")
    dst_geojson_path = os.path.join(des_dir_name, str(dataToWrite[0]).split(".")[0] + ".geojson")

    geojson_exists = os.path.isfile(src_geojson_path)
    if geojson_exists:
        try:
            shutil.copy2(src_geojson_path, dst_geojson_path)
            print("Annotation copied: " + dst_geojson_path)
        except FileNotFoundError: 
            pass


    #extract date time
    str_cap_datestamp = str(dataToWrite[0]).split(".")[0].split("_")
    cap_year   = str_cap_datestamp[1]
    cap_month  = str_cap_datestamp[2]
    cap_day    = str_cap_datestamp[3]
    cap_hour   = str_cap_datestamp[4]
    cap_min    = str_cap_datestamp[5]
    cap_second = str_cap_datestamp[6]
    
    # Set up the calibration 
    cam = calibration_io.read_json_calibration(path=src_dir)
    xmp_namespace_url          = cam["xmp"]["namespace_url"]
    xmp_model_type             = cam["xmp"]["model_type"]
    xmp_perspective_focal      = cam["xmp"]["perspective_focal"]
    xmp_principal_point        = cam["xmp"]["principal_point"]
    xmp_perspective_distortion = cam["xmp"]["perspective_distortion"]

    exif_make                       = cam["exif"]["make"]
    exif_model                      = cam["exif"]["model"]
    exif_focalplane_x_resolution    = cam["exif"]["focalplane_x_resolution"]
    exif_focalplane_y_resolution    = cam["exif"]["focalplane_y_resolution"]
    exif_focal_length               = cam["exif"]["focal_length"]
    exif_focalplane_resolution_unit = cam["exif"]["focalplane_resolution_unit"]

    exif_date_time_original = cap_year + ":" + cap_month + ":" + cap_day + " " + cap_hour + ":" + cap_min + ":" + cap_second

    # Write exif part
    # extracting a thumbnail to embed
    o = io.BytesIO()
    thumb_im = Image.open(src_image_path) #load image
    thumb_im.thumbnail((50, 50), Image.ANTIALIAS) #convert it into a thumbnail
    thumb_im.save(o, "jpeg") # save that thumbnail as a jpeg style
    thumbnail = o.getvalue() # convert that into bytes to attach to the exif
    
    zeroth_ifd = {piexif.ImageIFD.Make: exif_make, #u"Intel InSense", 
                  piexif.ImageIFD.Model: exif_model, #u"InSense Logitech",
                }
    exif_ifd = {piexif.ExifIFD.DateTimeOriginal: exif_date_time_original, #u"2019:05:08 01:02:03",
                piexif.ExifIFD.FocalPlaneResolutionUnit: exif_focalplane_resolution_unit, #2 = inch  3 = cm  4 = mm
                piexif.ExifIFD.FocalPlaneXResolution: exif_focalplane_x_resolution,
                piexif.ExifIFD.FocalPlaneYResolution: exif_focalplane_y_resolution,
                piexif.ExifIFD.FocalLength: exif_focal_length # length, rational, in mm
                }
    gps_ifd = {piexif.GPSIFD.GPSVersionID: (2, 3, 0, 0),
                piexif.GPSIFD.GPSLongitudeRef: bytes(dataToWrite[6], 'utf-8'),
                piexif.GPSIFD.GPSLongitude: ((int(dataToWrite[7]), 1), (int(dataToWrite[8]), 1), (int(dataToWrite[9]), int(dataToWrite[10]))), # last number eg. 10000 is the divisor
                piexif.GPSIFD.GPSLatitudeRef: bytes(dataToWrite[1], 'utf-8'),
                piexif.GPSIFD.GPSLatitude: ((int(dataToWrite[2]), 1), (int(dataToWrite[3]), 1), (int(dataToWrite[4]), int(dataToWrite[5]))), # last number eg. 10000 is the divisor
                piexif.GPSIFD.GPSAltitudeRef: 0,
                piexif.GPSIFD.GPSAltitude: (int(dataToWrite[11]),int(dataToWrite[12])), # <Altitude in meters>, <divisor> i.e. 432,2 = 43.2
                piexif.GPSIFD.GPSDateStamp: exif_date_time_original,  # u"2019:04:27 01:02:03",
                piexif.GPSIFD.GPSMapDatum: u"WGS-84",
                }
    first_ifd = {#piexif.ImageIFD.Make: u"Canon",
                #piexif.ImageIFD.XResolution: (40, 1),
                #piexif.ImageIFD.YResolution: (40, 1),
                #piexif.ImageIFD.Software: u"piexif"
                }
    exif_dict = {"0th":zeroth_ifd, 
                "Exif":exif_ifd, 
                "GPS":gps_ifd, 
                "1st":first_ifd, 
                "thumbnail":thumbnail}
    exif_bytes = piexif.dump(exif_dict)
    im = Image.open(src_image_path)
    # im.thumbnail((100, 100), Image.ANTIALIAS) # blocked out just in case it's useful later.  This tries to resize the image. 
    print("csvrwv1.py: writing " + dst_image_path + "... ")
    im.save(dst_image_path, quality=100, exif=exif_bytes)   #store the file into an output subdirectory.
        # QUALITY: The image quality, on a scale from 1 (worst) to 95 (best). The default is 75. Values above 95 should be avoided; 100 disables portions of the JPEG compression algorithm, and results in large files with hardly any gain in image quality.

    #write XMP part
    xmpfile = XMPFiles( file_path=dst_image_path, open_forupdate=True )
    xmp = xmpfile.get_xmp()
    
    # Hard coded camera parameters (if not using external reference file)
    # xmp_namespace_url          = u'http://pix4d.com/camera/1.0/'
    # xmp_model_type             = u"perspective"
    # xmp_principal_point        = u"12.9221,7.24935"
    # xmp_perspective_focal      = u'18.5779'
    # xmp_perspective_distortion = u"0.1976732, -0.5061321, 0.3403559, 0, 0"

    xmp.register_namespace(xmp_namespace_url, u'Camera')
    xmp.set_property(xmp_namespace_url, u'ModelType', xmp_model_type )
    xmp.set_property(xmp_namespace_url, u'PrincipalPoint', xmp_principal_point )
    xmp.set_property(xmp_namespace_url, u'PerspectiveFocalLength', xmp_perspective_focal)
    xmp.set_property(xmp_namespace_url, u'PerspectiveDistortion', xmp_perspective_distortion )

    xmpfile.put_xmp(xmp)
    xmpfile.close_file()

    print(" DateTimeOriginal :" + exif_date_time_original )
    print(" xmp " + xmp_model_type + " pp:" + xmp_principal_point + " pf:" + xmp_perspective_focal + " pdist:" + xmp_perspective_distortion)

##### PROGRAM

# function for writing one CSV row to the exif of a file. Note: Does not check to see if the file is valid
def csv_to_exif(src_dir, des_dir_name, Input_csv):
    # function for writing one CSV row to the exif of a file. Note: Does not check to see if the file is valid
    
    #create output directory
    try:
        os.mkdir(des_dir_name)
    except:
        print(des_dir_name + " exists.")
    
    # Open the CSV file
    with open(os.path.join(src_dir, Input_csv)) as csvfile:  # open the file for parsing
        readCSV = csv.reader(csvfile, delimiter=',') #parse file into a csv object
        singleImageData = [] #variable for holding a single row of data
        # For every row, store the row data and then run the EXIFWrite subroutine.
        for row in readCSV:
            singleImageData = row
            singleFileEXIFWrite(src_dir, des_dir_name, singleImageData)

csv_to_exif(sys.argv[1], sys.argv[2], 'outputllh.csv')