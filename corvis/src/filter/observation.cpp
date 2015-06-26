#include "observation.h"
#include "tracker.h"
#include "../numerics/kalman.h"
#include "utils.h"

stdev_scalar observation_vision_feature::stdev[2], observation_vision_feature::inn_stdev[2];
stdev_vector observation_accelerometer::stdev, observation_accelerometer::inn_stdev, observation_gyroscope::stdev, observation_gyroscope::inn_stdev;

int observation_queue::size()
{
    int size = 0;
    for(auto &o : observations)
        size += o->size;
    return size;
}

void observation_queue::predict()
{
    for(auto &o : observations)
        o->predict();
}

void observation_queue::measure_and_prune()
{
    observations.erase(remove_if(observations.begin(), observations.end(), [this](auto &o) {
       bool ok = o->measure();
       if (!ok)
           cache_recent(std::move(o));
       return !ok;
    }), observations.end());
}

void observation_queue::compute_innovation(matrix &inn)
{
    int count = 0;
    for(auto &o : observations) {
        o->compute_innovation();
        for(int i = 0; i < o->size; ++i) {
            inn[count + i] = o->innovation(i);
        }
        count += o->size;
    }
}

void observation_queue::compute_measurement_covariance(matrix &m_cov)
{
    int count = 0;
    for(auto &o : observations) {
        o->compute_measurement_covariance();
        for(int i = 0; i < o->size; ++i) {
            m_cov[count + i] = o->measurement_covariance(i);
        }
        count += o->size;
    }
}

void observation_queue::compute_prediction_covariance(const state &s, int meas_size)
{
    //project state cov onto measurement to get cov(meas, state)
    // matrix_product(LC, lp, A, false, false);
    int statesize = s.cov.size();
    int index = 0;
    for(auto &o : observations) {
        if(o->size) {
            matrix dst(&LC(index, 0), o->size, statesize, meas_size-index, LC.get_stride());
            o->cache_jacobians();
            o->project_covariance(dst, s.cov.cov);
            index += o->size;
        }
    }
    
    //project cov(state, meas)=(LC)' onto meas to get cov(meas, meas)
    index = 0;
    for(auto &o : observations) {
        if(o->size) {
            matrix dst(&res_cov(index, 0), o->size, meas_size, meas_size-index, res_cov.get_stride());
            o->project_covariance(dst, LC);
            index += o->size;
        }
    }
    
    //enforce symmetry
    for(int i = 0; i < res_cov.rows(); ++i) {
        for(int j = i + 1; j < res_cov.cols(); ++j) {
            res_cov(i, j) = res_cov(j, i);
        }
    }
    
    index = 0;
    for(auto &o : observations) {
        if(o->size) o->set_prediction_covariance(res_cov, index);
        index += o->size;
    }
}

void observation_queue::compute_innovation_covariance(const matrix &m_cov)
{
    int index = 0;
    for(auto &o : observations) {
        for(int i = 0; i < o->size; ++i) {
            res_cov(index + i, index + i) += m_cov[index + i];
        }
        o->innovation_covariance_hook(res_cov, index);
        index += o->size;
    }
}

bool observation_queue::update_state_and_covariance(state &s, const matrix &inn)
{
#ifdef TEST_POSDEF
    if(!test_posdef(res_cov)) { fprintf(stderr, "observation covariance matrix not positive definite before computing gain!\n"); }
    f_t rcond = matrix_check_condition(res_cov);
    if(rcond < .001) { fprintf(stderr, "observation covariance matrix not well-conditioned before computing gain! rcond = %e\n", rcond);}
#endif
    if(kalman_compute_gain(K, LC, res_cov))
    {
        matrix state(1, s.cov.size());
        s.copy_state_to_array(state);
        kalman_update_state(state, K, inn);
        s.copy_state_from_array(state);
        kalman_update_covariance(s.cov.cov, K, LC);
        //Robust update is not needed and is much slower
        //kalman_update_covariance_robust(f->s.cov.cov, K, LC, res_cov);
        return true;
    } else {
        return false;
    }
}

observation_queue::observation_queue(): LC((f_t*)LC_storage, MAXOBSERVATIONSIZE, MAXSTATESIZE, MAXOBSERVATIONSIZE, MAXSTATESIZE), K((f_t*)K_storage, MAXSTATESIZE, MAXOBSERVATIONSIZE, MAXSTATESIZE, MAXOBSERVATIONSIZE), res_cov((f_t*)res_cov_storage, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE)
 {}

bool observation_queue::process(state &s, sensor_clock::time_point time)
{
#ifdef TEST_POSDEF
    if(!test_posdef(s.cov.cov)) fprintf(stderr, "not pos def when starting process_observation_queue\n");
#endif
    bool success = true;
    s.time_update(time);

    stable_sort(observations.begin(), observations.end(), observation_comp_apparent);

    predict();

    int orig_meas_size = size();

    measure_and_prune();

    int meas_size = size(), statesize = s.cov.size();
    if(meas_size) {
        matrix inn(1, meas_size);
        matrix m_cov(1, meas_size);
        LC.resize(meas_size, statesize);
        res_cov.resize(meas_size, meas_size);

        //TODO: implement o->time_apparent != o->time_actual
        compute_innovation(inn);
        compute_measurement_covariance(m_cov);
        compute_prediction_covariance(s, meas_size);
        compute_innovation_covariance(m_cov);
        success = update_state_and_covariance(s, inn);
    } else if(orig_meas_size != 3) {
        if(log_enabled) fprintf(stderr, "In Kalman update, original measurement size was %d, ended up with 0 measurements!\n", orig_meas_size);
    }

    for (auto &o : observations)
        cache_recent(std::move(o));

    observations.clear();
    f_t delta_T = (s.T.v - s.last_position).norm();
    if(delta_T > .01) {
        s.total_distance += (float)delta_T;
        s.last_position = s.T.v;
    }
#ifdef TEST_POSDEF
    if(!test_posdef(s.cov.cov)) {fprintf(stderr, "not pos def when finishing process observation queue\n"); assert(0);}
#endif
    return success;
}

void observation_vision_feature::innovation_covariance_hook(const matrix &cov, int index)
{
    feature->innovation_variance_x = cov(index, index);
    feature->innovation_variance_y = cov(index + 1, index + 1);
    feature->innovation_variance_xy = cov(index, index +1);
    if(show_tuning) {
        fprintf(stderr, " predicted stdev is %e %e\n", sqrt(cov(index, index)), sqrt(cov(index+1, index+1)));
    }
}

void observation_vision_feature::predict()
{
    m4 Rr = to_rotation_matrix(state_group->Wr.v);
    m4 R = to_rotation_matrix(state.W.v);
    Rrt = Rr.transpose();
    Rc = to_rotation_matrix(state.Wc.v);
    Rct = Rc.transpose();
    Rtot = Rct * Rrt * Rc;
    Ttot = Rct * (Rrt * (state.Tc.v - state_group->Tr.v) - state.Tc.v);

    norm_initial.x = (float)(((feature->initial[0] - state.image_width / 2. + .5) / state.image_width - state.center_x.v) / state.focal_length.v);
    norm_initial.y = (float)(((feature->initial[1] - state.image_height / 2. + .5) / state.image_width - state.center_y.v) / state.focal_length.v);

    f_t r2, kr;
    state.fill_calibration(norm_initial, r2, kr);
    X0 = v4(norm_initial.x / kr, norm_initial.y / kr, 1., 0.);

    v4 X0_unscale = X0 * feature->v.depth(); //not homog in v4

    //Inverse depth
    //Should work because projection(R X + T) = projection(R (X/p) + T/p)
    //(This is not the same as saying that RX+T = R(X/p) + T/p, which is false)
    //Have verified that the above identity is numerically identical in my results
    v4 X_unscale = Rtot * X0_unscale + Ttot;

    X = X_unscale * feature->v.invdepth();

    feature->relative = Rc * X0_unscale + state.Tc.v;
    feature->local = Rrt * (feature->relative - state_group->Tr.v);
    feature->world = R * feature->local + state.T.v;
    feature->depth = X_unscale[2];
    v4 ippred = X / X[2]; //in the image plane
    if(fabs(ippred[2]-1.) > 1.e-7 || ippred[3] != 0.) {
        fprintf(stderr, "FAILURE in feature projection in observation_vision_feature::predict\n");
    }

    norm_predicted.x = (float)ippred[0];
    norm_predicted.y = (float)ippred[1];

    state.fill_calibration(norm_predicted, r2, kr);
    pred[0] = (norm_predicted.x * kr * state.focal_length.v + state.center_x.v) * state.image_width + state.image_width / 2. - .5;
    pred[1] = (norm_predicted.y * kr * state.focal_length.v + state.center_y.v) * state.image_width + state.image_height / 2. - .5;
    feature->prediction.x = (float)pred[0];
    feature->prediction.y = (float)pred[1];
}

void observation_vision_feature::cache_jacobians()
{
    //initial = (uncal - center) / (focal_length * kr)
    f_t r2, kr;
    state.fill_calibration(norm_initial, r2, kr);
#if estimate_camera_intrinsics
    v4 dX_dcx = Rtot * v4(-1. / (kr * state.focal_length.v), 0., 0., 0.);
    v4 dX_dcy = Rtot * v4(0., -1. / (kr * state.focal_length.v), 0., 0.);
    v4 dX_dF = Rtot * v4(-X0[0] / state.focal_length.v, -X0[1] / state.focal_length.v, 0., 0.);
    v4 dX_dk1 = Rtot * v4(-X0[0] / kr * r2, -X0[1] / kr * r2, 0., 0.);
    v4 dX_dk2 = Rtot * v4(-X0[0] / kr * (r2 * r2), -X0[1] / kr * (r2 * r2), 0., 0.);
#endif
    
    v4 RtotX0 = Rtot * X0;
    m4 Rrt_dRr_dWr = to_body_jacobian(state_group->Wr.v);
#if estimate_camera_extrinsics
    m4 Rct_dRc_dWc = to_body_jacobian(state.Wc.v);
#endif
    
    m4 dRtotX0_dWr = skew3(RtotX0) * (Rct * Rrt_dRr_dWr);
    m4 dTtot_dWr = Rct * skew3(Rrt * (state.Tc.v - state_group->Tr.v)) * Rrt_dRr_dWr;
    m4 dTtot_dTr = -Rct * Rrt;
#if estimate_camera_extrinsics
    m4 dRtotX0_dWc = skew3(RtotX0) * (Rct_dRc_dWc - Rtot * Rct_dRc_dWc);
    m4 dTtot_dWc = skew3(Ttot) * Rct_dRc_dWc;
    m4 dTtot_dTc = Rct * (Rrt - m4::Identity());
#endif
    
    state.fill_calibration(norm_predicted, r2, kr);
    f_t invZ = 1. / X[2];
    v4 dx_dX, dy_dX;
    dx_dX = state.image_width * kr * state.focal_length.v * v4(invZ, 0., -X[0] * invZ * invZ, 0.);
    dy_dX = state.image_width * kr * state.focal_length.v * v4(0., invZ, -X[1] * invZ * invZ, 0.);

    v4 dX_dp = Ttot * feature->v.invdepth_jacobian();
    dx_dp = dx_dX.dot(dX_dp);
    dy_dp = dy_dX.dot(dX_dp);
    f_t invrho = feature->v.invdepth();
    if(!feature->is_initialized()) {
#if estimate_camera_extrinsics
        dx_dWc = dx_dX.transpose() * dRtotX0_dWc;
        dy_dWc = dy_dX.transpose() * dRtotX0_dWc;
#endif
        dx_dWr = dx_dX.transpose() * dRtotX0_dWr;
        dy_dWr = dy_dX.transpose() * dRtotX0_dWr;
        //dy_dT = m4(0.);
        //dy_dT = m4(0.);
        //dy_dTr = m4(0.);
    } else {
#if estimate_camera_intrinsics
        dx_dF = state.image_width * norm_predicted.x * kr + dx_dX.dot(dX_dF);
        dy_dF = state.image_width * norm_predicted.y * kr + dy_dX.dot(dX_dF);
        dx_dk1 = state.image_width * norm_predicted.x * state.focal_length.v * r2        + dx_dX.dot(dX_dk1);
        dy_dk1 = state.image_width * norm_predicted.y * state.focal_length.v * r2        + dy_dX.dot(dX_dk1);
        dx_dk2 = state.image_width * norm_predicted.x * state.focal_length.v * (r2 * r2) + dx_dX.dot(dX_dk2);
        dy_dk2 = state.image_width * norm_predicted.y * state.focal_length.v * (r2 * r2) + dy_dX.dot(dX_dk2);
        dx_dcx = state.image_width *  + dx_dX.dot(dX_dcx);
        dx_dcy = dx_dX.dot(dX_dcy);
        dy_dcx = dy_dX.dot(dX_dcx);
        dy_dcy = state.image_width *  + dy_dX.dot(dX_dcy);
#endif
        dx_dWr = dx_dX.transpose() * (dRtotX0_dWr + dTtot_dWr * invrho);
        dx_dTr = dx_dX.transpose() * dTtot_dTr * invrho;
        dy_dWr = dy_dX.transpose() * (dRtotX0_dWr + dTtot_dWr * invrho);
        dy_dTr = dy_dX.transpose() * dTtot_dTr * invrho;
#if estimate_camera_extrinsics
        dx_dWc = dx_dX.transpose() * (dRtotX0_dWc + dTtot_dWc * invrho);
        dx_dTc = dx_dX.transpose() * dTtot_dTc * invrho;
        dy_dWc = dy_dX.transpose() * (dRtotX0_dWc + dTtot_dWc * invrho);
        dy_dTc = dy_dX.transpose() * dTtot_dTc * invrho;
#endif

    }
}

void observation_vision_feature::project_covariance(matrix &dst, const matrix &src)
{

    if(!feature->is_initialized()) {
        for(int j = 0; j < dst.cols(); ++j) {
#if estimate_camera_extrinsics
            v4 cov_Wc = state.Wc.copy_cov_from_row(src, j);
#endif
            v4 cov_Wr = state_group->Wr.copy_cov_from_row(src, j);
            dst(0, j) =
#if estimate_camera_extrinsics
            dx_dWc.dot(cov_Wc) +
#endif
            dx_dWr.dot(cov_Wr);
            dst(1, j) =
#if estimate_camera_extrinsics
            dy_dWc.dot(cov_Wc) +
#endif
            dy_dWr.dot(cov_Wr);
        }
    } else {
        for(int j = 0; j < dst.cols(); ++j) {
            f_t cov_feat = feature->copy_cov_from_row(src, j);
            v4 cov_Wr = state_group->Wr.copy_cov_from_row(src, j);
            v4 cov_Tr = state_group->Tr.copy_cov_from_row(src, j);

#if estimate_camera_intrinsics
            f_t cov_F = state.focal_length.copy_cov_from_row(src, j);
            f_t cov_cx = state.center_x.copy_cov_from_row(src, j);
            f_t cov_cy = state.center_y.copy_cov_from_row(src, j);
            f_t cov_k1 = state.k1.copy_cov_from_row(src, j);
            f_t cov_k2 = state.k2.copy_cov_from_row(src, j);
#endif
#if estimate_camera_extrinsics
            v4 cov_Wc = state.Wc.copy_cov_from_row(src, j);
            v4 cov_Tc = state.Tc.copy_cov_from_row(src, j);
#endif
            dst(0, j) = dx_dp * cov_feat +
#if estimate_camera_intrinsics
            dx_dF * cov_F +
            dx_dcx * cov_cx +
            dx_dcy * cov_cy +
            dx_dk1 * cov_k1 +
            dx_dk2 * cov_k2 +
#endif
#if estimate_camera_extrinsics
            dx_dWc.dot(cov_Wc) +
            dx_dTc.dot(cov_Tc) +
#endif
            dx_dWr.dot(cov_Wr) +
            dx_dTr.dot(cov_Tr);
            dst(1, j) = dy_dp * cov_feat +
#if estimate_camera_intrinsics
            dy_dF * cov_F +
            dy_dcx * cov_cx +
            dy_dcy * cov_cy +
            dy_dk1 * cov_k1 +
            dy_dk2 * cov_k2 +
#endif
#if estimate_camera_extrinsics
            dy_dWc.dot(cov_Wc) +
            dy_dTc.dot(cov_Tc) +
#endif
            dy_dWr.dot(cov_Wr) +
            dy_dTr.dot(cov_Tr);
        }
    }
}

f_t observation_vision_feature::projection_residual(const v4 & X, const xy &found)
{
    f_t invZ = 1./X[2];
    v4 ippred = X * invZ; //in the image plane
    if(fabs(ippred[2]-1.) > 1.e-7 || ippred[3] != 0.) {
        fprintf(stderr, "FAILURE in feature projection in observation_vision_feature::predict\n");
    }
    feature_t norm, uncalib;
    f_t r2, kr;
    
    norm.x = (float)ippred[0];
    norm.y = (float)ippred[1];
    
    state.fill_calibration(norm, r2, kr);
    
    uncalib.x = (float)((norm.x * kr * state.focal_length.v + state.center_x.v) * state.image_width + state.image_width / 2. - .5);
    uncalib.y = (float)((norm.y * kr * state.focal_length.v + state.center_y.v) * state.image_width + state.image_height / 2. - .5);
    f_t dx = uncalib.x - found.x;
    f_t dy = uncalib.y - found.y;
    return dx * dx + dy * dy;
}

void observation_vision_feature::update_initializing()
{
    if(feature->is_initialized()) return;
    f_t min = 0.01; //infinity-ish (100m)
    f_t max = 10.; //1/.10 for 10cm
    f_t min_d2, max_d2;
    v4 X_inf = Rtot * X0;
    
    v4 X_inf_proj = X_inf / X_inf[2];
    v4 X_0 = X_inf + max * Ttot;
    
    v4 X_0_proj = X_0 / X_0[2];
    v4 delta = (X_inf_proj - X_0_proj);
    f_t pixelvar = delta.dot(delta) * state.focal_length.v * state.focal_length.v * state.image_width * state.image_width;
    if(pixelvar > 5. * 5. * state_vision_feature::measurement_var) { //tells us if we have enough baseline
        feature->status = feature_normal;
    }
    
    xy bestkp;
    bestkp.x = (float)meas[0];
    bestkp.y = (float)meas[1];
    
    min_d2 = projection_residual(X_inf + min * Ttot, bestkp);
    max_d2 = projection_residual(X_inf + max * Ttot, bestkp);
    f_t best = min;
    f_t best_d2 = min_d2;
    for(int i = 0; i < 10; ++i) { //10 iterations = 1024 segments
        if(min_d2 < max_d2) {
            max = (min + max) / 2.;
            max_d2 = projection_residual(X_inf + max * Ttot, bestkp);
            if(min_d2 < best_d2) {
                best_d2 = min_d2;
                best = min;
            }
        } else {
            min = (min + max) / 2.;
            min_d2 = projection_residual(X_inf + min * Ttot, bestkp);
            if(max_d2 < best_d2) {
                best_d2 = max_d2;
                best = max;
            }
        }
    }
    if(best > .01 && best < 10.) {
        feature->v.set_depth_meters(1./best);
    }
    //repredict using triangulated depth
    predict();
}

bool observation_vision_feature::measure()
{
    xy bestkp = tracker.track(feature->patch, image, (float)feature->current[0] + feature->image_velocity.x, (float)feature->current[1] + feature->image_velocity.y, tracker.radius, tracker.min_match);

    // Not a good enough match, try the filter prediction
    if(bestkp.score < tracker.good_match) {
        xy bestkp2 = tracker.track(feature->patch, image, (float)pred[0], (float)pred[1], tracker.radius, bestkp.score);
        if(bestkp2.score > bestkp.score)
            bestkp = bestkp2;
    }
    // Still no match? Guess that we haven't moved at all
    if(bestkp.score < tracker.min_match) {
        xy bestkp2 = tracker.track(feature->patch, image, (float)feature->current[0], (float)feature->current[1], 5.5, bestkp.score);
        if(bestkp2.score > bestkp.score)
            bestkp = bestkp2;
    }

    bool valid = bestkp.x != INFINITY;

    if(valid) {
        feature->image_velocity.x  = bestkp.x - (float)feature->current[0];
        feature->image_velocity.y  = bestkp.y - (float)feature->current[1];
    }
    else {
        feature->image_velocity.x = 0;
        feature->image_velocity.y = 0;
    }

    meas[0] = feature->current[0] = bestkp.x;
    meas[1] = feature->current[1] = bestkp.y;

    if(valid) {
        stdev[0].data(meas[0]);
        stdev[1].data(meas[1]);
        if(!feature->is_initialized()) {
            update_initializing();
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
void observation_accelerometer::predict()
{
    Rt = to_rotation_matrix(state.W.v).transpose();
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
    v4 acc = v4(0., 0., state.g.v, 0.);
    if(!state.orientation_only)
    {
        acc += state.a.v;
    }
    m4 Rt_dR_dW = to_body_jacobian(state.W.v);
    da_dW = skew3(Rt * acc) * Rt_dR_dW;
}

void observation_accelerometer::project_covariance(matrix &dst, const matrix &src)
{
    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    assert(dst.cols() == src.rows());
    if(!state.orientation_only)
    {
        for(int j = 0; j < dst.cols(); ++j) {
            v4 cov_a_bias = state.a_bias.copy_cov_from_row(src, j);
            v4 cov_W = state.W.copy_cov_from_row(src, j);
            v4 cov_a = state.a.copy_cov_from_row(src, j);
            f_t cov_g = state.g.copy_cov_from_row(src, j);
            v4 res = cov_a_bias + da_dW * cov_W + Rt * (cov_a + v4(0., 0., cov_g, 0.));
            for(int i = 0; i < 3; ++i) {
                dst(i, j) = res[i];
            }
        }
    } else {
        for(int j = 0; j < dst.cols(); ++j) {
            v4 cov_a_bias = state.a_bias.copy_cov_from_row(src, j);
            v4 cov_W = state.W.copy_cov_from_row(src, j);
            v4 res = (state.estimate_bias ? cov_a_bias : v4(0,0,0,0)) + da_dW * cov_W;
            for(int i = 0; i < 3; ++i) {
                dst(i, j) = res[i];
            }
        }
    }
}

bool observation_accelerometer::measure()
{
    v4 g(meas[0], meas[1], meas[2], 0);
    stdev.data(g);
    if(!state.orientation_initialized)
    {
        state.initial_orientation = initial_orientation_from_gravity(g);
        state.W.v = to_rotation_vector(state.initial_orientation);
        state.orientation_initialized = true;
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
    for(int j = 0; j < dst.cols(); ++j) {
        v4 cov_w = state.w.copy_cov_from_row(src, j);
        v4 cov_wbias = state.w_bias.copy_cov_from_row(src, j);
        v4 res = cov_w + cov_wbias;
        for(int i = 0; i < 3; ++i) {
            dst(i, j) = res[i];
        }
    }
}
