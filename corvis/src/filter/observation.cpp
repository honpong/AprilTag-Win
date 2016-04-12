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
           this->cache_recent(std::move(o));
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
            matrix dst(LC, index, 0, o->size, statesize);
            o->cache_jacobians();
            o->project_covariance(dst, s.cov.cov);
            index += o->size;
        }
    }
    
    //project cov(state, meas)=(LC)' onto meas to get cov(meas, meas)
    index = 0;
    for(auto &o : observations) {
        if(o->size) {
            matrix dst(res_cov, index, 0, o->size, meas_size);
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
        //kalman_update_covariance_robust(s.cov.cov, K, LC, res_cov);
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
    } else if(orig_meas_size && orig_meas_size != 3) {
        //s.log->warn("In Kalman update, original measurement size was {}, ended up with 0 measurements!\n", orig_meas_size);
    }

    recent_f_map.clear();
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

void observation_spatial::innovation_covariance_hook(const matrix &cov, int index)
{
    if(show_tuning) {
        //s.log->info(" predicted stdev is {} {} {}", sqrt(cov(index, index)), sqrt(cov(index+1, index+1)), sqrt(cov(index+2, index+2)));
    }
}

void observation_vision_feature::innovation_covariance_hook(const matrix &cov, int index)
{
    feature->innovation_variance_x = cov(index, index);
    feature->innovation_variance_y = cov(index + 1, index + 1);
    feature->innovation_variance_xy = cov(index, index +1);
    if(show_tuning) {
        //s.log->info(" predicted stdev is {} {}", sqrt(cov(index, index)), sqrt(cov(index+1, index+1)));
    }
}

void observation_vision_feature::predict()
{
    m4 Rr = to_rotation_matrix(state_group->Qr.v);
    m4 R = to_rotation_matrix(state.Q.v);
    Rrt = Rr.transpose();
    Rtot = Rrt;
    Ttot = Rrt * ( - state_group->Tr.v);

    feature_t uncal = { feature->initial[0], feature->initial[1] };
    Xd = state.normalize_feature(uncal);
    norm_initial = state.undistort_feature(Xd);
    X0 = v4(norm_initial.x(), norm_initial.y(), 1., 0.);
    feature->Xcamera = X0 * feature->v.depth();

    X = Rtot * X0 + Ttot * feature->v.invdepth();

    feature->world = R * Rrt * (X0 * feature->v.depth() - state_group->Tr.v) + state.T.v;
    v4 ippred = X / X[2]; //in the image plane
#ifdef DEBUG
    if(fabs(ippred[2]-1.) > 1.e-7 || ippred[3] != 0.) {
        fprintf(stderr, "FAILURE in feature projection in observation_vision_feature::predict\n");
    }
#endif

    norm_predicted = {ippred[0], ippred[1]};
    feature_t prediction = state.unnormalize_feature(state.distort_feature(norm_predicted));
    pred[0] = prediction.x();
    pred[1] = prediction.y();
}

void observation_vision_feature::cache_jacobians()
{
    // v4 Xd = ((point - size/2 + .5) / height - C) / F
    // v4 dXd = - dC / F - Xd / F dF
    // v4 Xd_dXd = - Xd.dC / F - Xd.Xd / F dF
    // v4 X0 = ku_d * Xd
    // m4 dX0 = dku_d * Xd + ku_d * dXd = (dku_d_drd * Xd_dXd/rd + dku_d_dk * dk) * Xd - ku_d/F dC - X0/F dF
    // f_t drd = d sqrt(Xd.Xd) = Xd.dXd/sqrt(Xd.Xd) = Xd_dXd/rd
    // f_t dku_d = dku_d_dk dk + dku_d_drd * drd
    // v4 X = Rtot * X0 + Ttot / depth
    // v4 dX = dRtot * X0 + Rtot * dX0 + dTtot / depth - Tot / depth / depth ddepth

    v4 RtotX0 = Rtot * X0;
    m4 dRtotX0_dQr = skew3(RtotX0) * Rrt;
    m4 dTtot_dQr = skew3(Ttot) * Rrt;
    m4 dTtot_dTr = -Rrt;

    f_t invZ = 1/X[2];
    feature_t Xu = {X[0]*invZ, X[1]*invZ};
    f_t ru2, ru = sqrt(ru2=Xu.squaredNorm());
    f_t kd_u, dkd_u_dru, dkd_u_dk1, dkd_u_dk2, dkd_u_dk3;
    state.distort_feature(Xu, &kd_u, &dkd_u_dru, &dkd_u_dk1, &dkd_u_dk2, &dkd_u_dk3);
    // v2 xd = X/Xz * kd_u = Xu * kd_u
    // v2 dxd = dX/Xz * kd_u + Xu * dkd_u - Xu/Xz * dXz * kd_u = (dX/Xz - Xu/Xz * dXz) * kd_u + Xu * (dkd_u_dru * X.dX/ru + dkd_u_dw * dw)
    // v2 x = (xd * F + C) * height + height/2 - .5
    // v2 dx = height * (dxd * F + xd * dF + dC)
    //       = height * (((dX/Xz - Xu/Xz * dXz) * kd_u + Xu * (dkd_u_dru * X.dX/ru + dkd_u_dw * dw)) * F + Xu * kd_u * dF + dC)
    v4 dx_dX, dy_dX;
    dx_dX = state.image_height * kd_u * state.focal_length.v * v4(invZ + Xu[0]*dkd_u_dru*X[0]/ru,        Xu[0]*dkd_u_dru*X[1]/ru, -Xu[0] * invZ, 0);
    dy_dX = state.image_height * kd_u * state.focal_length.v * v4(       Xu[1]*dkd_u_dru*X[0]/ru, invZ + Xu[1]*dkd_u_dru*X[1]/ru, -Xu[1] * invZ, 0);

    v4 dX_dp = Ttot * feature->v.invdepth_jacobian();
    dx_dp = dx_dX.dot(dX_dp);
    dy_dp = dy_dX.dot(dX_dp);
    f_t invrho = feature->v.invdepth();
    if(!feature->is_initialized()) {
        dx_dQr = dx_dX.transpose() * dRtotX0_dQr;
        dy_dQr = dy_dX.transpose() * dRtotX0_dQr;
        //dy_dTr = m4(0.);
    } else {
        if(state.estimate_camera_intrinsics) {
            f_t rd2, rd = sqrt(rd2=Xd.squaredNorm());
            f_t ku_d, dku_d_drd, dku_d_dk1, dku_d_dk2, dku_d_dk3;
            /*X0 = */state.undistort_feature(Xd, &ku_d, &dku_d_drd, &dku_d_dk1, &dku_d_dk2, &dku_d_dk3);
            v4 dX_dcx = Rtot * v4(ku_d  + dku_d_drd*Xd.x()*Xd.x()/rd,         dku_d_drd*Xd.x()*Xd.y()/rd, 0, 0) / -state.focal_length.v;
            v4 dX_dcy = Rtot * v4(        dku_d_drd*Xd.y()*Xd.x()/rd, ku_d  + dku_d_drd*Xd.y()*Xd.y()/rd, 0, 0) / -state.focal_length.v;
            v4 dX_dF  = Rtot * v4(X0[0] + dku_d_drd*Xd.x()       *rd, X0[1] + dku_d_drd*Xd.y()       *rd, 0, 0) / -state.focal_length.v;
            v4 dX_dk1 = Rtot * v4(Xd.x() * dku_d_dk1, Xd.y() * dku_d_dk1, 0, 0);
            v4 dX_dk2 = Rtot * v4(Xd.x() * dku_d_dk2, Xd.y() * dku_d_dk2, 0, 0);
            v4 dX_dk3 = Rtot * v4(Xd.x() * dku_d_dk3, Xd.y() * dku_d_dk3, 0, 0);

            dx_dF = state.image_height * norm_predicted.x() * kd_u + dx_dX.dot(dX_dF);
            dy_dF = state.image_height * norm_predicted.y() * kd_u + dy_dX.dot(dX_dF);

            dx_dk1 = state.image_height * (dkd_u_dk1 * state.focal_length.v * X[0]*invZ + kd_u * dx_dX.dot(dX_dk1));
            dy_dk1 = state.image_height * (dkd_u_dk1 * state.focal_length.v * X[1]*invZ + kd_u * dy_dX.dot(dX_dk1));
            dx_dk2 = state.image_height * (dkd_u_dk2 * state.focal_length.v * X[0]*invZ + kd_u * dx_dX.dot(dX_dk2));
            dy_dk2 = state.image_height * (dkd_u_dk2 * state.focal_length.v * X[1]*invZ + kd_u * dy_dX.dot(dX_dk2));
            dx_dk3 = state.image_height * (dkd_u_dk3 * state.focal_length.v * X[0]*invZ + kd_u * dx_dX.dot(dX_dk3));
            dy_dk3 = state.image_height * (dkd_u_dk3 * state.focal_length.v * X[1]*invZ + kd_u * dy_dX.dot(dX_dk3));

            dx_dcx = state.image_height + dx_dX.dot(dX_dcx); dx_dcy =                      dx_dX.dot(dX_dcy);
            dy_dcx =                      dy_dX.dot(dX_dcx); dy_dcy = state.image_height + dy_dX.dot(dX_dcy);
        }
        dx_dQr = dx_dX.transpose() * (dRtotX0_dQr + dTtot_dQr * invrho);
        dx_dTr = dx_dX.transpose() * dTtot_dTr * invrho;
        dy_dQr = dy_dX.transpose() * (dRtotX0_dQr + dTtot_dQr * invrho);
        dy_dTr = dy_dX.transpose() * dTtot_dTr * invrho;
    }
}

void observation_vision_feature::project_covariance(matrix &dst, const matrix &src)
{

    if(!feature->is_initialized()) {
        for(int j = 0; j < dst.cols(); ++j) {
            const auto scov_Qr = state_group->Qr.from_row(src, j);
            dst(0, j) = dx_dQr.segment<3>(0).dot(scov_Qr);
            dst(1, j) = dy_dQr.segment<3>(0).dot(scov_Qr);
        }
    } else {
        for(int j = 0; j < dst.cols(); ++j) {
            const f_t cov_feat = feature->from_row(src, j);
            const auto scov_Qr = state_group->Qr.from_row(src, j);
            const auto cov_Tr = state_group->Tr.from_row(src, j);

            dst(0, j) = dx_dp * cov_feat +
                dx_dQr.segment<3>(0).dot(scov_Qr) +
                dx_dTr.segment<3>(0).dot(cov_Tr);

            dst(1, j) = dy_dp * cov_feat +
                dy_dQr.segment<3>(0).dot(scov_Qr) +
                dy_dTr.segment<3>(0).dot(cov_Tr);

            if(state.estimate_camera_intrinsics) {
                f_t cov_F = state.focal_length.from_row(src, j);
                f_t cov_cx = state.center_x.from_row(src, j);
                f_t cov_cy = state.center_y.from_row(src, j);
                f_t cov_k1 = state.k1.from_row(src, j);
                f_t cov_k2 = state.k2.from_row(src, j);
                f_t cov_k3 = state.k3.from_row(src, j);

                dst(0, j) +=
                dx_dF * cov_F +
                dx_dcx * cov_cx +
                dx_dcy * cov_cy +
                dx_dk1 * cov_k1 +
                dx_dk2 * cov_k2 +
                dx_dk3 * cov_k3;

                dst(1, j) +=
                dy_dF * cov_F +
                dy_dcx * cov_cx +
                dy_dcy * cov_cy +
                dy_dk1 * cov_k1 +
                dy_dk2 * cov_k2 +
                dy_dk3 * cov_k3;
            }
        }
    }
}

f_t observation_vision_feature::projection_residual(const v4 & X, const feature_t & found_undistorted)
{
    f_t invZ = 1./X[2];
    v4 ippred = X * invZ; //in the image plane
#ifdef DEBUG
    if(fabs(ippred[2]-1.) > 1.e-7 || ippred[3] != 0.) {
        fprintf(stderr, "FAILURE in feature projection in observation_vision_feature::predict\n");
    }
#endif
    feature_t norm = { ippred[0], ippred[1] };
    return (norm - found_undistorted).squaredNorm();
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
    f_t normalized_distance_to_px = state.focal_length.v * state.image_height;
    f_t pixelvar = delta.dot(delta) * normalized_distance_to_px * normalized_distance_to_px;
    if(pixelvar > 5. * 5. * state_vision_feature::measurement_var) { //tells us if we have enough baseline
        feature->status = feature_normal;
    }
    
    feature_t bestkp = {meas[0], meas[1]};
    feature_t bestkp_norm = state.undistort_feature(state.normalize_feature(bestkp));

    min_d2 = projection_residual(X_inf + min * Ttot, bestkp_norm);
    max_d2 = projection_residual(X_inf + max * Ttot, bestkp_norm);
    f_t best = min;
    f_t best_d2 = min_d2;
    for(int i = 0; i < 10; ++i) { //10 iterations = 1024 segments
        if(min_d2 < max_d2) {
            max = (min + max) / 2.;
            max_d2 = projection_residual(X_inf + max * Ttot, bestkp_norm);
            if(min_d2 < best_d2) {
                best_d2 = min_d2;
                best = min;
            }
        } else {
            min = (min + max) / 2.;
            min_d2 = projection_residual(X_inf + min * Ttot, bestkp_norm);
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
    float ratio = 1.f;
    if(feature->last_dt.count())
        ratio = (float)feature->dt.count() / feature->last_dt.count();

    xy bestkp = tracker.track(feature->patch, image, (float)feature->current[0] + feature->image_velocity.x()*ratio, (float)feature->current[1] + feature->image_velocity.y()*ratio, tracker.radius, tracker.min_match);

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

    feature->image_velocity.x() = valid ? bestkp.x - feature->current[0] : 0;
    feature->image_velocity.y() = valid ? bestkp.y - feature->current[1] : 0;

    meas[0] = feature->current[0] = bestkp.x;
    meas[1] = feature->current[1] = bestkp.y;

    if(valid) {
        stdev[0].data(meas[0]);
        stdev[1].data(meas[1]);
        if(!feature->is_initialized()) {
            update_initializing();
        }
    }

    feature->last_dt = feature->dt;

    // The covariance will become ill-conditioned if either the original predict()ion
    // or the predict()ion in update_initializing() lead to huge or non-sensical pred[]s
    if (X[2] < .01) { // throw away features that are predicted to be too close to or even behind the camera
        feature->drop();
        return false;
    }

    return valid;
}

void observation_vision_feature::compute_measurement_covariance()
{
    inn_stdev[0].data(inn[0]);
    inn_stdev[1].data(inn[1]);
    f_t ot = feature->outlier_thresh * feature->outlier_thresh * (state.image_height/240.)*(state.image_height/240.);

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
    Rt = to_rotation_matrix(state.Q.v).transpose();
    Rc = to_rotation_matrix(state.Qc.v);
    v4 acc = v4(0., 0., state.g.v, 0.);
    if(!state.orientation_only)
    {
        acc += state.a.v;
    }
    v4 pred_a = Rc * Rt * acc + state.a_bias.v;
    if(!state.orientation_only)
    {
        //TODO: shouldn't these have an Rc (possible Rt) term?
        pred_a += cross(state.w.v, cross(state.w.v, state.Tc.v)) + cross(state.dw.v, state.Tc.v);
    }
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
    da_dQ = Rc * Rt * skew3(acc);
    if(state.estimate_camera_extrinsics)
    {
        da_dQc = -skew3(Rc * Rt * acc);
        da_dTc = skew3(state.w.v) * skew3(state.w.v) + skew3(state.dw.v);
    }
    da_ddw = -skew3(state.Tc.v);
    da_dw = -skew3(cross(state.w.v, state.Tc.v)) * -skew3(state.Tc.v);
}

void observation_accelerometer::project_covariance(matrix &dst, const matrix &src)
{
    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    assert(dst.cols() == src.rows());
    if(!state.orientation_only)
    {
        for(int j = 0; j < dst.cols(); ++j) {
            const auto cov_a_bias = state.a_bias.from_row(src, j);
            const auto scov_Q = state.Q.from_row(src, j);
            const auto cov_a = state.a.from_row(src, j);
            const auto cov_w = state.w.from_row(src, j);
            const auto cov_dw = state.dw.from_row(src, j);
            f_t cov_g = state.g.from_row(src, j);
            v3 res =
                cov_a_bias +
                da_dQ.block<3,3>(0,0) * scov_Q +
                da_dw.block<3,3>(0,0) * cov_w +
                da_ddw.block<3,3>(0,0) * cov_dw +
                Rc.block<3,3>(0,0) * Rt.block<3,3>(0,0) * (cov_a + v3(0., 0., cov_g));
            if(state.estimate_camera_extrinsics) {
                const auto scov_Qc = state.Qc.from_row(src, j);
                const auto cov_Tc = state.Tc.from_row(src, j);
                res += da_dQc.block<3,3>(0,0) * scov_Qc + da_dTc.block<3,3>(0,0) * cov_Tc;
            }
            for(int i = 0; i < 3; ++i) {
                dst(i, j) = res[i];
            }
        }
    } else {
        for(int j = 0; j < dst.cols(); ++j) {
            const v3 cov_a_bias = state.a_bias.from_row(src, j);
            const auto scov_Q = state.Q.from_row(src, j);
            v3 res = (state.estimate_bias ? cov_a_bias : v3::Zero()) + da_dQ.block<3,3>(0,0) * scov_Q;
            if(state.estimate_camera_extrinsics) {
                const auto scov_Qc = state.Qc.from_row(src, j);
                res += da_dQc.block<3,3>(0,0) * scov_Qc;
            }
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
        state.initial_orientation = initial_orientation_from_gravity(state.Qc.v * g);
        state.Q.v = state.initial_orientation;
        state.orientation_initialized = true;
        return false;
    } else return observation_spatial::measure();
}

void observation_gyroscope::predict()
{
    Rc = to_rotation_matrix(state.Qc.v);
    v4 pred_w = state.w_bias.v + Rc * state.w.v;
    // w = w_bias + Qc * w;
    // dw = dw_bias + Qc * dw + (dQc Qc^-1) Qc * w;
    // dw = dw_bias + Qc * dw - skew3(Qc * w) * unskew3(dQc Qc^-1);

    for(int i = 0; i < 3; ++i) {
        pred[i] = pred_w[i];
    }
}

void observation_gyroscope::cache_jacobians()
{
    if(state.estimate_camera_extrinsics) {
        dw_dQc = - skew3(Rc * state.w_bias.v);
    }
}

void observation_gyroscope::project_covariance(matrix &dst, const matrix &src)
{
    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    for(int j = 0; j < dst.cols(); ++j) {
        const auto cov_w = state.w.from_row(src, j);
        const auto cov_wbias = state.w_bias.from_row(src, j);
        v3 res = cov_wbias +
            Rc.block<3,3>(0,0) * cov_w;
        if(state.estimate_camera_extrinsics) {
            v3 scov_Qc = state.Qc.from_row(src, j);
            res += dw_dQc.block<3,3>(0,0) * scov_Qc;
        }
        for(int i = 0; i < 3; ++i) {
            dst(i, j) = res[i];
        }
    }
}
