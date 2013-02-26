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

capture = cor.inbuffer()
cor.inbuffer_init(capture, 0)
capture.filename = replay_file
capture.file_writable = False
capture.mem_writable = True
capture.indexsize = 1000000
capture.threaded = True
capture.dispatch = cor.dispatch_t()
capture.dispatch.reorder_depth = 100
cor.dispatch_init(capture.dispatch);
cor.plugins_register(cor.inbuffer_open(capture))

calibdata = cor.mapbuffer()
calibdata.size = 32 * 1024 * 1024
calibdata.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(calibdata))

trackdata = cor.mapbuffer()
trackdata.size = 32 * 1024 * 1024
trackdata.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(trackdata))

solution = cor.mapbuffer()
solution.size = 32 * 1024 * 1024
solution.filename = replay_file + "_solution"
solution.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(solution))

track_control = cor.mapbuffer()
track_control.size = 1 * 1024 * 1024
track_control.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(track_control))

execfile(os.path.join(config_dir, "calibration_cfg.py"))
execfile(os.path.join(config_dir, "tracker_cfg.py"))
execfile(os.path.join(config_dir, "filter_cfg.py"))

sfm.output = solution
sfm.control = track_control
cor.dispatch_addclient(track_control.dispatch, track, tracker.control_cb)
cor.dispatch_addclient(capture.dispatch, sfm, filter.sfm_imu_measurement_cb)
cor.dispatch_addclient(capture.dispatch, sfm, filter.sfm_accelerometer_measurement_cb)
cor.dispatch_addclient(capture.dispatch, sfm, filter.sfm_gyroscope_measurement_cb)
cor.dispatch_addclient(calibdata.dispatch, sfm, filter.sfm_vis_measurement_cb)
cor.dispatch_addclient(calibdata.dispatch, sfm, filter.sfm_features_added_cb)

cor.dispatch_addclient(capture.dispatch, track, tracker.frame_cb);
cor.dispatch_addclient(trackdata.dispatch, cal, calibration.calibration_feature_cb)
cor.dispatch_addclient(trackdata.dispatch, sfm, filter.sfm_raw_trackdata_cb) #this must come after calibration because dispatch is done in reverse order.

if runvis:
    visbuf = cor.mapbuffer()
    visbuf.size = 32 * 1024 * 1024
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
    cor.plugins_register(cor.dispatch_init(trackdata.dispatch))
    cor.dispatch_addpython(trackdata.dispatch, featover.queue.put)
    sys.path.extend(["renderable/", "renderable/.libs"])
    import renderable
    structure = renderable.structure()
    motion = renderable.motion()
    motion.color=[0.,0.,1.,1.]

    measurement = renderable.measurement("/Library/Fonts/Tahoma.ttf", 12.)
    measurement.color=[0.,1.,0.,1.]

    filter_render = renderable.filter_state(sfm)
    
    myvis.frame_1.render_widget.renderables.append(structure.render)
    myvis.frame_1.render_widget.renderables.append(motion.render)
    myvis.frame_1.render_widget.renderables.append(measurement.render)
    myvis.frame_1.render_widget.renderables.append(filter_render.render)
    cor.dispatch_addclient(solution.dispatch, structure, renderable.structure_packet)
    cor.dispatch_addclient(solution.dispatch, motion, renderable.motion_packet)
    cor.dispatch_addclient(solution.dispatch, measurement, renderable.measurement_packet)
    sfm.visbuf = visbuf
else:
    from script_tools import time_printer
    tp = time_printer()
    cor.dispatch_addpython(capture.dispatch, tp.print_time)
