# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

import wx
import cor
from vis_gui import vis_gui
import threading

class Vis:
    def __init__(self):
        self.app = wx.App(0)
        wx.InitAllImageHandlers()
        self.frame_1 = vis_gui(None, -1, "")
        self.app.SetTopWindow(self.frame_1)
        self.frame_1.Show()
        #self.thread = threading.Thread(target = self.app.MainLoop)
        #self.thread.start()

    def show_filter_reconstruction(self, packet):
        print "packet"
        if packet.type != cor.packet_filter_reconstruction:
            return
        print "showing reconstruction"
        self.frame_1.window_2.structure = packet.data
        self.frame_1.window_2.Refresh(False)
        
    def stop(self):
        pass
