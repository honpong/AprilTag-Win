# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *

def dXA_dX(sx,A):
    """Finds derivative of tensordot(X,A,1) wrt A for generic X.
    sx = X.shape
    r_{xxx, yyy} = \sum_k x_{xxx,k} a_{k,yyy}
    therefore res{xxx,yyy,xxx,k} = a{k,yyy}"""

    xd = int(product(sx[:-1]))
    sa = A.shape
    a = A.reshape(A.shape[0],-1)
    res = zeros((xd, a.shape[1], xd, a.shape[0]))
    for xxx in range(xd):
        for yyy in range(a.shape[1]):
            for k in range(a.shape[0]):
                res[xxx,yyy,xxx,k] = a[k,yyy]
    return res.reshape(sx[:-1] + A.shape[1:] + sx)

def dAX_dX(A,sx):
    """Finds derivative of tensordot(A,X,1) wrt A for generic X.
    sx = X.shape
    r_{xxx, yyy} = \sum_k a_{xxx,k} x_{k,yyy}
    therefore res{xxx,yyy,k,yyy} = a{xxx,k}"""

    xd = int(product(sx[1:]))
    sa = A.shape
    a = A.reshape(-1,A.shape[-1])
    res = zeros((a.shape[0], xd, a.shape[1], xd))
    for xxx in range(a.shape[0]):
        for yyy in range(xd):
            for k in range(a.shape[1]):
                res[xxx,yyy,k,yyy] = a[xxx,k]
    return res.reshape(A.shape[:-1] + sx[1:] + sx)


#unit tests

def test():
    A=random.rand(10,9)
    B=random.rand(9,11)
    dABdA=dXA_dX(A.shape,B)
    assert alltrue(dot(A,B)-tensordot(dABdA,A,2) < 1e-15)
    dABdB=dAX_dX(A,B.shape)
    assert alltrue(dot(A,B)-tensordot(dABdB,B,2) < 1e-15)
    dABdA=dXA_dX(A.shape,B[:,0])
    assert alltrue(dot(A,B[:,0])-tensordot(dABdA,A,2) < 1e-15)
    dABdB=dAX_dX(A,B[:,0].shape)
    assert alltrue(dot(A,B[:,0])-tensordot(dABdB,B[:,0],1) < 1e-15)
    print True
