#!/usr/bin/python

#generates fisheye_timestamps.txt from a folder of images named with their timestamps, for converting vicalib input to folder format
import sys
import csv
from os import listdir
from os.path import isfile, split, join, relpath

if len(sys.argv) != 3:
    print "Usage:", sys.argv[0], "<folder of pgms> <output timestamp file>"
    sys.exit(1)

input_folder = sys.argv[1]
output_filename = sys.argv[2]
base, _ = split(output_filename)
relative = relpath(input_folder, base)

# list files in the directory
with open(output_filename, "wb") as csvfile:
    output_file = csv.writer(csvfile, delimiter=' ')

    files = [f for f in listdir(input_folder) if isfile(join(input_folder, f)) and f.endswith('pgm')]
    for filename in files:
        print filename
        timestamp_s = filename[:-4].split('_')[1]
        (whole_s, fractional_s) = timestamp_s.split('.')
        timestamp_ms = whole_s + fractional_s[:3] + "." + fractional_s[3:]
        output_file.writerow([join(relative, filename), timestamp_ms])

