# Created by Eagle Jones
# Copyright (c) 2009. The Regents of the University of California.
# All Rights Reserved.

from numpy import *
import Filter
import Models
import tracker
import sys, os
import quaternion
#from PIL import Image
import Tkinter
import Image, ImageTk
import myshow
import pylab
import readchar
import intrinsics
from IPython.Debugger import Tracer
from animplot import animplot
from playerparse import *

debug_here=Tracer()

#WORK ON RE-UNIFYING CODE - restructure for unscented, error state, essential, etc
#LOSING ALL GROUPS BEFORE I HAVE A CONVERGED REFERENCE
#TIME TO GAIN SCHEDULE!!!!
#FOR OUTLIERS, check if derivative of innovation is constant
#-->look for a bias -- but should only reject the worse ones, as a large outlier puts some bias everywhere
#so only reject a couple points per frame?
#outlier rejection allows the velocity drift to keep going, and initialization at mean depth feeds it back.
#need to deal with less than two good groups at a time
# there's really no good way to deal with velocity/scale drift -- it WILL get me eventually.
# add a velocity or gps reference?
#CALIBRATION CODE RETAINED GNORM -- check!

#I MUST BE ONE FRAME BEHIND/AHEAD -- innovations of features definitely lag or lead!
#REMOVE zeroing of omref, tref in unconverged groups?

#i think i'm integrating properly now

#question is how to differentiate between true outliers and drift?

innovation = []
covtrack = []
depthtrack = []
fulltrack = []
meashist = []
nummeas = []
packet_filter_position = 6
packet_filter_reconstruction = 7
def showarr(arr, fig):
    pylab.ioff()
    pylab.figure(fig)
    pylab.clf()
    pylab.imshow(abs(arr),interpolation='nearest',cmap=pylab.cm.gray)
    pylab.show()
    pylab.ioff()

def writeblock(fd, kind, user, time, data):
    size = len(data) + 16
    print size
    padsize = ((size + 15) /16) * 16
    print padsize
    header = array(padsize, dtype=uint32).tostring() + array(kind, dtype=uint16).tostring() + array(user, dtype=uint16).tostring() + array(time, dtype=uint64).tostring()
    fd.write(header)
    fd.write(data)
    if padsize - size:
        a= zeros(padsize-size, dtype = uint8)
        fd.write(a)

def writepacket_reconstruction(fd, time, points):
    writeblock(fd, packet_filter_reconstruction, points.shape[0], time, array(points,dtype=float32).tostring())

def writepacket_solution(fd, time, T, Om):
    writeblock(fd, packet_filter_position, 0, time, array(T,dtype=float32).tostring() + array(Om,dtype=float32).tostring())

def readimg(file, log, skip = False):
    #assume pgms have the formate P5\n[W] [H]\n255\n[DATA]...
    file.readline()
    dims = file.readline()
    width, height = map(int, dims.split())
    file.readline()
    if skip:
        file.seek(width*height, 1)
        return log()
    else:
        frm = file.read(width*height)
        return frm, width, height, log()

def readcrop(file, log, cropwidth, cropheight):
    frm, width, height, info = readimg(file, log)
    cropped = ''.join([frm[i*width:i*width+cropwidth] for i in xrange(cropheight)])
    return cropped, info

def readblock(file):
    header = file.read(16)
    size = fromstring(header, dtype=uint32, count=1)
    kind,user = fromstring(header[4:8], dtype=uint16, count=2)
    time = fromstring(header[8:], dtype=uint64, count=1)
    return kind, time, file.read(size-16)

def parsepgm(data):
    p5, blank, width, height, ignore = data[0:16].split(' ');
    width = int(width)
    height = int(height)
    return data[16:16+width*height]

def dosfm(simulation=False):
    doplots = True
    calibration = False
    savemeas = False
    robust = not calibration

    width = 640
    height = 768
    headersize = 16

    seterr("raise")
    myreadch = readchar.readchar()

    acal = intrinsics.camera("ladybug1.cal")

    infile = open(sys.argv[1], "rb")
    outputfile = open(sys.argv[2], "wb")

    frame = 0

    points = 0

    if calibration:
        maxpoints = 100
        minpoints = 40
        pointspergroup = 100
        maxgroups = 1
    else:
        maxpoints = 105
        minpoints = 40
        pointspergroup = 15
        maxgroups = 7

    simtrue = Models.RealFilter(maxpoints,maxgroups)
    model = Models.RealFilter(maxpoints,maxgroups)

    ndata = array(zeros(maxpoints*2),dtype='f')

    skip = 0
    if calibration:
        skip = 108
    else:
        skip = 420 #20 #120 #420
#    skip = 0;
    frame += skip
    #skip lots of frames to get past initialization, have some imu data to average
    imustack = list()
    for i in xrange(skip):
        kind = 0
        while kind != 1:
            kind, time, data = readblock(infile)
            if kind == 2:
                imustack.append(fromstring(data, dtype=float32, count=6))


    imudata2 = array(imustack).mean(0)
    imudata3 = imudata2
    imudata1 = imudata2
    imudata = imudata1

    alpha = imudata[0:3]
    om = imudata[3:6]

    gamma = -alpha.copy()
    ombias = array([0., 0., 0.]) #measurements.om.copy() 
    simtrue.state.gamma[:] = gamma.copy()
    simtrue.state.ombias[:] = ombias.copy()
    model.state.gamma[:] = gamma.copy()
    model.state.ombias[:] = ombias.copy()
    print gamma


    frm2 = parsepgm(data)
    time = time/1000000.



    pmap = array(zeros(maxpoints),dtype='b')
    t = tracker.tracker(width, height)

    # so we get notified when sim decides to drop outliers
    model.stateindex.register_callback(simtrue.stateindex, 'mask')
    model.measurementindex.register_callback(simtrue.measurementindex, 'mask')

    pwrap = Filter.DataModel(model.measurementindex, 'f')
    measurements = Filter.DataModel(model.measurementindex)
    measurements.gnorm[:] = 9.796 * 9.796 #(in los angeles) dot(gamma,gamma)

    simmeas = Filter.DataModel(simtrue.measurementindex)

    #c-migits:
    #array([-0.06123619,  0.24420784,  0.21121725])

    Tcb = array([0.06123619,  0.12020784,  0.3441725])
    #xsens: array([ 0.12686721,  0.12440865,  0.37039764])
#array([ 0.15040614,  0.03467871,  0.44670827]) #array([ 0.11189058,  0.12825021,  0.38620891]) #array([ 0.05779872,  0.08927856,  0.38413616]) #array([ 0.11189058,  0.12825021,  0.38620891]) #array([ 0.22479202,  0.1327969 ,  0.37050496])
    if Models.QUATERNION:
        Rcb = array([ 0.99186612, -0.01546045, -0.01448759,  0.12550969])
    else:
        Rcb =  array([-0.31568357,  1.52967114,  0.27678547])
 #xsens:       Rcb =  array([-0.21568357,  1.54467114,  0.22678547])
#array([-0.32765679,  1.58003521,  0.27498314])
# array([ 2.31297564, -2.244642  ,  2.75761907]) #array([ 2.32000717, -2.27573431,  2.79397666]) 
#
#array([-1.15195898,  1.09217769, -1.29894043])
#array([-0.01680306, -0.00316537,  0.27967243]) #array([-0.03636106, -0.03367608,  0.27045408])
    model.state.Tcb[:] = Tcb
    simtrue.state.Tcb[:] = Tcb
    model.state.Omcb[:] = Rcb
    simtrue.state.Omcb[:] = Rcb
    
    #model.stateindex.saturations.Omcb[:] = True
    #model.stateindex.saturations.Tcb[:] = True
    #model.stateindex.saturations.gamma[:] = True
    model.stateindex.saturations.abias[:] = True
    model.stateindex.saturations.ombias[:] = True
    model.measurementindex.saturations.gnorm[:] = True
    simtrue.measurementindex.saturations.gnorm[:] = True

    first = True
    badframes = 106, 593, 886, 985, 1148, 1214, 1649, 2540, 2774, 2974, 3035, 3533, 3619, 4075, 4299, 4706, 5112, 5446, 6087, 7023, 7660, 8233, 8245, 9471, 22455, 23152, 23568, 83167, 105402

    window1=myshow.myshow((width,height))

    histlen = 100

    pylab.ioff()
    pylab.figure(2)
    cam = array([[0,0,0],
                 [-1,0,1],
                 [1,0, 1],
                 [0,0,0]]).T
    track = animplot(2, histlen)
    track.t = track.add2d('b')
    track.s = track.add2d('r')
    track.tp = track.addsimple2d('b.')
    track.sp = track.addsimple2d('r.')
    track.tref = track.addsimple2d('bx-')
    track.sref = track.addsimple2d('rx-')
    track.camt, = track.axes.plot(cam[0,:], cam[1,:], 'b')
    track.cams, = track.axes.plot(cam[0,:], cam[1,:], 'r')
    pylab.axis('equal')

    plotz = animplot(3, histlen)
    plotz.t = plotz.addhist('b')
    plotz.s = plotz.addhist('r')
    depthplot = animplot(4, maxpoints)
    if simulation:
        depthplot.t = depthplot.addsimple('b.')
    depthplot.s = depthplot.addsimple('r.')

    plotTc = animplot(5, histlen)
    plotTc.x = plotTc.addhist('r')
    plotTc.y = plotTc.addhist('g')
    plotTc.z = plotTc.addhist('b')
    plotTc.axes.plot(ones(100) * simtrue.state.Tcb[0], 'r--')
    plotTc.axes.plot(ones(100) * simtrue.state.Tcb[1], 'g--')
    plotTc.axes.plot(ones(100) * simtrue.state.Tcb[2], 'b--')

    plotOc = animplot(6, histlen)
    plotOc.x = plotOc.addhist('r')
    plotOc.y = plotOc.addhist('g')
    plotOc.z = plotOc.addhist('b')
    if Models.QUATERNION:
        plotOc.axes.plot(ones(100) * simtrue.state.Omcb[1], 'r--')
        plotOc.axes.plot(ones(100) * simtrue.state.Omcb[2], 'g--')
        plotOc.axes.plot(ones(100) * simtrue.state.Omcb[3], 'b--')
    else:
        plotOc.axes.plot(ones(100) * simtrue.state.Omcb[0], 'r--')
        plotOc.axes.plot(ones(100) * simtrue.state.Omcb[1], 'g--')
        plotOc.axes.plot(ones(100) * simtrue.state.Omcb[2], 'b--')


    plotg = animplot(7, histlen)
    plotg.x = plotg.addhist('r')
    plotg.y = plotg.addhist('g')
    plotg.z = plotg.addhist('b')
    plotg.axes.plot(ones(100) * simtrue.state.gamma[0], 'r--')
    plotg.axes.plot(ones(100) * simtrue.state.gamma[1], 'g--')
    plotg.axes.plot(ones(100) * simtrue.state.gamma[2], 'b--')

    plotinna = animplot(8, histlen)
    plotinna.x = plotinna.addhist('r')
    plotinna.y = plotinna.addhist('g')
    plotinna.z = plotinna.addhist('b')

    plotinno = animplot(9, histlen)
    plotinno.x = plotinno.addhist('r')
    plotinno.y = plotinno.addhist('g')
    plotinno.z = plotinno.addhist('b')

    plotpts = animplot(11, histlen)
    plotpts.tp = plotpts.addsimple2d('b.')
    plotpts.sp = plotpts.addsimple2d('r.')
    plotpts.axes.set_xlim((-2,2))
    plotpts.axes.set_ylim((2,-2))

    simtrue.lasttime = time
    model.lasttime = time

    ss = False
    imustack = list()
    lastimu = 0
    while(True):
        c = myreadch()
        if c == 'p':
            doplots = not doplots
        if c == 's':
            ss = not ss
        if c == 'r':
            robust = not robust
        if c == 'm':
            pylab.figure(10)
            pylab.clf()
            tx = [ft[0] for ft in fulltrack]
            ty = [ft[1] for ft in fulltrack]
            pylab.plot(tx,ty)
            pylab.axis('equal')
            pylab.show()
        if c == ' ' or ss == True:
            c = None
            while c != ' ':
                c = myreadch()
                if c == 's':
                    ss = not ss


        frame=frame+1
        print frame

        kind = 0
        while True:
            kind, newtime, data = readblock(infile)
            if kind == 1:
                frm1 = frm2
                frm2 = parsepgm(data)
                vistime = newtime/1000000. #- .200 #subtract 200 ms for delay
                break
            if kind == 2:
                imutime = newtime / 1000000.
                simmeas = fromstring(data, dtype=float32, count=6)
                imustack.append((imutime, simmeas))
                simtrue.tick(imutime)
                simtrue.state.om[:] = simmeas[3:6]
                simtrue.state.alpha[:] = dot(Models.OmtoR(simtrue.state.Om), simmeas[0:3]) + simtrue.state.gamma
        
        while lastimu < vistime:
            imutime, simmeas = imustack.pop(0)
            measurements.alpha = simmeas[0:3]
            measurements.om = simmeas[3:6]
            model.imu_measurement(imutime + .100, measurements)
            lastimu = imutime + .100

        points = pwrap.points.size/2
        pointcount = t.track2(frm1, frm2, points, pwrap.points, pmap)

        ni = fromstring(frm2, 'B').reshape(height, width)
        window1(ni)

        rmask = model.outlier.rho > 10
        nrej = rmask.sum()
        writepacket_solution(outputfile, newtime, model.state.T, model.state.Om)
        if pointcount:
            worlds = model.measurement_model.get_world(model.state)
        if(points != pointcount or nrej):
            #we dropped features
            mask = pmap[:points].astype(bool)
            if nrej:
                print "rejected outliers: ", nrej
                mask[rmask] = False

            droppedworld = worlds[-mask]
            goodones = model.covar['rho','rho'].diagonal()[-mask] < .01
            if any(goodones):
                writepacket_reconstruction(outputfile, newtime, droppedworld[goodones])
            model.droppoints(mask)
            points = model.state.rho.size

        if points:
            measurements.points, mask = acal.normalize(pwrap.points+32.5)
            if not all(mask):
                model.droppoints(mask)
                points = model.state.rho.size
            meashist.append(hstack([measurements.alpha, measurements.om, measurements.points]))
#        m.tick(measurements, imudt)

        #now update the simulator
        print "*******REAL********"
        if simulation:
            simtrue.measurement_model.predict_measurements(simtrue.state, simmeas)
            model.vis_measurement(vistime, simmeas, False)
        else:
            if points:
                model.vis_measurement(vistime, measurements, robust)

        print model.state.Omref[0:Models.ROTD]

        mask = model.outlier.rho > 0
        mask = mask.repeat(2)
        window1.drawfeats(pwrap.points[mask]+32.5, (255,0,0))
        window1.drawfeats(pwrap.points[-mask]+32.5, (0,255,0))
        window1.draw()

        if c == 'd':
            debug_here()

        points = model.state.rho.size
        groups = len(model.state.groups)
        if groups < maxgroups:
            newpoints = t.addpoints2(points, pointspergroup, pwrap.points, ndata)
            if newpoints:
                normed, mask = acal.normalize(ndata[:newpoints*2] + 32.5)
                initialx = normed[0::2][mask]
                initialy = normed[1::2][mask]
                origpoints = newpoints
                newpoints = initialx.size
                model.addpoints(initialx, initialy)
                simtrue.addpoints(initialx, initialy)
                if calibration:
                    pointids = Filter.DataModel(model.stateindex, int32)
                    pointids.rho = arange(model.state.rho.size)
                #may have dropped an old group
                insertion = model.state['rho'].size - newpoints
                pwrap.points[insertion*2:] = (ndata[:origpoints*2][repeat(mask,2)])

                simtrue.state.rho[insertion:] = random.randn(newpoints)+3

                print model.state.T
                print model.state.V
                print "alpha:"
                print model.state.alpha - model.state.gamma
                R = Models.OmtoR(model.state.Om)
                print dot(R.T, model.state.alpha-model.state.gamma)
                print measurements.alpha
                print
                print model.state.Om
                print model.state.om
                print
                print model.state.gamma
                print model.state.Omcb
                print model.state.Tcb
                points += newpoints
                if calibration and not simulation:
                    goodpoints = array([ 0,  1,  6,  7, 14, 16, 19, 23, 25, 28, 29, 30, 32, 33, 34,
                                         36, 37, 38, 39, 40, 44, 45, 46, 47, 51, 53, 55, 56, 58, 59, 60, 61,
                                         63, 67, 71, 74, 76, 77, 78, 80, 83, 84, 85, 86, 87, 89, 94, 99])

#3, 17, 96 drifted
#41, 54, 72, 79, 81, 90 jumped
                    points = goodpoints.size
                    mask = zeros(100, dtype=bool)
                    mask[goodpoints] = True
                    model.droppoints(mask)
        if doplots:

            pylab.ioff()
            track.t(simtrue.state.T[0], simtrue.state.T[1])
            track.s(model.state.T[0], model.state.T[1])
            Rt = Models.OmtoR(simtrue.state.Om)
            Rbct = Models.OmtoR(simtrue.state.Omcb)
            Rs = Models.OmtoR(model.state.Om)
            Rbcs = Models.OmtoR(model.state.Omcb)
            Tt = simtrue.state.T.reshape((3,1))
            Tbct = simtrue.state.Tcb.reshape((3,1))
            Ts = model.state.T.reshape((3,1))
            Tbcs = model.state.Tcb.reshape((3,1))

            camt = dot(Rt, dot(Rbct, cam) + Tbct) + Tt
            cams = dot(Rs, dot(Rbcs, cam) + Tbcs) + Ts
            bp = array([[0,0,0],
                        [1,0,0]]).T
            camt = hstack([camt, (dot(Rt, bp) + Tt)])
            cams = hstack([cams, (dot(Rs, bp) + Ts)])

            track.camt.set_data(camt[0,:], camt[1,:])
            track.cams.set_data(cams[0,:], cams[1,:])

            
            track.tref(simtrue.state.Tref[0::3], simtrue.state.Tref[1::3])
            track.sref(model.state.Tref[0::3], model.state.Tref[1::3])
            
            if model.state.rho.size > 0:
                if simulation:
                    worldt = simtrue.measurement_model.get_world(simtrue.state)
                    track.tp(worldt[:,0], worldt[:,1])
                worlds = model.measurement_model.get_world(model.state)
                goodones = model.covar['rho','rho'].diagonal() < .01
                x = worlds[goodones,0]
                y = worlds[goodones,1]
                if(any(goodones)):
                    track.sp(x, y)

            plotz.t(simtrue.state.T[2])
            plotz.s(model.state.T[2])
            plotz.autoscale()
            if model.state.rho.size > 0:
                if simulation:
                    depthplot.t(simtrue.state.rho)
                depthplot.s(model.state.rho)
                depthplot.autoscale()
            plotTc.x(model.state.Tcb[0])
            plotTc.y(model.state.Tcb[1])
            plotTc.z(model.state.Tcb[2])
            plotTc.autoscale()
            if Models.QUATERNION:
                plotOc.x(model.state.Omcb[1])
                plotOc.y(model.state.Omcb[2])
                plotOc.z(model.state.Omcb[3])
            else:
                plotOc.x(model.state.Omcb[0])
                plotOc.y(model.state.Omcb[1])
                plotOc.z(model.state.Omcb[2])
            plotOc.autoscale()
            plotg.x(model.state.gamma[0])
            plotg.y(model.state.gamma[1])
            plotg.z(model.state.gamma[2])
            plotg.autoscale()
            plotinna.x(model.innovation.alpha[0])
            plotinna.y(model.innovation.alpha[1])
            plotinna.z(model.innovation.alpha[2])
            plotinna.autoscale()
            plotinno.x(model.innovation.om[0])
            plotinno.y(model.innovation.om[1])
            plotinno.z(model.innovation.om[2])
            plotinno.autoscale()
            if simulation:
                plotpts.tp(simmeas.points[0::2], simmeas.points[1::2])
            else:
                plotpts.tp(measurements.points[0::2], measurements.points[1::2])
            plotpts.sp(model.prediction.points[0::2], model.prediction.points[1::2])

            pylab.show()
            pylab.ioff()

        #if frame > 1100:
        #    model.test_meas(debug_here)
        fulltrack.append(model.state.T.copy())
        innovation.append( model.innovation.compact.copy())
        covtrack.append(model.covar.compact.diagonal().copy())
        depthtrack.append(model.state.rho.copy()[1:40])

    infile.close()
    outputfile.close()

if __name__ == "__main__":
    try:
        dosfm()
    except KeyboardInterrupt:
        pass
    infile.close()
    outputfile.close()

    innov = vstack([i[:20] for i in innovation])
    pylab.figure(4)
    pylab.clf()
    pylab.subplot(231)
    pylab.plot(innov[:,0])
    pylab.subplot(232)
    pylab.plot(innov[:,1])
    pylab.subplot(233)
    pylab.plot(innov[:,2])
    pylab.subplot(234)
    pylab.plot(innov[:,3])
    pylab.subplot(235)
    pylab.plot(innov[:,4])
    pylab.subplot(236)
    pylab.plot(innov[:,5])
    pylab.figure(6)
    pylab.clf()
    pylab.plot(innov[:,6])
    pylab.figure(7)
    pylab.clf()
    pylab.plot(innov[:,7:]*555) #555 is approx focal length - converts to pixels

    depths = array(depthtrack)
    pylab.figure(14)
    pylab.plot(exp(depths))

    pylab.figure(10)
    tx = []
    ty = []
    for t in fulltrack:
        tx.append(t[0])
        ty.append(t[1])

    pylab.plot(tx, ty)
    pylab.axis('equal')

    cov = array(covtrack)
    pylab.figure(15)
    pylab.plot(cov)
    pylab.show()
    
    m = array(meashist)
    pylab.figure(12)
    pylab.clf()
    pylab.plot(m[:,6::2])
    pylab.figure(13)
    pylab.clf()
    pylab.plot(m[:,7::2])

    if savemeas:
        savetxt('imuvis', m.T, fmt="%12.6G") 

    pylab.figure(14)
    pylab.plot(nummeas)

    pylab.show()
    
