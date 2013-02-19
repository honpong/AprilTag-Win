#include "observation.h"

void observation_queue::grow_matrices(int inc)
{
    lp.resize(lp.rows + inc, lp.cols);
    m_cov.resize(m_cov.cols + inc);
    pred.resize(inn.cols + inc);
    meas.resize(inn.cols + inc);
    inn.resize(inn.cols + inc);
}

observation_vision_feature *observation_queue::new_observation_vision_feature(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent)
{
    grow_matrices(2);
    observation_vision_feature *obs = new observation_vision_feature(_state, _time_actual, _time_apparent, meas_size, lp, m_cov, pred, meas, inn);
    observations.push_back(obs);
    meas_size += 2;
    return obs;
}

observation_accelerometer *observation_queue::new_observation_accelerometer(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent)
{
    grow_matrices(3);
    observation_accelerometer *obs = new observation_accelerometer(_state, _time_actual, _time_apparent, meas_size, lp, m_cov, pred, meas, inn);
    observations.push_back(obs);
    meas_size += 3;
    return obs;
}

observation_gyroscope *observation_queue::new_observation_gyroscope(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent)
{
    grow_matrices(3);
    observation_gyroscope *obs = new observation_gyroscope(_state, _time_actual, _time_apparent, meas_size, lp, m_cov, pred, meas, inn);
    observations.push_back(obs);
    meas_size += 3;
    return obs;
}

template<class T> T *observation_queue::new_preobservation(state *s)
{
    T *pre = new T(s);
    preobservations.push_back(pre);
    return pre;
}

template preobservation_vision_base *observation_queue::new_preobservation<preobservation_vision_base>(state_vision *_state);
template preobservation_vision_group *observation_queue::new_preobservation<preobservation_vision_group>(state_vision *_state);

int observation_queue::preprocess(bool linearize, int statesize)
{
    for(list<preobservation *>::iterator pre = preobservations.begin(); pre != preobservations.end(); ++pre) (*pre)->process(linearize);
    stable_sort(observations.begin(), observations.end(), observation_comp_apparent);
    return meas_size;
}

void observation_queue::clear()
{
    for(list<preobservation *>::iterator pre = preobservations.begin(); pre != preobservations.end(); pre = preobservations.erase(pre)) delete *pre;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) delete *obs;
    observations.clear();
    meas_size = 0;
    lp.resize(0, MAXSTATESIZE);
    m_cov.resize(0);
    pred.resize(0);
    meas.resize(0);
    inn.resize(0);
}

void observation_queue::predict(bool linearize, int statesize)
{
    lp.resize(lp.rows, statesize);
    preprocess(linearize, statesize);
    if(linearize) memset(lp_storage, 0, sizeof(lp_storage));
    preprocess(linearize, statesize);
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->predict(linearize);
    }
}

void observation_queue::compute_innovation()
{
    for(int i = 0; i < meas_size; ++i) inn[i] = meas[i] - pred[i];
}

void observation_queue::compute_covariance()
{
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->compute_covariance();
    }
}

observation_queue::observation_queue(): meas_size(0), lp((f_t*)lp_storage, 0, MAXSTATESIZE, MAXOBSERVATIONSIZE, MAXSTATESIZE), m_cov((f_t*)m_cov_storage, 1, 0, 1, MAXOBSERVATIONSIZE), pred((f_t*)pred_storage, 1, 0, 1, MAXOBSERVATIONSIZE), meas((f_t*)meas_storage, 1, 0, 1, MAXOBSERVATIONSIZE), inn((f_t*)inn_storage, 1, 0, 1, MAXOBSERVATIONSIZE)
 {}

void preobservation_vision_base::process(bool linearize)
{
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

void preobservation_vision_group::process(bool linearize)
{
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

void observation_vision_feature::predict(bool linearize)
{
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
    pred[0] = prediction[0];
    pred[1] = prediction[1];
    assert(fabs(prediction[2]-1.) < 1.e-7 && prediction[3] == 0.);

    if(linearize) {
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
            int mi = j;
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

void observation_vision_feature::compute_covariance()
{
    f_t ot = feature->outlier_thresh * feature->outlier_thresh;

    f_t residual = inn[0]*inn[0] + inn[1]*inn[1];
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
    m_cov[0] = robust_mc;
    m_cov[1] = robust_mc;
}

void observation_accelerometer::predict(bool linearize)
{
    m4v4 dR_dW;
    m4 Rt = transpose(rodrigues(state->W, linearize?&dR_dW:NULL));
    v4 acc = v4(0., 0., state->g, 0.);
    if(!initializing) acc += state->a;
    v4 pred_a = Rt * acc + state->a_bias;

    for(int i = 0; i < 3; ++i) {
        pred[i] = pred_a[i];
    }

    if(linearize) {
        m4 dya_dW = transpose(dR_dW) * acc;
        for(int i = 0; i < 3; ++i) {
            lp(i, state->g.index) = Rt[i][2];
            lp(i, state->a_bias.index + i) = 1.;
            for(int j = 0; j < 3; ++j) {
                if(!initializing) lp(i, state->a.index + j) = Rt[i][j];
                lp(i, state->W.index + j) = dya_dW[i][j];
            }
        }
    }
}

void observation_gyroscope::predict(bool linearize)
{
    v4
        pred_w = state->w_bias;
    if(!initializing) pred_w += state->w;

    for(int i = 0; i < 3; ++i) {
        pred[i] = pred_w[i];
    }

    if(linearize) {
        for(int i = 0; i < 3; ++i) {
            if(!initializing) lp(i, state->w.index + i) = 1.;
            lp(i, state->w_bias.index + i) = 1.;
        }
    }
}


