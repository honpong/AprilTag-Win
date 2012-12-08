# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

cor.cvar.cor_time_pb_real = False

replay_file = args[1]

class image_dump:
    def __init__(self):
        self.output_index = 0
        self.output_image = 0
        print "init dumper"
        self.init = False
        
    def frame(self, packet):
        if not self.init:
            self.init = True
            self.starttime = packet.header.time
        if packet.header.type == cor.packet_camera:
            self.packet = packet
            self.dump()
        if packet.header.type == cor.packet_gyroscope:
            print str(packet.header.time - self.starttime) + ", " + str(packet.w[0]) + ", " + str(packet.w[1]) + ", " + str(packet.w[2])

    def dump(self):
        packet = self.packet
        if packet.header.type == cor.packet_camera:
            path = replay_file + ".images/" + str(self.output_index)
            if not os.path.exists(path):
                os.makedirs(path)
                #            outfn = path + "/" + "%05d" % self.output_image + ".pgm"
            outfn = path + "/" + "%05d" % (packet.header.time - self.starttime) + ".pgm"
            self.output_image += 1
            cor.packet_camera_write_image(packet,outfn)

capture = cor.mapbuffer()

capture.file_writable = False
capture.mem_writable = True
capture.filename = replay_file
capture.indexsize = 600000
capture.ahead = 32*MB
capture.behind = 256*MB
capture.blocksize = 256*KB
cor.plugins_register(cor.mapbuffer_open(capture))

capturedispatch = cor.dispatch_t()
capturedispatch.mb = capture
capturedispatch.threaded = True
capturedispatch.reorder_depth = 0
cor.plugins_register(cor.dispatch_init(capturedispatch))
#cor.dispatch_add_rewrite(capturedispatch, cor.packet_imu, 72000)

#execfile("vis/vis_cfg.py")
dumper = image_dump()
cor.dispatch_addpython(capturedispatch, dumper.frame)
