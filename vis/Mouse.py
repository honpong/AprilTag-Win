# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

import wx, numpy

class Wheel:
    def __init__(self, *args, **kwds):
        # we should inherit from wx.EvtHandler here, but wxpy doesn't seem to do this properly
        # must be listed before the wxpy classes
        #super(Wheel, self).__init__(*args, **kwds)
        self.zoomfactor = 1.0
        self.zoomstep = .9
        self.Bind(wx.EVT_MOUSEWHEEL, self.OnMouseWheel)

    def OnMouseWheel(self, event):
        scroll = event.GetWheelRotation()
        if scroll < 0:
            self.zoomfactor *= self.zoomstep
        else:
            self.zoomfactor /= self.zoomstep
        self.Refresh(False)

class Drag:
    def __init__(self, *args, **kwds):
        # we should inherit from wx.EvtHandler here, but wxpy doesn't seem to do this properly
        # must be listed before the wxpy classes
        #super(Drag, self).__init__(*args, **kwds)
        self.right_button = False
        self.left_button = False
        self.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.Bind(wx.EVT_LEFT_UP, self.OnLeftUp)
        self.Bind(wx.EVT_RIGHT_DOWN, self.OnRightDown)
        self.Bind(wx.EVT_RIGHT_UP, self.OnRightUp)
        self.Bind(wx.EVT_MOTION, self.OnMotion)

    def OnLeftDown(self, event):
        self.lastpos = numpy.array(event.GetPositionTuple())
        self.left_button = True

    def OnRightDown(self, event):
        self.lastpos = numpy.array(event.GetPositionTuple())
        self.right_button = True

    def OnLeftUp(self, event):
        self.left_button = False

    def OnRightUp(self, event):
        self.right_button = False

    def OnMotion(self, event):
        if self.left_button or self.right_button:
            pos = numpy.array(event.GetPositionTuple())
            self.deltapos = pos - self.lastpos
            self.lastpos = pos
