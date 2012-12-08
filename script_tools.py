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

class time_printer:
    last_time = 0.
    def print_time(self, packet):
        time = packet.header.time / 1000000.
        if time - self.last_time > 1.:
            self.last_time = time
            print time

