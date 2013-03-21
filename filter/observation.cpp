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

observation_vision_feature_initializing *observation_queue::new_observation_vision_feature_initializing(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent)
{
    observation_vision_feature_initializing *obs = new observation_vision_feature_initializing(_state, _time_actual, _time_apparent, meas_size, lp, m_cov, pred, meas, inn);
    observations.push_back(obs);
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
    v4 ippred = X * invZ; //in the image plane
    assert(fabs(ippred[2]-1.) < 1.e-7 && ippred[3] == 0.);

    feature_t norm, delta, kr;
    norm.x = ippred[0];
    norm.y = ippred[1];
    cal_get_params(base->cal, norm, &kr, &delta);
    feature->prediction.x = pred[0] = (norm.x * kr.x + delta.x) * base->cal->F.x + base->cal->C.x;
    feature->prediction.y = pred[1] = (norm.y * kr.y + delta.y) * base->cal->F.y + base->cal->C.y;

    if(linearize) {
        m4  dy_dX;
        dy_dX.data[0] = kr.x * base->cal->F.x * v4(invZ, 0., -X[0] * invZ * invZ, 0.);
        dy_dX.data[1] = kr.y * base->cal->F.y * v4(0., invZ, -X[1] * invZ * invZ, 0.);
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

bool observation_vision_feature::measure()
{
    meas[0] = feature->current[0];
    meas[1] = feature->current[1];
    return (meas[0] != INFINITY);
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

void observation_vision_feature_initializing::predict(bool linearize)
{
    feature->prediction.x = feature->current[0];
    feature->prediction.y = feature->current[1];
}

f_t project_pt_to_segment(f_t x, f_t y, f_t x0, f_t y0, f_t x1, f_t y1)
{
    f_t dx = x1 - x0, dy = y1 - y0;
    f_t l2 = (dx * dx + dy * dy);
    f_t vx = x - x0, vy = y - y0;
    return (dx * vx + dy * vy) / l2;
}

//direct triangulation version
/*bool observation_vision_feature_initializing::measure()
{
    m4 Rr = rodrigues(feature->Wr, NULL);

    m4 
        Rw = Rr * base->Rbc,
        Rtot = base->RcbRt * Rw;
    v4
        Tw = Rr * state->Tc + feature->Tr,
        Ttot = base->Rcb * (base->Rt * (Tw - state->T) - state->Tc);

    v4 Rx = Rtot * feature->initial;

    f_t depth[3];
    depth[0] = -4.5; //1cm ~ exp(-4.5)
    depth[1] = 10.; //20km ~ exp(10)

    feature_t bounds[2];
    f_t answer;
    bool done = false;

    fprintf(stderr, "\nStarting a new feature, previous depth: %f, current %f %f\n", feature->v, feature->current[0], feature->current[1]);
    while(!done) {
        f_t param;
        for(int i = 0; i < 2; ++i) {
            f_t rho = exp(depth[i]);
            v4 X = Rx * rho + Ttot;
            v4 pred = X * (1. / X[2]);
            feature_t norm, delta, kr;
            norm.x = pred[0];
            norm.y = pred[1];
            cal_get_params(base->cal, norm, &kr, &delta);
            bounds[i].x = (norm.x * kr.x + delta.x) * base->cal->F.x + base->cal->C.x;
            bounds[i].y = (norm.y * kr.y + delta.y) * base->cal->F.y + base->cal->C.y;
            fprintf(stderr, "bounds %d = %f %f\n", i, bounds[i].x, bounds[i].y);
            fprintf(stderr, "depth %d=%f\n", i, depth[i]);
            if(i == 1) {
                param = project_pt_to_segment(feature->current[0], feature->current[1], bounds[0].x, bounds[0].y, bounds[1].x, bounds[1].y);
                fprintf(stderr, "param = %f\n", param);
                if(param < 0.) {
                    answer = depth[0];
                    done = true;
                    break;
                }
                if(param > 1.) {
                    answer = depth[1];
                    done = true;
                    break;
                }
                depth[2] = depth[0] + param * (depth[1] - depth[0]);
            }
        }
        if(done) break;
        f_t param1 = project_pt_to_segment(feature->current[0], feature->current[1], bounds[0].x, bounds[0].y, bounds[2].x, bounds[2].y);
        f_t param2 = project_pt_to_segment(feature->current[0], feature->current[1], bounds[2].x, bounds[2].y, bounds[1].x, bounds[1].y);
        f_t new_param;
        fprintf(stderr, "param1 is %f, param2 is %f\n", param1, param2);
        if(param1 >= 1. && param2 >= 0. && param2 <= 1.) {
            //it's in segment2
            new_param = param2;
            depth[0] = depth[2];
        } else if(param2 <= 0. && param1 >= 0. && param1 <= 1.) {
            //it's in segment1
            new_param = param1;
            depth[1] = depth[2];
        } else {
            //can't tell - just use this estimate
            answer = depth[2];
            done = true;
        }
        if(!done && fabs(param - new_param) < 1.e-2) {
            answer = depth[0] + new_param * (depth[1] - depth[0]);
            done = true;
        }
        param = new_param;
    }
    fprintf(stderr, "done with feature. depth = %f\n", answer);

    feature->v = answer;
    feature->variance = feature->min_add_vis_cov - .0001;
    //    if(feat->index != -1) f->s.cov(feat->index, feat->index) = feat->variance;
    if(feature->status == feature_initializing)
        if(feature->variance < feature->min_add_vis_cov) feature->status = feature_ready;
    if(feature->v < -3. || feature->v > 6. || isnan(feature->variance) || feature->variance <= 0.) feature->status = feature_reject; //avoid degenerate features
    return true;
}*/

//ukf version
bool observation_vision_feature_initializing::measure()
{
    f_t alpha, kappa, lambda, beta;
    alpha = 1.e-3;
    kappa = 0.;
    lambda = alpha * alpha * (1. + kappa) - 1.;
    beta = 2.;
    f_t W0m = lambda / (1. + lambda);
    f_t W0c = W0m + 1. - alpha * alpha + beta;
    f_t Wi = 1. / (2. * (1. + lambda));
    f_t gamma = sqrt(1. + lambda);

    W0m = 1/3.;
    W0c = 1./3.;
    Wi = 1./3.;
    gamma = sqrt(1./(3.));

    m4 Rr = rodrigues(feature->Wr, NULL);

    m4 
        Rw = Rr * base->Rbc,
        Rtot = base->RcbRt * Rw;
    v4
        Tw = Rr * state->Tc + feature->Tr,
        Ttot = base->Rcb * (base->Rt * (Tw - state->T) - state->Tc);

    f_t stdev = sqrt(feature->variance);
    f_t x[3];
    x[0] = *feature;
    x[1] = *feature + gamma * stdev;
    x[2] = *feature - gamma * stdev;
    //fprintf(stderr, "fv is %f, gamma is %f, stdev is %f, x[1] is %f, x[2] is %f\n", feature->variance, gamma, stdev, x[1], x[2]);
    MAT_TEMP(y, 3, 2);
    for(int i = 0; i < 3; ++i) {
        f_t rho = exp(x[i]);
        v4
            X0 = feature->initial * rho, //not homog in v4
            X = Rtot * X0 + Ttot;

        f_t invZ = 1./X[2];
        v4 prediction = X * invZ; //in the image plane
        assert(fabs(prediction[2]-1.) < 1.e-7 && prediction[3] == 0.);

        feature_t norm, delta, kr;
        norm.x = prediction[0];
        norm.y = prediction[1];
        cal_get_params(base->cal, norm, &kr, &delta);
        y(i, 0) = (norm.x * kr.x + delta.x) * base->cal->F.x + base->cal->C.x;
        y(i, 1) = (norm.y * kr.y + delta.y) * base->cal->F.y + base->cal->C.y;
    }
    f_t meas_mean[2];
    meas_mean[0] = W0m * y(0, 0) + Wi * (y(1, 0) + y(2, 0));
    meas_mean[1] = W0m * y(0, 1) + Wi * (y(1, 1) + y(2, 1));

    f_t inn[2];
    inn[0] = feature->current[0] - meas_mean[0];
    inn[1] = feature->current[1] - meas_mean[1];
    
    f_t m_cov = feature->measurement_var;
    MAT_TEMP(Pyy, 2, 2);
    for(int i = 0; i < 2; ++i) {
        for(int j = i; j < 2; ++j) {
            Pyy(i, j) = W0c * (y(0,i) - meas_mean[i]) * (y(0,j) - meas_mean[j]);
            for(int k = 1; k < 3; ++k) {
                Pyy(i, j) += Wi * (y(k, i) - meas_mean[i]) * (y(k, j) - meas_mean[j]);
            }
            Pyy(j, i) = Pyy(i, j);
        }
        Pyy(i,i) += m_cov;
    }
    MAT_TEMP(Pxy, 1, 2);
    for(int j = 0; j < 2; ++j) {
        Pxy(0, j) = W0c * (x[0] - *feature) * (y(0,j) - meas_mean[j]);
        for(int k = 1; k < 3; ++k) {
            Pxy(0, j) += Wi * (x[k] - *feature) * (y(k, j) - meas_mean[j]);
        }
    }

    MAT_TEMP(gain, 1, 2);
    MAT_TEMP (Pyy_inverse, Pyy.rows, Pyy.cols);
    f_t invdet = 1. / (Pyy(0,0) * Pyy(1,1) - Pyy(0,1) * Pyy(1,0));
    Pyy_inverse(0,0) = invdet * Pyy(1,1);
    Pyy_inverse(1,1) = invdet * Pyy(0,0);
    Pyy_inverse(1,0) = -invdet * Pyy(0,1);
    Pyy_inverse(0,1) = -invdet * Pyy(1,0);

    gain[0] = Pxy[0] * Pyy_inverse(0, 0) + Pxy[1] * Pyy_inverse(1, 0);
    gain[1] = Pxy[0] * Pyy_inverse(0, 1) + Pxy[1] * Pyy_inverse(1, 1);

    feature->v += inn[0] * gain[0] + inn[1] * gain[1];
    f_t PKt[2];
    PKt[0] = gain[0] * Pyy(0, 0) + gain[1] * Pyy(0, 1);
    PKt[1] = gain[0] * Pyy(1, 0) + gain[1] * Pyy(1, 1);
    feature->variance -= gain[0] * PKt[0] + gain[1] * PKt[1]; //feature->min_add_vis_cov - .001; 
    //    if(feat->index != -1) f->s.cov(feat->index, feat->index) = feat->variance;
    if(feature->status == feature_initializing)
        if(feature->variance < feature->min_add_vis_cov) feature->status = feature_ready;
    if(feature->v < -3. || feature->v > 6. || isnan(feature->variance) || feature->variance <= 0.) feature->status = feature_reject; //avoid degenerate features
    return true;
}

//#include "../numerics/kalman.h"

//ekf version
/*bool observation_vision_feature_initializing::measure()
{

    m4 Rr = rodrigues(feature->Wr, NULL);

    m4 
        Rw = Rr * base->Rbc,
        Rtot = base->RcbRt * Rw;
    v4
        Tw = Rr * state->Tc + feature->Tr,
        Ttot = base->Rcb * (base->Rt * (Tw - state->T) - state->Tc);


    f_t rho = exp(feature->v);
    v4
        X0 = feature->initial * rho, //not homog in v4
        Xr = base->Rbc * X0 + state->Tc,
        Xw = Rw * X0 + Tw,
        Xl = base->Rt * (Xw - state->T),
        X = Rtot * X0 + Ttot;
    feature->local = Xl;
    feature->relative = Xr;
    feature->world = Xw;
    f_t invZ = 1./X[2];
    v4 ippred = X * invZ; //in the image plane
    assert(fabs(ippred[2]-1.) < 1.e-7 && ippred[3] == 0.);

    feature_t norm, delta, kr;
    norm.x = ippred[0];
    norm.y = ippred[1];
    cal_get_params(base->cal, norm, &kr, &delta);
    feature->prediction.x = (norm.x * kr.x + delta.x) * base->cal->F.x + base->cal->C.x;
    feature->prediction.y = (norm.y * kr.y + delta.y) * base->cal->F.y + base->cal->C.y;

    m4  dy_dX;
    dy_dX.data[0] = kr.x * base->cal->F.x * v4(invZ, 0., -X[0] * invZ * invZ, 0.);
    dy_dX.data[1] = kr.y * base->cal->F.y * v4(0., invZ, -X[1] * invZ * invZ, 0.);
    v4
        dX_dp = Rtot * X0, // dX0_dp = X0
        dy_dp = dy_dX * dX_dp;

    MAT_TEMP(lp, 2, 1);
    lp(0, 0) = dy_dp[0];
    lp(1, 0) = dy_dp[1];
    MAT_TEMP(state, 1, 1);
    state[0] = feature->v;
    MAT_TEMP(inn, 1, 2);
    inn[0] = feature->current[0] - feature->prediction.x;
    inn[1] = feature->current[1] - feature->prediction.y;
    MAT_TEMP(cov, 1, 1);
    cov[0] = feature->variance;
    MAT_TEMP(m_cov, 1, 2);
    m_cov[0] = m_cov[1] = feature->measurement_var;
    //measurement covariance
    MAT_TEMP(S, 2, 2);
    S(0, 0) = dy_dp[0] * cov * dy_dp[0] + m_cov;
    S(0, 1) = dy_dp[0] * cov * dy_dp[1];
    S(1, 0) = S(0, 1);
    S(1, 1) = dy_dp[1] * cov * dy_dp[1] + m_cov;

    MAT_TEMP(gain, 1, 2);
    MAT_TEMP (S_inverse, 2, 2);
    f_t invdet = 1. / (S(0,0) * S(1,1) - S(0,1) * S(1,0));
    S_inverse(0,0) = invdet * S(1,1);
    S_inverse(1,1) = invdet * S(0,0);
    S_inverse(1,0) = -invdet * S(0,1);
    S_inverse(0,1) = -invdet * S(1,0);

    gain[0] = cov * (dy_dp[0] * S_inverse(0, 0) + dy_dp[1] * S_inverse(1, 0));
    gain[1] = cov * (dy_dp[0] * S_inverse(0, 1) + dy_dp[1] * S_inverse(1, 1));

    meas_update(state, cov, inn, lp, m_cov);

    fprintf(stderr, "feature depth was %f, variance %f\n", feature->v, feature->variance);
    feature->v = state[0];
    feature->variance = cov[0]; //-= gain[0] * dy_dp[0] * feature->variance + gain[1] * dy_dp[1] * feature->variance;
    fprintf(stderr, "feature depth is now %f, vairance %f\n", feature->v, feature->variance);
    //    if(feat->index != -1) f->s.cov(feat->index, feat->index) = feat->variance;
    if(feature->status == feature_initializing)
        if(feature->variance < feature->min_add_vis_cov) feature->status = feature_ready;
    if(feature->v < -3. || feature->v > 6. || isnan(feature->variance) || feature->variance <= 0.) feature->status = feature_reject; //avoid degenerate features
    return true;
}
*/
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
            //lp(i, state->g.index) = Rt[i][2];
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


