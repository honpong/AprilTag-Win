# Created by Eagle Jones and Jordan Miller
# Copyright (c) 2012. RealityCap, Inc.
# All Rights Reserved.

from OpenGL.GL import *
import cor

class render_visibility:
    def __init__(self):
        self.feature_id = list()
        self.feature_position = list()

    def receive_packet(self, packet):
        if packet.header.type == cor.packet_filter_feature_id_visible:
            print "got a visible packet"
            feature_id = cor.packet_filter_feature_id_visible_t_features(packet)
            print feature_id[0]
        if packet.header.type == cor.packet_filter_feature_id_association:
            print "got a association packet"
            feature_id = cor.packet_filter_feature_id_association_t_features(packet)
            print feature_id[0]
        if packet.header.type == cor.packet_filter_reconstruction:
            print "got a reconstruction packet"
            points = cor.packet_filter_reconstruction_t_points(packet)
            print points[0][0]

    def render(self):
        glBegin(GL_LINES)
        glColor4f(1., 1., 1., .25)
        glVertex3f(0.,0.,0.)
        glVertex3f(10000., 10000., 0.)
        glEnd()
        
