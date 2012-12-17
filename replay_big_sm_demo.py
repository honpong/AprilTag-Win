# Copyright (c) 2008-2012, Eagle Jones
# Copyright (c) 2012, RealityCap, Inc.
# All rights reserved.
#
# This file is based on the corvis framework
# Please see LICENSE file for details

from numpy import *
import time

seconds = 9.
fps = 30.
steps = int(seconds*fps)

startx = -1081.
starty = 3252.
stopx = 1674.
stopy = 2841.

treex = 4953.
treey = 2184.

treedx = 227.
treedy = -1463.

def do_rotation(thetax, thetay, thetaz):
    dr = numerics.rodrigues([thetax / steps, thetay / steps, thetaz / steps, 0.], None)
    for i in xrange(steps):
        myvis.frame_1.render_widget.view_transform = dot(myvis.frame_1.render_widget.view_transform, dr)
        time.sleep(1./fps)

def restore_rotation(old):
    vn = myvis.frame_1.render_widget.view_transform
    old1 = eye(4)
    old1[:3,:3] = old[:3,:3]
    vn1 = eye(4)
    vn1[:3,:3] = vn[:3,:3]
    dr = numerics.rodrigues(numerics.invrodrigues(dot(vn1.T, old1), None) / steps, None)

    for i in xrange(steps):
        myvis.frame_1.render_widget.view_transform = dot(myvis.frame_1.render_widget.view_transform, dr)
        time.sleep(1./fps)
    myvis.frame_1.render_widget.view_transform = old


def fade_color(object, color):
    delta = (color - object.color[3]) / steps
    for i in xrange(steps):
        object.color[3] += delta
        time.sleep(1./fps)
    object.color[3] = color

def scale(start, stop, timestart, timestop, current):
    return start + (stop-start) * (current - timestart) / (timestop - timestart)

def control_playback(packet):
    if packet.header.time < 15000000:
        cor.cvar.cor_time_pb_scale = 2.
    elif packet.header.time < 1000000000:
        myvis.frame_1.render_widget.zoomfactor = scale(1.37, .25, 15000000., 1000000000., packet.header.time)
        myvis.frame_1.render_widget.view_transform[3,0] =  scale(startx, stopx, 15000000., 1000000000., packet.header.time)
        myvis.frame_1.render_widget.view_transform[3,1] =  scale(starty, stopy, 15000000., 1000000000., packet.header.time)
        cor.cvar.cor_time_pb_scale = 0.001

def animate():
    fade_color(map, 0.)
    #    motion.color[3] = 0.
    
    for i in range(100):
        myvis.frame_1.render_widget.view_transform[3,0] =  scale(stopx, treex, 1., 100., i)
        myvis.frame_1.render_widget.view_transform[3,1] =  scale(stopy, treey, 1., 100., i)
        time.sleep(1./fps)

    for i in range(100):
        myvis.frame_1.render_widget.zoomfactor = scale(.25, 2.55, 1., 100., i)
        time.sleep(1./fps)

        #fade_color(motion, 0.)

    vt = myvis.frame_1.render_widget.view_transform
    do_rotation(-pi / 2., 0., 0.)
    do_rotation(0., pi / 4., 0.)
    restore_rotation(vt)
    

    map.theta = -44.7
    map.origin = array([-2793., -1408., 0.])

    fade_color(map, 1.)
    

cor.cvar.cor_time_pb_real = True

replay_file=args[1]

structure_data = cor.mapbuffer()
structure_data.filename = replay_file

cor.plugins_register(cor.mapbuffer_open(structure_data))

capturedispatch = cor.dispatch_t()
capturedispatch.mb = structure_data
capturedispatch.threaded = True
cor.plugins_register(cor.dispatch_init(capturedispatch))

execfile("vis/vis_cfg.py")
sys.path.extend(["renderable/", "renderable/.libs"])
import renderable

structure = renderable.structure(None)
motion = renderable.motion(None)
cor.dispatch_addclient(capturedispatch, structure, renderable.structure_packet)
cor.dispatch_addclient(capturedispatch, motion, renderable.motion_packet)
cor.dispatch_addpython(capturedispatch, control_playback)
motion.color = [0., 0., 1., 1.]

map = renderable.texture("../log/big_south.pgm")
map.origin = array([-3428., -1332., 0.])
map.width = 7440.
map.height = 5771.
map.theta = -40.

structure.theta = .2 - 40.
motion.theta = .2 - 40.
structure.origin = array([2967., -1573., 10.])
motion.origin = array([2967., -1573., 10.])

myvis.frame_1.render_widget.view_transform[2,2] = -1.
myvis.frame_1.render_widget.view_transform[3,0] = startx
myvis.frame_1.render_widget.view_transform[3,1] = starty
myvis.frame_1.render_widget.zoomfactor = 1.37
myvis.frame_1.render_widget.renderables = list()
myvis.frame_1.render_widget.renderables.append(map.render)
myvis.frame_1.render_widget.renderables.append(motion.render)
myvis.frame_1.render_widget.renderables.append(structure.render)

