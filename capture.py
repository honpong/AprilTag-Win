# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

import sys, os
import cor
from numpy import *

KB = 1024
MB = 1024*KB
GB = 1024*MB

cor.cvar.cor_time_pb_real = True
cor.cvar.capture_mode = True

sys.path.extend(["vis/", "vis/.libs"])
import vis
from vispy import MyVis

capture = cor.mapbuffer()

capture.filename = "log/test"
capture.size = 100*GB
capture.indexsize = 600000
capture.ahead = 256*MB
capture.behind = 32*MB
capture.blocksize = 256*KB
capture.mem_writable = True
capture.file_writable = True
capture.threaded = True
#capture.dispatch = cor.dispatch_t()
mbp = cor.mapbuffer_open(capture)
mbp.priority = 0
cor.plugins_register(mbp)

sys.path.extend(["drivers/"])
import camera
ladybug = camera.camera()
ladybug.name = "ladybug"
ladybug.guid = 0x00b09d01005bb89d
ladybug.pan=1
ladybug.shutter = 20;
ladybug.gain = 350;
ladybug.exposure = 0;
ladybug.brightness = 255;
ladybug.width = 1024;
ladybug.height = 768;
ladybug.color = True;
ladybug.bytes__pixel = 1;
ladybug.top = 0;
ladybug.left = 0;
ladybug.bytes_per_packet = 9000;
ladybug.sink = capture
camp = camera.init(ladybug)
camp.priority = 10
cor.plugins_register(camp)

import cmigits
imu = cmigits.c_migits()
sp = cor.serial_params()
sp.baud = 115200
sp.bits = 8
sp.stop = 0
sp.parity = 2
imu.device= "/dev/ttyS1" #"/dev/ttyFTCCGXCW"
imu.sp = sp
imu.output = capture
imup = cmigits.init(imu)
imup.priority = 20
cor.plugins_register(imup)

#myvis = MyVis()
#mvp = cor.plugins_initialize_python(myvis.start, myvis.stop)
#mvp.priority = 0
#cor.plugins_register(mvp)

#cor.dispatch_addclient(capture.dispatch, None, vis.show_filter_position_cb);
#cor.dispatch_addpython(capture.dispatch, myvis.frame_1.image_widget.packet_camera_cb);

#cor.dispatch_addclient(capture.dispatch, None, vis.show_filter_position_cb);
#cor.dispatch_addclient(capture.dispatch, None, vis.show_filter_reconstruction_cb);

#myvis.frame_1.render_widget.renderables.append(vis.vis_gl_redraw)

#id = image_dump()
#cor.dispatch_addpython(capture.dispatch, id.process_position);
#cor.dispatch_addpython(solution.dispatch, id.output_images);



