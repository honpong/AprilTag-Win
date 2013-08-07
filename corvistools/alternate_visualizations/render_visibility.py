# Created by Eagle Jones and Jordan Miller
# Copyright (c) 2012. RealityCap, Inc.
# All Rights Reserved.

from OpenGL.GL import *
import cor

class render_visibility:
    def __init__(self):
        self.feature_ids = list()
        self.feature_positions = list()
        self.visible_points = dict()
        self.sightlines = list()

    def receive_packet(self, packet):
        if packet.header.type == cor.packet_filter_feature_id_visible:
            cur_points = cor.packet_filter_feature_id_visible_t_features(packet)
            for point in cur_points:
                if self.visible_points.has_key(point) == False:
                    self.visible_points[point] = list()
                self.visible_points[point].append(packet.T)
    
        if packet.header.type == cor.packet_filter_feature_id_association:
            cur_ids = cor.packet_filter_feature_id_association_t_features(packet)
            self.feature_ids.extend(cur_ids)
            for id in cur_ids:
                self.add_sightlines(id)
                    
        if packet.header.type == cor.packet_filter_reconstruction:
            self.feature_positions.extend(cor.packet_filter_reconstruction_t_points(packet))

    def render(self):
        glBegin(GL_LINES)
        glColor4f(1., 1., 1., .01)
        for line in self.sightlines:
            glVertex3f(line[0][0],line[0][1],line[0][2])
            glVertex3f(line[1][0],line[1][1],line[1][2])
        glEnd()

    def add_sightlines(self, feature_id):
        feature_index = self.feature_ids.index(feature_id)
        feature_position = self.feature_positions[feature_index]
        
        for position in self.visible_points[feature_id]:
            self.sightlines.append([position, feature_position])
