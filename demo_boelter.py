# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

cor.cvar.cor_time_pb_real = False

replay_file="log/boelter_new_calib_rectified_solution"

if replay_file is None:
    replay_file = "log/latest"

structure_data = cor.mapbuffer()
structure_data.filename = replay_file
cor.plugins_register(cor.mapbuffer_open(structure_data))

capturepydispatch = cor.dispatch_t()

execfile("vis/vis_cfg.py")
sys.path.extend(["renderable/", "renderable/.libs"])
import renderable

boelter_map = renderable.texture("log/boelter_10pix_m_black.pgm")
boelter_map.origin = array([-19.5, 55.5, 0.])
boelter_map.width = 114.4
boelter_map.height = 114.4
boelter_map.theta = pi

structure = renderable.structure(structure_data)
#structure.theta = -75.5
#structure.origin = ucla_map.origin + array([3863., -1782., 10.]);

myvis.frame_1.render_widget.renderables.append(boelter_map.render)
myvis.frame_1.render_widget.renderables.append(structure.render)


