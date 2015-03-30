# Created by Eagle Jones
# Copyright (c) 2009. The Regents of the University of California.
# All Rights Reserved.

from quaternion import *
from rodrigues import *
from numpy import *
from derivatives import *
from Filter import *
from KalmanFilter import *
import skew3
from IPython.Debugger import Tracer
debug_here=Tracer()
import cor
import filter

import cProfile

############################
QUATERNION = False

if QUATERNION:
    ROTD = 4
    OmtoR = qtorot
else:
    ROTD = 3
    OmtoR = rodrigues

class StateModel(object):
    def __init__(self,stateindex):
        pass

    def time_update(self, state, dt, linearized_time_update=None):
        pass

class MeasurementModel(object):
    def __init__(self,measurementindex):
        pass

    def predict_measurements(self, state, prediction, linearized_measurement=None):
        pass

class PositionStateModel(StateModel):
    def __init__(self,stateindex):
        super(PositionStateModel, self).__init__(stateindex)
        stateindex.alloc('T',3)
        stateindex.alloc('Om',ROTD)

class MotionStateModel(PositionStateModel):
    def __init__(self, stateindex):
        super(MotionStateModel, self).__init__(stateindex)
        stateindex.alloc('V', 3)
        stateindex.alloc('om', 3)
        stateindex.alloc('alpha', 3)
        #stateindex.alloc('abias', 3)
        #stateindex.alloc('ombias', 3)
        stateindex.alloc('w', 3)
        stateindex.alloc('j', 3)

    def time_update(self, state, dt, linearized_time_update=None):
        super(MotionStateModel, self).time_update(state, dt, linearized_time_update)
        #update rotation
        omdt = state.om * dt
        if QUATERNION:
            R = state.Om
            rdt, drdt_domdt = domtoq(omdt, True)
            Rp = qdot(R, rdt)
            dRp_drdt = dqdot_dB(R)
            dRp_dR = dqdot_dA(rdt)
            state.Om[:] = Rp
        else:
            R, dR_dOm, dRt_dOm = rodrigues(state.Om, True)
            rdt, drdt_domdt, drdtt_domdt = rodrigues(omdt, True)
            Rp = dot(R, rdt)
            dRp_drdt = dAX_dX(R,rdt.shape)
            dRp_dR = dXA_dX(R.shape,rdt)
            state.Om[:], dOmp_dRp = invrodrigues(Rp, True)
        drdt_dom = drdt_domdt*dt #domdt_dom = dt
        #do the update
        state.T[:] = state.T + state.V*dt
        state.V[:] = state.V + (dot(R, state.alpha) + state.gamma)*dt
        da_dR = dXA_dX(R.shape, state.alpha*dt)
        state.alpha[:] = state.alpha + state.j*dt
        state.om[:] = state.om + state.w*dt
        eyedt = identity(3)*dt

        #linearization (aka F or A)
        #constant
        if linearized_time_update is not None:
            linearized_time_update['T', 'V'] = eyedt
            linearized_time_update['V', 'alpha'] = R*dt
            linearized_time_update['V', 'gamma'] = eyedt
            linearized_time_update['alpha', 'j'] = eyedt
            linearized_time_update['om', 'w'] = eyedt
            #variable
            if QUATERNION:
                dRp_dom = dot(dRp_drdt, drdt_dom)
                linearized_time_update['Om', 'Om'] = dRp_dR - eye(3)
                linearized_time_update['Om', 'om'] = dRp_dom
            else:
                dRp_dom = tensordot(dRp_drdt, drdt_dom)
                dRp_dOm = tensordot(dRp_dR, dR_dOm)
                linearized_time_update['Om', 'Om'] = tensordot(dOmp_dRp, dRp_dOm) - eye(3)
                linearized_time_update['Om', 'om'] = tensordot(dOmp_dRp, dRp_dom)
                linearized_time_update['V', 'Om'] = tensordot(da_dR, dR_dOm)
        return state.index['Tcb'].start

class SFMStateModel(PositionStateModel):
    def __init__(self, stateindex, N, maxgroups):
        super(SFMStateModel, self).__init__(stateindex)
        stateindex.alloc('Tcb', 3)
        stateindex.alloc('Omcb', ROTD)
        stateindex.alloc('rho', N)
        stateindex.alloc('Tref', 3*maxgroups)
        stateindex.alloc('Omref', ROTD*maxgroups)

class GravityStateModel(StateModel):
    def __init__(self, stateindex):
        super(GravityStateModel, self).__init__(stateindex)
        stateindex.alloc('gamma', 3)

class IMUSFMGravityStateModel(SFMStateModel, MotionStateModel, GravityStateModel):
    pass

class InertialMeasurementModel(MeasurementModel):
    def __init__(self, measurementindex):
        super(InertialMeasurementModel, self).__init__(measurementindex)
        measurementindex.alloc('alpha', 3)
        measurementindex.alloc('om', 3)
        
    def predict_measurements(self, state, prediction, linearized_measurement=None):
        super(InertialMeasurementModel, self).predict_measurements(state, prediction, linearized_measurement)
        self.predict_alpha(state, prediction, linearized_measurement)
        self.predict_om(state, prediction, linearized_measurement)

    def predict_om(self, state, prediction, linearized_measurement=None):
        if linearized_measurement is not None:
            linearized_measurement['om', 'om'] = identity(3)
            #linearized_measurement['om', 'ombias'] = identity(3)
        prediction.om[:] = state.om #+ state.ombias


class GravityMeasurementModel(InertialMeasurementModel):
    def __init__(self, measurementindex):
        super(GravityMeasurementModel, self).__init__(measurementindex)
        #measurementindex.alloc('gnorm', 1)

    def predict_measurements(self, state, prediction, linearized_measurement=None):
        super(GravityMeasurementModel, self).predict_measurements(state, prediction, linearized_measurement)
        #self.predict_gnorm(state, prediction,linearized_measurement)

    def predict_alpha(self, state, prediction, linearized_measurement=None):
        #R, dR_dOm, dRt_dOm = OmtoR(state.Om, True)
        #acc = state.alpha - state.gamma
        #dya_dRt = dXA_dX(R.shape, acc)      
        #dya_dOm = tensordot(dya_dRt, dRt_dOm)
        if linearized_measurement is not None:
            linearized_measurement['alpha','alpha'] = eye(3) #R.T
 #           linearized_measurement['alpha','gamma'] = -R.T
  #          linearized_measurement['alpha','Om'] = dya_dOm
 #           linearized_measurement['alpha','abias'] = eye(3)
        prediction.alpha[:] = state.alpha #+ state.abias #dot(R.T, acc) + state.abias
      
    def predict_gnorm(self, state, prediction, linearized_measurement=None):
        g = state.gamma
        if linearized_measurement is not None:
            linearized_measurement['gnorm','gamma'] = 2*g
        prediction.gnorm[:] = dot(g, g)

class SFMMeasurementModel(MeasurementModel):
    def __init__(self,measurementindex, N):
        super(SFMMeasurementModel, self).__init__(measurementindex)
        measurementindex.alloc('points', 2*N)

    def get_world(state):

        result = zeros((state.rho.size, 3))
        Rbc = OmtoR(state.Omcb)
        Tbc = state.Tcb
        Tref = state.Tref
        Omref = state.Omref
        Rref  = OmtoR(Omref)
        for i in xrange(state.rho.size):
            p = state.rho[i]
            if p < -20:
                state.rho[i] = -20
            elif p > 20:
                state.rho[i] = 20
            rho = exp(state.rho[i])
            x = state.initial['points'][i*2:i*2+2]
            X0 = array([x[0], x[1], 1])*(rho)
            Xc = dot(Rbc, X0) + Tbc
            Xr = dot(Rref, Xc) + Tref
            result[i,:] = Xr
        return result

    get_world = staticmethod(get_world)

    def predict_essential(state):
        R = OmtoR(state.Om)
        Rt = R.T
        T = state.T
        Rbc = OmtoR(state.Omcb)
        Rcb = Rbc.T
        Tbc = state.Tcb

        Tref = state.Tref
        Omref = state.Omref
        Rref = OmtoR(Omref)

        RcbRt = dot(Rcb,Rt)
        RcbRtRr = dot(RcbRt, Rref)
        Rtot = dot(RcbRtRr, Rbc)
    
        Tc = Tbc
        Tr = dot(Rref, Tc) + Tref
        Tb = dot(Rt, Tr - T)
        Ttot = dot(Rcb, Tb - Tbc)
        That = skew3.skew3(Ttot)
        Ess = dot(That, Rtot)
        return Ess

    predict_essential = staticmethod(predict_essential)


    def compute_essential(initial, current):
        N = initial.shape[1]
        chi = zeros((N,9))
        for i in xrange(N):
            #backwards
            chi[i] = tensordot(initial[:,i], current[:,i], 0).flatten()
        U, S, Vh = linalg.svd(chi)

        F = Vh[8].reshape((3,3))
        U,S,Vh = linalg.svd(F)
        S = array([1.,1.,0.])
        return dot(dot(U, diag(S)), Vh)        

    compute_essential = staticmethod(compute_essential)

    def test_correspondence(state, measurement, residual, outlier):
        initial = vstack([state.initial.points[0::2], state.initial.points[1::2], ones_like(state.initial.points[0::2])])
        current = vstack([measurement.points[0::2], measurement.points[1::2], ones_like(measurement.points[0::2])])
        #first, check if the predicted essential matrix works:
        Ess = SFMMeasurementModel.predict_essential(state)
        res = sum(current * dot(Ess, initial), 0)
        Ei = dot(Ess, initial) ** 2
        Ec = dot(Ess.T, current) ** 2
        residual1 = res**2 / (Ei[0] + Ei[1] + Ec[0] + Ec[1])
        #now recompute the essential matrix using only the best points
        order = argsort(residual1)
        newi = initial[:,order[:9]]
        newc = current[:,order[:9]]
        Ess2 = SFMMeasurementModel.compute_essential(newi, newc)
        res = sum(current * dot(Ess2, initial), 0)
        Ei = dot(Ess2, initial) ** 2
        Ec = dot(Ess2.T, current) ** 2
        residual2 = res**2 / (Ei[0] + Ei[1] + Ec[0] + Ec[1])

        #now generate the essential matrix based on the points
        Ess3 = SFMMeasurementModel.compute_essential(initial, current)
        res = sum(current * dot(Ess3, initial), 0)
        Ei = dot(Ess3, initial) ** 2
        Ec = dot(Ess3.T, current) ** 2
        residual3 = res**2 / (Ei[0] + Ei[1] + Ec[0] + Ec[1])

        in1 = (residual1 < outlier).sum()
        in2 = (residual2 < outlier).sum()
        in3 = (residual3 < outlier).sum()
        in4 = (residual.rho < outlier).sum()

        #if (in1 > in2):
        inbest = in1
        resbest = residual1
        #else:
        #    inbest = in2
        #    resbest = residual2
        if in3 > inbest:
            #mostly only used the first time
            inbest = in3
            resbest = residual3
        if inbest > in4:
            residual.rho = resbest
            
    test_correspondence = staticmethod(test_correspondence)

    def predict(state, prediction, linearized_measurement=None):
        R, dR_dOm, dRt_dOm = OmtoR(state.Om, True)
        Rt = R.T
        T = state.T

        Rbc, dRbc_dOmcb, dRbct_dOmcb = OmtoR(state.Omcb, True)
        Rcb = Rbc.T
        Tbc = state.Tcb

        RcbRt = dot(Rcb, Rt)
        dRcbRt_dRt = dAX_dX(Rcb, Rt.shape)
        dRcbRt_dOm = tensordot(dRcbRt_dRt, dRt_dOm)
        dRcbRtr_dRref = dAX_dX(RcbRt, Rt.shape)
        
        #dRtr_dOm = tensordot(dRtr_dRt, dRt_dOm)

        #dRtrV_dV = dAX_dX(Rtr,(3,))

        #Xt = dot(self.Rcal, (X0-reshape(self.state.T,(3,1)))) + self.Tcb #keep intermediates for linearization later
        #linearization
        Tref = state.Tref
        Omref = state.Omref
        Rref, dRref_dOmref, dRreft_dOmref = OmtoR(Omref,True)
        Rtr = dot(Rt, Rref)

        dX_dTcb = dot(Rcb, (Rtr - eye(3)))
        RcbRtr = dot(Rcb, Rtr)
        Rtot = dot(RcbRtr, Rbc)
        dRcbRtr_dOmref = tensordot(dRcbRtr_dRref, dRref_dOmref)
        dRtot_dRbc = dAX_dX(RcbRtr, Rbc.shape)

        y_pred = prediction.points
        for i in xrange(state.rho.size):
            p = state.rho[i]
            if p < -20:
                state.rho[i] = -20
            elif p > 20:
                state.rho[i] = 20
            rho = exp(state.rho[i])
            x = state.initial['points'][i*2:i*2+2]
            X0 = array([x[0], x[1], 1])*(rho)
            #Xt = array(dot(Rref,(x[0]*rho, x[1]*rho, rho))) - (self.state.T-self.state.Tref)
            Xc = dot(Rbc, X0) + Tbc
            Xr = dot(Rref, Xc) + Tref
            Xb = dot(Rt, Xr - T)
            X = dot(Rcb, Xb - Tbc)

            #X = dot(Rtot, X0) + Ttot
            invZ = 1./X[2]
            y_pred[i*2] = X[0] * invZ
            y_pred[i*2+1] = X[1] * invZ

            if linearized_measurement is not None:
                dy_dX = array([[invZ, 0, -y_pred[i*2] * invZ],
                               [0, invZ, -y_pred[i*2+1] * invZ]])

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
            
                ms = slice(i*2,i*2+2)
                linearized_measurement['points','rho'][ms,i] = dot(dy_dX, dX_drho)
                linearized_measurement['points','Om'][ms] = dot(dy_dX, dX_dOm)
                linearized_measurement['points','T'][ms] = -dot(dy_dX, RcbRt)
                linearized_measurement['points','Omcb'][ms] = dot(dy_dX, dX_dOmcb)
                linearized_measurement['points','Tcb'][ms] = dot(dy_dX, dX_dTcb)
                linearized_measurement['points','Omref'][ms] = dot(dy_dX, dX_dOmref)
                linearized_measurement['points','Tref'][ms] = dot(dy_dX, RcbRt)

    predict = staticmethod(predict)

    def predict_measurements(self, state, prediction, linearized_measurement=None):
        super(SFMMeasurementModel, self).predict_measurements(state, prediction, linearized_measurement)
        self.predict(state, prediction, linearized_measurement)

class SFMGroupsMeasurementModel(MeasurementModel):
    def __init__(self,measurementindex, N):
        super(SFMGroupsMeasurementModel, self).__init__(measurementindex)
        measurementindex.alloc('points', 2*N)

    def for_each_group(self, state, predictionindex):
        oTr = state.index['Tref']
        oOr = state.index['Omref']
        orho = state.index['rho']
        opoints = predictionindex['points']

        groups = len(state.groups)
        for group in xrange(groups):
            state.index['Tref'] = slice(oTr.start+group*3, oTr.start+group*3+3)
            state.index['Omref'] = slice(oOr.start+group*ROTD, oOr.start+group*ROTD+ROTD)
            g = state.groups[group]
            state.index['rho'] = slice(orho.start + g.start, orho.start + g.stop)
            predictionindex['points'] = slice(opoints.start + g.start*2, opoints.start + g.stop*2)
            yield group
        state.index['Tref'] = oTr
        state.index['Omref'] = oOr
        state.index['rho'] = orho
        predictionindex['points'] = opoints

    def get_world(self, state):
        result = []
        for i in self.for_each_group(state, state.initial.index):
            result.append(SFMMeasurementModel.get_world(state))
        return vstack(result)

    def predict_measurements(self, state, prediction, linearized_measurement=None):
        super(SFMGroupsMeasurementModel, self).predict_measurements(state, prediction, linearized_measurement)
        for i in self.for_each_group(state, prediction.index):
            SFMMeasurementModel.predict(state, prediction, linearized_measurement)
            if linearized_measurement is not None and not state.goodgroups[i]:
                if i != 0:
                    linearized_measurement['points','Om'] = 0
                    linearized_measurement['points','T'] = 0
                    linearized_measurement['points','Tref'] = 0
                    linearized_measurement['points','Omref'] = 0
                linearized_measurement['points','Omcb'] = 0
                linearized_measurement['points','Tcb'] = 0

    def test_correspondence(self, state, measurement, residuals, outlier):
        for i in self.for_each_group(state, measurement.index):
            #if state.goodgroups[i]:
            SFMMeasurementModel.test_correspondence(state, measurement, residuals, outlier)


class IMUSFMGravityMeasurementModel(SFMGroupsMeasurementModel,GravityMeasurementModel):
    pass

class RealFilter(FullFilter):
    def __init__(self, myvis, N, maxgroups):
        FullFilter.__init__(self)
        self.state_model = IMUSFMGravityStateModel(self.stateindex, N, maxgroups)
        self.measurement_model = IMUSFMGravityMeasurementModel(self.measurementindex,N)
        self.filt = KalmanFilter()

        self.outlier = DataModel(self.stateindex)

        self.maxpoints = N
        self.maxgroups = maxgroups
        self.features_per_group = N / maxgroups

        self.process_cov.Tref[:]  = 0.e-2
        self.process_cov.Omref[:]  = 0.e-2
        self.process_cov.T[:] = 0.e-4
        self.process_cov.Om[:] = 0.e-4
        self.process_cov.V[:] = 0.e-3
        self.process_cov.om[:] = 1.e-1   #changed by .06 in 5 frames
        self.process_cov.w[:] = 1
        self.process_cov.alpha[:] = 1.e-1  #400 = -.25, 500 = -1.75, 
        self.process_cov.j[:] = 1
        self.process_cov.gamma[:] = 0e-10
        self.process_cov.rho[:] = 0.e-5
        self.process_cov.Omcb[:] = 0e-20
        self.process_cov.Tcb[:] = 0e-20
        #self.process_cov.abias[:] = 5e-10
        #self.process_cov.ombias[:] = 5e-12
        # c-migits - table 4.3, page 75: [ 1.8e-1 ^2, 4.0e-4 ^2 ] (also correspond reasonably with measured)
        #these values are takedn from six-degree filter: [ 4.5e-2, 1.5e-4 ]
        self.meas_cov.alpha[:] = 1.8e-1**2 # 100 milli-g rms noise nominal, 400 milli-g rms max
        self.meas_cov.om[:] =  4.0e-4**2  #  360 deg/hr rms -> 1.7e-3 rad/s ??
##check this!!!
        #self.meas_cov.gnorm[:] = 0.e-6
        self.meas_cov.points[:] = 5.e-3**2

        self.init_cov.Tref[:]  = 0.e-1
        self.init_cov.Omref[:]  = 0.e-1
        self.init_cov.T[:] = 0 #1.e-1
        self.init_cov.Om[:] = 0 #1.e-1
        self.init_cov.V[:] = 1
        self.init_cov.om[:] = 1
        self.init_cov.w[:] = 1
        self.init_cov.alpha[:] = 1
        self.init_cov.j[:] = 1
        self.init_cov.gamma[:] = .1
        self.init_cov.rho[:] = 10
        self.init_cov.Omcb[:] = 1.e-5
        self.init_cov.Tcb[:] = 1.e-4
        #self.init_cov.ombias[:] = .01
        #self.init_cov.abias[:] = .01

        self.state.initial = DataModel(self.measurementindex)
        self.state.groups = []
        self.state.goodgroups = []
        self.stateindex.register_callback(self, 'fixgroups')
        self.stateindex.saturations.Tref[0:3] = True
        self.stateindex.saturations.Omref[0:ROTD] = True
        if QUATERNION:
            self.state.Om[0] = 1
            self.state.Omcb[0] = 1
        self.covar.storage[:] = diag(self.init_cov.storage)

        #freeze alpha while we initialize gamma
        #self.covar['alpha', 'alpha'] = 0.
        #self.covar['om', 'om'] = 0.
        #self.stateindex.saturations.alpha[:] = True
        #self.stateindex.saturations.om[:] = True
        self.stateindex.realloc('rho', -self.maxpoints)
        self.stateindex.realloc('Tref', -self.maxgroups*3)
        self.stateindex.realloc('Omref', -self.maxgroups*ROTD)
        self.measurementindex.realloc('points', -self.maxpoints*2)
        self.measurements = DataModel(self.measurementindex)
        #self.stateindex.saturations.abias[:] = True
        #self.stateindex.saturations.ombias[:] = True
        #self.measurementindex.saturations.gnorm[:] = True
        #self.stateindex.saturations.Tcb[:] = True
        #self.stateindex.saturations.Omcb[:] = True
        #self.stateindex.saturations.T[:] = True
        #self.stateindex.saturations.V[:] = True
        self.active = False
        self.frame = 0
        self.skip = 0
        self.mincov = .01
        self.lastimu = 0
        self.imustack = list()
        self.firstimu = True

        self.plotinna = myvis.frame.window_3.add("INN-a")
        self.plotinna.x = self.plotinna.addhist('r')
        self.plotinna.y = self.plotinna.addhist('g')
        self.plotinna.z = self.plotinna.addhist('b')

        self.plotinno = myvis.frame.window_3.add("INN-w")
        self.plotinno.x = self.plotinno.addhist('r')
        self.plotinno.y = self.plotinno.addhist('g')
        self.plotinno.z = self.plotinno.addhist('b')

    def tick(self, time):
        if hasattr(self, "lasttime"):
            dt = time - self.lasttime
            if dt > 0:
                self.time_update(self.process_cov, dt / 1000000.)
                self.lasttime = time
        else:
            self.lasttime = time
        rp = cor.mapbuffer_alloc(self.output, cor.packet_filter_position, 6*4)
        rp.position[:] = self.state.T
        rp.orientation[:] = self.state.Om
        cor.mapbuffer_enqueue(self.output, rp, time)
        if self.state.rho.size > 0:
            self.world = self.measurement_model.get_world(self.state)

    def vis_measurement(self, time, measurements, robust = True):
        self.tick(time)
        for i in xrange(len(self.state.groups)):
            if not self.state.goodgroups[i]:
                mediancov = median(self.covar['rho','rho'].diagonal()[self.state.groups[i]])
                if mediancov < self.mincov:
                    self.state.goodgroups[i] = True
        self.vis_meas_update(measurements, self.meas_cov,robust)
        if QUATERNION:
            normalize(self.state.Om)
            normalize(self.state.Omcb)
            for i in xrange(0, self.state.Omref.size, ROTD):
                normalize(self.state.Omref[i:i+ROTD])

    def initialize_gravity(self):
        #self.covar['alpha','alpha'] = diag(self.init_cov.alpha)
        #self.covar['om','om'] = diag(self.init_cov.om)
        self.stateindex.saturations.V[:] = False
        self.stateindex.saturations.om[:] = False
        self.stateindex.saturations.T[:] = False
        #self.stateindex.saturations.gamma[:] = True

    profile_count = 0
    def sfm_vis_measurement_profile(self, packet):
        print "vis_meas:"
        cProfile.runctx('self.sfm_vis_measurement_cb(packet)', globals=globals(), locals=locals(), filename="cor_profile"+str(self.profile_count))
        self.profile_count += 1

    def sfm_vis_measurement_cb(self, packet):
        if packet.header.type != cor.packet_feature_track:
            return
        self.frame += 1
        features = cor.packet_feature_track_t_features(packet)
        if not self.active:
            if self.frame > self.skip:
                #astack = list()
                #wstack = list()
                #for a, w, imutime in self.imustack:
                #    astack.append(a)
                #    wstack.append(w)
                #ast = array(astack)
                #wst = array(wstack)
                #debug_here()
                self.imustack = list()
                #TODO: ultimately this type of initialization needs to be wrapped in generic gain adjustments... based on vision, we can turn the acceleration / gravity switch
                self.active = True
                #self.initialize_gravity()
            else:
                return

        self.measurements.points[:] = features.flatten()

        #drop features
        dropmask = (features[:,0] != inf)
        rmask = self.outlier.rho > 100.
        dropmask[rmask] = False
        
        if not all(dropmask):
             self.droppoints(dropmask, packet.header.time)

        
        while self.lastimu < packet.header.time:
            try:
                a, w, imutime = self.imustack.pop(0)
            except IndexError:
                break
            imutime += 67000
            self.tick(imutime)
            if self.firstimu:
                self.state.alpha[:] = a
                self.state.gamma[:] = -a
                self.firstimu = False
            self.measurements.alpha = a
            self.measurements.om = w
            self.imu_meas_update(self.measurements, self.meas_cov)
            self.lastimu = imutime

        #do stuff
        self.tick(packet.header.time)
        for i in xrange(len(self.state.groups)):
            if not self.state.goodgroups[i]:
                mediancov = median(self.covar['rho','rho'].diagonal()[self.state.groups[i]])
                if mediancov < self.mincov:
                    self.state.goodgroups[i] = True
        self.vis_meas_update(self.measurements, self.meas_cov, True)

        #feedback feature pos
#        dp = cor.mapbuffer_alloc(self.feature_feedback, cor.packet_feature_track, self.state.rho.shape[0] * 4 * 2)
#        dp.header.user = self.state.rho.shape[0]
#        features = cor.packet_feature_track_t_features(dp)
#        SFMGroupsMeasurementModel.predict_measurements(self.measurement_model, self.state, self.prediction, None)
#        features[:] = self.prediction.points.reshape([-1,2])
#        cor.mapbuffer_enqueue(self.feature_feedback, dp, packet.header.time)

        #add features
        if len(self.state.groups) < self.maxgroups:
            dp = cor.mapbuffer_alloc(self.control, cor.packet_feature_select, 0)
            dp.header.user = self.features_per_group
            cor.mapbuffer_enqueue(self.control, dp, packet.header.time)

    def imu_measurement(self, time, measurements):
        self.tick(time)
        self.imu_meas_update(measurements, self.meas_cov)

    def sfm_imu_measurement_profile(self, packet):
        print "imu_meas:"
        cProfile.runctx('self.sfm_imu_measurement_cb(packet)', globals=globals(), locals=locals())

    def sfm_imu_measurement_cb(self, packet):
        if packet.header.type != cor.packet_imu:
            return
        self.imustack.append((packet.a, packet.w, packet.header.time))
        return
        self.tick(packet.header.time + 67000)
        self.measurements.alpha = packet.a
        self.measurements.om = packet.w
        self.imu_meas_update(self.measurements, self.meas_cov)

    def addpoints(self, initialx, initialy):
        assert(len(self.state.groups) < self.maxgroups)
        points = self.state['rho'].size
        newpoints = initialx.size
        size = self.state.index.size
        #time to add some features
        #clear out covar matrix
#        self.covar.storage[size:newpoints+size,:] = 0
#        self.covar.storage[:,size:] = 0

        oldTsize = self.state.Tref.size
        oldRsize = self.state.Omref.size
        self.stateindex.realloc('rho', newpoints)
        self.measurementindex.realloc('points', newpoints*2)
        self.stateindex.realloc('Tref', 3)
        self.stateindex.realloc('Omref', ROTD)
        self.covar.clear('rho', points, newpoints, self.init_cov)
        self.covar.clear('Tref', oldTsize, 3, self.init_cov)
        self.covar.clear('Omref', oldRsize, ROTD, self.init_cov)

        Rref = self.state.Om.copy()
        Tref = self.state.T.copy()
        self.state.Tref[oldTsize:] = Tref
        self.state.Omref[oldRsize:] = Rref

        Trslice = self.stateindex['Tref']
        Tslice = self.stateindex['T']
        Trslice = slice(Trslice.start + oldTsize, Trslice.stop)
        self.covar.storage[Trslice,:] = self.covar.storage[Tslice,:]
        self.covar.storage[:,Trslice] = self.covar.storage[:,Tslice]
        selfcov = self.covar['T','T'].copy()
#        selfcov[0][0] = 1
#        selfcov[1][1] = 1
#        selfcov[2][2] = 1
        self.covar.storage[Trslice,Tslice] = selfcov #diag(self.init_cov['Tref'][oldTsize:])
        self.covar.storage[Tslice,Trslice] = selfcov #diag(self.init_cov['Tref'][oldTsize:])

        Orslice = self.stateindex['Omref']
        Oslice = self.stateindex['Om']
        Orslice = slice(Orslice.start + oldRsize, Orslice.stop)
        self.covar.storage[Orslice,:] = self.covar.storage[Oslice,:]
        self.covar.storage[:,Orslice] = self.covar.storage[:,Oslice]
        selfcov = self.covar['Om','Om'].copy()
#        selfcov[0][0] = 1
#        selfcov[1][1] = 1
#        selfcov[2][2] = 1
        self.covar.storage[Orslice,Oslice] = selfcov #diag(self.init_cov['Tref'][oldTsize:])
        self.covar.storage[Oslice,Orslice] = selfcov #diag(self.init_cov['Tref'][oldTsize:])

        self.state.groups.append(slice(points,points+newpoints))
        self.state.goodgroups.append(False)

        meanrho = 0
        if points > 0:
            meanrho = mean(self.state.rho[:points])
        raddepth = log(1+initialx*initialx+initialy*initialy)
        #meanrd = mean(raddepth)
        initialrho = meanrho - (raddepth)

        self.covar['rho','rho'][points:,points:] = diag(self.init_cov.rho[points:])

        self.state.initial.points[points*2  ::2] = initialx
        self.state.initial.points[points*2+1::2] = initialy
        self.state.rho[points:] = initialrho
        self.outlier.rho[points:] = 0

    def sfm_add_profile(self, packet):
        print "add:"
        cProfile.runctx('self.sfm_add_cb(packet)', globals=globals(), locals=locals())

    def sfm_add_cb(self, packet):
        if packet.header.type != cor.packet_feature_select:
            return
        initialpts = cor.packet_feature_track_t_features(packet)
        self.addpoints(initialpts[:,0], initialpts[:,1])

    def droppoints(self, keep, time):
        """NOTE: mask is sort of inverted -- we keep the ones that are True"""
        groups = self.state.groups
        tkeep = ones(self.state.Tref.size, dtype=bool)
        rkeep = ones(self.state.Omref.size, dtype=bool)
        dropg = False
        for i in xrange(len(groups)):
            nc = keep[groups[i]].sum()
#            if nc >= 3:
#                grpin = self.outlier.rho[groups[i]][keep[groups[i]]] == 0.
#                print nc, grpin.sum()
#                nc = grpin.sum()
                
            if nc < 4:
                keep[groups[i]] = False
                dropg = True
                tkeep[i*3:i*3+3] = False
                rkeep[i*ROTD:i*ROTD+ROTD] = False

        world = self.measurement_model.get_world(self.state)
        droppedworld = world[-keep]
        goodones = (self.covar['rho','rho'].diagonal() < .0001)[-keep]
        if any(goodones):
            sp = cor.mapbuffer_alloc(self.output, cor.packet_filter_reconstruction, sum(goodones) * 3 * 4)
            sp.header.user = sum(goodones)
            points = cor.packet_filter_reconstruction_t_points(sp)
            points[:] = droppedworld[goodones]
            cor.mapbuffer_enqueue(self.output, sp, time)
        self.stateindex.mask('rho', keep)
        self.measurementindex.mask('points', keep.repeat(2))
        if dropg:
            self.stateindex.mask('Tref', tkeep)
            self.stateindex.mask('Omref', rkeep)
        droplist = nonzero(-keep)[0]
        dp = cor.mapbuffer_alloc(self.control, cor.packet_feature_drop, len(droplist) * 2)
        dp.header.user = len(droplist)
        indices = cor.packet_feature_drop_t_indices(dp)
        indices[:] = droplist
        cor.mapbuffer_enqueue(self.control, dp, time)


    def fixgroups(self, name, mask, oldslice):
        """Called by the index to remap the groups - leave it out of droppoints so changes get propagated"""
        if name == 'rho':
            groups = []
            goodgroups = []
            remap = 0
            for g, good in zip(self.state.groups, self.state.goodgroups):
                nc = mask[g].sum()
                if nc != 0:
                    groups.append(slice(remap, remap+nc))
                    goodgroups.append(good)
                    remap += nc
            self.state.groups = groups
            self.state.goodgroups = goodgroups

    def imu_meas_update(self, meas, meas_cov):
        linearized_measurement = MatrixModel(self.measurementindex, self.stateindex)
        GravityMeasurementModel.predict_measurements(self.measurement_model, self.state, self.prediction, linearized_measurement)
        self.innovation.compact = meas.compact - self.prediction.compact
      
        self.plotinna.x(self.innovation.alpha[0])
        self.plotinna.y(self.innovation.alpha[1])
        self.plotinna.z(self.innovation.alpha[2])
        self.plotinna.autoscale()
        self.plotinno.x(self.innovation.om[0])
        self.plotinno.y(self.innovation.om[1])
        self.plotinno.z(self.innovation.om[2])
        self.plotinno.autoscale()

        C = self.covar.compact
        lm = linearized_measurement.compact[:6]
        inn = self.innovation.compact[:6]
        state = self.state.compact
        mc = meas_cov.compact[:6]

        cmc = cor.matrix(C.shape[0], C.shape[1])
        cmlm = cor.matrix(lm.shape[0], lm.shape[1])
        cminn = cor.matrix(1, 6)
        cmstate = cor.matrix(1, state.shape[0])
        cmmc = cor.matrix(1, 6)
        
        cmc_d = cor.matrix_dereference(cmc)
        cmc_d[:] = C
        cmlm_d = cor.matrix_dereference(cmlm)
        cmlm_d[:] = lm
        cminn_d = cor.matrix_dereference(cminn)
        cminn_d[:] = inn.reshape((1,-1))
        cmstate_d = cor.matrix_dereference(cmstate)
        cmstate_d[:] = state.reshape((1,-1))
        cmmc_d = cor.matrix_dereference(cmmc)
        cmmc_d[:] = mc.reshape((1,-1))
#        debug_here()
        filter.meas_update(cmstate, cmc, cminn, cmlm, cmmc)
        self.covar.compact = cmc_d
        self.state.compact = cmstate_d

        cor.matrix_free(cmc)
        cor.matrix_free(cmlm)
        cor.matrix_free(cminn)    
        cor.matrix_free(cmstate)    
        cor.matrix_free(cmmc)    

#        self.filt.meas_update(self.state, self.covar, self.innovation.compact[0:6], linearized_measurement.compact[0:6], meas_cov.compact[0:6])

############################
    def vis_meas_update(self, meas, meas_cov, robust=True):
        if meas.points.size == 0:
            return
        linearized_measurement = MatrixModel(self.measurementindex, self.stateindex)
        SFMGroupsMeasurementModel.predict_measurements(self.measurement_model, self.state, self.prediction, linearized_measurement)
        self.innovation.points = meas.points - self.prediction.points

        if robust:
            residuals = DataModel(self.stateindex)
            residuals.rho = self.innovation.points[0::2]**2 + self.innovation.points[1::2]**2
            mc = DataModel(meas_cov.index, meas_cov.storage.copy())
            t = mc.points[0]
            outlier = 6
            #residuals.rho = sqrt(self.innovation.points[0::2]**2 + self.innovation.points[1::2]**2)
            thresh = t * outlier * outlier
            #SFMGroupsMeasurementModel.test_correspondence(self.measurement_model, self.state, meas, residuals, thresh)
            pinn = residuals.rho
            rv = sqrt(pinn / thresh)
#            print rv
            #for i in xrange(len(self.state.groups)):
            #    if not self.state.goodgroups[i]:
            #        rv[self.state.groups[i]] = 0.
            #make the badness the sum of the components
            outmask = rv > 1
            nout = outmask.sum()
            ntot = outmask.shape[0]
            if nout > ntot * .9:
                # always keep at least the 90th percentile
                thresh = sort(rv)[ntot * .1]
                rv = rv / thresh
                outmask = rv > 1
                nout = outmask.sum()
            if nout:
                mc.points[0::2][outmask] = (t * rv)[outmask]
                mc.points[1::2][outmask] = (t * rv)[outmask]
#only allow this measurement to update depth
                #linearized_measurement['points','Om'][mmask] = 0.
                #linearized_measurement['points','T'][mmask] = 0.
                #linearized_measurement['points','Omcb'][mmask] = 0.
                #linearized_measurement['points','Tcb'][mmask] = 0.
                #linearized_measurement['points','Omref'][mmask] = 0.
                #linearized_measurement['points','Tref'][mmask] = 0.
        
                self.outlier.rho += rv*outlier * outlier
                self.outlier.rho[-outmask] = 0.
                #self.outlier.rho[self.outlier.rho < 0] = 0
        else:
            mc = meas_cov

        C = self.covar.compact
        lm = linearized_measurement.compact[6:]
        inn = self.innovation.points
        state = self.state.compact
        mc = meas_cov.points

        cmc = cor.matrix(C.shape[0], C.shape[1])
        cmlm = cor.matrix(lm.shape[0], lm.shape[1])
        cminn = cor.matrix(1, self.innovation.points.shape[0])
        cmstate = cor.matrix(1, state.shape[0])
        cmmc = cor.matrix(1, mc.shape[0])
        
        cmc_d = cor.matrix_dereference(cmc)
        cmc_d[:] = C
        cmlm_d = cor.matrix_dereference(cmlm)
        cmlm_d[:] = lm
        cminn_d = cor.matrix_dereference(cminn)
        cminn_d[:] = inn.reshape((1,-1))
        cmstate_d = cor.matrix_dereference(cmstate)
        cmstate_d[:] = state.reshape((1,-1))
        cmmc_d = cor.matrix_dereference(cmmc)
        cmmc_d[:] = mc.reshape((1,-1))
#        debug_here()
        filter.meas_update(cmstate, cmc, cminn, cmlm, cmmc)
        self.covar.compact = cmc_d
        self.state.compact = cmstate_d

        cor.matrix_free(cmc)
        cor.matrix_free(cmlm)
        cor.matrix_free(cminn)    
        cor.matrix_free(cmstate)    
        cor.matrix_free(cmmc)    



        #self.filt.meas_update(self.state, self.covar, self.innovation.points, linearized_measurement.compact[6:], mc.points)

############################

class OneVarStateModel:
    def __init__(self, state):
        self.state = state
        self.state.alloc('x', 1)    

    def time_update(self, filt):
        pass
    
    def predict_y(self, filt):
        filt.linearized_measurement['y', 'x'] = identity(1)
        return self.state.x

class NullMeasurementModel:
    def __init__(self, prediction):
        self.prediction = prediction
        self.prediction.index.alloc('y', 1)

    def predict_measurements(self, state_model, filt):
        self.prediction.y[:] = state_model.predict_y(filt)

class ConstantFilter(FullFilter):
    def __init__(self):
        FullFilter.__init__(self)
        self.state_model = OneVarStateModel(self.state)
        self.measurement_model = NullMeasurementModel(self.prediction)
        self.filt = KalmanFilter(self.state, self.covar, self.prediction)

