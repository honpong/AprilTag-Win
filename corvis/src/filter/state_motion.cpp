//
//  state_motion.cpp
//  RC3DK
//
//  Created by Eagle Jones on 1/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "state_motion.h"

void state_motion_orientation::cache_jacobians(f_t dt)
{
    dW = (w.v + dt/2 * dw.v) * dt;
    rotation_vector dW_(dW[0],dW[1],dW[2]); // FIXME: remove this
    m3 R = Q.v.toRotationMatrix();
    JdW_s = to_spatial_jacobian(f_t(.5) * dW_);
    dQp_s_dW = R * JdW_s;
    Rt = R.transpose();  // FIXME: remove this?
}

void state_motion_orientation::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    //NOTE: Any changes here must also be reflected in state_vision:project_motion_covariance
    for(int i = 0; i < dst.cols(); ++i) {
        const auto cov_w = w.from_row(src, i);
        const auto cov_dw = dw.from_row(src, i);
        const auto cov_ddw = ddw.from_row(src, i);
        const v3 cov_dW = dt * (cov_w + dt/2 * (cov_dw + dt/3 * cov_ddw));
        const auto scov_Q = Q.from_row(src, i);
        w.to_col(dst, i) = cov_w + dt * (cov_dw + dt/2 * cov_ddw);
        dw.to_col(dst, i) = cov_dw + dt * cov_ddw;
        Q.to_col(dst, i) = scov_Q + dQp_s_dW * cov_dW;
    }
}

void state_motion_orientation::evolve_state(f_t dt)
{
    rotation_vector dW_(dW[0], dW[1], dW[2]); // FIXME: remove this!
    Q.v *= to_quaternion(dW_); // FIXME: use cached value?
    w.v += dw.v * dt;
}

void state_motion_orientation::compute_gravity(double latitude, double altitude)
{
    //http://en.wikipedia.org/wiki/Gravity_of_Earth#Free_air_correction
    double sin_lat = sin(latitude/180. * M_PI);
    double sin_2lat = sin(2*latitude/180. * M_PI);
    g.v = gravity_magnitude = (f_t)(9.780327 * (1 + 0.0053024 * sin_lat*sin_lat - 0.0000058 * sin_2lat*sin_2lat) - 3.086e-6 * altitude);
}

void state_motion::evolve_state(f_t dt)
{
    state_motion_orientation::evolve_state(dt);

    if (orientation_only) return;

    T.v += dT;
    V.v += dt * a.v;
}

void state_motion::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    state_motion_orientation::project_motion_covariance(dst, src, dt);

    if (orientation_only) return;

    //NOTE: Any changes here must also be reflected in state_vision:project_motion_covariance
    for(int i = 0; i < dst.cols(); ++i) {
        const auto cov_V = V.from_row(src, i);
        const auto cov_a = a.from_row(src, i);
        const auto cov_T = T.from_row(src, i);
        const auto cov_da = da.from_row(src, i);
        const v3 cov_dT = dt * (cov_V + dt/2 * (cov_a + dt/3 * cov_da));
        T.to_col(dst, i) = cov_T + cov_dT;
        V.to_col(dst, i) = cov_V + dt * (cov_a + dt/2 * cov_da);
        a.to_col(dst, i) = cov_a + dt * cov_da;
    }
}

void state_motion::cache_jacobians(f_t dt)
{
    state_motion_orientation::cache_jacobians(dt);

    if (orientation_only) return;

    dT = dt * (V.v + dt/2 * a.v);
}

void state_motion::remove_non_orientation_states()
{
    remove_child(&T);
    T.reset();
    remove_child(&V);
    V.reset();
    remove_child(&a);
    a.reset();
    remove_child(&da);
    da.reset();
}

void state_motion::add_non_orientation_states()
{
    children.push_back(&T);
    children.push_back(&V);
    children.push_back(&a);
    children.push_back(&da);
}

void state_motion::enable_orientation_only(bool remap_)
{
    if(orientation_only) return;
    orientation_only = true;
    remove_non_orientation_states();
    disable_bias_estimation(remap_);
}

void state_motion::disable_orientation_only(bool remap_)
{
    if(!orientation_only) return;
    orientation_only = false;
    add_non_orientation_states();
    enable_bias_estimation(remap_);
}

void state_imu_intrinsics::disable_bias_estimation()
{
    if(!estimate_bias) return;
    estimate_bias = false;
    a_bias.set_initial_variance(a_bias.variance()[0], a_bias.variance()[1], a_bias.variance()[2]);
    w_bias.set_initial_variance(w_bias.variance()[0], w_bias.variance()[1], w_bias.variance()[2]);
    remove_child(&a_bias);
    remove_child(&w_bias);
}

void state_motion::disable_bias_estimation(bool remap_)
{
    imu.intrinsics.disable_bias_estimation();
    if (remap_) remap();
}

void state_imu_intrinsics::enable_bias_estimation()
{
    if(estimate_bias) return;
    estimate_bias = true;
    children.push_back(&w_bias);
    children.push_back(&a_bias);
}

void state_motion::enable_bias_estimation(bool remap_)
{
    imu.intrinsics.enable_bias_estimation();
    if (remap_) remap();
}

void state_motion::copy_from(const state_motion &other)
{
    reset();

    if(other.orientation_only) enable_orientation_only();
    else disable_orientation_only();

    disable_bias_estimation();

    //remap done. structure should match other. now reset content

    Q = other.Q;
    w = other.w;
    dw = other.dw;
    ddw = other.ddw;

    g = other.g;
    T = other.T;
    V = other.V;
    a = other.a;
    da = other.da;

    imu.extrinsics.Q = other.imu.extrinsics.Q;
    imu.extrinsics.T = other.imu.extrinsics.T;

    imu.intrinsics.w_bias = other.imu.intrinsics.w_bias;
    imu.intrinsics.a_bias = other.imu.intrinsics.a_bias;

    orientation_initialized = other.orientation_initialized;
    current_time = other.current_time;
    loop_offset = other.loop_offset;

    world_up = other.world_up;
    world_initial_forward = other.world_initial_forward;
    body_forward = other.body_forward;

    f_t * temp_cov = cov.cov.data;
    f_t * temp_p_noise = cov.process_noise.data;
    cov.cov.data = other.cov.cov.data;
    cov.cov.resize(other.cov.size(), other.cov.size());
    cov.process_noise.data = other.cov.process_noise.data;
    cov.process_noise.resize(other.cov.size());

    orientation_initialized = other.orientation_initialized;
    current_time = other.current_time;
    loop_offset = other.loop_offset;

    remap();

    cov.cov_scratch.data = temp_cov;
    cov.process_scratch.data = temp_p_noise;
}
