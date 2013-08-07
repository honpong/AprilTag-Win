#!/opt/local/bin/python2.7
# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

import sys
import time
from numpy import *

sys.path.extend(['../'])
from corvis import cor, filter

if len(sys.argv) != 3:
    print "Usage:", sys.argv[0], "<data_filename> <configuration_name>"
    sys.exit(1)

cor.cvar.cor_time_pb_real = False

replay_file = sys.argv[1]
configuration_name = sys.argv[2]

capture = cor.mapbuffer()
capture.filename = replay_file
capture.replay = True
capture.size = 16 * 1024 * 1024
capture.dispatch = cor.dispatch_t()
capture.dispatch.reorder_depth = 20
capture.dispatch.max_latency = 25000
capture.dispatch.mb = capture;
cor.dispatch_init(capture.dispatch);
cor.plugins_register(cor.mapbuffer_open(capture))

outname = replay_file + "_solution"

dc = filter.corvis_device_parameters()

from util.device_parameters import set_device_parameters
set_device_parameters(dc, configuration_name) 

fc = filter.filter_setup(capture.dispatch, outname, dc)
fc.sfm.ignore_lateness = True

visbuf = cor.mapbuffer()
visbuf.size = 64 * 1024
visbuf.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(visbuf))

execfile("vis/vis_cfg.py")

#    capturepydispatch = cor.dispatch_t()
#capturepydispatch.mb = capture
# capturepydispatch.threaded = True
# cor.plugins_register(cor.dispatch_init(capturepydispatch))

cor.dispatch_addpython(visbuf.dispatch, myvis.frame_1.window_3.plot_dispatch);
cor.dispatch_addpython(visbuf.dispatch, myvis.frame_1.render_widget.packet_world);
cor.dispatch_addpython(visbuf.dispatch, featover.status_queue.put);
cor.dispatch_addpython(capture.dispatch, imageover.queue.put);

from util.script_tools import gravity_init
gi = gravity_init(fc.sfm)
cor.dispatch_addpython(capture.dispatch, gi.packet);

#  trackdatapydispatch = cor.dispatch_t()
#trackdatapydispatch.mb = trackdata
#trackdatapydispatch.threaded = True
#cor.plugins_register(cor.dispatch_init(trackdata.dispatch))
cor.dispatch_addpython(fc.trackdata.dispatch, featover.queue.put)
sys.path.extend(["renderable/", "renderable/.libs"])
import renderable
structure = renderable.structure()
motion = renderable.motion()
motion.color=[0.,0.,1.,1.]

measurement = renderable.measurement("/Library/Fonts/Tahoma.ttf", 12.)
measurement.color=[0.,1.,0.,1.]

filter_render = renderable.filter_state(fc.sfm)

myvis.frame_1.render_widget.add_renderable(structure.render, "Structure")
myvis.frame_1.render_widget.add_renderable(motion.render, "Motion")
myvis.frame_1.render_widget.add_renderable(measurement.render, "Measurement")
myvis.frame_1.render_widget.add_renderable(filter_render.render, "Filter state")
cor.dispatch_addclient(fc.solution.dispatch, structure, renderable.structure_packet)
cor.dispatch_addclient(fc.solution.dispatch, motion, renderable.motion_packet)
cor.dispatch_addclient(fc.solution.dispatch, measurement, renderable.measurement_packet)

fc.sfm.visbuf = visbuf


cor.cor_time_init()
cor.plugins_start()

myvis.app.MainLoop()()
