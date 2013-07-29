# Created by Eagle Jones
# Copyright (c) 2012. RealityCap, Inc.
# All Rights Reserved.

import time
seconds = 16.
fps = 60.
steps = int(seconds*fps)

class extents:
    def __init__(self, points, trans, thresh):
        newpoints = ones((points.shape[0], 4))
        newpoints[:,0:3] = points
        transpoints = dot(newpoints, trans)
        x = sort(transpoints[:,0])
        y = sort(transpoints[:,1])
        z = sort(transpoints[:,2])

        x0 = x[thresh]
        y0 = y[thresh]
        z0 = z[thresh]

        x1 = x[-1-thresh]
        y1 = y[-1-thresh]
        z1 = z[-1-thresh]

        self.dx = abs(x1-x0)
        self.dy = abs(y1-y0)
        self.dz = abs(z1-z0)

        vertices = ones((8,4))
        vertices[0] = [x0, y0, z0, 1.]
        vertices[1] = [x0, y1, z0, 1.]
        vertices[2] = [x1, y1, z0, 1.]
        vertices[3] = [x1, y0, z0, 1.]
        vertices[4] = [x0, y0, z1, 1.]
        vertices[5] = [x0, y1, z1, 1.]
        vertices[6] = [x1, y1, z1, 1.]
        vertices[7] = [x1, y0, z1, 1.]
        
        self.vertices = dot(vertices, trans.T)

        labelloc = (self.vertices[1] + self.vertices[2]) * .5;
        self.textx = "%0.2fm" % self.dx
        self.labelx = renderable.label("/Library/Fonts/Tahoma.ttf", self.textx, 20.)
        self.labelx.origin = labelloc[:3]

        labelloc = (self.vertices[2] + self.vertices[3]) * .5;
        self.texty = "%0.2fm" % self.dy
        self.labely = renderable.label("/Library/Fonts/Tahoma.ttf", self.texty, 20.)
        self.labely.origin = labelloc[:3]

        labelloc = (self.vertices[3] + self.vertices[7]) * .5;
        self.textz = "%0.2fm" % self.dz
        self.labelz = renderable.label("/Library/Fonts/Tahoma.ttf", self.textz, 20.)
        self.labelz.origin = labelloc[:3]

def do_rotation(thetax, thetay, thetaz):
    dr = numerics.rodrigues([thetax / steps, thetay / steps, thetaz / steps, 0.], None)
    for i in xrange(steps):
        myvis.frame_1.render_widget.view_transform = dot(myvis.frame_1.render_widget.view_transform, dr)
        time.sleep(1./fps)

def restore_rotation(old):
    vn = myvis.frame_1.render_widget.view_transform
    old1 = eye(4)
    old1[:3,:3] = old[:3,:3]
    vn1 = eye(4)
    vn1[:3,:3] = vn[:3,:3]
    dr = numerics.rodrigues(numerics.invrodrigues(dot(vn1.T, old1), None) / steps, None)

    for i in xrange(steps):
        myvis.frame_1.render_widget.view_transform = dot(myvis.frame_1.render_widget.view_transform, dr)
        time.sleep(1./fps)
    myvis.frame_1.render_widget.view_transform = old

def fade_color(object, color):
    delta = (color - object.color[3]) / steps
    for i in xrange(steps):
        object.color[3] += delta
        time.sleep(1./fps)
    object.color[3] = color

vt = myvis.frame_1.render_widget.view_transform
zf = myvis.frame_1.render_widget.zoomfactor
dz = (150. - zf) / steps

#fade out structure
fade_color(structure, 0.)

#get path
path = motion.get_path()
pathext = extents(path, vt, 0)

extbound = renderable.bounding_box(pathext.vertices[0], pathext.vertices[1], pathext.vertices[2], pathext.vertices[3], pathext.vertices[4], pathext.vertices[5], pathext.vertices[6], pathext.vertices[7]);
extbound.color = [1., 1., 1., .25]
extbound.show_faces = False
pathext.labelz.color[3] = 0.
myvis.frame_1.render_widget.add_renderable(pathext.labelx.render, "Path extents X label");
myvis.frame_1.render_widget.add_renderable(pathext.labely.render, "Path extents Y label");
myvis.frame_1.render_widget.add_renderable(pathext.labelz.render, "Path extents Z label");

myvis.frame_1.render_widget.add_renderable(extbound.render, "Path extents bounding box")

#rotate, showing motion only
fade_color(pathext.labely, 0.)
do_rotation(-pi / 2., 0., 0.)
fade_color(pathext.labelz, 1.)
fade_color(pathext.labelx, 0.)
do_rotation(0., pi / 2., 0.)
fade_color(pathext.labely, 1.)
restore_rotation(vt)
fade_color(pathext.labelx, 1.)

extbound.color[3] = 0.
pathext.labelx.color[3] = 0.
pathext.labely.color[3] = 0.
pathext.labelz.color[3] = 0.

fade_color(structure, .5)
fade_color(motion, 0.)

#rotate, showing structure only
do_rotation(-pi / 2., 0., 0.)
do_rotation(0., pi / 2., 0.)
restore_rotation(vt)

fade_color(motion, 1.)

features = structure.get_features()

thresh = 6

featpath = concatenate((features, path))
ext = extents(featpath, vt, thresh)

bound = renderable.bounding_box(ext.vertices[0], ext.vertices[1], ext.vertices[2], ext.vertices[3], ext.vertices[4], ext.vertices[5], ext.vertices[6], ext.vertices[7]);

ext.labelz.color[3] = 0.
myvis.frame_1.render_widget.add_renderable(ext.labelx.render, "Feat path extents X label");
myvis.frame_1.render_widget.add_renderable(ext.labely.render, "Feat path extents Y label");
myvis.frame_1.render_widget.add_renderable(ext.labelz.render, "Feat path extents Z label");

bound.color = [1., 1., 1., 0.]
myvis.frame_1.render_widget.add_renderable(bound.render, "Feat path extents bounding box")

fade_color(bound, .25)

fade_color(ext.labely, 0.)
do_rotation(-pi / 2., 0., 0.)
fade_color(ext.labelz, 1.)
fade_color(ext.labelx, 0.)
do_rotation(0., pi / 2., 0.)
fade_color(ext.labely, 1.)
restore_rotation(vt)
fade_color(ext.labelx, 1.)

fade_color(boelter_map, 1.)
