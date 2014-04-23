#include "observation.h"
#include "tracker.h"

stdev_scalar observation_vision_feature::stdev[2], observation_vision_feature::inn_stdev[2];
stdev_vector observation_accelerometer::stdev, observation_accelerometer::inn_stdev, observation_gyroscope::stdev, observation_gyroscope::inn_stdev;

void observation_queue::preprocess()
{
    stable_sort(observations.begin(), observations.end(), observation_comp_apparent);
}

void observation_queue::clear()
{
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) delete *obs;
    observations.clear();
}

void observation_queue::predict()
{
    preprocess();
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->predict();
    }
}

void observation_queue::compute_measurement_covariance()
{
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->compute_measurement_covariance();
    }
}

observation_queue::observation_queue(): LC((f_t*)LC_storage, MAXOBSERVATIONSIZE, MAXSTATESIZE, MAXOBSERVATIONSIZE, MAXSTATESIZE), K((f_t*)K_storage, MAXSTATESIZE, MAXOBSERVATIONSIZE, MAXSTATESIZE, MAXOBSERVATIONSIZE), res_cov((f_t*)res_cov_storage, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE)
 {}

void observation_vision_feature::predict()
{
    m4 R = to_rotation_matrix(state.W.v);
    Rt = transpose(R);
    Rbc = to_rotation_matrix(state.Wc.v);
    Rcb = transpose(Rbc);
    RcbRt = Rcb * Rt;

    Rr = to_rotation_matrix(state_group->Wr.v);
    Rw = Rr * Rbc;
    Rtot = RcbRt * Rw;
    Tw = Rr * state.Tc.v + state_group->Tr.v;
    Ttot = Rcb * (Rt * (Tw - state.T.v) - state.Tc.v);

    norm_initial.x = (feature->initial[0] - state.center_x.v) / state.focal_length.v;
    norm_initial.y = (feature->initial[1] - state.center_y.v) / state.focal_length.v;

    f_t r2, r4, r6, kr;
    state.fill_calibration(norm_initial, r2, r4, r6, kr);
    feature->calibrated = v4(norm_initial.x / kr, norm_initial.y / kr, 1., 0.);

    v4 X0_unscale = feature->calibrated * feature->v.depth(); //not homog in v4
    X0 = feature->calibrated;
    X = Rtot * feature->calibrated + Ttot * feature->v.invdepth();

    //Inverse depth
    //Should work because projection(R X + T) = projection(R (X/p) + T/p)
    //(This is not the same as saying that RX+T = R(X/p) + T/p, which is false)
    //Have verified that the above identity is numerically identical in my results
    v4 X_unscale = Rtot * X0_unscale + Ttot;

    feature->relative = Rbc * X0_unscale + state.Tc.v;
    feature->world = Rw * X0_unscale + Tw;
    feature->local = Rt * (feature->world - state.T.v);
    feature->depth = X_unscale[2];
    v4 ippred = X / X[2]; //in the image plane
    if(fabs(ippred[2]-1.) > 1.e-7 || ippred[3] != 0.) {
        fprintf(stderr, "FAILURE in feature projection in observation_vision_feature::predict\n");
    }

    norm_predicted.x = ippred[0];
    norm_predicted.y = ippred[1];

    state.fill_calibration(norm_predicted, r2, r4, r6, kr);
    feature->prediction.x = pred[0] = norm_predicted.x * kr * state.focal_length.v + state.center_x.v;
    feature->prediction.y = pred[1] = norm_predicted.y * kr * state.focal_length.v + state.center_y.v;
}

void observation_vision_feature::cache_jacobians()
{
    //initial = (uncal - center) / (focal_length * kr)
    f_t r2, r4, r6, kr;
    state.fill_calibration(norm_initial, r2, r4, r6, kr);
    v4 dX_dcx = Rtot * v4(-1. / (kr * state.focal_length.v), 0., 0., 0.);
    v4 dX_dcy = Rtot * v4(0., -1. / (kr * state.focal_length.v), 0., 0.);
    v4 dX_dF = Rtot * v4(-X0[0] / state.focal_length.v, -X0[1] / state.focal_length.v, 0., 0.);
    v4 dX_dk1 = Rtot * v4(-X0[0] / kr * r2, -X0[1] / kr * r2, 0., 0.);
    v4 dX_dk2 = Rtot * v4(-X0[0] / kr * r4, -X0[1] / kr * r4, 0., 0.);
    
    m4v4 dR_dW = to_rotation_matrix_jacobian(state.W.v);
    m4v4 dRbc_dWc = to_rotation_matrix_jacobian(state.Wc.v);
    m4v4 dRt_dW = transpose(dR_dW);
    m4v4 dRcb_dWc = transpose(dRbc_dWc);
    m4v4 dRr_dWr = to_rotation_matrix_jacobian(state_group->Wr.v);
    
    m4v4 dRtot_dW  = Rcb * dRt_dW * Rw;
    m4v4 dRtot_dWr = RcbRt * dRr_dWr * Rbc;
    m4v4 dRtot_dWc = dRcb_dWc * (Rt * Rw) + (RcbRt * Rr) * dRbc_dWc;
    m4 dTtot_dWc = dRcb_dWc * (Rt * (Tw - state.T.v) - state.Tc.v);
    m4 dTtot_dW  = Rcb * (dRt_dW * (Tw - state.T.v));
    m4 dTtot_dWr = RcbRt * (dRr_dWr * state.Tc.v);
    m4 dTtot_dT  =-RcbRt;
    m4 dTtot_dTc = RcbRt * Rr - Rcb;
    m4 dTtot_dTr = RcbRt;
    
    state.fill_calibration(norm_predicted, r2, r4, r6, kr);
    f_t invZ = 1. / X[2];
    v4 dx_dX, dy_dX;
    dx_dX = kr * state.focal_length.v * v4(invZ, 0., -X[0] * invZ * invZ, 0.);
    dy_dX = kr * state.focal_length.v * v4(0., invZ, -X[1] * invZ * invZ, 0.);
    
    dx_dF = norm_predicted.x * kr + sum(dx_dX * dX_dF);
    dy_dF = norm_predicted.y * kr + sum(dy_dX * dX_dF);
    dx_dk1 = norm_predicted.x * state.focal_length.v * r2 + sum(dx_dX * dX_dk1);
    dy_dk1 = norm_predicted.y * state.focal_length.v * r2 + sum(dy_dX * dX_dk1);
    dx_dk2 = norm_predicted.x * state.focal_length.v * r4 + sum(dx_dX * dX_dk2);
    dy_dk2 = norm_predicted.y * state.focal_length.v * r4 + sum(dy_dX * dX_dk2);
    dx_dcx = 1. + sum(dx_dX * dX_dcx);
    dx_dcy = sum(dx_dX * dX_dcy);
    dy_dcx = sum(dy_dX * dX_dcx);
    dy_dcy = 1. + sum(dy_dX * dX_dcy);
    
    v4 dX_dp = Ttot * feature->v.invdepth_jacobian();
    dx_dp = sum(dx_dX * dX_dp);
    dy_dp = sum(dy_dX * dX_dp);
    f_t invrho = feature->v.invdepth();
    if(!feature->is_initialized()) {
        dx_dW = dx_dX * (dRtot_dW * feature->calibrated),
        dx_dWc = dx_dX * (dRtot_dWc * feature->calibrated),
        dx_dWr = dx_dX * (dRtot_dWr * feature->calibrated);
        dy_dW = dy_dX * (dRtot_dW * feature->calibrated),
        dy_dWc = dy_dX * (dRtot_dWc * feature->calibrated),
        dy_dWr = dy_dX * (dRtot_dWr * feature->calibrated);
        //dy_dT = m4(0.);
        //dy_dT = m4(0.);
        //dy_dTr = m4(0.);
    } else {
        dx_dW = dx_dX * (dRtot_dW * X0 + dTtot_dW * invrho);
        dx_dWc = dx_dX * (dRtot_dWc * X0 + dTtot_dWc * invrho);
        dx_dWr = dx_dX * (dRtot_dWr * X0 + dTtot_dWr * invrho);
        dx_dT = dx_dX * dTtot_dT * invrho;
        dx_dTc = dx_dX * dTtot_dTc * invrho;
        dx_dTr = dx_dX * dTtot_dTr * invrho;
        dy_dW = dy_dX * (dRtot_dW * X0 + dTtot_dW * invrho);
        dy_dWc = dy_dX * (dRtot_dWc * X0 + dTtot_dWc * invrho);
        dy_dWr = dy_dX * (dRtot_dWr * X0 + dTtot_dWr * invrho);
        dy_dT = dy_dX * dTtot_dT * invrho;
        dy_dTc = dy_dX * dTtot_dTc * invrho;
        dy_dTr = dy_dX * dTtot_dTr * invrho;
    }
}

void observation_vision_feature::project_covariance(matrix &dst, const matrix &src)
{

    if(!feature->is_initialized()) {
        for(int j = 0; j < dst.cols; ++j) {
            f_t cov_feat = feature->copy_cov_from_row(src, j);
            f_t cov_F = state.focal_length.copy_cov_from_row(src, j);
            f_t cov_cx = state.center_x.copy_cov_from_row(src, j);
            f_t cov_cy = state.center_y.copy_cov_from_row(src, j);
            f_t cov_k1 = state.k1.copy_cov_from_row(src, j);
            f_t cov_k2 = state.k2.copy_cov_from_row(src, j);
            v4 cov_W = state.W.copy_cov_from_row(src, j);
            v4 cov_Wc = state.Wc.copy_cov_from_row(src, j);
            v4 cov_Wr = state_group->Wr.copy_cov_from_row(src, j);
            dst(0, j) = dx_dp * cov_feat +
            dx_dF * cov_F +
            dx_dcx * cov_cx +
            dx_dcy * cov_cy +
            dx_dk1 * cov_k1 +
            dx_dk2 * cov_k2 +
            //dy_dk3[i] * state.k3.copy_cov_from_row(src, j) +
            sum(dx_dW * cov_W) +
            sum(dx_dWc * cov_Wc) +
            sum(dx_dWr * cov_Wr);
            dst(1, j) = dy_dp * cov_feat +
            dy_dF * cov_F +
            dy_dcx * cov_cx +
            dy_dcy * cov_cy +
            dy_dk1 * cov_k1 +
            dy_dk2 * cov_k2 +
            //dy_dk3[i] * state.k3.copy_cov_from_row(src, j) +
            sum(dy_dW * cov_W) +
            sum(dy_dWc * cov_Wc) +
            sum(dy_dWr * cov_Wr);
        }
    } else {
        for(int j = 0; j < dst.cols; ++j) {
            f_t cov_feat = feature->copy_cov_from_row(src, j);
            f_t cov_F = state.focal_length.copy_cov_from_row(src, j);
            f_t cov_cx = state.center_x.copy_cov_from_row(src, j);
            f_t cov_cy = state.center_y.copy_cov_from_row(src, j);
            f_t cov_k1 = state.k1.copy_cov_from_row(src, j);
            f_t cov_k2 = state.k2.copy_cov_from_row(src, j);
            v4 cov_W = state.W.copy_cov_from_row(src, j);
            v4 cov_Wc = state.Wc.copy_cov_from_row(src, j);
            v4 cov_Wr = state_group->Wr.copy_cov_from_row(src, j);
            v4 cov_T = state.T.copy_cov_from_row(src, j);
            v4 cov_Tc = state.Tc.copy_cov_from_row(src, j);
            v4 cov_Tr = state_group->Tr.copy_cov_from_row(src, j);
            dst(0, j) = dx_dp * cov_feat +
            dx_dF * cov_F +
            dx_dcx * cov_cx +
            dx_dcy * cov_cy +
            dx_dk1 * cov_k1 +
            dx_dk2 * cov_k2 +
            //dy_dk3[i] * p[state.k3.index] +
            sum(dx_dW * cov_W) +
            sum(dx_dT * cov_T) +
            sum(dx_dWc * cov_Wc) +
            sum(dx_dTc * cov_Tc) +
            sum(dx_dWr * cov_Wr) +
            sum(dx_dTr * cov_Tr);
            dst(1, j) = dy_dp * cov_feat +
            dy_dF * cov_F +
            dy_dcx * cov_cx +
            dy_dcy * cov_cy +
            dy_dk1 * cov_k1 +
            dy_dk2 * cov_k2 +
            //dy_dk3[i] * p[state.k3.index] +
            sum(dy_dW * cov_W) +
            sum(dy_dT * cov_T) +
            sum(dy_dWc * cov_Wc) +
            sum(dy_dTc * cov_Tc) +
            sum(dy_dWr * cov_Wr) +
            sum(dy_dTr * cov_Tr);
        }
    }
}

f_t observation_vision_feature::projection_residual(const v4 & X_inf, const f_t inv_depth, const feature_t &found)
{
    v4 X = X_inf + inv_depth * Ttot;
    f_t invZ = 1./X[2];
    v4 ippred = X * invZ; //in the image plane
    if(fabs(ippred[2]-1.) > 1.e-7 || ippred[3] != 0.) {
        fprintf(stderr, "FAILURE in feature projection in observation_vision_feature::predict\n");
    }
    feature_t norm, uncalib;
    f_t r2, r4, r6, kr;
    
    norm.x = ippred[0];
    norm.y = ippred[1];
    
    state.fill_calibration(norm, r2, r4, r6, kr);
    
    uncalib.x = norm.x * kr * state.focal_length.v + state.center_x.v;
    uncalib.y = norm.y * kr * state.focal_length.v + state.center_y.v;
    f_t dx = uncalib.x - found.x;
    f_t dy = uncalib.y - found.y;
    return dx * dx + dy * dy;
}

bool observation_vision_feature::measure()
{
    f_t x1, y1, x2, y2;
    float error1, error2;
    feature_t bestkp, bestkp1, bestkp2;

    x1 = pred[0] - 5;
    x2 = pred[0] + 5;
    y1 = pred[1] - 5;
    y2 = pred[1] + 5;

    bestkp1 = tracker.track(im1, im2, feature->current[0], feature->current[1], x1, y1, x2, y2, error1);

    x1 = feature->current[0] + feature->image_velocity.x - 5;
    x2 = feature->current[0] + feature->image_velocity.x + 5;
    y1 = feature->current[1] + feature->image_velocity.y - 5;
    y2 = feature->current[1] + feature->image_velocity.y + 5;

    bestkp2 = tracker.track(im1, im2, feature->current[0], feature->current[1], x1, y1, x2, y2, error2);

    if(error1 < error2)
        bestkp = bestkp1;
    else
        bestkp = bestkp2;

    if(bestkp.x == INFINITY) {
        feature->image_velocity.x = 0;
        feature->image_velocity.y = 0;
    }
    else {
        feature->image_velocity.x  = bestkp.x - feature->current[0];
        feature->image_velocity.y  = bestkp.y - feature->current[1];
    }

    meas[0] = feature->current[0] = bestkp.x;
    meas[1] = feature->current[1] = bestkp.y;

    meas[0] = feature->current[0];
    meas[1] = feature->current[1];
    valid = meas[0] != INFINITY;
    if(valid) {
        stdev[0].data(meas[0]);
        stdev[1].data(meas[1]);
        if(!feature->is_initialized()) {
            //TODO: revisit this triangulation
            f_t min = 0.01; //infinity-ish (100m)
            f_t max = 10.; //1/.10 for 10cm
            f_t min_d2, max_d2;
            v4 X_inf = Rtot * feature->calibrated;

            v4 X_inf_proj = X_inf / X_inf[2];
            v4 X_0 = X_inf + max * Ttot;

            v4 X_0_proj = X_0 / X_0[2];
            v4 delta = (X_inf_proj - X_0_proj);
            f_t pixelvar = sum(delta * delta) * state.focal_length.v * state.focal_length.v;
            if(pixelvar > 1 * 1 * state_vision_feature::measurement_var) { //tells us if we have enough baseline
                feature->status = feature_normal;
            }

            min_d2 = projection_residual(X_inf, min, bestkp);
            max_d2 = projection_residual(X_inf, max, bestkp);
            f_t best = min;
            f_t best_d2 = min_d2;
            for(int i = 0; i < 10; ++i) { //10 iterations = 1024 segments
                if(min_d2 < max_d2) {
                    max = (min + max) / 2.;
                    max_d2 = projection_residual(X_inf, max, bestkp);
                    if(min_d2 < best_d2) {
                        best_d2 = min_d2;
                        best = min;
                    }
                } else {
                    min = (min + max) / 2.;
                    min_d2 = projection_residual(X_inf, min, bestkp);
                    if(max_d2 < best_d2) {
                        best_d2 = max_d2;
                        best = max;
                    }
                }
            }
            if(best > 0.01 && best < 10.) {
                feature->v.set_depth_meters(1./best);
            }
            //TODO: come back and look at this - previously was uselessly resetting feature->variance
            //state.cov(feature->index, feature->index) = state_vision_feature::initial_var;
            //repredict using triangulated depth
            predict();
        }
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

/*
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
}*/

//direct triangulation version
/*bool observation_vision_feature_initializing::measure()
{
    m4 Rr = rodrigues(feature->Wr, NULL);

    m4 
        Rw = Rr * base->Rbc,
        Rtot = base->RcbRt * Rw;
    v4
        Tw = Rr * state.Tc + feature->Tr,
        Ttot = base->Rcb * (base->Rt * (Tw - state.T) - state.Tc);

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
/*bool observation_vision_feature_initializing::measure()
{
    float error;
    feature_t bestkp = base->tracker.track(base->im1, base->im2, feature->current[0], feature->current[1], feature->current[0] - 10, feature->current[1] - 10, feature->current[0] + 10, feature->current[1] + 10, error);
    feature->current[0] = bestkp.x;
    feature->current[1] = bestkp.y;

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

    m4 Rr = to_rotation_matrix(feature->Wr);

    m4 
        Rw = Rr * base->Rbc,
        Rtot = base->RcbRt * Rw;
    v4
        Tw = Rr * state.Tc.v + feature->Tr,
        Ttot = base->Rcb * (base->Rt * (Tw - state.T.v) - state.Tc.v);

    f_t stdev = sqrt(feature->variance[0]);
    f_t x[3];
    x[0] = feature->v;
    x[1] = feature->v + gamma * stdev;
    x[2] = feature->v - gamma * stdev;
    //fprintf(stderr, "fv is %f, gamma is %f, stdev is %f, x[1] is %f, x[2] is %f\n", feature->variance, gamma, stdev, x[1], x[2]);
    MAT_TEMP(y, 3, 2);
    //do the two sigma points
    for(int i = 1; i < 3; ++i) {
        f_t rho = exp(x[i]);

        feature_t initial = {(float)feature->initial[0], (float)feature->initial[1]};
        feature_t calib = state.calibrate_feature(initial);
        feature->calibrated = v4(calib.x, calib.y, 1., 0.);

        v4
            X0 = feature->calibrated * rho, //not homog in v4
            X = Rtot * X0 + Ttot;

        f_t invZ = 1./X[2];
        v4 prediction = X * invZ; //in the image plane
        assert(fabs(prediction[2]-1.) < 1.e-7 && prediction[3] == 0.);

        feature_t norm = { (float)prediction[0], (float)prediction[1] };
        f_t r2, r4, r6, kr;
        state.fill_calibration(norm, r2, r4, r6, kr);

        y(i, 0) = norm.x * kr * state.focal_length.v + state.center_x.v;
        y(i, 1) = norm.y * kr * state.focal_length.v + state.center_y.v;
    }
    //do the mean, and save output
    f_t rho = exp(feature->v);
    
    feature_t initial = {(float)feature->initial[0], (float)feature->initial[1]};
    feature_t calib = state.calibrate_feature(initial);
    feature->calibrated = v4(calib.x, calib.y, 1., 0.);
    
    v4
        X0 = feature->calibrated * rho; //not homog in v4
    
    v4
        Xr = base->Rbc * X0 + state.Tc.v,
        Xw = Rw * X0 + Tw,
        Xl = base->Rt * (Xw - state.T.v),
        X = Rtot * X0 + Ttot;

    feature->local = Xl;
    feature->relative = Xr;
    feature->world = Xw;
    feature->depth = X[2];

    f_t invZ = 1./X[2];
    v4 prediction = X * invZ; //in the image plane
    assert(fabs(prediction[2]-1.) < 1.e-7 && prediction[3] == 0.);
    
    feature_t norm = { (float)prediction[0], (float)prediction[1] };
    f_t r2, r4, r6, kr;
    state.fill_calibration(norm, r2, r4, r6, kr);
    
    y(0, 0) = norm.x * kr * state.focal_length.v + state.center_x.v;
    y(0, 1) = norm.y * kr * state.focal_length.v + state.center_y.v;
    
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
        Pxy(0, j) = W0c * (x[0] - feature->v) * (y(0,j) - meas_mean[j]);
        for(int k = 1; k < 3; ++k) {
            Pxy(0, j) += Wi * (x[k] - feature->v) * (y(k, j) - meas_mean[j]);
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
    feature->variance[0] -= gain[0] * PKt[0] + gain[1] * PKt[1]; //feature->min_add_vis_cov - .001;
    //    if(feat->index != -1) f->s.cov(feat->index, feat->index) = feat->variance;
    if(feature->status == feature_initializing)
        if(feature->variance[0] < feature->min_add_vis_cov) feature->status = feature_ready;
    if(feature->v < -3. || feature->v > 6. || isnan(feature->variance[0]) || feature->variance[0] <= 0.) feature->status = feature_reject; //avoid degenerate features
    return true;
}*/

//#include "../numerics/kalman.h"

//ekf version
/*bool observation_vision_feature_initializing::measure()
{

    m4 Rr = rodrigues(feature->Wr, NULL);

    m4 
        Rw = Rr * base->Rbc,
        Rtot = base->RcbRt * Rw;
    v4
        Tw = Rr * state.Tc + feature->Tr,
        Ttot = base->Rcb * (base->Rt * (Tw - state.T) - state.Tc);


    f_t rho = exp(feature->v);
    v4
        X0 = feature->initial * rho, //not homog in v4
        Xr = base->Rbc * X0 + state.Tc,
        Xw = Rw * X0 + Tw,
        Xl = base->Rt * (Xw - state.T),
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
void observation_accelerometer::predict()
{
    Rt = transpose(to_rotation_matrix(state.W.v));
    v4 acc = v4(0., 0., state.g.v, 0.);
    if(!state.orientation_only)
    {
        acc += state.a.v;
    }
    v4 pred_a = Rt * acc + state.a_bias.v;
    for(int i = 0; i < 3; ++i) {
        pred[i] = pred_a[i];
    }
}

void observation_accelerometer::cache_jacobians()
{
    dR_dW = to_rotation_matrix_jacobian(state.W.v);
    v4 acc = v4(0., 0., state.g.v, 0.);
    if(!state.orientation_only)
    {
        acc += state.a.v;
    }
    dya_dW = transpose(dR_dW) * acc;
}

void observation_accelerometer::project_covariance(matrix &dst, const matrix &src)
{
    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    assert(dst.cols == src.rows);
    if(!state.orientation_only)
    {
        for(int j = 0; j < dst.cols; ++j) {
            v4 cov_a_bias = state.a_bias.copy_cov_from_row(src, j);
            v4 cov_W = state.W.copy_cov_from_row(src, j);
            v4 cov_a = state.a.copy_cov_from_row(src, j);
            f_t cov_g = state.g.copy_cov_from_row(src, j);
            v4 res = cov_a_bias + dya_dW * cov_W + Rt * (cov_a + v4(0., 0., cov_g, 0.));
            for(int i = 0; i < 3; ++i) {
                dst(i, j) = res[i];
            }
        }
    } else {
        for(int j = 0; j < dst.cols; ++j) {
            v4 cov_a_bias = state.a_bias.copy_cov_from_row(src, j);
            v4 cov_W = state.W.copy_cov_from_row(src, j);
            v4 res = cov_a_bias + dya_dW * cov_W;
            for(int i = 0; i < 3; ++i) {
                dst(i, j) = res[i];
            }
        }
    }
}

bool observation_accelerometer::measure()
{
    stdev.data(v4(meas[0], meas[1], meas[2], 0.));
    if(!state.orientation_initialized)
    {
        //first measurement - use to determine orientation
        //cross product of this with "up": (0,0,1)
        v4 s = v4(meas[1], -meas[0], 0., 0.) / norm(v4(meas[0], meas[1], meas[2], 0.));
        v4 s2 = s * s;
        f_t sintheta = sqrt(sum(s2));
        f_t theta = asin(sintheta);
        if(meas[2] < 0.) {
            //direction of z component tells us we're flipped - sin(x) = sin(pi - x)
            theta = M_PI - theta;
        }
        if(sintheta < 1.e-7) {
            state.W.v = rotation_vector(s[0], s[1], s[2]);
        } else{
            v4 snorm = s * (theta / sintheta);
            state.W.v = rotation_vector(snorm[0], snorm[1], snorm[2]);
        }
        state.orientation_initialized = true;
        valid = false;
        return false;
    } else return observation_spatial::measure();
}

void observation_gyroscope::predict()
{
    v4 pred_w = state.w_bias.v + state.w.v;

    for(int i = 0; i < 3; ++i) {
        pred[i] = pred_w[i];
    }
}

void observation_gyroscope::cache_jacobians()
{
}

void observation_gyroscope::project_covariance(matrix &dst, const matrix &src)
{
    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    for(int j = 0; j < dst.cols; ++j) {
        v4 cov_w = state.w.copy_cov_from_row(src, j);
        v4 cov_wbias = state.w_bias.copy_cov_from_row(src, j);
        v4 res = cov_w + cov_wbias;
        for(int i = 0; i < 3; ++i) {
            dst(i, j) = res[i];
        }
    }
}
