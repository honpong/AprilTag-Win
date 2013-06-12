# Created by Brian Fulkerson
# Copyright (c) 2013. RealityCap, Inc.
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

import sys, os
import getopt
import ctypes

KB = 1024
MB = 1024*KB

sys.path.extend(['cor/'])
import cor
sys.path.extend(['numerics/'])
import numerics

cor.cvar.cor_time_pb_real = False

replay_file = sys.argv[1]

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


cor.cor_time_init()
cor.plugins_start()

class monitor():
    done = False
    def __init__(self, capture):
        self.capture = capture

    def finished(self, packet):
        if self.done: return
        if self.capture.dispatch.mb.total_bytes == self.capture.dispatch.bytes_dispatched:
          self.done = True

mc = monitor(capture)
cor.dispatch_addpython(fc.solution.dispatch, mc.finished)

import time
while not mc.done:
    time.sleep(0.1)

print "Total path length (m):", fc.sfm.s.total_distance
print "Straight line length(m):", fc.sfm.s.T.v
