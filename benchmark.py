# Created by Brian Fulkerson
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

cor.cvar.cor_time_pb_real = False

replay_file = args[1]

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

sys.path.extend(["filter/", "filter/.libs"])
import filter

dc = filter.corvis_device_parameters()

from device_parameters import set_device_parameters
set_device_parameters(dc, 'ipad2') 

fc = filter.filter_setup(capture.dispatch, outname, dc)
fc.sfm.ignore_lateness = True
