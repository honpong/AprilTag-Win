//
//  state_motion.cpp
//  RC3DK
//
//  Created by Eagle Jones on 1/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include <cmath>
#include "state_motion.h"

void state_motion_orientation::cache_jacobians(f_t dt)
{
    dW = (w.v + dt/2 * dw.v) * dt;
    m3 R = Q.v.toRotationMatrix();
    JdW_s = to_spatial_jacobian(rotation_vector(dW)/2);
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
        dw.to_col(dst, i) = cov_dw + dt * cov_ddw;
        w.to_col(dst, i) = cov_w + dt * (cov_dw + dt/2 * cov_ddw);
        Q.to_col(dst, i) = scov_Q + dQp_s_dW * cov_dW;
    }
}

void state_motion_orientation::evolve_state(f_t dt)
{
    Q.v *= to_quaternion(rotation_vector(dW)); // FIXME: use cached value?
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

    T.v += dT;
    V.v += dt * a.v;
}

void state_motion::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    state_motion_orientation::project_motion_covariance(dst, src, dt);

    //NOTE: Any changes here must also be reflected in state_vision:project_motion_covariance
    for(int i = 0; i < dst.cols(); ++i) {
        const auto cov_V = V.from_row(src, i);
        const auto cov_a = a.from_row(src, i);
        const auto cov_T = T.from_row(src, i);
        const auto cov_da = da.from_row(src, i);
        const v3 cov_dT = dt * (cov_V + dt/2 * (cov_a + dt/3 * cov_da));
        a.to_col(dst, i) = cov_a + dt * cov_da;
        V.to_col(dst, i) = cov_V + dt * (cov_a + dt/2 * cov_da);
        T.to_col(dst, i) = cov_T + cov_dT;
    }
}

void state_motion::cache_jacobians(f_t dt)
{
    state_motion_orientation::cache_jacobians(dt);

    dT = dt * (V.v + dt/2 * a.v);
}

void state_motion::enable_orientation_only(bool remap_)
{
    non_orientation.disable_estimation();
    V.reset(); a.reset(); da.reset(); // stop moving linearly
    if (remap_) remap();
}

void state_motion::disable_orientation_only(bool remap_)
{
    non_orientation.enable_estimation();
    if (remap_) remap();
}

void state_motion::disable_bias_estimation(bool remap_)
{
    for (auto &imu : imus.children)
        imu->intrinsics.disable_estimation();
    if (remap_) remap();
}

void state_motion::enable_bias_estimation(bool remap_)
{
    for (auto &imu : imus.children)
        imu->intrinsics.enable_estimation();
    if (remap_) remap();
}
