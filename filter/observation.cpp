#include "observation.h"
#include "tracker.h"

stdev_scalar observation_vision_feature::stdev[2], observation_vision_feature::inn_stdev[2];
stdev_vector observation_accelerometer::stdev, observation_accelerometer::inn_stdev, observation_gyroscope::stdev, observation_gyroscope::inn_stdev;

void observation_queue::grow_matrices(int inc)
{
    m_cov.resize(m_cov.cols + inc);
    pred.resize(inn.cols + inc);
    meas.resize(inn.cols + inc);
    inn.resize(inn.cols + inc);
    inn_cov.resize(inn_cov.cols + inc);
}

observation_vision_feature *observation_queue::new_observation_vision_feature(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent)
{
    grow_matrices(2);
    observation_vision_feature *obs = new observation_vision_feature(_state, _time_actual, _time_apparent, meas_size, m_cov, pred, meas, inn, inn_cov);
    observations.push_back(obs);
    meas_size += 2;
    return obs;
}

observation_vision_feature_initializing *observation_queue::new_observation_vision_feature_initializing(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent)
{
    observation_vision_feature_initializing *obs = new observation_vision_feature_initializing(_state, _time_actual, _time_apparent, meas_size, m_cov, pred, meas, inn, inn_cov);
    observations.push_back(obs);
    return obs;
}

observation_accelerometer *observation_queue::new_observation_accelerometer(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent)
{
    grow_matrices(3);
    observation_accelerometer *obs = new observation_accelerometer(_state, _time_actual, _time_apparent, meas_size, m_cov, pred, meas, inn, inn_cov);
    observations.push_back(obs);
    meas_size += 3;
    return obs;
}

observation_gyroscope *observation_queue::new_observation_gyroscope(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent)
{
    grow_matrices(3);
    observation_gyroscope *obs = new observation_gyroscope(_state, _time_actual, _time_apparent, meas_size, m_cov, pred, meas, inn, inn_cov);
    observations.push_back(obs);
    meas_size += 3;
    return obs;
}

preobservation_vision_base *observation_queue::new_preobservation_vision_base(state_vision *state, int width, int height, struct tracker t)
{
    preobservation_vision_base *pre = new preobservation_vision_base(state, width, height, t);
    preobservations.push_back(pre);
    return pre;
}

preobservation_vision_group *observation_queue::new_preobservation_vision_group(state_vision *_state)
{
    preobservation_vision_group *pre = new preobservation_vision_group(_state);
    preobservations.push_back(pre);
    return pre;
}

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
    m_cov.resize(0);
    pred.resize(0);
    meas.resize(0);
    inn.resize(0);
    inn_cov.resize(0);
}

void observation_queue::predict(bool linearize, int statesize)
{
    preprocess(linearize, statesize);
    preprocess(linearize, statesize);
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->predict(linearize);
    }
}

void observation_queue::compute_innovation()
{
    for(int i = 0; i < meas_size; ++i) inn[i] = meas[i] - pred[i];
}

void observation_queue::compute_measurement_covariance()
{
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->compute_measurement_covariance();
    }
}

observation_queue::observation_queue(): meas_size(0), m_cov((f_t*)m_cov_storage, 1, 0, 1, MAXOBSERVATIONSIZE), pred((f_t*)pred_storage, 1, 0, 1, MAXOBSERVATIONSIZE), meas((f_t*)meas_storage, 1, 0, 1, MAXOBSERVATIONSIZE), inn((f_t*)inn_storage, 1, 0, 1, MAXOBSERVATIONSIZE), inn_cov((f_t*)inn_cov_storage, 1, 0, 1, MAXOBSERVATIONSIZE), LC((f_t*)LC_storage, MAXOBSERVATIONSIZE, MAXSTATESIZE, MAXOBSERVATIONSIZE, MAXSTATESIZE), K((f_t*)K_storage, MAXSTATESIZE, MAXOBSERVATIONSIZE, MAXSTATESIZE, MAXOBSERVATIONSIZE), res_cov((f_t*)res_cov_storage, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE)
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
    feature_t norm, calib;
    f_t r2, r4, r6, kr;
    norm.x = (feature->initial[0] - state->center_x.v) / state->focal_length;
    norm.y = (feature->initial[1] - state->center_y.v) / state->focal_length;
    //forward calculation - guess calibrated from initial
    state->fill_calibration(norm, r2, r4, r6, kr);
    calib.x = norm.x / kr;
    calib.y = norm.y / kr;
    //backward calbulation - use calibrated guess to get new parameters and recompute
    state->fill_calibration(calib, r2, r4, r6, kr);
    v4 calibrated = v4(norm.x / kr, norm.y / kr, 1., 0.);

    X0 = calibrated * rho; /*not homog in v4*/

    v4
        Xr = base->Rbc * X0 + state->Tc,
        Xw = group->Rw * X0 + group->Tw,
        Xl = base->Rt * (Xw - state->T),
        X = group->Rtot * X0 + group->Ttot;

    //initial = (uncal - center) / (focal_length * kr)
    v4 dX_dcx = group->Rtot * v4(-rho / (kr * state->focal_length), 0., 0., 0.);
    v4 dX_dcy = group->Rtot * v4(0., -rho / (kr * state->focal_length), 0., 0.);
    v4 dX_dF = group->Rtot * v4(-X0[0] / state->focal_length, -X0[1] / state->focal_length, 0., 0.);
    v4 dX_dk1 = group->Rtot * v4(-X0[0] / kr * r2, -X0[1] / kr * r2, 0., 0.);
    v4 dX_dk2 = group->Rtot * v4(-X0[0] / kr * r4, -X0[1] / kr * r4, 0., 0.);
    v4 dX_dk3 = group->Rtot * v4(-X0[0] / kr * r6, -X0[1] / kr * r6, 0., 0.);

    feature->local = Xl;
    feature->relative = Xr;
    feature->world = Xw;
    feature->depth = X[2];
    f_t invZ = 1./X[2];
    v4 ippred = X * invZ; //in the image plane
    if(fabs(ippred[2]-1.) > 1.e-7 || ippred[3] != 0.) {
        fprintf(stderr, "FAILURE in feature projection in observation_vision_feature::predict\n");
    }

    norm.x = ippred[0];
    norm.y = ippred[1];

    state->fill_calibration(norm, r2, r4, r6, kr);
    feature->prediction.x = pred[0] = norm.x * kr * state->focal_length + state->center_x;
    feature->prediction.y = pred[1] = norm.y * kr * state->focal_length + state->center_y;
    dy_dX.data[0] = kr * state->focal_length * v4(invZ, 0., -X[0] * invZ * invZ, 0.);
    dy_dX.data[1] = kr * state->focal_length * v4(0., invZ, -X[1] * invZ * invZ, 0.);

    dy_dF[0] = norm.x * kr + sum(dy_dX[0] * dX_dF);
    dy_dF[1] = norm.y * kr + sum(dy_dX[1] * dX_dF);
    dy_dk1[0] = norm.x * state->focal_length * r2 + sum(dy_dX[0] * dX_dk1);
    dy_dk1[1] = norm.y * state->focal_length * r2 + sum(dy_dX[1] * dX_dk1);
    dy_dk2[0] = norm.x * state->focal_length * r4 + sum(dy_dX[0] * dX_dk2);
    dy_dk2[1] = norm.y * state->focal_length * r4 + sum(dy_dX[1] * dX_dk2);
    dy_dk3[0] = norm.x * state->focal_length * r6 + sum(dy_dX[0] * dX_dk3);
    dy_dk3[1] = norm.y * state->focal_length * r6 + sum(dy_dX[1] * dX_dk3);
    dy_dcx = v4(1. + sum(dy_dX[0] * dX_dcx), sum(dy_dX[1] * dX_dcx), 0., 0.);
    dy_dcy = v4(sum(dy_dX[0] * dX_dcy), 1. + sum(dy_dX[1] * dX_dcy), 0., 0.);
}

void observation_vision_feature::project_covariance(matrix &dst, const matrix &src)
{
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

    for(int i = 0; i < 2; ++i) {
        for(int j = 0; j < dst.cols; ++j) {
            const f_t *p = &src(j, 0);
            dst(i, j) = dy_dp[i] * p[feature->index] +
                dy_dF[i] * p[state->focal_length.index] +
                dy_dcx[i] * p[state->center_x.index] + 
                dy_dcy[i] * p[state->center_y.index] +
                dy_dk1[i] * p[state->k1.index] + 
                dy_dk2[i] * p[state->k2.index] + 
                //dy_dk3[i] * p[state->k3.index] + 
                sum(dy_dW[i] * v4(p[state->W.index], p[state->W.index + 1], p[state->W.index + 2], 0.)) +
                sum(dy_dT[i] * v4(p[state->T.index], p[state->T.index + 1], p[state->T.index + 2], 0.)) + 
                (state->estimate_calibration ?
                sum(dy_dWc[i] * v4(p[state->Wc.index], p[state->Wc.index + 1], p[state->Wc.index + 2], 0.)) + 
                                                                                                              sum(dy_dTc[i] * v4(p[state->Tc.index], p[state->Tc.index + 1], p[state->Tc.index + 2], 0.))
                 : 0.) +
                sum(dy_dWr[i] * v4(p[state_group->Wr.index], p[state_group->Wr.index + 1], p[state_group->Wr.index + 2], 0.)) + 
                sum(dy_dTr[i] * v4(p[state_group->Tr.index], p[state_group->Tr.index + 1], p[state_group->Tr.index + 2], 0.));
        }
    }
}

bool observation_vision_feature::measure()
{
    f_t x1, y1, x2, y2;
    x1 = pred[0] - 10;
    x2 = pred[0] + 10;
    y1 = pred[1] - 10;
    y2 = pred[1] + 10;

    feature_t bestkp = base->tracker.track(base->im1, base->im2, feature->current[0], feature->current[1], x1, y1, x2, y2);
    meas[0] = feature->current[0] = feature->uncalibrated[0] = bestkp.x;
    meas[1] = feature->current[1] = feature->uncalibrated[1] = bestkp.y;

    meas[0] = feature->current[0];
    meas[1] = feature->current[1];
    valid = meas[0] != INFINITY;
    if(valid) {
        stdev[0].data(meas[0]);
        stdev[1].data(meas[1]);
    }
    return valid;
}

void observation_vision_feature::compute_measurement_covariance()
{
    inn_stdev[0].data(inn[0]);
    inn_stdev[1].data(inn[1]);
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
    feature_t bestkp = base->tracker.track(base->im1, base->im2, feature->current[0], feature->current[1], feature->current[0] - 10, feature->current[1] - 10, feature->current[0] + 10, feature->current[1] + 10);
    feature->current[0] = feature->uncalibrated[0] = bestkp.x;
    feature->current[1] = feature->uncalibrated[1] = bestkp.y;

    valid = feature->current[0] != INFINITY;
    if(!valid) return valid;

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

        feature_t initial = {(float)feature->initial[0], (float)feature->initial[1]};
        feature_t calib = state->calibrate_feature(initial);
        v4 calibrated = v4(calib.x, calib.y, 1., 0.);

        v4
            X0 = calibrated * rho, //not homog in v4
            X = Rtot * X0 + Ttot;

        f_t invZ = 1./X[2];
        v4 prediction = X * invZ; //in the image plane
        assert(fabs(prediction[2]-1.) < 1.e-7 && prediction[3] == 0.);

        feature_t norm = { (float)prediction[0], (float)prediction[1] };
        f_t r2, r4, r6, kr;
        state->fill_calibration(norm, r2, r4, r6, kr);

        y(i, 0) = norm.x * kr * state->focal_length + state->center_x;
        y(i, 1) = norm.y * kr * state->focal_length + state->center_y;
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
    m4 Rt = transpose(rodrigues(state->W, NULL));
    v4 acc = v4(0., 0., state->g, 0.);
    if(!initializing) acc += state->a;
    v4 pred_a = Rt * acc + state->a_bias;

    for(int i = 0; i < 3; ++i) {
        pred[i] = pred_a[i];
    }
}

void observation_accelerometer::project_covariance(matrix &dst, const matrix &src)
{
    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    m4v4 dR_dW;
    m4 Rt = transpose(rodrigues(state->W, &dR_dW));
    v4 acc = v4(0., 0., state->g, 0.);
    if(!initializing) acc += state->a;
    m4 dya_dW = transpose(dR_dW) * acc;

    assert(dst.cols == src.rows);
    if(initializing) {
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < dst.cols; ++j) {
                const f_t *p = &src(j, 0);
                dst(i, j) = p[state->a_bias.index + i] + sum(dya_dW[i] * v4(p[state->W.index], p[state->W.index+1], p[state->W.index+2], 0.));
            }
        }
    } else {
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < dst.cols; ++j) {
                const f_t *p = &src(j, 0);
                dst(i, j) = p[state->a_bias.index + i] +
                    sum(dya_dW[i] * v4(p[state->W.index], p[state->W.index+1], p[state->W.index+2], 0.)) + 
                    sum(Rt[i] * v4(p[state->a.index], p[state->a.index+1], p[state->a.index+2], 0.));
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
}

void observation_gyroscope::project_covariance(matrix &dst, const matrix &src)
{
    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    if(initializing) {
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < dst.cols; ++j) {
                dst(i, j) = src(j, state->w_bias.index + i);
            }
        }
    } else {
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < dst.cols; ++j) {
                dst(i, j) = src(j, state->w.index + i) + src(j, state->w_bias.index + i);
            }
        }
    }
}
