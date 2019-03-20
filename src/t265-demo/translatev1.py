##########
# translateV1.py 
# Rowland Marshall 13 Mar 2019
# importing data from a CSV, translating it from local to GPS, and writing to another CSV
##########

###### IMPORTS
# import io
# from PIL import Image # https://pillow.readthedocs.io/en/stable/
# import piexif # https://piexif.readthedocs.io
import csv
import math
import os

###### DEFINITIONS
# convert an x,y,z coord from the InSense orientation into lat long height
def conv_xyz_llh(x, y, z):
    ###### PROG

    ###### CONSTANTS
    # Fixed GPS constants start point to define where we should set 0,0,0 for the InSense data
    GPS_H0 = 331.67 # Height it is 330.67 but lift it up off the ground by 1m)
    GPS_LAT0 = 33.30816051 #Latitude in degrees, +tive is North
    GPS_LON0 = -111.93072923 #Longitude in degrees, +tive is East

    # Geo constants
    APPROX_RAD_GPS_0 = 6371665.588 #Earth approx radius at GPS_0 location

    ####### VARIABLES
    InSense_x = x
    InSense_y = y
    InSense_z = z

    #
    ##### InSense camera reference system
    # z_+tive points backwards from the center of the camera, perpendicular to the lense surface
    # y_+tive points upwards from the center of the camera
    # x_+tive points to the left when viewing the z_+tive axis from the origin
    # For now, let's assume that y_+tive points away from the earth parallel to gravity, z_+tive points South, and x_+tive points East 
    #
    ### Filetype syntax for the coordinates
    # Original Camera Files
    #  `[filename, tx, ty, tz, omega, phi, kappa, m00, m01, m02, tx, m10, m11, m12, ty, m20, m21, m22, tz]`
    # where
    # • `m` is 3x3 rotation matrix,
    # • `[tx,ty,tz]` is translation.
    # • `[omega,phi,kappa]` was my attempt to convert from rotation-translation to axis-rotation angle using the formula on a Pix4D reference document. But there are 2 formulas I was not sure which one is correct so I kept my original rotation-translation data.
    #
    # Revised camera files
    # the latest pose.txt will have
    # `[filename, tx, ty, tz, rw, rx, ry, rz]` where
    # • `[tx, ty, tz]` is translation (same as before).
    # • `[rw, rx, ry, rz]` is a 4x1 quaternion of rotation.
    # These values are coming strict from librealsense API without modification.
    #####

    # test out the conversion
    InSense_h = GPS_H0 + InSense_y
    InSense_lat = math.asin(((-InSense_z) / 2) / (InSense_h + APPROX_RAD_GPS_0)) * 180 / math.pi + GPS_LAT0
    InSense_lon = math.asin((InSense_x / 2) / (InSense_h + APPROX_RAD_GPS_0)) * 180 / math.pi + GPS_LON0

    # debug prints
    # print(InSense_h)
    # print(InSense_lat)
    # print(InSense_lon)

    #break up latitude into chunks
    if InSense_lat > 0: Lat_a = "N" 
    else: Lat_a = "S"
    Lat_b = math.floor(abs(InSense_lat))
    Lat_c = math.floor((abs(InSense_lat) % 1) * 60)
    Lat_e = 10000
    Lat_d = math.floor((((abs(InSense_lat) % 1) *60) % 1) * 60 * Lat_e)

    #break up longitude into chunks
    if InSense_lon > 0: Lon_a = "E" 
    else: Lon_a = "W"
    Lon_b = math.floor(abs(InSense_lon))
    Lon_c = math.floor((abs(InSense_lon) % 1) * 60)
    Lon_e = 10000
    Lon_d = math.floor((((abs(InSense_lon) % 1) *60) % 1) * 60 * Lat_e)

    # break up altitude into chunks
    Hei_b = 1000
    Hei_a = math.floor(abs(InSense_h)* Hei_b)

    row = [Lat_a, Lat_b, Lat_c, Lat_d, Lat_e, Lon_a, Lon_b, Lon_c, Lon_d, Lon_e, Hei_a, Hei_b]
    return row

# Open a source CSV file with the format [filename, tx, ty, tz, rw, rx, ry, rz] and convert it to the exif format in lat long height
def conv_xyzcsv_llhcsv(Input_csv):
    # Open the CSV file
    with open(Input_csv) as csv_read_file:  # open the file for parsing
        with open('outputllh.csv', 'w', newline='') as csv_write_file: # open file to write to
            readCSV = csv.reader(csv_read_file, delimiter=',') #parse file into a csv object
            writeCSV = csv.writer(csv_write_file, delimiter=',') #set up to write
            singleRow = [] #variable for holding a single row of data
            # For every row, store the row data and then run the EXIFWrite subroutine.
            firstline = True
            for row in readCSV: 
                if firstline:
                    firstline = False
                    continue
                singleRow = row
                print ("translatev1.py: reading " + singleRow[0])
                writeCSV.writerow([singleRow[0]] + conv_xyz_llh(float(singleRow[1]), float(singleRow[2]), float(singleRow[3])))

#test call
conv_xyzcsv_llhcsv(os.path.join(sys.argv[1], 'pose.txt'))


# #test call
# print(conv_xyz_llh(-0.0000111000, -0.0003651590, -0.0010720200))
# print(conv_xyz_llh(-0.0004047300, -0.0003997430, -0.0010691900))
# print(conv_xyz_llh(-0.0016432700, -0.0003907820, 0.0000840000))
# print(conv_xyz_llh(-0.0035835100, -0.0000142000, 0.0009191420))
# print(conv_xyz_llh(-0.0063624500, 0.0027325200, 0.0024578800))