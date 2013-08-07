# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from numpy import *
from Filter import *
import scipy.linalg
from IPython.Debugger import Tracer
debug_here=Tracer()
import cor

import filter

class KalmanFilter:
    def __init__(self):
        pass

    def time_update(self, model, process_cov, dt, linearized_time_update):
        order = model.state_model.time_update(model.state, dt, linearized_time_update)
        F = linearized_time_update.compact
        order = F.shape[0]
        C = model.covar.compact
        pc = process_cov.compact
        cmf = cor.matrix(order, order)
        cmc = cor.matrix(C.shape[0], C.shape[1])
        cmpc = cor.matrix(1, pc.shape[0])
        cmf_d = cor.matrix_dereference(cmf)
        cmf_d[:] = F[:order, :order]
        cmc_d = cor.matrix_dereference(cmc)
        cmc_d[:] = C
        cmpc_d = cor.matrix_dereference(cmpc)
        cmpc_d[:] = pc.reshape((1,-1))
#        debug_here()
        filter.time_update(cmc, cmf, cmpc, dt)
        model.covar.compact = cmc_d
        cor.matrix_free(cmf)
        cor.matrix_free(cmc)
        cor.matrix_free(cmpc)
#        TODO: directly compare the results

#model.covar.compact = dot(dot(F,model.covar.compact),transpose(F)) + diag(process_cov.compact * dt)

    def meas_update(self, state, covar, innovation, linearized_measurement, meas_cov):
        """Kalman Filter update step."""

        lm = asmatrix(linearized_measurement)
        C = asmatrix(covar.compact)
        R = asmatrix(diag(meas_cov))
      
        Cmt = C*lm.T
        LAMBDA = lm*Cmt+R
        kalman_gain = asmatrix(scipy.linalg.lstsq(LAMBDA.T, Cmt.T)[0].T)
        s = state.compact
        GAMMA = asmatrix(eye(s.size) - kalman_gain*lm)

        state.compact = s + dot(asarray(kalman_gain), asarray(innovation))
        covar.compact = GAMMA*C*GAMMA.T + kalman_gain*R*kalman_gain.T

