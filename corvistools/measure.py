#!/opt/local/bin/python2.7
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
    sys.path.extend(['../'])
    from corvis import cor, filter

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

    from util.device_parameters import set_device_parameters
    set_device_parameters(dc, configuration_name)

    outname = filename + "_solution"
    fc = filter.filter_setup(capture.dispatch, outname, dc)
    fc.sfm.ignore_lateness = True

    cor.cor_time_init()
    filter.filter_start_processing_video(fc.sfm)
    cor.plugins_start()

    from util.script_tools import feature_stats
    fs = feature_stats(fc.sfm)
    cor.dispatch_addpython(fc.solution.dispatch, fs.packet);
    cor.dispatch_addpython(capture.dispatch, fs.capture_packet)

    from util.script_tools import sequence_stats
    ss = sequence_stats()
    cor.dispatch_addpython(capture.dispatch, ss.packet);

    from util.script_tools import monitor
    mc = monitor(capture)
    cor.dispatch_addpython(fc.solution.dispatch, mc.finished)

    last_bytes = 0
    last_time = 0
    while capture.dispatch.progress < 1.0:
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

    cor.plugins_stop()

    print "Filename:", filename
    fs.print_stats()
    ss.print_stats()
    distance = float(fc.sfm.s.total_distance)*100.
    measurement = float(sqrt(sum(fc.sfm.s.T.v**2)))*100.
    return (distance, measurement)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print "Usage:", sys.argv[0], "<data_filename> <configuration_name>"
        sys.exit(1)
    print "Filename:", sys.argv[1]
    print "Configuration name:", sys.argv[2]
    (distance, measurement) = measure(filename=sys.argv[1], configuration_name=sys.argv[2])
    print "Total path length (cm): %f" % (distance)
    print "Straight line length (cm): %f" % (measurement)
