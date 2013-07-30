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

if len(sys.argv) != 2:
    print "Usage:", sys.argv[0], "<data_foldername>"
    sys.exit(1)

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

myvis.frame_1.render_widget.add_renderable(structure.render, "Structure")
myvis.frame_1.render_widget.add_renderable(motion.render, "Motion")
myvis.frame_1.render_widget.add_renderable(measurement.render, "Measurement")

myvis.frame_1.render_widget.add_renderable(filter_render.render, "Filter state")
cor.dispatch_addclient(fc.solution.dispatch, structure, renderable.structure_packet)
cor.dispatch_addclient(fc.solution.dispatch, motion, renderable.motion_packet)
cor.dispatch_addclient(fc.solution.dispatch, measurement, renderable.measurement_packet)

from OpenGL.GL import *
class render_ground_truth:
    def __init__(self):
        self.positions = list()
        self.rotations = list()
        self.velocities = list()

    def receive_packet(self, packet):
        if packet.header.type == cor.packet_ground_truth:
           self.positions.append(packet.T[:])
           self.rotation = packet.rotation[:]

    def render(self):
        glBegin(GL_LINE_STRIP)
        glColor4f(1., 1., 1., 1)
        for p in self.positions:
            glVertex3f(p[0],p[1],p[2])
        glEnd()

        glPushMatrix()
        glPushAttrib(GL_ENABLE_BIT)

        glEnable(GL_LINE_STIPPLE)
        axis = self.rotation[:3] 
        angle = self.rotation[3] * 180. / pi
        glTranslatef(self.positions[-1][0], self.positions[-1][1], self.positions[-1][2]);
        glRotatef(angle, axis[0], axis[1], axis[2])
        glLineStipple(3, 0xAAAA)
        glBegin(GL_LINES)
        glColor3f(1, 0, 0)
        glVertex3f(0, 0, 0)
        glVertex3f(2, 0, 0)

        glColor3f(0, 1, 0)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 2, 0)
          
        glColor3f(0, 0, 1)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 0, 2)
        glEnd()

        glPopAttrib()
        glPopMatrix()


gt_render = render_ground_truth()
myvis.frame_1.render_widget.add_renderable(gt_render.render, "Simulation truth")
cor.dispatch_addpython(capture.dispatch, gt_render.receive_packet);


fc.sfm.visbuf = visbuf


sys.path.extend(["simulator/"])
sys.path.extend(["alternate_visualizations/pylib/"])

use_data = True
import simulator
if use_data:
    sim = simulator.data_simulator(sys.argv[1])
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

"""
from IPython.frontend.terminal.embed import InteractiveShellEmbed

from threading import Thread
def shell_function():
    ipshell = InteractiveShellEmbed()
    ipshell()

thread = Thread(target = shell_function)
thread.start()
cor.cor_time_pb_pause()
"""

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
