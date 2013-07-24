from imusim.all import *
import csv
import numpy
import os


def write_measurements(filename, obj, fromt = None, to = None):
    f = open(filename, 'w')
    w = csv.writer(f)
    (dims, measurements) = obj.values.shape
    for m in range(measurements):
        timestamp = [obj.timestamps[m]]
        measurement = obj.values[:,m]
        timestamp.extend(measurement)
        if fromt and timestamp[0] < fromt: continue
        if to and timestamp[0] > to: continue
        if fromt: timestamp[0] -= fromt
        w.writerow(timestamp)

def quaternion_to_axisangle(fxn, t):
    sample = fxn(t)
    value = numpy.zeros((4,1))
    if not (sample.w == 1 or math.isnan(sample.w)):
        aa = sample.toAxisAngle()
        for i in range(3): # axis
            value[i] = aa[0][i]
        value[3] = aa[1] # angle
    return value

def offset_position(position, start_time):
    (dims, measurements) = position.values.shape
    first = position.values[:,0]
    for i in range(measurements):
        if not numpy.isnan(numpy.sum(position.values[:,i])):
            first = position.values[:,i].copy()
            break

    for i in range(measurements):
        position.values[:,i] = position.values[:,i] - first

    return position

class Measurement:
    def __init__(self, timestamps, measurement_callback):
        self.timestamps = timestamps
        N = len(timestamps)
        sample = measurement_callback(timestamps[0])
        self.values = numpy.zeros((sample.shape[0], N))
        for t in range(len(self.timestamps)):
            sample = measurement_callback(timestamps[t])
            for v in range(sample.shape[0]):
                self.values[v, t] = sample[v]

class Sequence:
    def __init__(self, position_cb, rotation_cb = None, rate = 0.01, N = 1000):
        self.rate = rate
        self.N = 1000
        self.seconds = rate * N
        positions = numpy.zeros((3,N)) # in xyz
        rotations = numpy.zeros((N,4)) # in quaternions
        timestamps = range(N) # 
        for i in range(N):
            rotations[i,0] = 1 # identity rotation
            percent = 1. * i / N
            t = rate * i
            positions[:,i] = position_cb(t, percent)
            timestamps[i] = t
            if rotation_cb:
                rotations[i,:] = rotation_cb(t, percent)

        rotationKeyframes = TimeSeries(timestamps, QuaternionArray(rotations))
        positionKeyframes = TimeSeries(timestamps, positions)
        sampled = SampledTrajectory(positionKeyframes, rotationKeyframes)
        splined = SplinedTrajectory(sampled)

        sim = Simulation()
        sim.environment.gravitationalField = ConstantGravitationalField(-9.8065)
        trajectory = splined
        imu = IdealIMU(sim, trajectory)
        samplingPeriod = 0.01
        behaviour = BasicIMUBehaviour(imu, samplingPeriod)
        sim.time = trajectory.startTime
        sim.run(trajectory.endTime)

        self.sim = sim
        self.imu = imu

    def write_sequence(self, name):
        ts = self.imu.accelerometer.rawMeasurements.timestamps
        velocity = Measurement(ts, self.imu.trajectory.velocity)
        position = Measurement(ts, self.imu.trajectory.position)
        rotation = Measurement(ts, lambda t:
            quaternion_to_axisangle(self.imu.trajectory.rotation, t))
        rotationalvelocity = Measurement(ts, self.imu.trajectory.rotationalVelocity)
        rotationalacceleration = Measurement(ts, self.imu.trajectory.rotationalAcceleration)

        if not os.path.exists(name): os.makedirs(name)
        st = self.imu.trajectory.startTime
        et = self.imu.trajectory.endTime
        write_measurements(os.path.join(name, 'gyro.csv'), self.imu.gyroscope.rawMeasurements)
        write_measurements(os.path.join(name, 'accelerometer.csv'), self.imu.accelerometer.rawMeasurements)        
        write_measurements(os.path.join(name, 'velocity.csv'), velocity, st, et)
        position = offset_position(position, st)
        write_measurements(os.path.join(name, 'position.csv'), position, st, et)
        write_measurements(os.path.join(name, 'rotation.csv'), rotation, st, et)
        write_measurements(os.path.join(name, 'rotationalvelocity.csv'), rotationalvelocity, st, et)
        write_measurements(os.path.join(name, 'rotationalacceleration.csv'), rotationalacceleration, st, et)


import math

def cosx_t(t, percent):
    position = [0, 0, 0]
    position[0] = 2*math.cos(2*math.pi*percent)
    return position

def x3_t(t, percent):
    position = [0, 0, 0]
    position[0] = percent**3
    return position

def static_t(t, percent):
    position = [0, 0, 0]
    return position

def rotatingz_t(t, percent):
    q = Quaternion.fromEuler(angles=(0,0,percent*2*math.pi),order='xyz',inDegrees=False)
    return q.components

def generate():
    cost = Sequence(cosx_t)
    cost.write_sequence('data/cosx')
    x3t = Sequence(x3_t)
    x3t.write_sequence('data/x3')
    static = Sequence(static_t)
    static.write_sequence('data/static')
    rotatingz = Sequence(static_t, rotatingz_t)
    rotatingz.write_sequence('data/rotatingz')


