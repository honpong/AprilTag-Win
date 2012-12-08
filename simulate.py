# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

cor.cvar.cor_time_pb_real = False
cor.cvar.capture_mode = False

capture = cor.mapbuffer()
capture.mem_writable = True
capture.size = 100*MB
capture.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(capture))

capturedispatch = capture.dispatch
calibdata = cor.mapbuffer()
calibdata.mem_writable = True
calibdata.size = 100*MB
calibdata.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(calibdata))

Tcb = array([ 0.150,  -0.069,  0.481, 0.])
Wcb = array([0.331, -2.372, 2.363, 0. ])

sys.path.extend(["simulator/"])
sys.path.extend(["pylib/"])

import simulator
sim = simulator.simulator()
sim.trackbuf = calibdata
sim.imubuf = capture
sim.Tc = Tcb[:3] # + random.randn(3)*.1
import rodrigues
sim.Rc = rodrigues.rodrigues(Wcb[:3]) # + random.randn(3)*.05)
simp = cor.plugins_initialize_python(sim.run, None)
cor.plugins_register(simp)

execfile("vis/vis_cfg.py")
execfile("filter/filter_cfg.py")


