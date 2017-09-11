#!/usr/bin/env python

import matplotlib.pyplot as plt
import sys
import os

TIME = 0
X = 1
Y = 2
Z = 3

def read_tum(filename) :
    '''
    Read tum file, return a generic name and dictinary of the form:
    d[device-id] = { timebase: [] point_x: [] point_y: [] point_z: [] }
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
            print 'misformatted line at %d' % lc
            continue
        device_id = 0
        if len(data) > 8:
            device_id = int(data[8])
        if (device_id in d.keys()) == False:
            print name, ': found new device id: ', device_id
            d[device_id] = { TIME: [], X:[], Y:[], Z:[] }

        d[device_id][TIME].append( float(data[0]))
        d[device_id][X].append( float( data[1]) )
        d[device_id][Y].append( float( data[2]) )
        d[device_id][Z].append( float( data[3]) )

    return name, d

def plot_tum(name, data, fig, plot_projections = False) :
    nkeys = len(data.keys())
    l = 0
    for k in data.keys():
        l += 1
        if plot_projections == False:
            xy_plt = plt.subplot(nkeys, 1, l)
            xy_plt.plot(data[k][TIME], data[k][X], label="X_%s_%d" % (name, k))
            xy_plt.plot(data[k][TIME], data[k][Y], label="Y_%s_%d" % (name, k))
            xy_plt.plot(data[k][TIME], data[k][Z], label="Z_%s_%d" % (name, k))
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
    files = []
    plot_projections = False
    for c in sys.argv[1:]:
        if c.strip() == '-p':
            plot_projections = True
        else:
            files.append(c.strip())
    if len(files) == 0:
        print 'usage: %s [-p] files' % sys.argv[0]
        sys.exit(1)
    fig = plt.Figure()
    for f in files:
        name, data = read_tum(f)
        plot_tum(name, data, fig, plot_projections = plot_projections)
    plt.show()

if __name__ == '__main__':
    main()
        
        



