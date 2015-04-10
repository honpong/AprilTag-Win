#!/usr/bin/env python
# Created by Brian Fulkerson
# Copyright (c) 2013. RealityCap, Inc.
# All rights reserved.

import sys
import time
from numpy import *
import re

def get_parameters_for_filename(filename):
    # Parse filenames in the form _<width>_<height>_<rate>Hz
    width, height, framerate = (640, 480, 30)
    match = re.match(".*_(\d+)_(\d+)_(\d+)Hz.*", filename)
    if match:
        width = int(match.group(1))
        height = int(match.group(2))
        framerate = int(match.group(3))
    return (width, height, framerate)

def configure_mapbuffer(filename):
    from corvis import cor

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

    return capture

def configure_device(configuration_name, width=640, height=480, framerate=30):
    from corvis import filter
    dc = filter.corvis_device_parameters()

    from util.device_parameters import set_device_parameters
    set_device_parameters(dc, configuration_name)
    filter.device_set_resolution(dc, width, height)
    filter.device_set_framerate(dc, framerate)

    return dc

def configure_visualization(capture, fc):
    from corvis import cor
    visbuf = cor.mapbuffer()
    visbuf.size = 64 * 1024
    visbuf.dispatch = cor.dispatch_t()
    cor.plugins_register(cor.mapbuffer_open(visbuf))

    from vis.vis import Vis
    import vis.ImagePanel
    myvis = Vis()
    mvp = cor.plugins_initialize_python(None, myvis.stop)
    mvp.priority = 0
    cor.plugins_register(mvp)

    ip = myvis.frame.image_widget
    imageover = vis.ImagePanel.ImageOverlay(ip)
    featover = vis.ImagePanel.FeatureOverlay(ip)

    ip.renderables.append(imageover)
    ip.renderables.append(featover)

    cor.dispatch_addpython(visbuf.dispatch, myvis.frame.window_3.plot_dispatch);
    cor.dispatch_addpython(visbuf.dispatch, myvis.frame.render_widget.packet_world);
    cor.dispatch_addpython(visbuf.dispatch, featover.status_queue.put);
    cor.dispatch_addpython(capture.dispatch, imageover.queue.put);

    cor.dispatch_addpython(fc.trackdata.dispatch, featover.queue.put)
    cor.dispatch_addpython(fc.trackdata.dispatch, featover.pred_queue.put)
    sys.path.extend(["renderable/", "renderable/.libs"])
    import renderable
    structure = renderable.structure()
    motion = renderable.motion()
    motion.color=[0.,0.,1.,1.]

    measurement = renderable.measurement("/Library/Fonts/Tahoma.ttf", 12.)
    measurement.color=[0.,1.,0.,1.]

    filter_render = renderable.filter_state(fc.sfm)

    myvis.frame.render_widget.add_renderable(structure.render, "Structure")
    myvis.frame.render_widget.add_renderable(motion.render, "Motion")
    myvis.frame.render_widget.add_renderable(measurement.render, "Measurement")
    myvis.frame.render_widget.add_renderable(filter_render.render, "Filter state")
    cor.dispatch_addclient(fc.solution.dispatch, structure, renderable.structure_packet)
    cor.dispatch_addclient(fc.solution.dispatch, motion, renderable.motion_packet)
    cor.dispatch_addclient(fc.solution.dispatch, measurement, renderable.measurement_packet)

    return (visbuf, myvis)


def measure(filename, configuration_name, realtime=False, shell=False,
        vis=False, sim=False):
    sys.path.extend(['../'])
    from corvis import cor, filter

    cor.cvar.cor_time_pb_real = realtime

    if not sim:
        capture = configure_mapbuffer(filename)
    else:
        sys.path.extend(["alternate_visualizations/pylib/"])
        from simulator import simulator
        capture = configure_mapbuffer("simulator/data/dummy")
        """
        # function based simulator
        sim = simulator.simulator()
        sim.trackbuf = capture
        sim.imubuf = capture
        Tcb = array([ 0.150,  -0.069,  0.481, 0.])
        Wcb = array([0.331, -2.372, 2.363, 0. ])
        sim.Tc = Tcb[:3] # + random.randn(3)*.1
        import rodrigues
        sim.Rc = rodrigues.rodrigues(Wcb[:3]) # + random.randn(3)*.05)
        """
        # data based simulator
        sim = simulator.data_simulator(filename)
        sim.imubuf = capture

        simp = cor.plugins_initialize_python(sim.run, None)
        cor.plugins_register(simp)

    (width, height, framerate) = get_parameters_for_filename(filename)
    dc = configure_device(configuration_name, width, height, framerate)

    fc = filter.filter_setup(capture.dispatch, dc)
    fc.sfm.ignore_lateness = True

    if not vis:
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

    if vis:
        (visbuf, myvis) = configure_visualization(capture, fc)

        fc.sfm.visbuf = visbuf

        if sim:
            import vis.simulator
            gt_render = vis.simulator.simulator_ground_truth()
            myvis.frame.render_widget.add_renderable(gt_render.render, "Simulation truth")
            cor.dispatch_addpython(capture.dispatch, gt_render.receive_packet);

    cor.cor_time_init()
    #filter.filter_start_qr_benchmark(fc.sfm, 0.1825)
    if not sim:
        filter.filter_start_dynamic(fc.sfm)
    if sim:
        filter.filter_start_simulator(fc.sfm)

    cor.plugins_start()

    if shell:
        from IPython.terminal.embed import InteractiveShellEmbed
        from threading import Thread
        def shell_function():
            # refer to variables we might want to access in the
            # shell
            dummy = (fc, cor, filter)
            ipshell = InteractiveShellEmbed(display_banner=False)
            ipshell()

        thread = Thread(target = shell_function)
        thread.setDaemon(True)
        thread.start()

    if vis:
        myvis.app.MainLoop()

    else: # no visualization
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
