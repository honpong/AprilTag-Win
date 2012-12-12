# Created by Eagle Jones and Jordan Miller
# Copyright (c) 2012. RealityCap, Inc.
# All Rights Reserved.

import time
seconds = 8.
fps = 800.
steps = int(seconds*fps)

def bounding_box(path):
    mimimum_indexes = argmin(path,axis=0)
    maximum_indexes = argmax(path,axis=0)
    bounding_box = array([path[mimimum_indexes[0]], path[mimimum_indexes[1]], path[maximum_indexes[0]], path[maximum_indexes[1]]])
    return bounding_box

def lengths(bb):
    length = array([ ((bb[0][0]-bb[1][0])**2+(bb[0][1]-bb[1][1])**2)**.5,
                    ((bb[1][0]-bb[2][0])**2+(bb[1][1]-bb[2][1])**2)**.5,
                    ((bb[2][0]-bb[3][0])**2+(bb[2][1]-bb[3][1])**2)**.5,
                    ((bb[3][0]-bb[0][0])**2+(bb[3][1]-bb[0][1])**2)**.5 ])
    print length
    return length

class extents:
    def __init__(self, points, trans, thresh):
        newpoints = ones((points.shape[0], 4))
        newpoints[:,0:3] = points
        transpoints = dot(newpoints, trans)
        x = sort(transpoints[:,0])
        y = sort(transpoints[:,1])
        z = sort(transpoints[:,2])

        bb = bounding_box(points)

        print bb
        
        lngths = lengths(bb)
        
        self.dx = lngths[0]
        self.dy = lngths[1]
        self.dz = 0

        
        
        vertices = ones((8,4))
        vertices[0] = [bb[0][0], bb[0][1], 0, 1.]
        vertices[1] = [bb[0][0], bb[0][1], 0, 1.]
        vertices[2] = [bb[1][0], bb[1][1], 0, 1.]
        vertices[3] = [bb[1][0], bb[1][1], 0, 1.]
        vertices[6] = [bb[2][0], bb[2][1], 0, 1.]
        vertices[7] = [bb[2][0], bb[2][1], 0, 1.]
        vertices[4] = [bb[3][0], bb[3][1], 0, 1.]
        vertices[5] = [bb[3][0], bb[3][1], 0, 1.]
        
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

#get path
path = motion.get_path()
pathext = extents(path, vt, 0)

extbound = renderable.bounding_box(pathext.vertices[0], pathext.vertices[1], pathext.vertices[2], pathext.vertices[3], pathext.vertices[4], pathext.vertices[5], pathext.vertices[6], pathext.vertices[7]);
extbound.color = [1., 1., 1., .25]
extbound.show_faces = False
pathext.labelz.color[3] = 0.
myvis.frame_1.render_widget.renderables.append(pathext.labelx.render);
myvis.frame_1.render_widget.renderables.append(pathext.labely.render);
myvis.frame_1.render_widget.renderables.append(pathext.labelz.render);

myvis.frame_1.render_widget.renderables.append(extbound.render)

#rotate, showing motion only
fade_color(pathext.labely, 0.)
do_rotation(-pi / 2., 0., 0.)
fade_color(pathext.labelx, 0.)
do_rotation(0., pi / 2., 0.)
fade_color(pathext.labely, 1.)
restore_rotation(vt)
fade_color(pathext.labelx, 1.)

extbound.color[3] = 0.
pathext.labelx.color[3] = 0.
pathext.labely.color[3] = 0.
pathext.labelz.color[3] = 0.




