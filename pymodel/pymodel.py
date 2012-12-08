# Created by Eagle Jones
# Copyright (c) 2009. The Regents of the University of California.
# All Rights Reserved.

from rodrigues import *
from numpy import *
from derivatives import *
import cor
OmtoR = rodrigues
#need sfm, g, i x = state.initial[g][i] defined
state = sfm.state.s
R, dR_dOm, dRt_dOm = OmtoR(state.W, True)
Rt = R.T
T = state.T

Rbc, dRbc_dOmcb, dRbct_dOmcb = OmtoR(state.Wc, True)
Rcb = Rbc.T
Tbc = state.Tc

RcbRt = dot(Rcb, Rt)
dRcbRt_dRt = dAX_dX(Rcb, Rt.shape)
dRcbRt_dOm = tensordot(dRcbRt_dRt, dRt_dOm)
dRcbRtr_dRref = dAX_dX(RcbRt, Rt.shape)

Tref = state.Tr[g]
Omref = state.Wr[g]
Rref, dRref_dOmref, dRreft_dOmref = OmtoR(Omref,True)
Rtr = dot(Rt, Rref)

dX_dTcb = dot(Rcb, (Rtr - eye(3)))
RcbRtr = dot(Rcb, Rtr)
Rtot = dot(RcbRtr, Rbc)
dRcbRtr_dOmref = tensordot(dRcbRtr_dRref, dRref_dOmref)
dRtot_dRbc = dAX_dX(RcbRtr, Rbc.shape)

p = state.p[g][i]
rho = exp(p)

X0 = array([x[0], x[1], 1])*(rho)

Xc = dot(Rbc, X0) + Tbc
Xr = dot(Rref, Xc) + Tref
Xb = dot(Rt, Xr - T)
X = dot(Rcb, Xb - Tbc)

invZ = 1./X[2]
pred = X[0:2] * invZ

dy_dX = array([[invZ, 0, -pred[i*2] * invZ],
               [0, invZ, -pred[i*2+1] * invZ]])

dX0_drho = hstack([x[0], x[1], 1])*rho
dX_dRtot = dXA_dX(Rtot.shape, X0)
dX_dRcbRtr = dXA_dX(RcbRtr.shape, Xc)
dX_dRcbRt = dXA_dX(RcbRt.shape, Xr - T)
dX_dRcb = dXA_dX(Rcb.shape, Xb - Tbc)

dX_drho = dot(Rtot, dX0_drho)
dX_dOm = tensordot(dX_dRcbRt, dRcbRt_dOm)
dX_dOmref = tensordot(dX_dRcbRtr, dRcbRtr_dOmref)
dX_dRbc = tensordot(dX_dRtot, dRtot_dRbc)
dX_dOmcb = tensordot(dX_dRcb, dRbct_dOmcb) + tensordot(dX_dRbc, dRbc_dOmcb)
