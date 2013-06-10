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

class progress_printer:
    last_time = 0
    last_percent = 0

    def __init__(self, dispatch):
        self.dispatch = dispatch

    def print_progress(self, packet):
        if self.dispatch.mb == None: return
        packet_time = packet.header.time / 1000000.
        progress_percent = 100 * self.dispatch.bytes_dispatched / self.dispatch.mb.total_bytes
        force_print = (progress_percent == 100 and self.last_percent != 100)

        if (packet_time - self.last_time > 1. and progress_percent -
          self.last_percent > 1) or force_print:
            self.last_time = packet_time
            self.last_percent = progress_percent
            print "%3.0f%% replayed" % progress_percent

class measurement_printer:
    last_time = 0
    def __init__(self, sfm):
        self.sfm = sfm

    def print_measurement(self, packet):
        if packet.header.type == cor.packet_filter_position:
            packet_time = packet.header.time / 1000000.
            if packet_time - self.last_time > 1.:
              self.last_time = packet_time
              print "Total path length (m): ", self.sfm.s.total_distance
              print "Straight line length (m): ", self.sfm.s.T.v

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
