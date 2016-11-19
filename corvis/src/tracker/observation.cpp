#include "observation.h"
#include "kalman.h"
#include "utils.h"

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

void observation_queue::compute_prediction_covariance(const state_root &s, int meas_size)
{
    int statesize = s.cov.size();
    int index = 0;
    for(auto &o : observations) {
        matrix dst(HP, index, 0, o->size, statesize);
        o->cache_jacobians();
        o->project_covariance(dst, s.cov.cov); // HP = H * P'
        index += o->size;
    }
    
    index = 0;
    for(auto &o : observations) {
        matrix dst(res_cov, index, 0, o->size, meas_size);
        o->project_covariance(dst, HP); // res_cov = H * (H * P')' = H * P * H'
        index += o->size;
    }
    
    //enforce symmetry
    for(int i = 0; i < res_cov.rows(); ++i) {
        for(int j = i + 1; j < res_cov.cols(); ++j) {
            res_cov(i, j) = res_cov(j, i);
        }
    }
    
    index = 0;
    for(auto &o : observations) {
        o->set_prediction_covariance(res_cov, index);
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

bool observation_queue::update_state_and_covariance(state_root &s, const matrix &inn)
{
#ifdef TEST_POSDEF
    if(!test_posdef(res_cov)) { fprintf(stderr, "observation covariance matrix not positive definite before computing gain!\n"); }
    f_t rcond = matrix_check_condition(res_cov);
    if(rcond < .001) { fprintf(stderr, "observation covariance matrix not well-conditioned before computing gain! rcond = %e\n", rcond);}
#endif
    if(kalman_compute_gain(K, HP, res_cov, res_tmp))
    {
        state.resize(1, s.cov.size());
        s.copy_state_to_array(state);
        kalman_update_state(state, K, inn);
        s.copy_state_from_array(state);
        kalman_update_covariance(s.cov.cov, K, HP);
        //Robust update is not needed and is much slower
        //kalman_update_covariance_robust(s.cov.cov, K, HP, res_cov);
        return true;
    } else {
        return false;
    }
}

void observation_queue::preprocess(state_root &s, sensor_clock::time_point time)
{
#ifdef TEST_POSDEF
    if(!test_posdef(s.cov.cov)) fprintf(stderr, "not pos def when starting process_observation_queue\n");
#endif
    s.time_update(time);

    stable_sort(observations.begin(), observations.end(), observation_comp_apparent);

    predict();
}

bool observation_queue::process(state_root &s)
{
    bool success = true;

    int orig_meas_size = size();

    measure_and_prune();

    int meas_size = size(), statesize = s.cov.size();
    if(meas_size) {
        inn.resize(1, meas_size);
        m_cov.resize(1, meas_size);
        HP.resize(meas_size, statesize + s.fake_statesize);
        res_cov.resize(meas_size, meas_size);

        //TODO: implement o->time_apparent != o->time_actual
        compute_innovation(inn);
        compute_measurement_covariance(m_cov);
        compute_prediction_covariance(s, meas_size);
        compute_innovation_covariance(m_cov);
        HP.resize(meas_size, statesize);
        success = update_state_and_covariance(s, inn);
    } else if(orig_meas_size && orig_meas_size != 3) {
        //s.log->warn("In Kalman update, original measurement size was {}, ended up with 0 measurements!\n", orig_meas_size);
    }

    recent_f_map.clear();
    for (auto &o : observations)
        cache_recent(std::move(o));

    observations.clear();
#ifdef TEST_POSDEF
    if(!test_posdef(s.cov.cov)) {fprintf(stderr, "not pos def when finishing process observation queue\n"); assert(0);}
#endif
    return success;
}

void observation_spatial::innovation_covariance_hook(const matrix &cov, int index)
{
    if(show_tuning) {
        fprintf(stderr, " predicted stdev is %e %e %e\n", sqrt(cov(index, index)), sqrt(cov(index+1, index+1)), sqrt(cov(index+2, index+2)));
    }
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
    Rrt = feature->group.Qr.v.toRotationMatrix().transpose();
    Rct = extrinsics.Q.v.toRotationMatrix().transpose();
    m3 Rb = Rrt * Rct.transpose();
    v3 Tb = Rrt * (extrinsics.T.v - feature->group.Tr.v);
    Rtot = Rct * Rb;
    Ttot = Rct * (Tb - extrinsics.T.v);

    Xd = intrinsics.normalize_feature(feature->initial);
    norm_initial = intrinsics.undistort_feature(Xd);
    X0 = v3(norm_initial.x(), norm_initial.y(), 1);
    feature->body = Rb * X0 * feature->v.depth() + Tb;

    X = Rtot * X0 + Ttot * feature->v.invdepth();
    v3 ippred = X / X[2]; //in the image plane
#ifdef DEBUG
    if(fabs(ippred[2]-1.) > 1.e-7) {
        fprintf(stderr, "FAILURE in feature projection in observation_vision_feature::predict\n");
    }
#endif

    norm_predicted = {ippred[0], ippred[1]};
    feature->prediction = intrinsics.unnormalize_feature(intrinsics.distort_feature(norm_predicted));
    pred = feature->prediction;
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

    // d RtotX0 = (Rtot X0)^ Rct Rrt scov_Rr + RtotX0^ (I - Rtot) Rct scov_Rc +  + Rct Rrt Rc dX0;
    // d Ttot = Ttot^ Rct cov_Rc + Rct * (-Rrt cov_Rr^ * (Tc - Tr) + Rrt * (cov_Tc - cov_Tr) - cov_Tc);

    v3 RtotX0 = Rtot * X0;
    m3 dRtotX0_dQr = skew(RtotX0) * Rct * Rrt;
    m3 dRtotX0_dQc = skew(RtotX0) * Rct * (m3::Identity() - Rrt);
    m3 dTtot_dQr = Rtot * (Rct * skew(extrinsics.T.v - feature->group.Tr.v));
    m3 dTtot_dQc = skew(Ttot) * Rct;
    m3 dTtot_dTc = -Rct * (m3::Identity() - Rrt);
    m3 dTtot_dTr = -Rct * Rrt;

    f_t invZ = 1/X[2];
    feature_t Xu = {X[0]*invZ, X[1]*invZ};
    feature_t dkd_u_dXu;
    f_t kd_u, dkd_u_dk1, dkd_u_dk2, dkd_u_dk3;
    kd_u = intrinsics.get_distortion_factor(Xu, &dkd_u_dXu, &dkd_u_dk1, &dkd_u_dk2, &dkd_u_dk3);
    //dx_dX = height * focal_length * d(kd_u * Xu + center)_dX
    //= height * focal_length * (dkd_u_dX * Xu + kd_u * dXu_dX)
    //= height * focal_length * (dkd_u_dXu * dXu_dX * Xu + kd_u * dXu_dX)
    v3 dx_dX, dy_dX;
    v3 dXu_dX[2];
    dXu_dX[0] = v3(invZ, 0, -Xu[0] * invZ);
    dXu_dX[1] = v3(0, invZ, -Xu[1] * invZ);
    v3 dkd_u_dX = dkd_u_dXu[0] * dXu_dX[0] + dkd_u_dXu[1] * dXu_dX[1];
    dx_dX = intrinsics.image_height * intrinsics.focal_length.v * (dkd_u_dX * Xu[0] + kd_u * dXu_dX[0]);
    dy_dX = intrinsics.image_height * intrinsics.focal_length.v * (dkd_u_dX * Xu[1] + kd_u * dXu_dX[1]);

    v3 dX_dp = Ttot * feature->v.invdepth_jacobian();
    dx_dp = dx_dX.dot(dX_dp);
    dy_dp = dy_dX.dot(dX_dp);
    f_t invrho = feature->v.invdepth();
    if(!feature->is_initialized()) {
        dx_dQr = dx_dX.transpose() * dRtotX0_dQr;
        dy_dQr = dy_dX.transpose() * dRtotX0_dQr;
        //dy_dTr = m4(0.);
    } else {
        if(intrinsics.estimate) {
            feature_t dku_d_dXd;
            f_t ku_d, dku_d_dk1, dku_d_dk2, dku_d_dk3;
            ku_d = intrinsics.get_undistortion_factor(Xd, &dku_d_dXd, &dku_d_dk1, &dku_d_dk2, &dku_d_dk3);
            v3 dX_dcx = Rtot * v3(ku_d  + dku_d_dXd.x()*Xd.x(),         dku_d_dXd.y()*Xd.x(), 0) / -intrinsics.focal_length.v;
            v3 dX_dcy = Rtot * v3(        dku_d_dXd.x()*Xd.y(), ku_d  + dku_d_dXd.y()*Xd.y(), 0) / -intrinsics.focal_length.v;
            v3 dX_dF  = Rtot * v3(Xd.x(), Xd.y(), 0) * (ku_d + dku_d_dXd.dot(Xd))                / -intrinsics.focal_length.v;
            v3 dX_dk1 = Rtot * v3(Xd.x() * dku_d_dk1, Xd.y() * dku_d_dk1, 0);
            v3 dX_dk2 = Rtot * v3(Xd.x() * dku_d_dk2, Xd.y() * dku_d_dk2, 0);
            v3 dX_dk3 = Rtot * v3(Xd.x() * dku_d_dk3, Xd.y() * dku_d_dk3, 0);

            dx_dF = intrinsics.image_height * Xu[0] * kd_u + dx_dX.dot(dX_dF);
            dy_dF = intrinsics.image_height * Xu[1] * kd_u + dy_dX.dot(dX_dF);

            dx_dk1 = intrinsics.image_height * intrinsics.focal_length.v * Xu[0] * dkd_u_dk1 + dx_dX.dot(dX_dk1);
            dy_dk1 = intrinsics.image_height * intrinsics.focal_length.v * Xu[1] * dkd_u_dk1 + dy_dX.dot(dX_dk1);
            dx_dk2 = intrinsics.image_height * intrinsics.focal_length.v * Xu[0] * dkd_u_dk2 + dx_dX.dot(dX_dk2);
            dy_dk2 = intrinsics.image_height * intrinsics.focal_length.v * Xu[1] * dkd_u_dk2 + dy_dX.dot(dX_dk2);
            dx_dk3 = intrinsics.image_height * intrinsics.focal_length.v * Xu[0] * dkd_u_dk3 + dx_dX.dot(dX_dk3);
            dy_dk3 = intrinsics.image_height * intrinsics.focal_length.v * Xu[1] * dkd_u_dk3 + dy_dX.dot(dX_dk3);

            dx_dcx = intrinsics.image_height + dx_dX.dot(dX_dcx); dx_dcy =                           dx_dX.dot(dX_dcy);
            dy_dcx =                           dy_dX.dot(dX_dcx); dy_dcy = intrinsics.image_height + dy_dX.dot(dX_dcy);
        }
        dx_dQr = dx_dX.transpose() * (dRtotX0_dQr + dTtot_dQr * invrho);
        dx_dTr = dx_dX.transpose() * dTtot_dTr * invrho;
        dy_dQr = dy_dX.transpose() * (dRtotX0_dQr + dTtot_dQr * invrho);
        dy_dTr = dy_dX.transpose() * dTtot_dTr * invrho;
        dx_dQc = dx_dX.transpose() * (dRtotX0_dQc + dTtot_dQc * invrho);
        dx_dTc = dx_dX.transpose() * dTtot_dTc * invrho;
        dy_dQc = dy_dX.transpose() * (dRtotX0_dQc + dTtot_dQc * invrho);
        dy_dTc = dy_dX.transpose() * dTtot_dTc * invrho;
    }
}

void observation_vision_feature::project_covariance(matrix &dst, const matrix &src)
{

    if(!feature->is_initialized()) {
        for(int j = 0; j < dst.cols(); ++j) {
            const auto scov_Qr = feature->group.Qr.from_row(src, j);
            dst(0, j) = dx_dQr.dot(scov_Qr);
            dst(1, j) = dy_dQr.dot(scov_Qr);
        }
    } else {
        for(int j = 0; j < dst.cols(); ++j) {
            const f_t cov_feat = feature->from_row(src, j);
            const auto scov_Qr = feature->group.Qr.from_row(src, j);
            const auto cov_Tr = feature->group.Tr.from_row(src, j);
            dst(0, j) = dx_dp * cov_feat +
                dx_dQr.dot(scov_Qr) +
                dx_dTr.dot(cov_Tr);
            dst(1, j) = dy_dp * cov_feat +
                dy_dQr.dot(scov_Qr) +
                dy_dTr.dot(cov_Tr);

            if (extrinsics.estimate) {
                const auto scov_Qc = extrinsics.Q.from_row(src, j);
                const auto cov_Tc = extrinsics.T.from_row(src, j);
                dst(0, j) +=
                    dx_dQc.dot(scov_Qc) +
                    dx_dTc.dot(cov_Tc);
                dst(1, j) +=
                    dy_dQc.dot(scov_Qc) +
                    dy_dTc.dot(cov_Tc);
            }

            if(intrinsics.estimate) {
                f_t cov_F = intrinsics.focal_length.from_row(src, j);
                f_t cov_cx = intrinsics.center_x.from_row(src, j);
                f_t cov_cy = intrinsics.center_y.from_row(src, j);
                f_t cov_k1 = intrinsics.k1.from_row(src, j);
                f_t cov_k2 = intrinsics.k2.from_row(src, j);
                f_t cov_k3 = intrinsics.k3.from_row(src, j);

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

f_t observation_vision_feature::projection_residual(const v3 & X, const feature_t & found_undistorted)
{
    f_t invZ = 1/X[2];
    v3 ippred = X * invZ; //in the image plane
#ifdef DEBUG
    if(fabs(ippred[2]-1) > 1.e-7f) {
        fprintf(stderr, "FAILURE in feature projection in observation_vision_feature::predict\n");
    }
#endif
    feature_t norm = { ippred[0], ippred[1] };
    return (norm - found_undistorted).squaredNorm();
}

void observation_vision_feature::update_initializing()
{
    if(feature->is_initialized()) return;
    f_t min = 0.01f; //infinity-ish (100m)
    f_t max = 10; //1/.10 for 10cm
    f_t min_d2, max_d2;
    v3 X_inf = Rtot * X0;
    
    v3 X_inf_proj = X_inf / X_inf[2];
    v3 X_0 = X_inf + max * Ttot;
    
    v3 X_0_proj = X_0 / X_0[2];
    v3 delta = (X_inf_proj - X_0_proj);
    f_t normalized_distance_to_px = intrinsics.focal_length.v * intrinsics.image_height;
    f_t pixelvar = delta.dot(delta) * normalized_distance_to_px * normalized_distance_to_px;
    if(pixelvar > 5. * 5. * source.measurement_variance) { //tells us if we have enough baseline
        feature->status = feature_normal;
    }
    
    feature_t bestkp = meas;
    feature_t bestkp_norm = intrinsics.undistort_feature(intrinsics.normalize_feature(bestkp));

    min_d2 = projection_residual(X_inf + min * Ttot, bestkp_norm);
    max_d2 = projection_residual(X_inf + max * Ttot, bestkp_norm);
    f_t best = min;
    f_t best_d2 = min_d2;
    for(int i = 0; i < 10; ++i) { //10 iterations = 1024 segments
        if(min_d2 < max_d2) {
            max = (min + max) / 2;
            max_d2 = projection_residual(X_inf + max * Ttot, bestkp_norm);
            if(min_d2 < best_d2) {
                best_d2 = min_d2;
                best = min;
            }
        } else {
            min = (min + max) / 2;
            min_d2 = projection_residual(X_inf + min * Ttot, bestkp_norm);
            if(max_d2 < best_d2) {
                best_d2 = max_d2;
                best = max;
            }
        }
    }
    if(best > .01 && best < 10.) {
        feature->v.set_depth_meters(1/best);
    }
    //repredict using triangulated depth
    predict();
}

bool observation_vision_feature::measure()
{
    meas = feature->current;

    bool valid = meas[0] != INFINITY;

    if(valid) {
        source.meas_stdev.data(meas);
        if(!feature->is_initialized()) {
            update_initializing();
        }
    }

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
    source.inn_stdev.data(inn);
    f_t ot = feature->outlier_thresh * feature->outlier_thresh * (intrinsics.image_height/240.f)*(intrinsics.image_height/240.f);

    f_t residual = inn[0]*inn[0] + inn[1]*inn[1];
    f_t badness = residual; //outlier_count <= 0  ? outlier_inn[i] : outlier_ess[i];
    f_t robust_mc;
    f_t thresh = source.measurement_variance * ot;
    if(badness > thresh) {
        f_t ratio = sqrt(badness / thresh);
        robust_mc = ratio * source.measurement_variance;
        feature->outlier += ratio;
    } else {
        robust_mc =         source.measurement_variance;
        feature->outlier = 0.;
    }
    m_cov[0] = robust_mc;
    m_cov[1] = robust_mc;
}

void observation_accelerometer::predict()
{
    Rt = state.Q.v.conjugate().toRotationMatrix();
    Ra = extrinsics.Q.v.toRotationMatrix();
    v3 acc = state.world.up * state.g.v + state.a.v;
    xcc = Rt * acc;
    if(!state.orientation_only)
    {
        xcc += state.w.v.cross(state.w.v.cross(extrinsics.T.v)) + state.dw.v.cross(extrinsics.T.v);
    }
    pred = Ra.transpose() * xcc + intrinsics.a_bias.v;
}

void observation_accelerometer::cache_jacobians()
{
    v3 acc = state.world.up * state.g.v + state.a.v;
    da_dacc = Ra.transpose() * Rt;
    da_dQ = Ra.transpose() * Rt * skew(acc);
    if(extrinsics.estimate)
    {
        da_dTa = Ra.transpose() * (skew(state.w.v) * skew(state.w.v) + skew(state.dw.v));
        da_dQa = Ra.transpose() * skew(xcc);
    }
    if (!state.orientation_only) {
        da_ddw = Ra.transpose() * skew(-extrinsics.T.v);
        da_dw = Ra.transpose() * (skew(extrinsics.T.v.cross(state.w.v)) - skew(state.w.v) * skew(extrinsics.T.v));
    } else {
        da_ddw = m3::Zero();
        da_dw = m3::Zero();
        da_dTa = m3::Zero(); // FIXME: remove extrinsics.T from the state when orientation_only
    }
}

void observation_accelerometer::project_covariance(matrix &dst, const matrix &src)
{
    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    assert(dst.cols() == src.rows());
    for(int j = 0; j < dst.cols(); ++j) {
        const auto cov_a_bias = intrinsics.a_bias.from_row(src, j);
        const auto scov_Q = state.Q.from_row(src, j);
        const auto cov_a = state.a.from_row(src, j);
        const auto cov_w = state.w.from_row(src, j);
        const auto cov_dw = state.dw.from_row(src, j);
        f_t cov_g = state.g.from_row(src, j);
        v3 res =
            cov_a_bias +
            da_dQ * scov_Q +
            da_dw * cov_w +
            da_ddw * cov_dw +
            da_dacc * (cov_a + state.world.up * cov_g);
        if(extrinsics.estimate) {
            const auto scov_Qa = extrinsics.Q.from_row(src, j);
            const auto cov_Ta = extrinsics.T.from_row(src, j);
            res += da_dQa * scov_Qa + da_dTa * cov_Ta;
        }
        for(int i = 0; i < 3; ++i) {
            dst(i, j) = res[i];
        }
    }
}

bool observation_accelerometer::measure()
{
    source.meas_stdev.data(meas);
    return observation_spatial::measure();
}

void observation_gyroscope::predict()
{
    Rw = extrinsics.Q.v.toRotationMatrix();
    pred = intrinsics.w_bias.v + Rw.transpose() * state.w.v;
}

void observation_gyroscope::cache_jacobians()
{
    if(extrinsics.estimate) {
        dw_dQw = Rw.transpose() * skew(state.w.v);
    }
}

void observation_gyroscope::project_covariance(matrix &dst, const matrix &src)
{
    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    for(int j = 0; j < dst.cols(); ++j) {
        const auto cov_w = state.w.from_row(src, j);
        const auto cov_wbias = intrinsics.w_bias.from_row(src, j);
        v3 res = cov_wbias +
            Rw.transpose() * cov_w;
        if(extrinsics.estimate) {
            v3 scov_Qw = extrinsics.Q.from_row(src, j);
            res += dw_dQw * scov_Qw;
        }
        for(int i = 0; i < 3; ++i) {
            dst(i, j) = res[i];
        }
    }
}
