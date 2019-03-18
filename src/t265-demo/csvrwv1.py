##########
# csvreadandwrite.py 
# Rowland Marshall 10 Mar 2019
# importing data from a CSV and write it into the exif
##########

###### imports
import io
from PIL import Image # https://pillow.readthedocs.io/en/stable/
import piexif # https://piexif.readthedocs.io
import csv

###### DEFINITIONS

def singleFileEXIFWrite(dataToWrite):
    # extracting a thumbnail to embed
    o = io.BytesIO()
    thumb_im = Image.open(str(dataToWrite[0])) #load image
    thumb_im.thumbnail((50, 50), Image.ANTIALIAS) #convert it into a thumbnail
    thumb_im.save(o, "jpeg") # save that thumbnail as a jpeg style
    thumbnail = o.getvalue() # convert that into bytes to attach to the exif

    zeroth_ifd = {piexif.ImageIFD.Make: u"Intel InSense", 
                piexif.ImageIFD.Model: u"TinyCam",
                }
    exif_ifd = {piexif.ExifIFD.DateTimeOriginal: u"2019:03:08 01:02:03",
                }
    gps_ifd = {piexif.GPSIFD.GPSVersionID: (2, 3, 0, 0),
                piexif.GPSIFD.GPSLongitudeRef: bytes(dataToWrite[6], 'utf-8'),
                piexif.GPSIFD.GPSLongitude: ((int(dataToWrite[7]), 1), (int(dataToWrite[8]), 1), (int(dataToWrite[9]), int(dataToWrite[10]))), # last number eg. 10000 is the divisor
                piexif.GPSIFD.GPSLatitudeRef: bytes(dataToWrite[1], 'utf-8'),
                piexif.GPSIFD.GPSLatitude: ((int(dataToWrite[2]), 1), (int(dataToWrite[3]), 1), (int(dataToWrite[4]), int(dataToWrite[5]))), # last number eg. 10000 is the divisor
                piexif.GPSIFD.GPSAltitudeRef: 0,
                piexif.GPSIFD.GPSAltitude: (int(dataToWrite[11]),int(dataToWrite[12])), # <Altitude in meters>, <divisor> i.e. 432,2 = 43.2
                #piexif.GPSIFD.GPSDateStamp: u"1999:99:99 99:99:99",
                }
    first_ifd = {#piexif.ImageIFD.Make: u"Canon",
                #piexif.ImageIFD.XResolution: (40, 1),
                #piexif.ImageIFD.YResolution: (40, 1),
                #piexif.ImageIFD.Software: u"piexif"
                }

    exif_dict = {"0th":zeroth_ifd, "Exif":exif_ifd, "GPS":gps_ifd, "1st":first_ifd, "thumbnail":thumbnail}
    exif_bytes = piexif.dump(exif_dict)
    im = Image.open(str(dataToWrite[0]))
    # im.thumbnail((100, 100), Image.ANTIALIAS) # blocked out just in case it's useful later.  This tries to resize the image. 
    im.save("output/" + str(dataToWrite[0]) + "-out.jpg", exif=exif_bytes)   #store the file into an output subdirectory.

def csv_to_exif(Input_csv):
    # function for writing one CSV row to the exif of a file. Note: Does not check to see if the file is valid
    # Open the CSV file
    with open(Input_csv) as csvfile:  # open the file for parsing
        readCSV = csv.reader(csvfile, delimiter=',') #parse file into a csv object
        singleImageData = [] #variable for holding a single row of data
        # For every row, store the row data and then run the EXIFWrite subroutine.
        for row in readCSV: 
            singleImageData = row
            print (singleImageData[0])
            singleFileEXIFWrite(singleImageData)

csv_to_exif('outputllh.csv')