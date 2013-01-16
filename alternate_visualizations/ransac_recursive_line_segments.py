# Created by Jordan Miller
# Copyright (c) 2013. RealityCap, Inc.
# All Rights Reserved.

sys.path.extend(["third_party_python_libraries/"])
sys.path.extend(["pylib/"])
import modified_ransac
from OpenGL.GL import *

def dist(a, b):
    return sqrt((a[0]-b[0])**2+(a[1]-b[1])**2)

class line_segment:
    def __init__(self,pnt1,pnt2, alpha=1):
        self.pnt1 = pnt1
        self.pnt2 = pnt2
        self.alpha = alpha
    def render(self):
        glBegin(GL_LINES)
        glColor4f(1., 0., 1., self.alpha)
        glVertex3f(self.pnt1[0],self.pnt1[1],self.pnt1[2])
        glVertex3f(self.pnt2[0],self.pnt2[1],self.pnt2[2])
        glEnd()

def distnace_to_line(a, b, c): # a, b define line, c is point measured. 

    t = b[0]-a[0], b[1]-a[1]           # Vector ab
    dd = sqrt(t[0]**2+t[1]**2)         # Length of ab
    t = t[0]/dd, t[1]/dd               # unit vector of ab
    n = -t[1], t[0]                    # normal unit vector to ab
    ac = c[0]-a[0], c[1]-a[1]          # vector ac
    return fabs(ac[0]*n[0]+ac[1]*n[1]) # Projection of ac to n (the minimum distance)


def distance_to_line_segment(a, b, c): # a = segment_start_point, b = segment_end_point, c = measured_point_coordinates
    r_numerator = (c[0]-a[0])*(b[0]-a[0]) + (c[1]-a[1])*(b[1]-a[1]);
    r_denomenator = (b[0]-a[0])*(b[0]-a[0]) + (b[1]-a[1])*(b[1]-a[1]);
    r = r_numerator / r_denomenator;

    px = a[0] + r*(b[0]-a[0]);
    py = a[1] + r*(b[1]-a[1]);

    s =  ((a[1]-c[1])*(b[0]-a[0])-(a[0]-c[0])*(b[1]-a[1]) ) / r_denomenator;
        
    distanceLine = fabs(s)*sqrt(r_denomenator);

    #(xx,yy) is the point on the lineSegment closest to (c[0],c[1])

    xx = px;
    yy = py;
    
    if (r >= 0) and (r <= 1):
        distanceSegment = distanceLine;
    else:
        dist1 = (c[0]-a[0])*(c[0]-a[0]) + (c[1]-a[1])*(c[1]-a[1]);
        dist2 = (c[0]-b[0])*(c[0]-b[0]) + (c[1]-b[1])*(c[1]-b[1]);
        if dist1 < dist2 :
            xx = a[0];
            yy = a[1];
            distanceSegment = sqrt(dist1);
        else:
            xx = b[0];
            yy = b[1];
            distanceSegment = sqrt(dist2);

    return distanceSegment




class line_segment_model:
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
        two_points = [[data[0][0],data[0][1]], [data[1][0],data[1][1]]]
        print two_points
        return two_points
    
    def get_error( self, data, model):
        #the minimum distance between the point and each of the four model lines will be the error
        
        error = ones(data.shape[0]).dot(10000) #initialize our 'error' array with 1000 meteres error
        for i, pnt in enumerate(data):
            dist = distance_to_line_segment( model[0], model[1], pnt)
            if dist < error[i]:
                error[i] = dist**2
        
        return error




#get setup and exicute ransac run
debug = True

data = structure.get_features()[:,0:2]

for i in range(30):
    model = line_segment_model([], [], debug) #model = box_model(visibility.sightlines, motion.get_path(), debug)
    n = 2     #n - the minimum number of data values required to fit the model
    k = 1000  #k - the maximum number of iterations allowed in the algorithm
    t = .005   #t - a threshold value for determining when a data point fits a model
    d = max(int(len(data)*.03),10)
    
    ransac_fit, ransac_data = modified_ransac.modified_ransac(data, model, n, k, t, debug=debug)
    print 'ransac_fit', ransac_fit
    print 'number of inliers', len(ransac_data['inliers'])
    print 'inlier threshold count', d
    
    if len(ransac_data['inliers']) < d:
        break

    myvis.frame_1.render_widget.renderables.append(
                                                   line_segment(
                                                                [ransac_fit[0][0],ransac_fit[0][1], 0.],
                                                                [ransac_fit[1][0],ransac_fit[1][1], 0.],
                                                                max((1/(i+1)**.5),.1)
                                                                ).render
                                                   )

    data = delete(data, ransac_data['inliers'], axis=0) #remove the inliers for the next iteration of our model

#show output of ransac
#ransac_fit_render_lines = []
    #for i, ln in enumerate(ransac_fit):
    #ransac_fit_render_lines.append(line_segment([ln[0][0],ln[0][1], 0.], [ln[1][0],ln[1][1], 0.]))
#myvis.frame_1.render_widget.renderables.append(ransac_fit_render_lines[i].render)