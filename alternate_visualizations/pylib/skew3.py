# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

dV_dv = array([[[ 0, 0, 0], [ 0, 0,-1], [ 0, 1, 0]],
               [[ 0, 0, 1], [ 0, 0, 0], [-1, 0, 0]],
               [[ 0,-1, 0], [ 1, 0, 0], [ 0, 0, 0]]])

dv_dV = dV_dv * .5

def skew3(v):
    """Calculates skew-symmetric matrix from a 3-vector."""

    return array([[  0  ,-v[2], v[1]],
                  [ v[2],  0  ,-v[0]],
                  [-v[1], v[0],  0  ]])
    
def invskew3(V):
    """Calculates 3-vector from a skew-symmetric matrix.
    Does too much work, so also works for Rotation matrices."""

    return array([V[2,1]-V[1,2], V[0,2]-V[2,0], V[1,0]-V[0,1]]) * .5

#unit test

def test():
    v = random.random(3)
    V = skew3(v)
    assert allclose(dot(dV_dv, v), V)
    assert allclose(invskew3(V), v)
    assert allclose(tensordot(dv_dV, V, 2), v) 
    print True

