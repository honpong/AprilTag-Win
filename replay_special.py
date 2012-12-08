# Created by Eagle Jones
# Copyright (c) 2012. RealityCap, Inc.
# All Rights Reserved.

from numpy import *

cor.cvar.cor_time_pb_real = False

replay_file = args[1]
config_dir = "config"
if len(args) >= 3:
    config_dir = args[2]

capture = cor.mapbuffer()
calibdata = cor.mapbuffer()
cor.mapbuffer_init(capture, 0)
capture.filename = replay_file
capture.file_writable = False
capture.mem_writable = True
capture.indexsize = 1000000
capture.threaded = True
cor.plugins_register(cor.mapbuffer_open(capture))

capturedispatch = cor.dispatch_t()
capturedispatch.mb = capture
capturedispatch.threaded = True
capturedispatch.reorder_depth = 10
cor.plugins_register(cor.dispatch_init(capturedispatch))

siftdata = cor.mapbuffer()
cor.mapbuffer_init(siftdata, 1000)
siftdata.mem_writable = True
siftdata.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(siftdata))

descriptor_data = cor.mapbuffer()
cor.mapbuffer_init(descriptor_data, 1000)
descriptor_data.mem_writable = True
descriptor_data.file_writable = True
descriptor_data.filename = replay_file + "_descriptors"
descriptor_data.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(descriptor_data))

cor.mapbuffer_init(calibdata, 1000)
calibdata.mem_writable = True
calibdata.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(calibdata))

trackdata = cor.mapbuffer()
cor.mapbuffer_init(trackdata, 1000)
trackdata.mem_writable = True
trackdata.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(trackdata))

solution = cor.mapbuffer()
cor.mapbuffer_init(solution, 1000)
solution.filename = replay_file + "_solution"
solution.mem_writable = True
solution.file_writable = True
solution.dispatch = cor.dispatch_t()
cor.plugins_register(cor.mapbuffer_open(solution))

execfile(os.path.join(config_dir, "calibration_cfg.py"))
execfile(os.path.join(config_dir, "tracker_cfg.py"))
execfile(os.path.join(config_dir, "filter_cfg.py"))
execfile(os.path.join(config_dir, "recognition_cfg.py"))

sfm.output = solution
cor.dispatch_addclient(capturedispatch, sfm, filter.sfm_imu_measurement_cb)
cor.dispatch_addclient(calibdata.dispatch, sfm, filter.sfm_vis_measurement_cb)
cor.dispatch_addclient(calibdata.dispatch, sfm, filter.sfm_features_added_cb)

cor.dispatch_addclient(capturedispatch, track, tracker.frame_cb);
cor.dispatch_addclient(trackdata.dispatch, cal, calibration.calibration_feature_rectified_cb)

sfm.s.mapperbuf = descriptor_data
sfm.recognition_buffer = siftdata
cor.dispatch_addclient(siftdata.dispatch, sift, recognition.recognition_packet_cb)
cor.dispatch_addclient(siftdata.dispatch, cal, calibration.calibration_recognition_feature_denormalize_inplace)
sift.sink = descriptor_data
sift.imagebuf = capture

if runvis:
    visbuf = cor.mapbuffer()
    cor.mapbuffer_init(visbuf, 100)
    visbuf.mem_writable = True
    cor.plugins_register(cor.mapbuffer_open(visbuf))
    visdispatch = cor.dispatch_t()
    visdispatch.threaded = True
    visdispatch.mb = visbuf
    cor.plugins_register(cor.dispatch_init(visdispatch))

    execfile("vis/vis_cfg.py")

    capturepydispatch = cor.dispatch_t()
    capturepydispatch.mb = capture
    capturepydispatch.threaded = True
    cor.plugins_register(cor.dispatch_init(capturepydispatch))

    cor.dispatch_addpython(visdispatch, myvis.frame_1.window_3.plot_dispatch);
    cor.dispatch_addpython(visdispatch, myvis.frame_1.render_widget.packet_world);
    cor.dispatch_addpython(visdispatch, featover.status_queue.put);
    cor.dispatch_addpython(capturepydispatch, imageover.queue.put);

    trackdatapydispatch = cor.dispatch_t()
    trackdatapydispatch.mb = trackdata
    trackdatapydispatch.threaded = True
    cor.plugins_register(cor.dispatch_init(trackdatapydispatch))
    cor.dispatch_addpython(trackdatapydispatch, featover.queue.put)
    sys.path.extend(["renderable/", "renderable/.libs"])
    import renderable

    boelter_map = renderable.texture("log/boelter_10pix_m_black.pgm")
    boelter_map.origin = array([-18.8, 56.5, 0.])
    boelter_map.width = 114.4
    boelter_map.height = 114.4
    boelter_map.theta = 197.
    
    structure = renderable.structure(None)
    motion = renderable.motion(None)
    motion.color = [0., 0., 1., 1.]
    structure.color[3] = .5
    myvis.frame_1.render_widget.renderables.append(boelter_map.render)
    myvis.frame_1.render_widget.renderables.append(structure.render)
    myvis.frame_1.render_widget.renderables.append(motion.render)
    cor.dispatch_addclient(solution.dispatch, structure, renderable.structure_packet)
    cor.dispatch_addclient(solution.dispatch, motion, renderable.motion_packet)
    sfm.visbuf = visbuf

    theta = pi + pi / 10.5
    myvis.frame_1.render_widget.view_transform[0,0] = cos(theta);
    myvis.frame_1.render_widget.view_transform[1,1] = cos(theta);
    myvis.frame_1.render_widget.view_transform[1,0] = sin(theta);
    myvis.frame_1.render_widget.view_transform[0,1] = -sin(theta);
    myvis.frame_1.render_widget.view_transform[2,2] = -1.;
    #    myvis.frame_1.render_widget.view_transform[3,0] = -3.85;
    #myvis.frame_1.render_widget.view_transform[3,1] = 1.45;

    boelter_map.color[3] = 0.

else:
    from script_tools import time_printer
    tp = time_printer()
    cor.dispatch_addpython(capturedispatch, tp.print_time)

