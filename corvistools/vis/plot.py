# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text


import wx
from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg as Canvas
import numpy
from LockPaint import LockPaint
import Mouse
EVT_CREATE_PLOT = wx.NewId()

from numpy import *
import matplotlib as mpl
from bisect import *

if __name__ == "__main__":
    import sys
    sys.path.extend(['../'])

from corvis import cor

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
        if self.miny == self.maxy:
            self.maxy = 1
        self.posnomplot.set_xdata((start, stop))
        self.negnomplot.set_xdata((start, stop))
        buffer_area = (self.maxy - self.miny)*.05
        self.axes.set_ylim(self.miny-buffer_area, self.maxy+buffer_area)
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
            self.miny = min(self.miny, min(ma.masked_invalid(yr)))
            self.maxy = max(self.maxy, max(ma.masked_invalid(yr)))


class CreatePlotEvent(wx.PyEvent):
    def __init__(self, data):
        wx.PyEvent.__init__(self)
        self.SetEventType(EVT_CREATE_PLOT)
        self.data = data

class Plot(LockPaint, wx.Panel):
    def __init__(self, plot, *args, **kwds):
        super(Plot, self).__init__(*args, **kwds)
        self.plot = plot
        self.figure = plot.figure
        self.axes = plot.axes
        self.canvas = Canvas(self, -1, self.figure)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.canvas,1,wx.EXPAND)
        self.SetSizer(sizer)

    def update(self, start, stop):
        self.BeginPaint()
        self.plot.refresh(start, stop)
        self.figure.canvas.draw()
        self.EndPaint()

class PlotNotebook(wx.Panel, Mouse.Wheel, Mouse.Drag):
    def __init__(self, *args, **kwargs):
        wx.Panel.__init__(self, *args, **kwargs)
        Mouse.Wheel.__init__(self, *args, **kwargs)
        self.nb = wx.Notebook(self)
        sizer = wx.BoxSizer()
        sizer.Add(self.nb, 1, wx.EXPAND)
        self.SetSizer(sizer)
        self.Connect(-1, -1, EVT_CREATE_PLOT, self.OnCreatePlot)
        self.plots = dict()
        self.zoomfactor = 1.0
        self.origin = numpy.array([0,0])
        self.latest = 0.
        self.latest_update = 0.

    def add(self, name="plot", nominal = 0.):
        plot = animplot(name, nominal)
        wx.PostEvent(self, CreatePlotEvent(plot))
        return plot

    def plot_dispatch(self, packet):
        update = False
        if packet.header.type == cor.packet_plot:
            plot = self.plots[packet.header.user]
            self.latest = packet.header.time/1000000.
            plot.packet_plot(packet)
            #max 30Hz update
            if self.latest > self.latest_update + 1/30.:
                self.latest_update = self.latest
                update = True
        elif packet.header.type == cor.packet_plot_info:
            plot = self.add(packet.identity, packet.nominal)
            self.plots[packet.header.user] = plot
            update = True
        elif packet.header.type == cor.packet_plot_drop:
            plot = self.plots[packet.header.user]
            self.plots.pop(plot)
            update = True

        if update:
            page = self.nb.GetCurrentPage()
            if page is not None:
                stop = self.latest + self.origin[0];
                start = stop - 1./self.zoomfactor;
                page.update(start, stop)
            self.Update()
        
    def OnMotion(self, event):
        Mouse.Drag.OnMotion(self, event)
        if(self.left_button):
            self.origin += self.deltapos*zoomfactor
            print self.origin
            if(self.origin[0] > 0.):
                self.origin[0] = 0.

    def OnCreatePlot(self, event):
        page = Plot(event.data, self.nb)
        self.nb.AddPage(page, event.data.name)
#        plot = event.data
#        plot.axes = page.figure


def demo():
    app = wx.App(False)
    frame = wx.Frame(None,-1,'Plotter')
    plotter = PlotNotebook(frame)
    axes1 = plotter.add('figure 1').axes
    axes1.plot([1,2,3],[2,1,4])
    axes2 = plotter.add('figure 2').axes
    axes2.plot([1,2,3,4,5],[2,1,4,2,3])
    #axes1.figure.canvas.draw()
    #axes2.figure.canvas.draw()
    frame.Show()
    app.MainLoop()

if __name__ == "__main__": demo()

