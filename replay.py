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
capture.dispatch.mb = capture;
cor.dispatch_init(capture.dispatch);
cor.plugins_register(cor.mapbuffer_open(capture))

outname = replay_file + "_solution"

sys.path.extend(['calibration'])
import calibration
sys.path.extend(["filter/", "filter/.libs"])
import filter

dc = filter.corvis_device_parameters()
#parameters for eagle's ipad 3
dc.Fx = 604.241;
dc.Fy = 604.028;
dc.Cx = 317.576;
dc.Cy = 237.755;
dc.px = -2.65791e-3;
dc.py = 6.48762e-4;
dc.K[0] = .2774956;
dc.K[1] = -1.0795446;
dc.K[2] = 1.14524733;
dc.a_bias[0] = .0367;
dc.a_bias[1] = -.0112;
dc.a_bias[2] = -.187;
dc.w_bias[0] = .0113;
dc.w_bias[1] = -.0183;
dc.w_bias[2] = .0119;
dc.Tc[0] = -.0940;
dc.Tc[1] = .0396;
dc.Tc[2] = .0115;
dc.Wc[0] = .000808;
dc.Wc[1] = .00355;
dc.Wc[2] = -1.575;
dc.a_bias_var[0] = 1.e-4;
dc.a_bias_var[1] = 1.e-4;
dc.a_bias_var[2] = 1.e-4;
dc.w_bias_var[0] = 1.e-7;
dc.w_bias_var[1] = 1.e-7;
dc.w_bias_var[2] = 1.e-7;
dc.Tc_var[0] = 1.e-6;
dc.Tc_var[1] = 1.e-6;
dc.Tc_var[2] = 1.e-6;
dc.Wc_var[0] = 1.e-7;
dc.Wc_var[1] = 1.e-7;
dc.Wc_var[2] = 1.e-7;
w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
dc.w_meas_var = w_stdev * w_stdev;
a_stdev = .000218 * sqrt(50.) * 9.8; #218 ug / sqrt(hz) at 50 hz
dc.a_meas_var = a_stdev * a_stdev;
dc.image_width = 640;
dc.image_height = 480;
dc.shutter_delay = 0;
dc. shutter_period = 31000;


fc = filter.filter_setup(capture.dispatch, outname, dc)

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
    from script_tools import time_printer
    tp = time_printer()
    cor.dispatch_addpython(fc.capture.dispatch, tp.print_time)
