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

capture = cor.mapbuffer()

capture.filename = "test"
capture.size = 32 * 1024 * 1024
capture.replay = False
capture.block_when_full = True
capture.dispatch = cor.dispatch_t()
capture.dispatch.reorder_depth = 20
capture.dispatch.max_latency = 40000
# Enabling threading segfaults
# capture.dispatch->threaded = true;

cor.plugins_register(cor.mapbuffer_open(capture))
cor.plugins_register(cor.dispatch_init(capture.dispatch))

dc = filter.corvis_device_parameters()

from util.device_parameters import set_device_parameters
set_device_parameters(dc, "simulator") 

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

filter_render = renderable.filter_state(fc.sfm)

myvis.frame_1.render_widget.renderables.append(structure.render)
myvis.frame_1.render_widget.renderables.append(motion.render)
myvis.frame_1.render_widget.renderables.append(measurement.render)
myvis.frame_1.render_widget.renderables.append(filter_render.render)
cor.dispatch_addclient(fc.solution.dispatch, structure, renderable.structure_packet)
cor.dispatch_addclient(fc.solution.dispatch, motion, renderable.motion_packet)
cor.dispatch_addclient(fc.solution.dispatch, measurement, renderable.measurement_packet)


fc.sfm.visbuf = visbuf


sys.path.extend(["simulator/"])
sys.path.extend(["alternate_visualizations/pylib/"])

use_data = True
import simulator
if use_data:
    #sim = simulator.data_simulator('simulator/data/walking_L459')
    #sim = simulator.data_simulator('simulator/data/rotating_L0')
    #sim = simulator.data_simulator('simulator/data/static_L0')
    sim = simulator.data_simulator('simulator/data/sinx_L4')
    sim.imubuf = capture
else:
    sim = simulator.simulator()
    sim.trackbuf = capture
    sim.imubuf = capture

    Tcb = array([ 0.150,  -0.069,  0.481, 0.])
    Wcb = array([0.331, -2.372, 2.363, 0. ])
    sim.Tc = Tcb[:3] # + random.randn(3)*.1
    import rodrigues
    sim.Rc = rodrigues.rodrigues(Wcb[:3]) # + random.randn(3)*.05)

simp = cor.plugins_initialize_python(sim.run, None)
cor.plugins_register(simp)

fc.sfm.got_image = True
fc.sfm.active = True
fc.sfm.visbuf = visbuf
filter.filter_reset_position(fc.sfm)
a = array([sim.a[0], sim.a[1], sim.a[2], 0.])
gravity = array([sim.g[0], sim.g[1], sim.g[2], 0.])
w = array([sim.w[0], sim.w[1], sim.w[2], 0.])
w_bias = array([0., 0., 0., 0.])
filter.filter_set_initial_conditions(fc.sfm, a, gravity, w, w_bias, int(sim.time*1000000))

cor.cor_time_init()
cor.plugins_start()

import signal, sys, wx
def stop_and_exit():
    cor.plugins_stop()
    sys.exit(0)

def signal_handler(signal, frame):
    stop_and_exit()
signal.signal(signal.SIGINT, signal_handler)

def window_closed(event):
    stop_and_exit()

myvis.frame_1.Bind(wx.EVT_CLOSE, window_closed)

myvis.app.MainLoop()()
