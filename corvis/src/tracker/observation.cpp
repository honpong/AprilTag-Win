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
    observations.erase(remove_if(observations.begin(), observations.end(), [this](std::unique_ptr<observation> &o) {
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
    f_t rcond = matrix_check_condition(res_cov);
    if(rcond < .001) { fprintf(stderr, "observation covariance matrix not well-conditioned before computing gain! rcond = %e\n", rcond);}
#endif
    if(kalman_compute_gain(K, HP, res_cov))
    {
        state.resize(s.cov.size());
        s.copy_state_to_array(state);
        kalman_update_state(state, K, inn);
        s.copy_state_from_array(state);
        kalman_update_covariance(s.cov.cov, K, HP);
        return true;
    } else {
        return false;
    }
}

void observation_queue::preprocess(state_root &s, sensor_clock::time_point time)
{
    s.time_update(time);

    predict();
}

bool observation_queue::process(state_root &s)
{
    bool success = true;

    int orig_meas_size = size();

    measure_and_prune();

    int meas_size = size(), statesize = s.cov.size();
    if(meas_size) {
        inn.resize(meas_size);
        m_cov.resize(meas_size);
        HP.resize(meas_size, statesize);
        res_cov.resize(meas_size, meas_size);

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
    Rct = curr.camera.extrinsics.Q.v.toRotationMatrix().transpose();
    m3 Ro = orig.camera.extrinsics.Q.v.toRotationMatrix();
    m3 Rb = Rrt * Ro;
    v3 Tb = Rrt * (orig.camera.extrinsics.T.v - feature->group.Tr.v);
    Rtot = Rct * Rb;
    Ttot = Rct * (Tb - curr.camera.extrinsics.T.v);

    Xd = orig.camera.intrinsics.normalize_feature(feature->initial);
    X0 = orig.camera.intrinsics.undistort_feature(Xd).homogeneous();
    feature->body = Rb * X0 * feature->v.depth() + Tb;

    X = Rtot * X0 + Ttot * feature->v.invdepth();
    v2 Xu = X.segment(0,2) / X[2];
    feature->prediction = curr.camera.intrinsics.unnormalize_feature(curr.camera.intrinsics.distort_feature(Xu));
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

    f_t invZ = 1/X[2];
    v2 Xu = {X[0]*invZ, X[1]*invZ};
    m<1,2> dkd_u_dXu;
    m<1,4> dkd_u_dk;
    f_t kd_u = curr.camera.intrinsics.get_distortion_factor(Xu, &dkd_u_dXu, &dkd_u_dk);
    //dx_dX = height * focal_length * d(kd_u * Xu + center)_dX
    //= height * focal_length * (dkd_u_dX * Xu + kd_u * dXu_dX)
    //= height * focal_length * (dkd_u_dXu * dXu_dX * Xu + kd_u * dXu_dX)
    m<2,3> dXu_dX = {{ invZ, 0, -Xu[0] * invZ },
                     { 0, invZ, -Xu[1] * invZ }};
    m<1,3> dkd_u_dX = dkd_u_dXu * dXu_dX;
    m<2,3> dx_dX = curr.camera.intrinsics.image_height * curr.camera.intrinsics.focal_length.v * (Xu * dkd_u_dX + dXu_dX * kd_u);
    v3 dX_dp = Ttot * (feature->is_initialized() ? feature->v.invdepth_jacobian() : 0);
    dx_dp = dx_dX * dX_dp;
    f_t invrho = feature->is_initialized() ? feature->v.invdepth() : 0;

    // d(Rtot X0) = (Rc' Rr' Ro X0)^ Rc' (dRc Rc')v + (Rc' Rr' Ro X0)^ Rc' Rr' (dRr Rr')v  - (Rr' Rc' Ro X0)^ Rc' Rr' (dRo Ro')v + Rtot dX0
    // d Ttot = Ttot^ Rc' (dRc Rc')v  - Rc' Tc^ (dRc Rc')v + (Rc' Rr' Rc) Rc' (To - Tr)^ (dRr Rr')v + Rc' Rr' (dTo - dTr) - Rc dTc
    v3 RtotX0 = Rtot * X0;
    m3 dRtotX0_dQr = skew(RtotX0) * Rct * Rrt;
    m3 dTtot_dQr = Rtot * (Rct * skew(orig.camera.extrinsics.T.v - feature->group.Tr.v));
    m3 dTtot_dTr = -(Rct * Rrt);

    dx_dQr = dx_dX * (dTtot_dQr * invrho + dRtotX0_dQr);
    dx_dTr = dx_dX *  dTtot_dTr * invrho;

    if(orig.camera.intrinsics.estimate) {
        m<1,2> dku_d_dXd;
        f_t ku_d; m<1,4> dku_d_dk;
        ku_d = orig.camera.intrinsics.get_undistortion_factor(Xd, &dku_d_dXd, &dku_d_dk);
        m<3,2> dX_dc = Rtot * (v3(Xd.x(), Xd.y(), 0) *  dku_d_dXd +      ku_d * m<3,2>::Identity()) / -orig.camera.intrinsics.focal_length.v;
        m<3,1> dX_dF = Rtot *  v3(Xd.x(), Xd.y(), 0) * (dku_d_dXd * Xd + ku_d * m<1,1>::Identity()) / -orig.camera.intrinsics.focal_length.v;
        m<3,4> dX_dk = Rtot *  v3(Xd.x(), Xd.y(), 0) *  dku_d_dk;

        orig.dx_dF = dx_dX * dX_dF;
        orig.dx_dk = dx_dX * dX_dk;
        orig.dx_dc = dx_dX * dX_dc;
    }

    if (curr.camera.intrinsics.estimate) {
        curr.dx_dF = curr.camera.intrinsics.image_height * Xu * kd_u;
        curr.dx_dk = curr.camera.intrinsics.image_height * curr.camera.intrinsics.focal_length.v * Xu * dkd_u_dk;
        curr.dx_dc = curr.camera.intrinsics.image_height * m<2,2>::Identity();
    }

    if (curr.camera.extrinsics.estimate) {
        m3 dRtotX0_dQc = skew(RtotX0) * Rct;
        m3 dTtot_dQc = skew(Ttot) * Rct - Rct * skew(curr.camera.extrinsics.T.v);
        m3 dTtot_dTc = -Rct;
        curr.dx_dQ = dx_dX * (dTtot_dQc * invrho + dRtotX0_dQc);
        curr.dx_dT = dx_dX *  dTtot_dTc * invrho;
    }

    if (orig.camera.extrinsics.estimate) {
        m3 dTtot_dQo = m3::Zero();
        m3 dTtot_dTo = Rct * Rrt;
        m3 dRtotX0_dQo = -(skew(RtotX0) * Rct * Rrt);
        orig.dx_dQ = dx_dX * (dTtot_dQo * invrho + dRtotX0_dQo);
        orig.dx_dT = dx_dX *  dTtot_dTo * invrho;
    }
}

void observation_vision_feature::project_covariance(matrix &dst, const matrix &src)
{
    for(int j = 0; j < dst.cols(); ++j) {
        const auto cov_feat = feature->from_row(src, j);
        const auto scov_Qr  = feature->group.Qr.from_row(src, j);
        const auto  cov_Tr  = feature->group.Tr.from_row(src, j);
        col(dst, j) = dx_dp * cov_feat + dx_dQr * scov_Qr + dx_dTr * cov_Tr;

        if (curr.camera.extrinsics.estimate) {
            const auto scov_Qc = curr.camera.extrinsics.Q.from_row(src, j);
            const auto  cov_Tc = curr.camera.extrinsics.T.from_row(src, j);
            col(dst, j) += curr.dx_dQ * scov_Qc + curr.dx_dT * cov_Tc;
        }

        if (orig.camera.extrinsics.estimate) {
            const auto scov_Qo = orig.camera.extrinsics.Q.from_row(src, j);
            const auto  cov_To = orig.camera.extrinsics.T.from_row(src, j);
            col(dst, j) += orig.dx_dQ * scov_Qo + orig.dx_dT * cov_To;
        }

        if (curr.camera.intrinsics.estimate) {
            const auto cov_F = curr.camera.intrinsics.focal_length.from_row(src, j);
            const auto cov_c = curr.camera.intrinsics.center.from_row(src, j);
            const auto cov_k = curr.camera.intrinsics.k.from_row(src, j);
            col(dst, j) += curr.dx_dF * cov_F + curr.dx_dc * cov_c + curr.dx_dk * cov_k;
        }

        if (orig.camera.intrinsics.estimate) {
            const auto cov_F = orig.camera.intrinsics.focal_length.from_row(src, j);
            const auto cov_c = orig.camera.intrinsics.center.from_row(src, j);
            const auto cov_k = orig.camera.intrinsics.k.from_row(src, j);
            col(dst, j) += orig.dx_dF * cov_F + orig.dx_dc * cov_c + orig.dx_dk * cov_k;
        }
    }
}

f_t observation_vision_feature::projection_residual(const v3 & X, const feature_t & found_undistorted)
{
    v2 Xu = X.segment(0,2) / X[2];
    return (Xu - found_undistorted).squaredNorm();
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
    auto &intrinsics = curr.camera.intrinsics;
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
    f_t ot = feature->outlier_thresh * feature->outlier_thresh * (curr.camera.intrinsics.image_height/240.f)*(curr.camera.intrinsics.image_height/240.f);

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
    if(state.non_orientation.estimate)
        xcc += state.w.v.cross(state.w.v.cross(extrinsics.T.v)) + state.dw.v.cross(extrinsics.T.v);
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
    if (state.non_orientation.estimate) {
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
        col(dst, j) = cov_a_bias + da_dQ * scov_Q + da_dw * cov_w + da_ddw * cov_dw + da_dacc * (cov_a + state.world.up * cov_g);
        if(extrinsics.estimate) {
            const auto scov_Qa = extrinsics.Q.from_row(src, j);
            const auto cov_Ta = extrinsics.T.from_row(src, j);
            col(dst, j) += da_dQa * scov_Qa + da_dTa * cov_Ta;
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
    for(int j = 0; j < dst.cols(); ++j) {
        const auto cov_w = state.w.from_row(src, j);
        const auto cov_wbias = intrinsics.w_bias.from_row(src, j);
        col(dst, j) = cov_wbias + Rw.transpose() * cov_w;
        if(extrinsics.estimate) {
            v3 scov_Qw = extrinsics.Q.from_row(src, j);
            col(dst, j) += dw_dQw * scov_Qw;
        }
    }
}
