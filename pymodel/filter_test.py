# Created by Eagle Jones
# Copyright (c) 2009. The Regents of the University of California.
# All Rights Reserved.

import cor
import corpp
import filter
import rodrigues
from numpy import *

def __dotest_rod(v):
    dv=zeros((4))
    dv[:3] = array(random.random(3)-.5) * 1e-5

    #verify rod(rod(v)) == v
    dVdv = corpp.m4v4()
    dvdV = corpp.v4m4()
    V = corpp.rodrigues(v, dVdv)
    vi = corpp.invrodrigues(V,dvdV)
    assert alltrue(abs(v-vi) < 1e-8)

    V_py, dVdv_py, ign = rodrigues.rodrigues(v[:3], True)
    vi_py, dvdV_py = rodrigues.invrodrigues(V_py, True)
    assert(alltrue(abs(V_py - V[:3,:3]) < 1e-8))
    mydVdv = corpp.m4v4_get_array(dVdv)
    mydvdV = corpp.v4m4_get_array(dvdV)
    assert(alltrue(abs(dVdv_py - mydVdv[:3,:3,:3]) < 1e-8))
    assert(alltrue(abs(dvdV_py - mydvdV[:3,:3,:3]) < 1e-8))
    

def test_rod(doprint=True):
    #test special case 
    v=array(zeros(4))
    __dotest_rod(v)

    theta = (random.random()-.5)*1.92*pi
    x = random.random(3)-.5
    x /= linalg.norm(x)
    v[:3] = x*theta
    __dotest_rod(v)

    v[:3] = theta * array([1,0,0])
    __dotest_rod(v)
    v[:3] = theta * array([0,1,0])
    __dotest_rod(v)
    v[:3] = theta * array([0,0,1])
    __dotest_rod(v)
    
    if doprint:
        print True

for i in xrange(1000):
    test_rod(False)

print "rodrigues.rodrigues succeeded."
