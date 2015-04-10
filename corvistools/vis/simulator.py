from OpenGL.GL import *
from corvis import cor
from numpy import pi

class simulator_ground_truth:
    def __init__(self):
        self.positions = list()
        self.rotations = list()
        self.velocities = list()

    def receive_packet(self, packet):
        if packet.header.type == cor.packet_ground_truth:
           self.positions.append(packet.T[:])
           self.rotations.append(packet.rotation[:])

    def render(self):
        if len(self.rotations) == 0:
            return

        glBegin(GL_LINE_STRIP)
        glColor4f(1., 1., 1., 1)
        for p in self.positions:
            glVertex3f(p[0],p[1],p[2])
        glEnd()
        
        glPushMatrix()
        glPushAttrib(GL_ENABLE_BIT)

        glEnable(GL_LINE_STIPPLE)
        axis = self.rotations[-1][:3]
        angle = self.rotations[-1][3] * 180. / pi
        glTranslatef(self.positions[-1][0], self.positions[-1][1], self.positions[-1][2]);
        glRotatef(angle, axis[0], axis[1], axis[2])
        glLineStipple(3, 0xAAAA)
        glBegin(GL_LINES)
        glColor3f(1, 0, 0)
        glVertex3f(0, 0, 0)
        glVertex3f(2, 0, 0)

        glColor3f(0, 1, 0)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 2, 0)
          
        glColor3f(0, 0, 1)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 0, 2)
        glEnd()

        glPopAttrib()
        glPopMatrix()
