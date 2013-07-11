# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *
from skew3 import *
from derivatives import *
import math

#epsilon is an angle
EPS = 1e-6

def rodrigues(v,derivative=False):
    """Transform a vector into a rotation matrix.
    if derivative==True, outputs a tuple (V, dV/dv)"""

    #epsilon starts as theta
    epsilon = 0 #EPS*EPS
    #in this direction there is only one problem case, theta = 0
    theta2 = dot(v,v)
    if theta2 <= epsilon:
        if derivative:
            return eye(3), dV_dv, transpose(dV_dv, (1,0,2))
        return eye(3)
    theta = sqrt(theta2)
    sintheta = sin(theta)
    costheta = cos(theta)
    sinterm = sintheta/theta
    costerm = (1-costheta)/theta2
    V = skew3(v)
    V2 = dot(V,V)
    R = eye(3) + V * sinterm + V2*costerm
    if derivative:
        #derivatives computed from chain rule by hand
        dt_dv = v/theta
        dsterm_dt = (costheta - sinterm)/theta
        dcterm_dt = (sinterm - 2*costerm)/theta
        dsterm_dv = dsterm_dt * dt_dv
        dcterm_dv = dcterm_dt * dt_dv
        #dV_dv is defined in skew3
        dV2_dV = dAX_dX(V, V.shape) + dXA_dX(V.shape, V)
        dR_dv = dV_dv*sinterm + tensordot(V,dsterm_dv,0) + tensordot(dV2_dV,dV_dv,2)*costerm + tensordot(V2,dcterm_dv,0)
        return R, dR_dv, transpose(dR_dv, (1,0,2))
    return R


def invrodrigues(R,derivative=False):
    """Transform a rotation matrix into a vector
    if derivative==True, outputs a tuple (V, dV/dv)"""

    #epsilon starts as theta
    epsilon1 = 3. #- EPS*EPS #cos(1e-6)*2+1
    epsilon2 = -1. #+ EPS*EPS #cos(pi-1e-6)*2+1
    trc=trace(R)
    #two cases of sin(theta) = 0
    if trc >= epsilon1: #theta = 0
        if derivative:
            return (zeros(3), dv_dV)
        return zeros(3)
    else:
        if trc <= epsilon2: #theta = pi
            assert False            
            
    costheta=(trc-1)/2
    theta = math.acos(costheta)
    sin2theta = 1-costheta*costheta
    sintheta = sqrt(sin2theta)
    
    thetaf = theta / sintheta
    s = invskew3(R)
    v = thetaf * s
	
    if derivative:
        dtrc_dR=eye(3)
        dtheta_dtrc = -0.5 / sintheta
        dtheta_dR = dtheta_dtrc * dtrc_dR
        dtf_dt = 1/sintheta - theta*costheta/sin2theta

        dv_dR = thetaf*dv_dV + tensordot(s*dtf_dt,dtheta_dR,0)
        return (v, dv_dR)
    return v

def __dotest(v):
    dv=array(random.random(3)-.5) * 1e-5
    dt = .33
    #verify rod(rod(v)) == v
    V,dVdv,ign = rodrigues(v, True)
    vi,dvdV = invrodrigues(V,True)
    assert alltrue(abs(v-vi) < 1e-8)

    #verify derivative
    Vn = rodrigues(v+dv)
    Vne = V + dot(dVdv,dv)
    assert alltrue(abs(Vne-Vn) < 1e-8)

    Vdt, dVdtdvdt, ign = rodrigues(v*dt, True)
    Vndt = rodrigues((v+dv)*dt)
    Vnedt = Vdt + dot(dVdtdvdt*dt,dv)
    assert alltrue(abs(Vnedt-Vndt) < 1e-10)

    #verify inverse derivative
    dve = tensordot(dvdV,Vn-V,2)
    assert alltrue(abs(dve-dv) < 1e-8)
    
    #verify that dvdV(dVdv) == dvdv == I
    assert alltrue(abs(eye(3)-tensordot(dvdV,dVdv)) < 1e-8)
    

def test(doprint=True):
    #test special case 
    v=array(zeros(3))
    __dotest(v)

    theta = (random.random()-.5)*1.92*pi
    x = random.random(3)-.5
    x /= linalg.norm(x)
    v = x*theta
    __dotest(v)

    v = theta * array([1,0,0])
    __dotest(v)
    v = theta * array([0,1,0])
    __dotest(v)
    v = theta * array([0,0,1])
    __dotest(v)
    
    if doprint:
        print True

