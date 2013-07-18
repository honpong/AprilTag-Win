# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *
from rodrigues import *
from corvis import cor
#from IPython.Debugger import Tracer
#debug_here = Tracer()
# this should be a fairly smooth acceleration to constant velocity

imu_a_cov = 1.8e-1*0
imu_w_cov = 4.0e-4*0
vis_cov = 5.e-3*0
use_imu = False

def gen_T(time):
    x = -sin(time)
    y = sin(time*2)
    z = time + sin(time) * .1
    T = array([x,y,z])
    if time < 1:
        T *= 0
    elif time < 2:
        T *= time - 1
    return T

def gen_W(time):
    x = -sin(time/2.) * .1
    y = -sin(time/3.) * .1
    z = sin(time) * .1
    W = array([x,y,z])
    if time < 1:
        W *= 0
    elif time < 2:
        W *= time
    return W

class simulator:
    def __init__(self):
        self.depthrange = 5.
        self.T = zeros(3)
        self.V = zeros(3)
        self.a = zeros(3)
        self.da = zeros(3)
        self.g = zeros(3)
        self.g[2] = 9.8
        self.W = zeros(3)
        self.w = zeros(3)
        self.dw = zeros(3)
        self.time = 0.
        self.dt = .01
        self.dv = .033
        self.nextvis = 0.
        self.feats = list()

    def tick(self, dt):
        self.time += dt

        oldT = self.T.copy()
        self.T = gen_T(self.time)
        oldV = self.V.copy()
        self.V = (self.T - oldT) / dt
        self.a = (self.V - oldV) / dt

        oldR = rodrigues(self.W)
        self.W = gen_W(self.time)
        self.R = rodrigues(self.W)
        r = dot(oldR.T, self.R)
        self.w = invrodrigues(r) / dt

        Wp = invrodrigues(dot(oldR, rodrigues(self.w * dt)))

    def vis_meas(self, T, W):
        feature_count = len(self.feats)

        np = cor.mapbuffer_alloc(self.trackbuf, cor.packet_feature_track, feature_count * 4 * 2)
        np.header.user = feature_count
        i = 0
        tracks = cor.packet_feature_track_t_features(np)
        for pt, depth, Tr, Rr in self.feats:
            rho = exp(depth)
            X0 = array([pt[0], pt[1], 1.]) * rho
            Xc = dot(self.Rc, X0) + self.Tc
            Xr = dot(Rr, Xc) + Tr
            Xb = dot(rodrigues(W).T, Xr - T)
            X = dot(self.Rc.T, Xb - self.Tc)
            x = X[:2]
            if X[2] <= 1.:
                x[:] = inf
            else:
                x /= X[2]
                if abs(x[0]) > 1. or abs(x[1]) > 1.:
                    x[:] = inf
                tracks[i] = x + random.randn(2) * vis_cov;
            i += 1

        cor.mapbuffer_enqueue(self.trackbuf, np, int(self.time * 1000000))

    def accel_meas(self):
        ip = cor.mapbuffer_alloc(self.imubuf, cor.packet_accelerometer, 3 * 4)
        ip.a[:] = dot(self.R.T, self.a + self.g) + random.randn(3) * imu_a_cov
        cor.mapbuffer_enqueue(self.imubuf, ip, int(self.time*1000000))

    def gyro_meas(self):
        ip = cor.mapbuffer_alloc(self.imubuf, cor.packet_gyroscope, 3 * 4)
        ip.w[:] = self.w + random.randn(3) * imu_w_cov
        cor.mapbuffer_enqueue(self.imubuf, ip, int(self.time*1000000))

    def imu_meas(self):
        ip = cor.mapbuffer_alloc(self.imubuf, cor.packet_imu, 6 * 4)
        ip.a[:] = dot(self.R.T, self.a + self.g) + random.randn(3) * imu_a_cov
        ip.w[:] = self.w + random.randn(3) * imu_w_cov
        cor.mapbuffer_enqueue(self.imubuf, ip, int(self.time*1000000))

    def run(self):
        while(1):
            if self.time + self.dt > self.nextvis:
                T = gen_T(self.nextvis)
                W = gen_W(self.nextvis)
                self.vis_meas(T, W)
                self.nextvis += self.dv
            self.tick(self.dt)
            if use_imu:
                self.imu_meas()
            else:
                self.accel_meas()
                self.gyro_meas()

    def addfeatures(self, count):
        pts = random.random([count, 2])-.5
        depths = random.random(count) * self.depthrange
        for i in xrange(count):
            self.feats.append([pts[i], depths[i], self.T.copy(), self.R.copy()])
            
        np = cor.mapbuffer_alloc(self.trackbuf, cor.packet_feature_select, count * 4 * 2)
        np.header.user = count
        tracks = cor.packet_feature_track_t_features(np)
        tracks[:] = pts
        cor.mapbuffer_enqueue(self.trackbuf, np, int(self.time * 1000000))

    def dropfeatures(self, count, ids):
        self.feats = [self.feats[i] for i in xrange(len(self.feats)) if i not in ids]
            
    def control_cb(self, packet):
        if packet.header.type == cor.packet_feature_select:
            self.addfeatures(packet.header.user)
        elif packet.header.type == cor.packet_feature_drop:
            self.dropfeatures(packet.header.user, cor.packet_feature_drop_t_indices(packet))



import csv

def read_meas(filename):
    f = open(filename)
    r = csv.reader(f)
    lines = []
    for line in r:
      floats = [float(item) for item in line]
      lines.append(floats)
    return lines


import time, os.path
class data_simulator:
    def __init__(self, datapath):
        accelfilename = os.path.join(datapath, 'accelerometer.csv')
        gyrofilename = os.path.join(datapath, 'gyro.csv')
        self.time = 0.
        self.accel = read_meas(accelfilename)
        self.gyro = read_meas(gyrofilename)
        self.calc_gravity(10)
        self.imubuf = None

    def calc_gravity(self, num_measurements):
        # alpha is calculated as t / (t + dT)
        # with t, the low-pass filter's time-constant
        # and dT, the event delivery rate
        alpha = 0.8
        gravity = array(self.accel[0][1:]);
        for i in range(num_measurements):
            gravity = alpha*gravity + (1 - alpha)*array(self.accel[i][1:])
        self.g = gravity

    def accel_meas(self, i):
        ip = cor.mapbuffer_alloc(self.imubuf, cor.packet_accelerometer, 3 * 4)
        self.time = self.accel[i][0]
        ip.a[:] = self.accel[i][1:]
        cor.mapbuffer_enqueue(self.imubuf, ip, int(self.time*1000000))

    def gyro_meas(self, i):
        ip = cor.mapbuffer_alloc(self.imubuf, cor.packet_gyroscope, 3 * 4)
        self.time = self.gyro[i][0]
        ip.w[:] = self.gyro[i][1:]
        cor.mapbuffer_enqueue(self.imubuf, ip, int(self.time*1000000))

    def run(self):
        for i in range(len(self.accel)):
            self.accel_meas(i)
            self.gyro_meas(i)
            time.sleep(0.01)
