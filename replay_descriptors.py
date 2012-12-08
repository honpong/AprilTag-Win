# Created by Eagle Jones
# Copyright (c) 2012. RealityCap, Inc.
# All Rights Reserved.

from numpy import *

cor.cvar.cor_time_pb_real = False

replay_file=args[1]

if replay_file is None:
    replay_file = "log/latest"

#capture = cor.mapbuffer()

#capture.file_writable = False
#capture.mem_writable = True
#capture.filename = replay_file
#capture.indexsize = 1000000
#capture.ahead = 32*MB
#capture.behind = 256*MB
#capture.blocksize = 256*KB
#cor.plugins_register(cor.mapbuffer_open(capture))

#capturepydispatch = cor.dispatch_t()
#capturepydispatch.mb = capture
#capturepydispatch.threaded = True
#capturepydispatch.reorder_depth = 10
#cor.plugins_register(cor.dispatch_init(capturepydispatch))
#cor.dispatch_add_rewrite(capturepydispatch, cor.packet_imu, 72000)

descriptor_data = cor.mapbuffer()
descriptor_data.mem_writable = False
descriptor_data.file_writable = False
descriptor_data.filename = replay_file
cor.plugins_register(cor.mapbuffer_open(descriptor_data))

capturedispatch = cor.dispatch_t()
capturedispatch.mb = descriptor_data
capturedispatch.threaded = True
cor.plugins_register(cor.dispatch_init(capturedispatch))
capturepydispatch = capturedispatch

execfile("vis/vis_cfg.py")
sys.path.extend(["mapper/", "mapper/.libs"])
import mapper

mymap = mapper.mapper();
mymap.no_search = False
cor.dispatch_addclient(capturedispatch, mymap, mapper.packet_handler_cb);
myvis.frame_1.render_widget.renderables.append(mymap.render)
