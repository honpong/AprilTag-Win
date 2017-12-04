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
import sys
import os
import math
import argparse

TIME = 0
X = 1
Y = 2
Z = 3
D = 4

# plot modes
PLOT_MODE_AXIS = 0 # X,Y,Z trajectories
PLOT_MODE_DIST = 1 # distance from origin
PLOT_MODE_PROJECTIONS = 2 # plane projections

def read_tum(filename) :
    '''
    Read tum file, return a generic name and dictinary of the form:
    d[device-id] = { TIME: [] X: [] Y: [] Z: [] D: [] }
    '''
    d = {}
    name = os.path.basename(filename)
    name = os.path.splitext(name)[0]
    fp = open(filename)
    lc = 0
    for l in fp.readlines():
        lc += 1
        data = l.split()
        if len(data) < 4:
            print '%s: misformatted line at %d' % (filename, lc)
            continue
        device_id = 0
        if len(data) > 8:
            device_id = int(data[8])
        if (device_id in d.keys()) == False:
            d[device_id] = { TIME: [], X:[], Y:[], Z:[], D:[] }

        d[device_id][TIME].append( float(data[0]))
        x = float(data[1])
        y = float(data[2])
        z = float(data[3])
        dist = math.sqrt(x * x + y * y + z * z)
        d[device_id][X].append( x )
        d[device_id][Y].append( y )
        d[device_id][Z].append( z )
        d[device_id][D].append( dist )

    return name, d

def plot_tum(nkeys, name, data, fig, plot_mode = PLOT_MODE_AXIS) :
    l = 0
    for k in data.keys():
        l += 1
        if plot_mode == PLOT_MODE_AXIS or plot_mode == PLOT_MODE_DIST:
            xy_plt = plt.subplot(nkeys, 1, l)
            if plot_mode == PLOT_MODE_AXIS:
                xy_plt.plot(data[k][TIME], data[k][X], label="X_%s_%d" % (name, k))
                xy_plt.plot(data[k][TIME], data[k][Y], label="Y_%s_%d" % (name, k))
                xy_plt.plot(data[k][TIME], data[k][Z], label="Z_%s_%d" % (name, k))
            else:
                xy_plt.plot(data[k][TIME], data[k][D], label="%s_%d" % (name, k))

            xy_plt.legend(loc='upper right')
        else:
            xy_plt = plt.subplot(3, nkeys, l + nkeys * 0)
            xy_plt.set_title('dev: %d [Y / X] ' % k)
            xy_plt.plot(data[k][X], data[k][Y], label = name)
            xy_plt.legend(loc='upper right')
            xy_plt.set_aspect('equal', 'datalim')

            xz_plt = plt.subplot(3, nkeys, l + nkeys * 1)
            xz_plt.set_title('dev: %d [Z / X]' % k)
            xz_plt.plot(data[k][X], data[k][Z], label=name)
            xz_plt.legend(loc='upper right')
            xz_plt.set_aspect('equal', 'datalim')

            yz_plt = plt.subplot(3, nkeys, l + nkeys * 2)
            yz_plt.set_title('dev: %d [Z / Y]' % k)
            yz_plt.plot(data[k][Y], data[k][Z], label=name)
            yz_plt.legend(loc='upper right')
            yz_plt.set_aspect('equal', 'datalim')


def main() :
    parser = argparse.ArgumentParser('Plot tum/tumx results')
    parser.add_argument('-p', '--plane', action='store_true', help='Plane projected display mode')
    parser.add_argument('-x', '--axis', action='store_true', help='Axis display mode')
    parser.add_argument('-o', '--output', help='output image filename')
    parser.add_argument('files', nargs='+', help='tum/tumx files')
    args = parser.parse_args(sys.argv[1:])

    plot_mode = PLOT_MODE_DIST
    if args.plane: plot_mode = PLOT_MODE_PROJECTIONS
    elif args.axis: plot_mode = PLOT_MODE_AXIS

    fig = plt.Figure()
    datasets = []
    for f in args.files:
        name, data = read_tum(f)
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
