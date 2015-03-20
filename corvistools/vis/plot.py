# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

import wx
import wx.aui
import matplotlib as mpl
from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg as Canvas
from matplotlib.backends.backend_wxagg import NavigationToolbar2Wx as Toolbar
import numpy
from animplot import animplot
#myEVT_TYPE = wx.NewEventType()
#EVT_MY_EVENT = wx.PyEventBinder(myEVT_TYPE, 1)
from corvis import cor
from LockPaint import LockPaint
import Mouse
EVT_CREATE_PLOT = wx.NewId()

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
        self.axes.set_position([.05,.05,.9,.9])
        self.canvas = Canvas(self, -1, self.figure)
        #self.toolbar = Toolbar(self.canvas)
        #self.toolbar.Realize()

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.canvas,1,wx.EXPAND)
        #sizer.Add(self.toolbar, 0 , wx.LEFT | wx.EXPAND)
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
    app = wx.PySimpleApp()
    frame = wx.Frame(None,-1,'Plotter')
    plotter = PlotNotebook(frame)
    axes1 = plotter.add('figure 1').gca()
    axes1.plot([1,2,3],[2,1,4])
    axes2 = plotter.add('figure 2').gca()
    axes2.plot([1,2,3,4,5],[2,1,4,2,3])
    #axes1.figure.canvas.draw()
    #axes2.figure.canvas.draw()
    frame.Show()
    app.MainLoop()

if __name__ == "__main__": demo()

