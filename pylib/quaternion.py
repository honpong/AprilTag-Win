# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

def qdot(A, B):
    a = A[1:]
    b = B[1:]
    at = A[0]
    bt = B[0]
    return hstack([at * bt - dot(a,b), at*b + bt*a + cross(a,b)])

def dqdot_dA(B):
    t = B[0]
    x = B[1]
    y = B[2]
    z = B[3]
    return array([[t, -x, -y, -z],
                  [x,  t,  z, -y],
                  [y, -z,  t,  x],
                  [z,  y, -x,  t]])

def dqdot_dB(A):
    t = A[0]
    x = A[1]
    y = A[2]
    z = A[3]
    return array([[t, -x, -y, -z],
                  [x,  t, -z,  y],
                  [y,  z,  t, -x],
                  [z, -y,  x,  t]])

def domtoq(om, derivative=False):
    nrm2 = dot(om, om)
    #small angle assumption from "Quaternion-based rigid body rotation integration algorithm for use in particle methods"
    #all this does is take the quaternion (1, om/2) and normalizes it
    scale = 1./sqrt(4.+nrm2)
    omscale = scale * om
    q = hstack([2.*scale, omscale])
    if derivative:
        d2 = -omscale / (4+nrm2)
        dq_dom = vstack([d2, diag(scale + om * d2)])
        return q, dq_dom
    else:
        return q

#def omdtqdot(om, Q):
#    q = Q[1:]
#    qt = Q[0]
#    om2 = om/2
#    return array([Q[0] - dot(om2,q), q + qt*om2 + cross(om2,q)])

#def domq_dom(Q):
#    t = Q[0]
#    x = Q[1]
#    y = Q[2]
#    z = Q[3]
#    return array([[-x, -y, -z],
#                  [ t, -z,  y],
#                  [ z,  t, -x],
#                  [-y,  x,  t]])

#def domq_dQ(om):
#    x = om[0]
#    y = om[1]
#    z = om[2]
#    return array([[0, -x, -y, -z],
#                  [x,  0, -z,  y],
#                  [y,  z,  0, -x],
#                  [z, -y,  x,  0]])

dR_dq0 = array([[[ 0, 0, 0, 0], [ 0, 0, 0,-1], [ 0, 0, 1, 0]],
                [[ 0, 0, 0, 1], [ 0, 0, 0, 0], [ 0,-1, 0, 0]],
                [[ 0, 0,-1, 0], [ 0, 1, 0, 0], [ 0, 0, 0, 0]]])

dRt_dq0 = array([[[ 0, 0, 0, 0], [ 0, 0, 0, 1], [ 0, 0,-1, 0]],
                [[ 0, 0, 0,-1], [ 0, 0, 0, 0], [ 0, 1, 0, 0]],
                [[ 0, 0, 1, 0], [ 0,-1, 0, 0], [ 0, 0, 0, 0]]])

def qtorot(Q, derivative=False):
    a,b,c,d = Q
    ab = a*b
    ac = a*c
    ad = a*d
    bb = b*b
    bc = b*c
    bd = b*d
    cc = c*c
    cd = c*d
    dd = d*d

    
    R = array([[1-2*(cc+dd), 2*(bc-ad), 2*(ac+bd)],
               [2*(ad+bc), 1-2*(bb+dd), 2*(cd-ab)],
               [2*(bd-ac), 2*(ab+cd), 1-2*(bb+cc)]])
    if derivative:
        dR_dQ  = 2*array([[[0,0,-2*c,-2*d], [-d, c, b,-a], [ c, d, a, b]],
                          [[ d, c, b, a], [0,-2*b,0,-2*d], [-b,-a, d, c]],
                          [[-c, d,-a, b], [ b, a, d, c], [0,-2*b,-2*c,0]]])

        dRt_dQ = 2*array([[[0,0,-2*c,-2*d], [ d, c, b, a], [-c, d,-a, b]],
                          [[-d, c, b,-a], [0,-2*b,0,-2*d], [ b, a, d, c]],
                          [[ c, d, a, b], [-b,-a, d, c], [0,-2*b,-2*c,0]]])
        return R, dR_dQ, dRt_dQ
    else:
        return R

def test3():
    A=random.rand(4)
    B=random.rand(4)
    dABdA=dqdot_dA(B)
    assert alltrue(qdot(A,B)-dot(dABdA,A) < 1e-15)
    dABdB=dqdot_dB(A)
    assert alltrue(qdot(A,B)-dot(dABdB,B) < 1e-15)
    print True

def __dotest2(v):
    v = array(random.random(3)-.5) * 1e-5
    dv=array(random.random(3)-.5) * 1e-5

    #verify rod(rod(v)) == v
    V,dVdv = domtoq(v, True)

    #verify derivative
    Vn = domtoq(v+dv)
    Vne = V + dot(dVdv,dv)
    assert alltrue(abs(Vne-Vn) < 1e-8)
    

def test2(doprint=True):
    #test special case 
    v=array(zeros(3))
    __dotest2(v)

    theta = (random.random()-.5)*1.92*pi
    x = random.random(3)-.5
    x /= linalg.norm(x)
    v = x*theta
    __dotest2(v)

    v = theta * array([1,0,0])
    __dotest2(v)
    v = theta * array([0,1,0])
    __dotest2(v)
    v = theta * array([0,0,1])
    __dotest2(v)
    
    if doprint:
        print True
        
def __dotest(v):
    dv=array(random.random(4)-.5) * 1e-1

    #verify rod(v)*rod(v) == rod(v*v)
    V = qdot(v,v)
    R = qtorot(V)
    r, dr_dv, drt_dv = qtorot(v,True)
    R2 = dot(r,r)
    assert alltrue(abs(R-R2) < 1e-8)

    #verify derivative
    Vn = qtorot(v+dv)
    Vne = r + dot(dr_dv,dv)
    assert alltrue(abs(Vne-Vn) < 1e-8)
    

def test(doprint=True):
    #test special case 
    v=array(zeros(4))
    v[0] = 1
    __dotest(v)

    

    theta = (random.random()-.5)*1.92*pi
    x = random.random(3)-.5
    x /= linalg.norm(x)
    v = hstack([cos(theta/2), x*sin(theta/2)])
    __dotest(v)

    array([0,1,0,0])
    __dotest(v)
    array([0,0,1,0])
    __dotest(v)
    array([0,0,0,1])
    __dotest(v)
    
    if doprint:
        print True

def normalize(Q):
    Q[:] = Q / sqrt(dot(Q,Q))
