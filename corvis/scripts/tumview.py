#!/usr/bin/env python

import matplotlib.pyplot as plt
import sys
import os

def plot_file(filename, fig, plot_projections = False) :
    name = os.path.basename(filename)
    name = os.path.splitext(name)[0]
    fp = open(filename)
    timebase = []
    point_x = []
    point_y = []
    point_z = []
    lc = 0
    timeoffset = -1.0
    for l in fp.readlines():
        lc += 1
        data = l.split()
        if len(data) < 4:
            print 'misformatted line at %d' % lc
            continue
        if timeoffset < 0.0:
            timeoffset = float(data[0])
            timebase.append(0.0)
        else:
            timebase.append(float(data[0]) - timeoffset)

        point_x.append( float( data[1]) )
        point_y.append( float( data[2]) )
        point_z.append( float( data[3]) )

    if plot_projections == False:
        xy_plt = plt.subplot(1, 1, 1)
        xy_plt.plot(timebase, point_x, label="X_" + name)
        xy_plt.plot(timebase, point_y, label="Y_" + name)
        xy_plt.plot(timebase, point_z, label="Z_" + name)
        xy_plt.legend(loc='upper right')

    else: # projects plot
        xy_plt = plt.subplot(3, 1, 1)
        xy_plt.set_title('Y over X')
        xy_plt.plot(point_x, point_y, label = name)
        xy_plt.legend(loc='upper right')
        xy_plt.set_aspect('equal', 'datalim')

        xz_plt = plt.subplot(3, 1, 2)
        xz_plt.set_title('Z over X projection')
        xz_plt.plot(point_x, point_z, label=name)
        xz_plt.legend(loc='upper right')
        xz_plt.set_aspect('equal', 'datalim')

        yz_plt = plt.subplot(3, 1, 3)
        yz_plt.set_title('Z over Y projection')
        yz_plt.plot(point_y, point_z, label=name)
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
        plot_file(f, fig, plot_projections = plot_projections)
    plt.show()

if __name__ == '__main__':
    main()
        
        



