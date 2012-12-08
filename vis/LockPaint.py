# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

import wx

# children must set self.LockPaint = False at the end of OnPaint

class LockPaint(object):
    def __init__(self, *args, **kwargs):
        self.Painting = False
        super(LockPaint, self).__init__(*args, **kwargs)
        #bind after everybody else has, so we can set the flag after painting is done

    def Refresh(self, *args, **kwargs):
        if self.Painting:
            return
        super(LockPaint, self).Refresh(*args, **kwargs)

    def BeginPaint(self):
        self.Painting = True

    def EndPaint(self):
        self.Painting = False
