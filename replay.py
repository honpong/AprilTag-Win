# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

cor.cvar.cor_time_pb_real = False

replay_file = args[1]
config_dir = "config"
if len(args) >= 3:
    config_dir = args[2]

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

sys.path.extend(['calibration'])
import calibration
sys.path.extend(["filter/", "filter/.libs"])
import filter

dc = filter.corvis_device_parameters()

from device_parameters import set_device_parameters
set_device_parameters(dc, 'ipad2') 

fc = filter.filter_setup(capture.dispatch, outname, dc)
fc.sfm.ignore_lateness = True

if runvis:
    visbuf = cor.mapbuffer()
    visbuf.size = 64 * 1024
    visbuf.dispatch = cor.dispatch_t()
    cor.plugins_register(cor.mapbuffer_open(visbuf))

    execfile("vis/vis_cfg.py")

    #    capturepydispatch = cor.dispatch_t()
    #capturepydispatch.mb = capture
    # capturepydispatch.threaded = True
    # cor.plugins_register(cor.dispatch_init(capturepydispatch))

    cor.dispatch_addpython(visbuf.dispatch, myvis.frame_1.window_3.plot_dispatch);
    cor.dispatch_addpython(visbuf.dispatch, myvis.frame_1.render_widget.packet_world);
    cor.dispatch_addpython(visbuf.dispatch, featover.status_queue.put);
    cor.dispatch_addpython(capture.dispatch, imageover.queue.put);

    #  trackdatapydispatch = cor.dispatch_t()
    #trackdatapydispatch.mb = trackdata
    #trackdatapydispatch.threaded = True
    #cor.plugins_register(cor.dispatch_init(trackdata.dispatch))
    cor.dispatch_addpython(fc.trackdata.dispatch, featover.queue.put)
    sys.path.extend(["renderable/", "renderable/.libs"])
    import renderable
    structure = renderable.structure()
    motion = renderable.motion()
    motion.color=[0.,0.,1.,1.]

    measurement = renderable.measurement("/Library/Fonts/Tahoma.ttf", 12.)
    measurement.color=[0.,1.,0.,1.]

    filter_render = renderable.filter_state(fc.sfm)
    
    myvis.frame_1.render_widget.renderables.append(structure.render)
    myvis.frame_1.render_widget.renderables.append(motion.render)
    myvis.frame_1.render_widget.renderables.append(measurement.render)
    myvis.frame_1.render_widget.renderables.append(filter_render.render)
    cor.dispatch_addclient(fc.solution.dispatch, structure, renderable.structure_packet)
    cor.dispatch_addclient(fc.solution.dispatch, motion, renderable.motion_packet)
    cor.dispatch_addclient(fc.solution.dispatch, measurement, renderable.measurement_packet)
    fc.sfm.visbuf = visbuf
else:
    from script_tools import time_printer, measurement_printer, progress_printer
    tp = time_printer()
    mp = measurement_printer(fc.sfm)
    pp = progress_printer(capture.dispatch)
    cor.dispatch_addpython(capture.dispatch, tp.print_time)
    cor.dispatch_addpython(fc.solution.dispatch, mp.print_measurement)
    cor.dispatch_addpython(capture.dispatch, pp.print_progress)
