# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

cor.cvar.cor_time_pb_real = False

replay_file=args[1]

structure_data = cor.mapbuffer()
structure_data.filename = replay_file
cor.plugins_register(cor.mapbuffer_open(structure_data))

execfile("vis/vis_cfg.py")
sys.path.extend(["renderable/", "renderable/.libs"])
import renderable

motion = renderable.motion(structure_data)
structure = renderable.structure(structure_data)
motion.color = [0., 0., 1., 1.]

myvis.frame_1.render_widget.renderables.append(motion.render)
myvis.frame_1.render_widget.renderables.append(structure.render)

