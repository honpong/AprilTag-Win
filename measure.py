# Created by Brian Fulkerson
# Copyright (c) 2013. RealityCap, Inc.
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text
import sys
import time
from numpy import *

def measure(filename, configuration_name):
    sys.path.extend(['cor/'])
    sys.path.extend(['numerics/'])
    sys.path.extend(["filter/", "filter/.libs"])
    import cor
    import numerics
    import filter

    cor.cvar.cor_time_pb_real = False

    capture = cor.mapbuffer()
    capture.filename = filename
    capture.replay = True
    capture.size = 16 * 1024 * 1024
    capture.dispatch = cor.dispatch_t()
    capture.dispatch.reorder_depth = 20
    capture.dispatch.max_latency = 25000
    capture.dispatch.mb = capture;
    cor.dispatch_init(capture.dispatch);
    cor.plugins_register(cor.mapbuffer_open(capture))

    dc = filter.corvis_device_parameters()

    from device_parameters import set_device_parameters
    set_device_parameters(dc, configuration_name) 

    outname = filename + "_solution"
    fc = filter.filter_setup(capture.dispatch, outname, dc)
    fc.sfm.ignore_lateness = True

    cor.cor_time_init()
    cor.plugins_start()

    class monitor():
        def __init__(self, capture):
            self.capture = capture
            self.bytes_dispatched = 0
            self.done = False
            self.percent = 0

        def finished(self, packet):
            if self.done: return

            self.total_bytes = self.capture.dispatch.mb.total_bytes
            self.bytes_dispatched = self.capture.dispatch.bytes_dispatched
            self.percent = 100. * self.bytes_dispatched / self.total_bytes
            if self.bytes_dispatched == self.total_bytes:
              self.done = True

    mc = monitor(capture)
    cor.dispatch_addpython(fc.solution.dispatch, mc.finished)

    last_bytes = 0
    last_time = 0
    while not mc.done: 
        if last_bytes == mc.bytes_dispatched and mc.percent > 90:
            if last_time == 0: 
                last_time = time.time()
            if time.time() - last_time > 2:
                print "No packets received recently, stopping"
                break
        else:
            last_time = 0
            
        last_bytes = mc.bytes_dispatched
        time.sleep(0.1)

    return fc.sfm.s

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print "Usage:", sys.argv[0], "<data_filename> <configuration_name>"
        sys.exit(1)
    state = measure(filename=sys.argv[1], configuration_name=sys.argv[2])
    print "Filename:", sys.argv[1]
    print "Configuration name:", sys.argv[2]
    distance = float(state.total_distance)*100.
    measurement = float(sqrt(sum(state.T.v**2)))*100.
    print "Total path length (cm): %f" % (distance)
    print "Straight line length (cm): %f" % (measurement)
