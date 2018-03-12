#!/usr/bin/env python
from __future__ import print_function
import numpy as np
import transformations as tm

def read_tum(filename):
    with open(filename, 'rt') as f:
        for l in f:
            yield tuple(float(i) for i in l.split())

def tm_from_pose(pose):
    (x, y, z, qx, qy, qz, qw) = pose[:7]; assert(np.isclose(1, qx*qx + qy*qy + qz*qz + qw*qw))
    return np.dot(tm.translation_matrix([x, y, z]), tm.quaternion_matrix([qw, qx, qy, qz]))

def pose_from_tm(m):
    qw,qx,qy,qz = tm.quaternion_from_matrix(m)
    return (m[0,3], m[1, 3], m[2,3], qx, qy, qz, qw)

def project_controllers_to_hmd(hmd_poses, controller_poses, hmd_relative_controller_poses, verbose=False):
    hmd_poses                  = list(read_tum(hmd_poses))
    controller_poses           = list(read_tum(controller_poses))

    with open(hmd_relative_controller_poses, 'wt') as output:
        j = 0
        for c in controller_poses:
            if verbose: print('c', c)
            ctrl_ts, ctrl_pose = c[0], tm_from_pose(c[1:8])
            while True:
                h = hmd_poses[j]
                if verbose: print('h', h)
                hmd_ts, hmd_pose = h[0], tm_from_pose(h[1:8])
                assert(j+1 < len(hmd_poses))
                j = j + 1
                if abs(hmd_poses[j][0] - ctrl_ts) > abs(hmd_ts - ctrl_ts):
                    inv_hmd_ctrl = np.dot(np.linalg.inv(hmd_pose), ctrl_pose)
                    if verbose: print('hic', ctrl_ts,*pose_from_tm(inv_hmd_ctrl))
                    print(ctrl_ts,*pose_from_tm(inv_hmd_ctrl), file=output)
                    break

if __name__ == "__main__":
    import sys
    if len(sys.argv) <= 3: exit("usage: {} <hmd-tum> <controller-tum> <projected-controllers-tum>".format(sys.argv[0]))
    project_controllers_to_hmd(hmd_poses=sys.argv[1], controller_poses=sys.argv[2], hmd_relative_controller_poses=sys.argv[3])
    exit(0)
