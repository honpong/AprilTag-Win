# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *
import matplotlib as mpl
import Queue
from corvis import cor
from bisect import *

class animplot(object):
    def __init__(self, name, nominal = 0., dpi=None):
        self.figure = mpl.figure.Figure(dpi=dpi, figsize=(2,2))
        self.axes = self.figure.gca()
        self.axes.set_color_cycle(("red", "green", "blue"))
        self.name = name
        self.xdata = list()
        self.miny = 0
        self.maxy = 0
        self.count = 0
        self.plots= list()
        self.posnomplot, = self.axes.plot((0, 0), (nominal, nominal), 'r--')
        self.negnomplot, = self.axes.plot((0, 0), (-nominal, -nominal), 'r--')

    def refresh(self,start, stop):
        self.miny = 0
        self.maxy = 0
        for p in self.plots:
            self.refreshhist(p,start,stop)
        self.posnomplot.set_xdata((start, stop))
        self.negnomplot.set_xdata((start, stop))
        self.axes.set_ylim(self.miny, self.maxy)
        self.axes.set_xlim(start, stop)

    def packet_plot(self, packet):
        if self.count < packet.count:
            for i in xrange(self.count, packet.count):
                self.plots.append(self.addhist())
            self.count = packet.count
        i = 0
        data = cor.packet_plot_t_data(packet)
        self.xdata.append(packet.header.time / 1000000.)
        for p in self.plots:
            if i >= packet.count:
                break
            p[0].append(data[i])
            i += 1

    def addhist(self):
        data = list()
        line, = self.axes.plot((0,0), (0,0))
        return (data, line)

    def refreshhist(self, plot, start, stop):
        ydata, line = plot
        left = bisect_left(self.xdata, start, 0, len(ydata))
        if(left > 0):
            left -= 1
        right = bisect(self.xdata, stop, 0, len(ydata))
        if(right < len(ydata)):
            right += 1
        xr = self.xdata[left:right]
        yr = ydata[left:right]
        line.set_data(xr, yr)
        if(len(yr)):
            self.miny = min(self.miny, min(yr))
            self.maxy = max(self.maxy, max(yr))

