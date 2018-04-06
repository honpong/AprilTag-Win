#!/usr/bin/python
import numpy as np

class Poses(object):
    def __init__(self, filename):
        # 130777180477627115    0.08218356   -0.99205852   -0.09521491  207.50582886   -0.88409311   -0.11667039    0.45251244   36.77946472   -0.46002755    0.04698976   -0.88666040 -2555.18896484        0.1901
        data = np.loadtxt(fname=filename, dtype={'names': ('t', 'SE3', '?'), 'formats': ('i8', '(3,4)f8', 'f8')}).view(np.recarray)
        self.t = data.t
        self.R = data.SE3[:,0:3,0:3]
        self.T_m = data.SE3[:,0:3,3] / 1000
        self.SE3 = np.append(data.SE3, np.array([[[0,0,0,1]]]  * len(data)), 1)
    def L_cm(self):
        return 100 * np.linalg.norm(self.T_m[0] - self.T_m[-1])
    def PL_cm(self):
        total_distance_m = 0
        last_position_m = self.T_m[0]
        for i in range(len(self.T_m)):
            delta_T_m = np.linalg.norm(self.T_m[i] - last_position_m)
            if delta_T_m > .01:
                total_distance_m += delta_T_m;
                last_position_m = self.T_m[i]
        return 100 * total_distance_m;

if __name__ == '__main__':
    import sys
    data = Poses(sys.argv[1])
    print "PL%d_L%d" % (data.PL_cm(), data.L_cm())
