#!/usr/bin/env python
'''
tumview.py - Extended .tum visualization tool
---------------------------------------------
tumview.py enables visualization of .tum or .tumx files. 
.tum file format is defined in https://vision.in.tum.de/data/datasets/rgbd-dataset/file_formats
which is space 8 space delimited lines in a form of
timestamp, Tx, Ty, Tz, Qi, Qj, Qk, Qw
In addition, tumview can use 9th field device-id data if exists to visualize each device
seperatly.
'''

import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import sys
import os
import math
import argparse

TIME = 0
X = 1
Y = 2
Z = 3
D = 4
AX = 5
AY = 6
AZ = 7

# plot modes
PLOT_MODE_AXIS = 0 # X,Y,Z trajectories
PLOT_MODE_DIST = 1 # distance from origin
PLOT_MODE_PROJECTIONS = 2 # plane projections
PLOT_MODE_ANGLES = 3 # Angles
PLOT_MODE_3D = 4

RAD_TO_DEG = 180.0 / math.pi

def set_axes_equal(ax):
    '''Make axes of 3D plot have equal scale so that spheres appear as spheres,
    cubes as cubes, etc..  This is one possible solution to Matplotlib's
    ax.set_aspect('equal') and ax.axis('equal') not working for 3D.

    Input
      ax: a matplotlib axis, e.g., as output from plt.gca().
    '''

    x_limits = ax.get_xlim3d()
    y_limits = ax.get_ylim3d()
    z_limits = ax.get_zlim3d()

    x_range = abs(x_limits[1] - x_limits[0])
    x_middle = sum(x_limits)/len(x_limits)
    y_range = abs(y_limits[1] - y_limits[0])
    y_middle = sum(y_limits)/len(y_limits)
    z_range = abs(z_limits[1] - z_limits[0])
    z_middle = sum(z_limits)/len(z_limits)

    # The plot bounding box is a sphere in the sense of the infinity
    # norm, hence I call half the max range the plot radius.
    plot_radius = 0.5*max([x_range, y_range, z_range])

    ax.set_xlim3d([x_middle - plot_radius, x_middle + plot_radius])
    ax.set_ylim3d([y_middle - plot_radius, y_middle + plot_radius])
    ax.set_zlim3d([z_middle - plot_radius, z_middle + plot_radius])

def QtoEuler(qx, qy, qz, qw):
    '''
    Calclulate yaw, pitch, roll from Quaternion
    returns yaw,pitch,roll
    source: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    roll, pitch, yaw is defined as in the article
    '''
    ysqr = qy * qy
    t0 = 2.0 * (qw * qx + qy * qz)
    t1 = 1.0 - 2 * (qx * qx + ysqr)
    roll = math.atan2(t0, t1)

    t2 = 2 * (qw * qy - qz * qx)
    if t2 > 1.0: t2 = 1.0
    if t2 < -1.0: t2 = -1.0
    pitch = math.asin(t2)

    t3 = 2 * (qw * qz + qx * qy)
    t4 = 1.0 - 2 * (ysqr + qz * qz)
    yaw = math.atan2(t3, t4)
    return yaw,pitch,roll

def read_tum(filename, args) :
    '''
    Read tum file, return a generic name and dictinary of the form:
    d[device_id] = { TIME: [], X:[], Y:[], Z:[], D:[], AX:[], AY: [], AZ: []}
    '''
    d = {}
    name = os.path.basename(filename)
    name = os.path.splitext(name)[0]
    fp = open(filename)
    lc = 0
    last_time = 0.0
    for l in fp.readlines():
        lc += 1
        data = l.split()
        if len(data) == 0: continue
        if len(data) < 8:
            print '%s: misformatted line at %d' % (filename, lc)
            print data
            continue
        device_id = 0
        if args.tum == False:
            if len(data) > 8:
                device_id = int(data[8])
            if len(data) > 9:
                confidence = int(data[9])
        if (device_id in d.keys()) == False:
            d[device_id] = { TIME: [], X:[], Y:[], Z:[], D:[], AX:[], AY: [], AZ: []}

        if args.sample_number:
            d[device_id][TIME].append(lc)
        else:
            d[device_id][TIME].append( float(data[0]))

        if last_time > 0.0 and d[device_id][TIME] < last_time:
            print '%s : device: %d timestamp is not monotonic current: %f last: %f' % (name, device_id, d[device_id][TIME], last_time)
        last_time = d[device_id]

        (x, y, z, qi, qj, qk, qw) = (float(f) for f in data[1:8])
        dist = math.sqrt(x * x + y * y + z * z)
        ax,ay,az = QtoEuler(qi, qj, qk, qw)

        d[device_id][X].append( x )
        d[device_id][Y].append( y )
        d[device_id][Z].append( z )
        d[device_id][D].append( dist )
        d[device_id][AX].append( ax  * RAD_TO_DEG)
        d[device_id][AY].append( ay  * RAD_TO_DEG)
        d[device_id][AZ].append( az  * RAD_TO_DEG)

    return name, d

def plot_tum(nkeys, name, data, fig, plot_mode = PLOT_MODE_AXIS) :
    l = 0
    for k in data.keys():
        l += 1
        if plot_mode == PLOT_MODE_AXIS:
            xy_plt = plt.subplot(nkeys, 1, l)
            xy_plt.plot(data[k][TIME], data[k][X], label="X_%s_%d" % (name, k))
            xy_plt.plot(data[k][TIME], data[k][Y], label="Y_%s_%d" % (name, k))
            xy_plt.plot(data[k][TIME], data[k][Z], label="Z_%s_%d" % (name, k))
            xy_plt.legend(loc='upper right')
        elif plot_mode == PLOT_MODE_ANGLES:
            xy_plt = plt.subplot(nkeys, 1, l)
            xy_plt.plot(data[k][TIME], data[k][AX], label="AX_%s_%d" % (name, k))
            xy_plt.plot(data[k][TIME], data[k][AY], label="AY_%s_%d" % (name, k))
            xy_plt.plot(data[k][TIME], data[k][AZ], label="AZ_%s_%d" % (name, k))
            xy_plt.legend(loc='upper right')
        elif plot_mode == PLOT_MODE_DIST:
            xy_plt = plt.subplot(nkeys, 1, l)
            xy_plt.plot(data[k][TIME], data[k][D], label="%s_%d" % (name, k))
            xy_plt.legend(loc='upper right')
        elif plot_mode == PLOT_MODE_3D:
            print "plot mode 3d"
            ax3d = plt.gca(projection='3d')
            ax3d.plot(data[k][X], data[k][Y], data[k][Z], label=name, linewidth=1)
            ax3d.mouse_init()
            set_axes_equal(ax3d)
            #ax3d.set_aspect('equal', 'datalim')
            ax3d.set_xlabel("X")
            ax3d.set_ylabel("Y")
            ax3d.set_zlabel("Z")
            ax3d.legend()
        else: # PLOT_MODE_PROJECTIONS
            xy_plt = plt.subplot(3, nkeys, l + nkeys * 0)
            xy_plt.set_title('dev: %d [Y / X] ' % k)
            xy_plt.plot(data[k][X], data[k][Y], label = name, linewidth=1)
            xy_plt.legend(loc='upper right')
            xy_plt.set_aspect('equal', 'datalim')

            xz_plt = plt.subplot(3, nkeys, l + nkeys * 1)
            xz_plt.set_title('dev: %d [Z / X]' % k)
            xz_plt.plot(data[k][X], data[k][Z], label=name, linewidth=1)
            xz_plt.legend(loc='upper right')
            xz_plt.set_aspect('equal', 'datalim')

            yz_plt = plt.subplot(3, nkeys, l + nkeys * 2)
            yz_plt.set_title('dev: %d [Z / Y]' % k)
            yz_plt.plot(data[k][Y], data[k][Z], label=name, linewidth=1)
            yz_plt.legend(loc='upper right')
            yz_plt.set_aspect('equal', 'datalim')

def main() :
    parser = argparse.ArgumentParser('Plot tum/tumx results')
    parser.add_argument('-p', '--plane', action='store_true', help='Plane projected display mode')
    parser.add_argument('-x', '--axis', action='store_true', help='Axis display mode')
    parser.add_argument('-a', '--angles', action='store_true', help='Angles display mode')
    parser.add_argument('-3', '--three', action='store_true', help='3D display mode')
    parser.add_argument('-t', '--tum', action='store_true', help='Read as tum file')
    parser.add_argument('-n', '--sample-number', action='store_true', help='Use sample number for timeline')
    parser.add_argument('-o', '--output', help='output image filename')
    parser.add_argument('files', nargs='+', help='tum/tumx files')
    args = parser.parse_args(sys.argv[1:])

    plot_mode = PLOT_MODE_DIST
    if args.plane: plot_mode = PLOT_MODE_PROJECTIONS
    elif args.axis: plot_mode = PLOT_MODE_AXIS
    elif args.angles: plot_mode = PLOT_MODE_ANGLES
    elif args.three: plot_mode = PLOT_MODE_3D

    fig = plt.Figure()
    datasets = []
    for f in args.files:
        name, data = read_tum(f, args)
        datasets.append((name, data))
    nitems = 0
    for d in datasets:
        if len(d[1].keys()) > nitems: nitems = len(d[1].keys())

    for dataset in datasets:
        plot_tum(nitems, dataset[0], dataset[1], fig, plot_mode = plot_mode)
    if args.output == None:
        plt.show()
    else:
        output = args.output
        ext = os.path.splitext(output)
        print 'ext:', ext[1]
        if ext[1] == '': output += '.png'
        plt.savefig(output)

if __name__ == '__main__':
    main()
