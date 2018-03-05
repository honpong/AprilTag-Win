#!/usr/bin/env python
'''

'''
import numpy as np
import transformations as tm
import math

# read the intervals file and convert from pose number to index
def read_intervals(interval_filename):
    intervals = [int(i)-1 for i in open(interval_filename).readlines()]
    return intervals

def tum_to_pose_array_bu(pose_filename):
    # tum format is:
    # time_s x_m y_m z_m qx qy qz qw
    pose_list = []
    for l in open(pose_filename).readlines():
        data = [f for f in l.split()]
        device_id = 0
        if len(data) > 8:
            try:
                device_id = int(data[8])
            except ValueError: # happens with robot poses which have floats in position 8
                device_id = 0
        if device_id == 0:
            pose_list.append([float(d) for d in data[:8]])
    print "Read", len(pose_list), "tm2 poses"
    return pose_list

def tum_to_pose_array(pose_filename):
# tum format is:
# time_s x_m y_m z_m qx qy qz qw
    pose_list = []
    for l in open(pose_filename).readlines():
        data = [f for f in l.split()]
        if len(data) > 7:
            pose_list.append([float(d) for d in data[:8]])
    print "Read", len(pose_list), " tm2 poses"
    return pose_list



def pose_array_to_string(pose_list):
    pose_string = ""
    for p in pose_list:
        pose_string += " ".join(p) + "\n"
    return pose_string

def robot_pose_to_array(robot_pose_filename):
    robot_pose_list = []
    for l in open(robot_pose_filename).readlines():
        data = [f for f in l.split()]
        device_id = 0
        if len(data) >=5: 
            robot_pose_list.append([float(d) for d in data[:6]])
    print "Read", len(robot_pose_list), " robot poses"
    robot_pose = np.array(robot_pose_list)
    robot_pose[:,0:3]*=0.001 #change units from mm to m
    return robot_pose

def check_input(poses,intervals): 
    intervals=intervals.flatten()
    if not np.array_equal( np.sort(intervals, axis=0), intervals):
        print "Error: Intervals are out of order\n"
        return 0
    
    if intervals[-1] > len(poses):
        print "Error: Not enough poses for specified intervals\n"
        return 0 
    return 1

def pose_to_transform_matrix_bu(pose):
    (time, x, y, z, qx, qy, qz, qw) = pose[:8]
    R = tm.quaternion_matrix([qw, qx, qy, qz])
    T = tm.translation_matrix([x, y, z])
    return np.dot(T,R)

def pose_to_transform_matrix(pose):
    (time, x, y, z, qx, qy, qz, qw) = pose[:8]
    R = tm.quaternion_matrix([qw, qx, qy, qz])
    T = tm.translation_matrix([x, y, z])
    return np.dot(T,R)
    
def vector_to_transform_matrix(pose):
    (x, y, z, q1, q2, q3) = pose[:6]
    R = tm.rotation_matrix_from_vector([q1, q2, q3])
    T = tm.translation_matrix([x, y, z])
    return np.dot(T,R)

def vector_degree_to_rad(pose):
    pose[3:6]*=np.ones(3)*(math.pi / 180)
    return pose
 
def average_transformation(poses):
    accum_transform = np.zeros([4,4])
    rows = np.size(poses, 0)
    for r in range(rows):
        accum_transform += pose_to_transform_matrix(poses[r,:])
    accum_transform /= rows
    (U, S, V) = np.linalg.svd(accum_transform[0:3, 0:3])
    detS = np.linalg.det(np.dot(U, V))
    accum_transform[0:3,0:3] = np.dot(U, np.dot(detS, V))
    return accum_transform

def norm_transformation(transformation):
    norm_transform = np.linalg.norm(transformation[0:3,3])
    return norm_transform


def rotation_magnitude(transformation):
    #Input:[Rad] Output:[Dg] 
    tr = np.trace(transformation[0:3,0:3])
    magnitude = math.acos((tr - 1)/2) * 180/math.pi
    return magnitude

def _import_module(name, package=None, warn=True, prefix='_py_', ignore='_'):
    """Try import all public attributes from module into global namespace.

    Existing attributes with name clashes are renamed with prefix.
    Attributes starting with underscore are ignored by default.

    Return True on successful import.

    """
    import warnings
    from importlib import import_module
    try:
        if not package:
            module = import_module(name)
        else:
            module = import_module('.' + name, package=package)
    except ImportError:
        if warn:
            warnings.warn("failed to import module %s" % name)
    else:
        for attr in dir(module):
            if ignore and attr.startswith(ignore):
                continue
            if prefix:
                if attr in globals():
                    globals()[prefix + attr] = globals()[attr]
                elif warn:
                    warnings.warn("no Python implementation of " + attr)
            globals()[attr] = getattr(module, attr)
        return True


_import_module('_transformations', warn=False)

if __name__ == "__main__":
    import doctest
    import random  # used in doctests
    np.set_printoptions(suppress=True, precision=5)
    doctest.testmod()
