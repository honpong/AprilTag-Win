# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

import wx
from corvis import cor
import numpy
import Mouse
from LockPaint import LockPaint
from Queue import *

def parsepgm(header):
    p5, blank, width, height, ignore = header.split(' ');
    width = int(width)
    height = int(height)
    return width, height

class PacketQueue(Queue):
    def __init__(self, ptype):
        Queue.__init__(self, 0)
        self.ptype = ptype
        self.latest_time = 0
        self.current_time = 0

    def put(self, packet):
        if packet.header.type == self.ptype:
            self.latest_time = packet.header.time
            Queue.put(self, packet)

    def get(self):
        self.current = Queue.get(self, False)
        self.current_time = self.current.header.time
        return self.current

    def get_time(self, time):
        while self.current_time < time:
            self.get()
        if self.current_time == time:
            return self.current
        else:
            raise Empty, "nothing for that time"

class Overlay:
    def __init__(self, ptype):
        self.time = 0
        self.queue = PacketQueue(ptype)

    def draw(self, dc, time):
        if time != self.time:
            try:
                self.packet = self.queue.get_time(time)
                self.time = time
                self.process_packet(self.packet)
            except Empty:
                return
        if time != 0:
            self.do_draw(dc)

    def get_latest(self):
        return self.queue.latest_time

class FeatureOverlay(Overlay):
    def __init__(self):
        Overlay.__init__(self, cor.packet_feature_track)
        self.status_queue = PacketQueue(cor.packet_feature_status)
        self.pred_queue = PacketQueue(cor.packet_feature_prediction_variance)

    def get_latest(self):
        if(self.status_queue.latest_time):
            return min(self.queue.latest_time, self.status_queue.latest_time)
        else:
            return self.queue.latest_time

#    def draw(self, dc, time):
        #render
#        pred = numpy.array(self.rf.prediction.points.reshape([-1,2]),dtype=numpy.float32)
#        cor.calibration_denormalize_rectified(self.cal, pred, pred, pred.shape[0])
#        for f in xrange(pred.shape[0]):
#            try:
#                if self.rf.outlier.rho[f]:
#                    dc.SetPen(wx.Pen('RED', 1, wx.SOLID))
#                    dc.SetBrush(wx.Brush('RED', wx.TRANSPARENT))
#                else:
#                    dc.SetPen(wx.Pen('GREEN', 1, wx.SOLID))
#                    dc.SetBrush(wx.Brush('GREEN', wx.TRANSPARENT))
#                dc.DrawCirclePoint(pred[f], 1.5)
#            except:
#                pass

    def process_packet(self, packet):
        feats = cor.packet_feature_track_t_features(packet)
        goodm = feats[:,0] != numpy.inf
        if not goodm.shape[0]:
            return
        self.feats = feats[goodm]
        try:
            status = cor.packet_feature_status_t_status(self.status_queue.get_time(self.time))
        except:
            status = numpy.ones(goodm.shape[0]);
        self.status = status[goodm]

        try:
            pred =  cor.packet_feature_prediction_variance_t_variance(self.pred_queue.get_time(self.time))
        except:
            pred = numpy.ones((goodm.shape[0], 5));
        self.pred = pred[goodm]

    colormap = { 0: 'RED', 1: 'GREEN', 2: 'YELLOW' }

    def do_draw(self, dc):
        gc = wx.GraphicsContext.Create(dc);
        try:
            feats = self.feats
            pred = self.pred
        except:
            return
        for f in xrange(feats.shape[0]):
            color = self.colormap[self.status[f]]
            gc.SetPen(wx.Pen(color, 1, wx.SOLID))
            gc.SetBrush(wx.Brush(color, wx.TRANSPARENT))
            x = feats[f][0]
            y = feats[f][1]
            gc.DrawLines(((x-2, y), (x+2, y)))
            gc.DrawLines(((x, y-2), (x, y+2)))
            gc.PushState()
            gc.SetPen(wx.Pen('BLUE', 1, wx.SOLID))
            gc.SetBrush(wx.Brush('BLUE', wx.TRANSPARENT))
            w = pred[f][2]
            h = pred[f][3]
            gc.Translate(pred[f][0] , pred[f][1])
            gc.Rotate(pred[f][4])
            gc.DrawEllipse(-w/2, -h/2, w, h)
            gc.PopState()

class ImageOverlay(Overlay):
    def __init__(self):
        Overlay.__init__(self, cor.packet_camera)

    def process_packet(self, packet):
        image = cor.packet_camera_t_image(packet)
        try:
            self.color_ver[:] = numpy.repeat(image, 3)
        except AttributeError:
            self.color_ver = numpy.repeat(image, 3)
        self.bmp = wx.BitmapFromBuffer(image.shape[0], image.shape[1], self.color_ver)

    def do_draw(self, dc):
        dc.DrawBitmapPoint(self.bmp,(0,0))

import math

class ImagePanel(LockPaint, wx.Panel, Mouse.Wheel, Mouse.Drag):
    def __init__(self, *args, **kwds):
        super(ImagePanel, self).__init__(*args, **kwds)
        self.renderables = list()
        # wxpy doesn't use super, so
        self.origin = numpy.array([0,0])
        Mouse.Wheel.__init__(self, *args, **kwds)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_IDLE, self.OnIdle)

    def OnIdle(self, event):
        self.Refresh(True)
        wx.WakeUpIdle()

    def OnPaint(self, event):
        self.BeginPaint()
        dc = wx.AutoBufferedPaintDCFactory(self)
        # python can run out of memory or churn cpu forever if we go overboard
        if(self.zoomfactor > 4.0):
            self.zoomfactor = 4.0
        if(self.zoomfactor < .1):
            self.zoomfactor = .1
        scale = 2 ** math.floor(math.log(self.zoomfactor,2) + .5)
        dc.SetUserScale(scale, scale)
        dc.SetDeviceOriginPoint(self.origin)
        times = [i.get_latest() for i in self.renderables]
        try:
            latest = min([i for i in times if i != 0])
        except ValueError:
            self.EndPaint()
            return
        for i in self.renderables:
            i.draw(dc, latest)
        self.EndPaint()

    def OnMotion(self, event):
        Mouse.Drag.OnMotion(self, event)
        if(self.left_button):
            self.origin += self.deltapos
        
