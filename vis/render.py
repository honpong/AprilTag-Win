# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from wx.glcanvas import GLCanvas, GLContext
import wx
from OpenGL.GLUT import *
from OpenGL.GLU import *
from OpenGL.GL import *
import sys,math
from ArcBall import ArcBallT
import numpy
from math import cos,sin
import Mouse
from LockPaint import LockPaint
import cor

name = 'ball_glut'
from numpy import *

class MyGLCanvas(LockPaint, GLCanvas, Mouse.Wheel, Mouse.Drag):
    def __init__(self, *args, **kwds):
        super(MyGLCanvas, self).__init__(*args, **kwds)
        # wxpy doesn't use super, so
        Mouse.Wheel.__init__(self, *args, **kwds)
        Mouse.Drag.__init__(self, *args, **kwds)
        self.renderables = [self.DrawGrid]
        self.arcball = ArcBallT(self.GetClientSize())
        self.view_transform = numpy.identity(4, 'f')
	self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_IDLE, self.OnIdle)
        self.context = GLContext(self)
#        self.InitGL()
	self.init = 0
	return

    def OnIdle(self, event):
        self.Refresh()

    def OnRightDown(self, event):
        Mouse.Drag.OnRightDown(self, event)
        self.initial_rot = self.view_transform[:3,:3].copy()
        self.arcball.click(event.GetPositionTuple())

    def OnMotion(self, event):
        Mouse.Drag.OnMotion(self, event)
        if self.right_button:
            deltar = self.arcball.drag(event.GetPositionTuple())
            # Use correct Linear Algebra matrix multiplication C = A * B
            self.view_transform[:3,:3] = numpy.dot(self.initial_rot[:3,:3], deltar)
            self.Refresh(False)
        if self.left_button:
            self.view_transform[3, :2] -= self.deltapos / (-self.zoomfactor, self.zoomfactor)
            self.Refresh(False)
        

    def OnSize(self, event):
        self.arcball.setBounds(self.GetSize())
        self.init = 0

    def OnPaint(self,event):
        self.BeginPaint()
	dc = wx.PaintDC(self)
	self.SetCurrent(self.context)
        
	if not self.init:
	    self.InitGL()
	    self.init = 1
	self.OnDraw()
        self.EndPaint()

    def OnDraw(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glScale(self.zoomfactor, self.zoomfactor, 1.)
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glMultMatrixf(self.view_transform)
	color = [1.0,0.,0.,1.]
	#glMaterialfv(GL_FRONT,GL_DIFFUSE,color)

        for i in self.renderables:
            i()

        glMatrixMode(GL_MODELVIEW)
	glPopMatrix()
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()
        self.SwapBuffers()
        return
	
    def InitGL(self):
        # set viewing projection
        #if(!strstr((char *)glGetString(GL_RENDERER), "Intel")) {
        #    glEnable(GL_LINE_STIPPLE);
        #    glEnable(GL_LINE_SMOOTH);
        #    glEnable(GL_POLYGON_SMOOTH);
        #    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
        #    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
        #}
        glEnable(GL_POINT_SMOOTH);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

        glEnable(GL_LINE_SMOOTH);
        glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        width, height = self.GetClientSize()
        glViewport (0, 0, width, height)
        glOrtho(-width/2., width/2., -height/2., height/2., 10000., -10000.);

        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

        return
        
    def DrawGrid(self):
        scale = 1
        glBegin(GL_LINES)
        glColor3f(.2, .2, .2)
        for x in xrange(-10*scale, 11*scale, scale):
            glVertex3f(x, -10*scale, 0)
            glVertex3f(x, 10*scale, 0)
            glVertex3f(-10*scale, x, 0)
            glVertex3f(10*scale, x, 0)
            glVertex3f(-0, -10*scale, 0)
            glVertex3f(-0, 10*scale, 0)
            glVertex3f(-10*scale, -0, 0)
            glVertex3f(10*scale, -0, 0)

            glVertex3f(0, -.1*scale, x)
            glVertex3f(0, .1*scale, x)
            glVertex3f(-.1*scale, 0, x)
            glVertex3f(.1*scale, 0, x)
            glVertex3f(0, -.1*scale, -x)
            glVertex3f(0, .1*scale, -x)
            glVertex3f(-.1*scale, 0, -x)
            glVertex3f(.1*scale, 0, -x)
        glColor3f(1., 0., 0.)
        glVertex3f(0, 0, 0)
        glVertex3f(1, 0, 0)
        glColor3f(0., 1., 0.)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 1, 0)
        glColor3f(0., 0., 1.)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 0, 1)
        glEnd()
        
#write and hook up ui for functions to switch between perspective and ortho, get rid of clipping plane, and change render color
    def packet_world(self, packet):
        if packet.header.type == cor.packet_filter_reconstruction:
            self.world = cor.packet_filter_reconstruction_t_points(packet)

    def DrawCurrent(self):
        try:
            world = self.world
            #Tref = self.rf.state.Tref
        except:
            return
        glBegin(GL_POINTS)
        glColor3f(0., 0., 1.)
        for x,y,z in world:
            glVertex3f(x, y, z)
        #glColor3f(0., 1., 1.)
        #for i in xrange(0,Tref.size,3):
        #    glVertex(Tref[i], Tref[i+1], Tref[i+2])
        glEnd()

def main():
    app = wx.PySimpleApp()
    frame = wx.Frame(None,-1,'ball_wx',wx.DefaultPosition,(400,400))
    canvas = myGLCanvas(frame)
    glutInit()
    frame.Show()
    app.MainLoop()

if __name__ == '__main__': main()
