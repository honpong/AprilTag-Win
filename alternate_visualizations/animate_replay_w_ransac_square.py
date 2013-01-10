# Created by Eagle Jones and Jordan Miller
# Copyright (c) 2013. RealityCap, Inc.
# All Rights Reserved.

sys.path.extend(["third_party_python_libraries/"])
import ransac
import time
seconds = 16.
fps = 60.
steps = int(seconds*fps)

def find_most_distance_in_each_quadrant(path):
    most_distant_points = [[0,0],[0,0],[0,0],[0,0]]
    for pnt in path:
        if pnt[0] >= 0 and pnt[1] >= 0:
            if (pnt[0]**2 + pnt[1]**2) > (most_distant_points[0][0]**2 + most_distant_points[0][1]**2):
                most_distant_points[0] = pnt
        elif pnt[0] >= 0 and pnt[1] < 0:
            if (pnt[0]**2 + pnt[1]**2) > (most_distant_points[1][0]**2 + most_distant_points[1][1]**2):
                most_distant_points[1] = pnt
        elif pnt[0] < 0 and pnt[1] >= 0:
            if (pnt[0]**2 + pnt[1]**2) > (most_distant_points[2][0]**2 + most_distant_points[2][1]**2):
                most_distant_points[2] = pnt
        elif pnt[0] < 0 and pnt[1] < 0:
            if (pnt[0]**2 + pnt[1]**2) > (most_distant_points[3][0]**2 + most_distant_points[3][1]**2):
                most_distant_points[3] = pnt
    return array(most_distant_points)


def bounding_box(path):
    mimimum_indexes = argmin(path,axis=0)
    maximum_indexes = argmax(path,axis=0)
    bounding_box = array([path[mimimum_indexes[0]], path[mimimum_indexes[1]], path[maximum_indexes[0]], path[maximum_indexes[1]]])
    return bounding_box


def distnace_to_line_segment(a, b, c): # a = segment_start_point, b = segment_end_point, c = measured_point_coordinates
    t = b[0]-a[0], b[1]-a[1]           # Vector ab
    dd = sqrt(t[0]**2+t[1]**2)         # Length of ab
    t = t[0]/dd, t[1]/dd               # unit vector of ab
    n = -t[1], t[0]                    # normal unit vector to ab
    ac = c[0]-a[0], c[1]-a[1]          # vector ac
    return fabs(ac[0]*n[0]+ac[1]*n[1]) # Projection of ac to n (the minimum distance)


class box_model:
    """
        This implements the inteface required for the Ransac implementation.
        
        the fit method returns a 'model' which is then input into the get_error method
        
        in this case the 'model' is a list of lines, which are meant to represent where walls are.
        they are defined as an array of x,y point pairs.
        
    """
    
    def __init__(self, sightlines, path, debug=False):
        self.path = path
        self.sightlines = sightlines
        self.debug = debug
    
    def fit(self, data):
        #bb = bounding_box(data)
        bb = find_most_distance_in_each_quadrant(data) #trying a new model generation technique
        if debug:
            print bb
        model =  [
                  [ [bb[0][0],bb[0][1]], [bb[1][0],bb[1][1]] ],
                  [ [bb[1][0],bb[1][1]], [bb[2][0],bb[2][1]] ],
                  [ [bb[2][0],bb[2][1]], [bb[3][0],bb[3][1]] ],
                  [ [bb[3][0],bb[3][1]], [bb[0][0],bb[0][1]] ],
                  ]
        
        return model
    
    def get_error( self, data, model):
        #the minimum distance between the point and each of the four model lines will be the error
        
        error = ones(data.shape[0]).dot(10000) #initialize our 'error' array with 1000 meteres error
        for i, pnt in enumerate(data):
            for line_segment in model:
                dist = distnace_to_line_segment( line_segment[0], line_segment[1], pnt)
                if dist < error[i]:
                    error[i] = dist
    
        return error


def fade_color(object, color):
    delta = (color - object.color[3]) / steps
    for i in xrange(steps):
        object.color[3] += delta
        time.sleep(1./fps)
    object.color[3] = color



#get setup and exicute ransac run
debug = True

data = structure.get_features()[:,0:2]
model = box_model([], [], debug) #model = box_model(visibility.sightlines, motion.get_path(), debug)
n = 25     #n - the minimum number of data values required to fit the model
k = 500   #k - the maximum number of iterations allowed in the algorithm
t = .05    #t - a threshold value for determining when a data point fits a model
d = int(.15 * data.shape[0])     #d - the number of close data values required to assert that a model fits well to data

ransac_fit, ransac_data = ransac.ransac(data, model, n, k, t, d, debug=debug,return_all=True)
print ransac_fit

vertices = []
vertices.append(array([ransac_fit[0][0][0], ransac_fit[0][0][1], 0, 1.]))
vertices.append(array([ransac_fit[0][0][0], ransac_fit[0][0][1], 0, 1.]))
vertices.append(array([ransac_fit[1][0][0], ransac_fit[1][0][1], 0, 1.]))
vertices.append(array([ransac_fit[1][0][0], ransac_fit[1][0][1], 0, 1.]))
vertices.append(array([ransac_fit[2][0][0], ransac_fit[2][0][1], 0, 1.]))
vertices.append(array([ransac_fit[2][0][0], ransac_fit[2][0][1], 0, 1.]))
vertices.append(array([ransac_fit[3][0][0], ransac_fit[3][0][1], 0, 1.]))
vertices.append(array([ransac_fit[3][0][0], ransac_fit[3][0][1], 0, 1.]))
                                  

#show output of ransac
extbound = renderable.bounding_box(vertices[0], vertices[1], vertices[2], vertices[3], vertices[4], vertices[5], vertices[6], vertices[7]);
extbound.color = [1., 1., 1., .25]
extbound.show_faces = False
myvis.frame_1.render_widget.renderables.append(extbound.render)
fade_color(extbound, .25)
