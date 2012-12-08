# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

cor.cvar.cor_time_pb_real = False

replay_file="slog/ucla_good_rectified_solution"

if replay_file is None:
    replay_file = "log/latest"

structure_data = cor.mapbuffer()
structure_data.filename = replay_file
cor.plugins_register(cor.mapbuffer_open(structure_data))

capturepydispatch = cor.dispatch_t()

execfile("vis/vis_cfg.py")
sys.path.extend(["renderable/", "renderable/.libs"])
import renderable

ucla_map = renderable.texture("slog/ucla_texture.pgm")
ucla_map.origin = array([-3072., 1024., 0.])
ucla_map.width = 4096.
ucla_map.height = 2048.

big_map = renderable.texture("slog/big_south.pgm")
big_map.origin = ucla_map.origin + array([-3428., -1332., 0.])
big_map.width = 7440.
big_map.height = 5771.

boelter_map = renderable.texture("slog/boelter_10pix_m.pgm")
boelter_map.origin = ucla_map.origin + array([3156., -977., 10.])
boelter_map.width = 114.4
boelter_map.height = 114.4

structure = renderable.structure(structure_data)
structure.theta = -75.5
structure.origin = ucla_map.origin + array([3863., -1782., 10.]);

big_sm_data = cor.mapbuffer()
big_sm_data.filename = "slog/big_sm_yay_rectified_solution"
cor.plugins_register(cor.mapbuffer_open(big_sm_data))

big_sm = renderable.structure(big_sm_data)
big_sm.theta = -.2
big_sm.color = array([1., 1., 1., 1.])
big_sm.origin = ucla_map.origin + array([2967., -1573., 10.])

boelter_data = cor.mapbuffer()
boelter_data.filename = "slog/boelter_double_rectified_solution"
cor.plugins_register(cor.mapbuffer_open(boelter_data))

boelter = renderable.structure(boelter_data)
boelter.theta = 149.
boelter.color = array([0., 1., 0., 1.])
boelter.origin = boelter_map.origin + array([19.5, -55.5, 0.])

myvis.frame_1.render_widget.renderables.append(big_map.render)
myvis.frame_1.render_widget.renderables.append(ucla_map.render)
myvis.frame_1.render_widget.renderables.append(boelter_map.render)
myvis.frame_1.render_widget.renderables.append(structure.render)
myvis.frame_1.render_widget.renderables.append(boelter.render)
myvis.frame_1.render_widget.renderables.append(big_sm.render)


