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

cor.cvar.cor_time_pb_real = False

KB = 1024
MB = 1024*KB
GB = 1024*MB

capture = cor.mapbuffer()

capture.filename = "test"
capture.size = 1*GB
capture.indexsize = 600000
capture.ahead = 256*MB
capture.behind = 32*MB
capture.blocksize = 256*KB
capture.mem_writable = True
capture.file_writable = True
capture.threaded = True
capture.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(capture))

dc = filter.corvis_device_parameters()

from util.device_parameters import set_device_parameters
set_device_parameters(dc, "simulator") 

#fc = filter.filter_setup(dc)
outname = "simulator_solution"
fc = filter.filter_setup(capture.dispatch, outname, dc)
fc.sfm.ignore_lateness = True

visbuf = cor.mapbuffer()
visbuf.size = 64 * 1024
visbuf.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(visbuf))

execfile("vis/vis_cfg.py")

cor.dispatch_addpython(visbuf.dispatch, myvis.frame_1.window_3.plot_dispatch);
cor.dispatch_addpython(visbuf.dispatch, myvis.frame_1.render_widget.packet_world);
cor.dispatch_addpython(visbuf.dispatch, featover.status_queue.put);

sys.path.extend(["renderable/", "renderable/.libs"])
import renderable
structure = renderable.structure()
motion = renderable.motion()
motion.color=[0.,0.,1.,1.]

measurement = renderable.measurement("/Library/Fonts/Tahoma.ttf", 12.)
measurement.color=[0.,1.,0.,1.]
# _databuffer->block_when_full = true;

filter_render = renderable.filter_state(fc.sfm)

myvis.frame_1.render_widget.renderables.append(structure.render)
myvis.frame_1.render_widget.renderables.append(motion.render)
myvis.frame_1.render_widget.renderables.append(measurement.render)
myvis.frame_1.render_widget.renderables.append(filter_render.render)
cor.dispatch_addclient(visbuf.dispatch, structure, renderable.structure_packet)
cor.dispatch_addclient(visbuf.dispatch, motion, renderable.motion_packet)
cor.dispatch_addclient(visbuf.dispatch, measurement, renderable.measurement_packet)

fc.sfm.visbuf = visbuf


sys.path.extend(["simulator/"])
sys.path.extend(["alternate_visualizations/pylib/"])

Tcb = array([ 0.150,  -0.069,  0.481, 0.])
Wcb = array([0.331, -2.372, 2.363, 0. ])

import simulator
sim = simulator.simulator()
sim.trackbuf = capture
sim.imubuf = capture
sim.Tc = Tcb[:3] # + random.randn(3)*.1
import rodrigues
sim.Rc = rodrigues.rodrigues(Wcb[:3]) # + random.randn(3)*.05)
simp = cor.plugins_initialize_python(sim.run, None)
cor.plugins_register(simp)

fc.sfm.got_image = True
fc.sfm.active = True
filter.filter_reset_position(fc.sfm)
gravity = array([sim.g[0], sim.g[1], sim.g[2], 0.])
filter.filter_gravity_init(fc.sfm, gravity, 0)

cor.cor_time_init()
cor.plugins_start()


myvis.app.MainLoop()()
