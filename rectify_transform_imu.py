# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

cor.cvar.cor_time_pb_real = False

from numpy import *

replay_file = args[1];

capture = cor.mapbuffer()
calibdata = cor.mapbuffer()

capture.file_writable = False
capture.mem_writable = False
capture.filename = replay_file
capture.indexsize = 1000000
capture.ahead = 32*MB
capture.behind = 256*MB
capture.blocksize = 256*KB
cor.plugins_register(cor.mapbuffer_open(capture))

capturedispatch = cor.dispatch_t()
capturedispatch.mb = capture
capturedispatch.threaded = True
cor.plugins_register(cor.dispatch_init(capturedispatch))

calibdata.file_writable = True
calibdata.filename = replay_file + "_rectified"
calibdata.mem_writable = True
calibdata.size = 100000 * MB
calibdata.indexsize = 1000000
calibdata.ahead = 32*MB
calibdata.behind = 256*MB
calibdata.blocksize = 256*KB
calibdata.threaded = True
calibdata.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(calibdata))

def copy_to_output(packet):
    print packet.header.time, packet.header.type
    if packet.header.type > cor.packet_navsol:
        print "Got a bad packet! Forcing termination!"
        cor.plugins_stop()
        return
    if(packet.header.type == cor.packet_imu):
        newp = cor.mapbuffer_alloc(calibdata, cor.packet_imu, 6*4)
        newp.a[0] = packet.a[1]
        newp.a[1] = -packet.a[0]
        newp.a[2] = packet.a[2]
        newp.w[0] = packet.w[1]
        newp.w[1] = -packet.w[0]
        newp.w[2] = packet.w[2]
        cor.mapbuffer_enqueue(calibdata, newp, packet.header.time)
        return

    if(packet.header.type != cor.packet_camera):
        cor.mapbuffer_copy_packet(calibdata, packet)
        return

execfile("calibration/calibration_cfg.py")

cor.dispatch_addclient(capturedispatch, cal, calibration.calibration_rectify_cb)
cor.dispatch_addpython(capturedispatch, copy_to_output)
