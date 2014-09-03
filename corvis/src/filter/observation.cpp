#include "observation.h"
#include "tracker.h"
#include "kalman.h"

stdev_scalar observation_vision_feature::stdev[2], observation_vision_feature::inn_stdev[2];
stdev_vector observation_accelerometer::stdev, observation_accelerometer::inn_stdev, observation_gyroscope::stdev, observation_gyroscope::inn_stdev;

int observation_queue::preprocess()
{
    stable_sort(observations.begin(), observations.end(), observation_comp_apparent);
    int size = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        size += (*obs)->size;
    }
    return size;
}

void observation_queue::clear()
{
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) delete *obs;
    observations.clear();
}

void observation_queue::predict()
{
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->predict();
    }
}

void observation_queue::measure()
{
    //measure; calculate innovation and covariance
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); ++obs) {
        (*obs)->measure();
    }
}

void observation_queue::compute_innovation(matrix &inn)
{
    int count = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); ++obs) {
        (*obs)->compute_innovation();
        for(int i = 0; i < (*obs)->size; ++i) {
            inn[count + i] = (*obs)->innovation(i);
        }
        count += (*obs)->size;
    }
}

void observation_queue::compute_measurement_covariance(matrix &m_cov)
{
    int count = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); obs++) {
        (*obs)->compute_measurement_covariance();
        for(int i = 0; i < (*obs)->size; ++i) {
            m_cov[count + i] = (*obs)->measurement_covariance(i);
        }
        count += (*obs)->size;
    }
}

void observation_queue::compute_prediction_covariance(const state &s, int meas_size)
{
    //project state cov onto measurement to get cov(meas, state)
    // matrix_product(LC, lp, A, false, false);
    int statesize = s.cov.size();
    int index = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); ++obs) {
        if((*obs)->size) {
            matrix dst(&LC(index, 0), (*obs)->size, statesize, LC.maxrows, LC.stride);
            (*obs)->cache_jacobians();
            (*obs)->project_covariance(dst, s.cov.cov);
            index += (*obs)->size;
        }
    }
    
    //project cov(state, meas)=(LC)' onto meas to get cov(meas, meas)
    index = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); ++obs) {
        if((*obs)->size) {
            matrix dst(&res_cov(index, 0), (*obs)->size, meas_size, res_cov.maxrows, res_cov.stride);
            (*obs)->project_covariance(dst, LC);
            index += (*obs)->size;
        }
    }
    
    //enforce symmetry
    for(int i = 0; i < res_cov.rows; ++i) {
        for(int j = i + 1; j < res_cov.cols; ++j) {
            res_cov(i, j) = res_cov(j, i);
        }
    }
    
    index = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); ++obs) {
        if((*obs)->size) (*obs)->set_prediction_covariance(res_cov, index);
        index += (*obs)->size;
    }
}

void observation_queue::compute_innovation_covariance(const matrix &m_cov)
{
    int index = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); ++obs) {
        for(int i = 0; i < (*obs)->size; ++i) {
            res_cov(index + i, index + i) += m_cov[index + i];
        }
        (*obs)->innovation_covariance_hook(res_cov, index);
        index += (*obs)->size;
    }
}

int observation_queue::remove_invalid_measurements(const state &s, int orig_size, matrix &inn)
{
    int map[orig_size];
    int src = 0;
    int new_size = 0;
    for(vector<observation *>::iterator obs = observations.begin(); obs != observations.end(); ++obs) {
        for(int i = 0; i < (*obs)->size; ++i) {
            if((*obs)->valid) map[new_size++] = src;
            ++src;
        }
    }

    for(int i = 0; i < new_size; ++i)
    {
        inn[i] = inn[map[i]];
    }
    inn.resize(new_size);

    for(int i = 0; i < new_size; ++i)
    {
        memcpy(&LC(i, 0), &LC(map[i], 0), sizeof(f_t) * LC.stride);
    }
    LC.resize(new_size, LC.cols);

    for(int i = 0; i < new_size; ++i)
    {
        for(int j = 0; j < new_size; ++j)
        {
            res_cov(i, j) = res_cov(map[i], map[j]);
        }
    }
    res_cov.resize(new_size, new_size);
    return new_size;
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

bool observation_queue::process(state &s, uint64_t time)
{
#ifdef TEST_POSDEF
    if(!test_posdef(s.cov.cov)) fprintf(stderr, "not pos def when starting process_observation_queue\n");
#endif
    bool success = true;
    s.time_update(time);
    if(!observations.size()) return success;
    int statesize = s.cov.size();

    int meas_size = preprocess();
    if(meas_size) {
        matrix inn(1, meas_size);
        matrix m_cov(1, meas_size);
        LC.resize(meas_size, statesize);
        res_cov.resize(meas_size, meas_size);

        //TODO: implement (*obs)->time_apparent != (*obs)->time_actual
        predict();
        measure();
        compute_innovation(inn);
        compute_measurement_covariance(m_cov);
        compute_prediction_covariance(s, meas_size);
        compute_innovation_covariance(m_cov);
        int count = remove_invalid_measurements(s, meas_size, inn);
        if(count) {
            success = update_state_and_covariance(s, inn);
        } else {
            if(log_enabled && meas_size != 3) fprintf(stderr, "In Kalman update, original measurement size was %d, ended up with 0 measurements!\n", meas_size);
        }
    }
    
    clear();
    f_t delta_T = norm(s.T.v - s.last_position);
    if(delta_T > .01) {
        s.total_distance += norm(s.T.v - s.last_position);
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
        fprintf(stderr, " predicted stdev is %e %e\n", sqrtf(cov(index, index)), sqrtf(cov(index+1, index+1)));
    }
}

void observation_vision_feature::predict()
{
    m4 Rr = to_rotation_matrix(state_group->Wr.v);
    m4 R = to_rotation_matrix(state.W.v);
    Rrt = transpose(Rr);
    Rbc = to_rotation_matrix(state.Wc.v);
    Rcb = transpose(Rbc);
    RcbRrt = Rcb * Rrt;
    Rtot = RcbRrt * Rbc;
    Ttot = Rcb * (Rrt * (state.Tc.v - state_group->Tr.v) - state.Tc.v);

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
    feature->local = Rrt * (feature->relative - state_group->Tr.v);
    feature->world = R * feature->local + state.T.v;
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
    
    m4v4 dRr_dWr = to_rotation_matrix_jacobian(state_group->Wr.v);
    m4v4 dRbc_dWc = to_rotation_matrix_jacobian(state.Wc.v);
    m4v4 dRrt_dWr = transpose(dRr_dWr);
    m4v4 dRcb_dWc = transpose(dRbc_dWc);
    
    m4v4 dRtot_dWr  = Rcb * dRrt_dWr * Rbc;
    m4v4 dRtot_dWc = dRcb_dWc * (Rrt * Rbc) + (Rcb * Rrt) * dRbc_dWc;
    m4 dTtot_dWc = dRcb_dWc * (Rrt * (state.Tc.v - state_group->Tr.v) - state.Tc.v);
    m4 dTtot_dWr  = Rcb * (dRrt_dWr * (state.Tc.v - state_group->Tr.v));
    m4 dTtot_dTc = Rcb * Rrt - Rcb;
    m4 dTtot_dTr = -Rcb * Rrt;
    
    state.fill_calibration(norm_predicted, r2, r4, r6, kr);
    f_t invZ = 1. / X[2];
    v4 dx_dX, dy_dX;
    dx_dX = kr * state.focal_length.v * v4(invZ, 0., -X[0] * invZ * invZ, 0.);
    dy_dX = kr * state.focal_length.v * v4(0., invZ, -X[1] * invZ * invZ, 0.);

    v4 dX_dp = Ttot * feature->v.invdepth_jacobian();
    dx_dp = sum(dx_dX * dX_dp);
    dy_dp = sum(dy_dX * dX_dp);
    f_t invrho = feature->v.invdepth();
    if(!feature->is_initialized()) {
        dx_dWc = dx_dX * (dRtot_dWc * feature->calibrated);
        dx_dWr = dx_dX * (dRtot_dWr * feature->calibrated);
        dy_dWc = dy_dX * (dRtot_dWc * feature->calibrated);
        dy_dWr = dy_dX * (dRtot_dWr * feature->calibrated);
        //dy_dT = m4(0.);
        //dy_dT = m4(0.);
        //dy_dTr = m4(0.);
    } else {
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
        dx_dWc = dx_dX * (dRtot_dWc * X0 + dTtot_dWc * invrho);
        dx_dWr = dx_dX * (dRtot_dWr * X0 + dTtot_dWr * invrho);
        dx_dTc = dx_dX * dTtot_dTc * invrho;
        dx_dTr = dx_dX * dTtot_dTr * invrho;
        dy_dWc = dy_dX * (dRtot_dWc * X0 + dTtot_dWc * invrho);
        dy_dWr = dy_dX * (dRtot_dWr * X0 + dTtot_dWr * invrho);
        dy_dTc = dy_dX * dTtot_dTc * invrho;
        dy_dTr = dy_dX * dTtot_dTr * invrho;
    }
}

void observation_vision_feature::project_covariance(matrix &dst, const matrix &src)
{

    if(!feature->is_initialized()) {
        for(int j = 0; j < dst.cols; ++j) {
            v4 cov_Wc = state.Wc.copy_cov_from_row(src, j);
            v4 cov_Wr = state_group->Wr.copy_cov_from_row(src, j);
            dst(0, j) =
            sum(dx_dWc * cov_Wc) +
            sum(dx_dWr * cov_Wr);
            dst(1, j) =
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
            v4 cov_Wc = state.Wc.copy_cov_from_row(src, j);
            v4 cov_Wr = state_group->Wr.copy_cov_from_row(src, j);
            v4 cov_Tc = state.Tc.copy_cov_from_row(src, j);
            v4 cov_Tr = state_group->Tr.copy_cov_from_row(src, j);
            dst(0, j) = dx_dp * cov_feat +
            dx_dF * cov_F +
            dx_dcx * cov_cx +
            dx_dcy * cov_cy +
            dx_dk1 * cov_k1 +
            dx_dk2 * cov_k2 +
            //dy_dk3[i] * p[state.k3.index] +
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
            sum(dy_dWc * cov_Wc) +
            sum(dy_dTc * cov_Tc) +
            sum(dy_dWr * cov_Wr) +
            sum(dy_dTr * cov_Tr);
        }
    }
}

f_t observation_vision_feature::projection_residual(const v4 & X_inf, const f_t inv_depth, const xy &found)
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

void observation_vision_feature::update_initializing()
{
    if(feature->is_initialized()) return;
    f_t min = 0.01; //infinity-ish (100m)
    f_t max = 10.; //1/.10 for 10cm
    f_t min_d2, max_d2;
    v4 X_inf = Rtot * feature->calibrated;
    
    v4 X_inf_proj = X_inf / X_inf[2];
    v4 X_0 = X_inf + max * Ttot;
    
    v4 X_0_proj = X_0 / X_0[2];
    v4 delta = (X_inf_proj - X_0_proj);
    f_t pixelvar = sum(delta * delta) * state.focal_length.v * state.focal_length.v;
    if(pixelvar > 5. * 5. * state_vision_feature::measurement_var) { //tells us if we have enough baseline
        feature->status = feature_normal;
    }
    
    xy bestkp;
    bestkp.x = meas[0];
    bestkp.y = meas[1];
    
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
    if(best > .01 && best < 10.) {
        feature->v.set_depth_meters(1./best);
    }
    //repredict using triangulated depth
    predict();
}

bool observation_vision_feature::measure()
{
    xy bestkp, bestkp1, bestkp2;

    bestkp1 = tracker.track(feature->patch, im2, pred[0], pred[1], 5.5, .40);

    bestkp2 = tracker.track(feature->patch, im2, feature->current[0] + feature->image_velocity.x, feature->current[1] + feature->image_velocity.y, 5.5, bestkp1.score);

    if(bestkp1.score >= bestkp2.score)
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
            v4 res = state.estimate_bias ? cov_a_bias + dya_dW * cov_W : dya_dW * cov_W;
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
