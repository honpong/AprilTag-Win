# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

class image_dump:
    def __init__(self):
        self.output_image = 0
        self.do_output = False

    def output_images(self, packet):
        if not self.do_output:
            return
        if packet.header.type == cor.packet_camera:
            path = replay_file + "_images/"
            if not os.path.exists(path):
                os.makedirs(path)
            outfn = path + "/" + str(self.output_image) + ".pgm"
            self.output_image += 1
            cor.packet_camera_write_image(packet,outfn)
            self.do_output = False

import cor
from numpy import *
class measurement_printer:
    last_point = None
    start_point = None
    straight = None
    path_length = 0.

    def print_measurement(self, packet):
        if packet.header.type == cor.packet_filter_position:
            current_point = array(packet.position)
            if self.start_point is None:
                self.start_point = current_point
                self.last_point = current_point
                self.path_length = 0
            self.path_length += sqrt(sum((self.last_point - current_point)**2))
            #print self.path_length
            self.last_point = current_point
            self.straight = (self.start_point - self.last_point)
            print "Total path length (m): ", self.path_length
            print "Straight line length (m): ", self.straight

class time_printer:
    last_time = 0.
    def print_time(self, packet):
        time = packet.header.time / 1000000.
        if time - self.last_time > 1.:
            self.last_time = time
            print time

import time
class time_printer_pause:
    last_time = 0.
    def print_time(self, packet):
        ptype = "Ignored"
        if packet.header.type == cor.packet_camera:
            ptype = "Camera"
        if packet.header.type == cor.packet_gyroscope:
            ptype = "Gyro " + str(packet.w[0]) + ", " + str(packet.w[1]) + ", " + str(packet.w[2])
        if packet.header.type == cor.packet_accelerometer:
            ptype = "Accel " + str(packet.a[0]) + ", " + str(packet.a[1]) + ", " + str(packet.a[2])
        thistime = packet.header.time / 1000000.
        print str(thistime) + ": " + ptype
        time.sleep(.25)
