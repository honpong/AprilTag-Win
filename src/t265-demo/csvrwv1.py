##########
# csvrwv3.py 
# Rowland Marshall 1 Apr 2019
# importing data from a CSV and write it into the exif
# v3 make it sys callable, include xmp
##########

###### imports
import io
from PIL import Image # https://pillow.readthedocs.io/en/stable/
import piexif # https://piexif.readthedocs.io
import csv
import os
import sys
import calibration_make as calibration_io
from libxmp import XMPFiles, consts

###### DEFINITIONS

def singleFileEXIFWrite(src_dir, des_dir, dataToWrite):
    
    # define file sources and destinations
    src_image_path = os.path.join(src_dir, str(dataToWrite[0]))
    dst_image_path = os.path.join(des_dir , "tagged_" + str(dataToWrite[0]));
    
    #BLOCK TEMPORARILY
    cam = calibration_io.read_json_calibration(path=src_dir)
    xmp_namespace_url          = cam["xmp"]["namespace_url"]
    xmp_model_type             = cam["xmp"]["model_type"]
    xmp_perspective_focal      = cam["xmp"]["perspective_focal"]
    xmp_principal_point        = cam["xmp"]["principal_point"]
    xmp_perspective_distortion = cam["xmp"]["perspective_distortion"]

    # Write exif part
    # extracting a thumbnail to embed
    o = io.BytesIO()
    thumb_im = Image.open(src_image_path) #load image
    thumb_im.thumbnail((50, 50), Image.ANTIALIAS) #convert it into a thumbnail
    thumb_im.save(o, "jpeg") # save that thumbnail as a jpeg style
    thumbnail = o.getvalue() # convert that into bytes to attach to the exif
    
    zeroth_ifd = {piexif.ImageIFD.Make: u"Intel InSense", 
                piexif.ImageIFD.Model: u"InSense Logitech",
                }
    exif_ifd = {piexif.ExifIFD.DateTimeOriginal: u"2019:03:08 01:02:03",
                piexif.ExifIFD.FocalPlaneResolutionUnit: 4, #2 = inch  3 = cm  4 = mm
                piexif.ExifIFD.FocalPlaneXResolution: (16126,100),
                piexif.ExifIFD.FocalPlaneYResolution: (16126,100),
                piexif.ExifIFD.FocalLength: (185779,10000) # length, rational, in mm
                }
    gps_ifd = {piexif.GPSIFD.GPSVersionID: (2, 3, 0, 0),
                piexif.GPSIFD.GPSLongitudeRef: bytes(dataToWrite[6], 'utf-8'),
                piexif.GPSIFD.GPSLongitude: ((int(dataToWrite[7]), 1), (int(dataToWrite[8]), 1), (int(dataToWrite[9]), int(dataToWrite[10]))), # last number eg. 10000 is the divisor
                piexif.GPSIFD.GPSLatitudeRef: bytes(dataToWrite[1], 'utf-8'),
                piexif.GPSIFD.GPSLatitude: ((int(dataToWrite[2]), 1), (int(dataToWrite[3]), 1), (int(dataToWrite[4]), int(dataToWrite[5]))), # last number eg. 10000 is the divisor
                piexif.GPSIFD.GPSAltitudeRef: 0,
                piexif.GPSIFD.GPSAltitude: (int(dataToWrite[11]),int(dataToWrite[12])), # <Altitude in meters>, <divisor> i.e. 432,2 = 43.2
                piexif.GPSIFD.GPSDateStamp: u"1999:99:99 99:99:99",
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
    print("csvrwv1.py: writing " + dst_image_path + "... ", end="")
    im.save(dst_image_path, exif=exif_bytes)   #store the file into an output subdirectory.

    #write XMP part
    xmpfile = XMPFiles( file_path=dst_image_path, open_forupdate=True )
    xmp = xmpfile.get_xmp()
    
    xmp.register_namespace(xmp_namespace_url, u'Camera')
    xmp.set_property(xmp_namespace_url, u'ModelType', xmp_model_type )
    xmp.set_property(xmp_namespace_url, u'PrincipalPoint', xmp_principal_point )
    xmp.set_property(xmp_namespace_url, u'PerspectiveFocalLength', xmp_perspective_focal)
    xmp.set_property(xmp_namespace_url, u'PerspectiveDistortion', xmp_perspective_distortion )

    xmpfile.put_xmp(xmp)
    xmpfile.close_file()

    print(" xmp " + xmp_model_type + " pp:" + xmp_principal_point + " pdist:" + xmp_perspective_distortion)

##### PROGRAM

# function for writing one CSV row to the exif of a file. Note: Does not check to see if the file is valid
def csv_to_exif(src_dir, des_dir, Input_csv):
    # function for writing one CSV row to the exif of a file. Note: Does not check to see if the file is valid
    
    #create output directory
    try:
        os.mkdir(des_dir)
    except:
        print(des_dir + " exists.")
    
    # Open the CSV file
    with open(os.path.join(src_dir, Input_csv)) as csvfile:  # open the file for parsing
        readCSV = csv.reader(csvfile, delimiter=',') #parse file into a csv object
        singleImageData = [] #variable for holding a single row of data
        # For every row, store the row data and then run the EXIFWrite subroutine.
        for row in readCSV:
            singleImageData = row
            singleFileEXIFWrite(src_dir, des_dir, singleImageData)

csv_to_exif(sys.argv[1], sys.argv[2], 'outputllh.csv')