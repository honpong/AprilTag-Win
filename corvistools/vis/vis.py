# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

import wx
from vis_gui import vis_gui

class Vis:
    def __init__(self):
        self.app = wx.App(False)
        self.frame = vis_gui(None, -1, "")
        self.app.SetTopWindow(self.frame)
        self.frame.Show()

    def stop(self):
        pass
