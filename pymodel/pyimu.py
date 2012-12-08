# Created by Eagle Jones
# Copyright (c) 2009. The Regents of the University of California.
# All Rights Reserved.

from rodrigues import *
from numpy import *
from derivatives import *
import cor
#need sfm, dt
OmtoR = rodrigues
state = sfm.state.s
omdt = state.w * dt

R, dR_dOm, dRt_dOm = rodrigues(state.W, True)
rdt, drdt_domdt, drdtt_domdt = rodrigues(omdt, True)
Rp = dot(R, rdt)
dRp_drdt = dAX_dX(R,rdt.shape)
dRp_dR = dXA_dX(R.shape,rdt)
Wp, dOmp_dRp = invrodrigues(Rp, True)
drdt_dom = drdt_domdt*dt #domdt_dom = dt

Tp = state.T + state.V*dt
Vp = state.V + (dot(R, state.a) + state.g)*dt
da_dR = dXA_dX(R.shape, state.a*dt)
eyedt = identity(3)*dt

print "linearized_time_update['T', 'V'] =", eyedt
print "linearized_time_update['V', 'alpha'] =", R*dt
print "linearized_time_update['V', 'gamma'] =", eyedt
print "linearized_time_update['alpha', 'j'] =", eyedt
print "linearized_time_update['om', 'w'] =", eyedt

dRp_dom = tensordot(dRp_drdt, drdt_dom)
dRp_dOm = tensordot(dRp_dR, dR_dOm)
print "linearized_time_update['Om', 'Om'] =", tensordot(dOmp_dRp, dRp_dOm) - eye(3)
print "linearized_time_update['Om', 'om'] =", tensordot(dOmp_dRp, dRp_dom)
print "linearized_time_update['V', 'Om'] =", tensordot(da_dR, dR_dOm)
