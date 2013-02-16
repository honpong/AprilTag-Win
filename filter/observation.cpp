#include "observation.h"

void observation_queue::add_observation(observation *obs) { observations.push_back(obs); meas_size += obs->size; }

void observation_queue::add_preobservation(preobservation *pre) { preobservations.push_back(pre); }

int observation_queue::preprocess(state_vision *state, bool linearize) {
    for(list<preobservation *>::iterator pre = preobservations.begin(); pre != preobservations.end(); ++pre) (*pre)->process(state, linearize);
    sort(observations.begin(), observations.end(), observation_comp);
    return meas_size;
}

void observation_queue::clear() {
    for(list<preobservation *>::iterator pre = preobservations.begin(); pre != preobservations.end(); pre = preobservations.erase(pre)) delete *pre;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) delete *obs;
    observations.clear();
    meas_size = 0;
}

void observation_queue::predict(state_vision *state, matrix &pred, matrix *_lp) {
    preprocess(state, _lp?true:false);
    int index = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->predict(state, pred, index, _lp);
        index += (*obs)->size;
    }
}

void observation_queue::measure(matrix &meas) {
    int index = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->measure(meas, index);
        index += (*obs)->size;
    }
}

void observation_queue::robust_covariance(matrix &inn, matrix &m_cov) {
    int index = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->robust_covariance(inn, m_cov, index);
        index += (*obs)->size;
    }
}

observation_queue::observation_queue(): meas_size(0) {}

void preobservation_vision_base::process(state_vision *state, bool linearize) {
    R = rodrigues(state->W, linearize?&dR_dW:NULL);
    Rt = transpose(R);
    Rbc = rodrigues(state->Wc, linearize?&dRbc_dWc:NULL);
    Rcb = transpose(Rbc);
    RcbRt = Rcb * Rt;

    if(linearize) {
        dRt_dW = transpose(dR_dW);
        dRcb_dWc = transpose(dRbc_dWc);
    }
}

void preobservation_vision_group::process(state_vision *state, bool linearize) {
    Rr = rodrigues(group->Wr, linearize?&dRr_dWr:NULL);
    Rw = Rr * base->Rbc;
    Rtot = base->RcbRt * Rw;
    Tw = Rr * state->Tc + group->Tr;
    Ttot = base->Rcb * (base->Rt * (Tw - state->T) - state->Tc);
    
    if(linearize) {
        dRtot_dW  = base->Rcb * base->dRt_dW * Rw;
        dRtot_dWr = (base->Rcb * base->Rt) * dRr_dWr * base->Rbc;
        dRtot_dWc = base->dRcb_dWc * (base->Rt * Rw) + (base->RcbRt * Rr) * base->dRbc_dWc;
        dTtot_dWc = base->dRcb_dWc * (base->Rt * (Tw - state->T) - state->Tc);
        dTtot_dW  = base->Rcb * (base->dRt_dW * (Tw - state->T));
        dTtot_dWr = base->RcbRt * (dRr_dWr * state->Tc);
        dTtot_dT  =-base->RcbRt;
        dTtot_dTc = base->Rcb * (base->Rt * Rr - m4_identity);
        dTtot_dTr = base->RcbRt;
    }
}

void observation_vision_feature::predict(state_vision *state, matrix &pred, int index, matrix *_lp) {
    //fprintf(stderr, "this feature's uncalibrated scanline is %f, translates to time delta %f\n", i->uncalibrated[1], i->uncalibrated[1]/480. * 33000.);
    f_t rho = exp(feature->v);
    v4
        X0 = feature->initial * rho, /*not homog in v4*/
        Xr = base->Rbc * X0 + state->Tc,
        Xw = group->Rw * X0 + group->Tw,
        Xl = base->Rt * (Xw - state->T),
        X = group->Rtot * X0 + group->Ttot;
    feature->local = Xl;
    feature->relative = Xr;
    feature->world = Xw;
    f_t invZ = 1./X[2];
    v4 prediction = X * invZ;
    pred[index] = prediction[0];
    pred[index+1] = prediction[1];
    assert(fabs(prediction[2]-1.) < 1.e-7 && prediction[3] == 0.);

    if(_lp) {
        matrix &lp = *_lp;
        m4  dy_dX;
        dy_dX.data[0] = v4(invZ, 0., -X[0] * invZ * invZ, 0.);
        dy_dX.data[1] = v4(0., invZ, -X[1] * invZ * invZ, 0.);
        v4
            dX_dp = group->Rtot * X0, // dX0_dp = X0
            dy_dp = dy_dX * dX_dp;
        
        m4
            dy_dW = dy_dX * (group->dRtot_dW * X0 + group->dTtot_dW),
            dy_dT = dy_dX * group->dTtot_dT,
            dy_dWc = dy_dX * (group->dRtot_dWc * X0 + group->dTtot_dWc),
            dy_dTc = dy_dX * group->dTtot_dTc,
            dy_dWr = dy_dX * (group->dRtot_dWr * X0 + group->dTtot_dWr),
            dy_dTr = dy_dX * group->dTtot_dTr;
        
        for(int j = 0; j < 2; ++j) {
            int mi = index + j;
            if(feature->status) // != feature_initializing && i->status != feature_ready)
                lp(mi, feature->index) = dy_dp[j];
            for(int k = 0; k < 3; ++k) {
                if(state_group->status != group_initializing) {
                    lp(mi, state->W.index + k) = dy_dW[j][k];
                    lp(mi, state->T.index + k) = dy_dT[j][k];
                    if(state->estimate_calibration) {
                        lp(mi, state->Wc.index + k) = dy_dWc[j][k];
                        lp(mi, state->Tc.index + k) = dy_dTc[j][k];
                    }
                    lp(mi, state_group->Wr.index + k) = dy_dWr[j][k];
                    //if(g->status != group_reference) {
                    lp(mi, state_group->Tr.index + k) = dy_dTr[j][k];
                    //    }
                }
            }
        }
    }
}

void observation_vision_feature::measure(matrix &meas, int index) {
    meas[index] = measurement[0];
    meas[index+1] = measurement[1];
}

void observation_vision_feature::robust_covariance(matrix &inn, matrix &m_cov, int index) {
    f_t ot = feature->outlier_thresh * feature->outlier_thresh;

    f_t residual = inn[index]*inn[index] + inn[index+1]*inn[index+1];
    f_t badness = residual; //outlier_count <= 0  ? outlier_inn[i] : outlier_ess[i];
    f_t robust_mc;
    f_t thresh = feature->measurement_var * ot;
    if(badness > thresh) {
        f_t ratio = sqrt(badness / thresh);
        robust_mc = ratio * feature->measurement_var;
        feature->outlier += ratio;
    } else {
        robust_mc = feature->measurement_var;
        feature->outlier = 0.;
    }
    m_cov[index] = robust_mc;
    m_cov[index+1] = robust_mc;
}
