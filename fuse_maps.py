# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

cor.cvar.cor_time_pb_real = False

replay_file=args[1]
other_file = args[2]
output_file = args[3]

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

other_data = cor.mapbuffer()
other_data.mem_writable = False
other_data.file_writable = False
other_data.filename = other_file
cor.plugins_register(cor.mapbuffer_open(other_data))

fused = cor.mapbuffer()
fused.mem_writable = True
fused.file_writable = True
fused.filename = output_file
fused.size = 10000 * MB
fused.ahead = 32*MB
fused.behind = 256*MB
fused.blocksize = 256*KB
cor.plugins_register(cor.mapbuffer_open(fused))

execfile("vis/vis_cfg.py")
sys.path.extend(["mapper/", "mapper/.libs"])
import mapper

mymap = mapper.mapper();
mymap.output_map = fused;
cor.dispatch_addclient(capturedispatch, mymap, mapper.packet_handler_cb);
myvis.frame_1.render_widget.renderables.append(mymap.render)

def open_map(name):
    data = cor.mapbuffer()
    data.mem_writable = False
    data.file_writable = False
    data.filename = name
    cor.plugins_register(cor.mapbuffer_open(data))
    return data

def do_next_map():
    mapper.run_new_map(mymap,other_data)
