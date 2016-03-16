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
    dW = (w.v + f_t(.5) * dt * dw.v) * dt;
    rotation_vector dW_(dW[0],dW[1],dW[2]); // FIXME: remove this
    m4 R = to_rotation_matrix(Q.v);
    JdW_s = to_spatial_jacobian(f_t(.5) * dW_);
    dQp_s_dW = R * JdW_s;
    Rt = R.transpose();  // FIXME: remove this?
}

void state_motion_orientation::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    for(int i = 0; i < src.rows(); ++i) {
        v4 scov_Q = Q.copy_cov_from_row(src, i);
        v4 cov_w = w.copy_cov_from_row(src, i);
        v4 cov_dw = dw.copy_cov_from_row(src, i);
        w.copy_cov_to_col(dst, i, cov_w + dt * cov_dw);
        v4 cov_dW = (cov_w + f_t(.5) * dt * cov_dw) * dt;
        Q.copy_cov_to_col(dst, i, scov_Q + dQp_s_dW * cov_dW);
    }
}

void state_motion_orientation::evolve_state(f_t dt)
{
    rotation_vector dW_(dW[0], dW[1], dW[2]); // FIXME: remove this!
    Q.v = Q.v * to_quaternion(dW_); // FIXME: use cached value?
    w.v = w.v + dw.v * dt;

    static stdev_vector w_dev, dw_dev;
    w_dev.data(w.v);
    dw_dev.data(dw.v);
}

void state_motion_orientation::compute_gravity(double latitude, double altitude)
{
    //http://en.wikipedia.org/wiki/Gravity_of_Earth#Free_air_correction
    double sin_lat = sin(latitude/180. * M_PI);
    double sin_2lat = sin(2*latitude/180. * M_PI);
    g.v = gravity_magnitude = 9.780327 * (1 + 0.0053024 * sin_lat*sin_lat - 0.0000058 * sin_2lat*sin_2lat) - 3.086e-6 * altitude;
}

void state_motion::evolve_state(f_t dt)
{
    state_motion_orientation::evolve_state(dt);

    if (orientation_only) return;

    T.v = T.v + dT;
    V.v = V.v + dt * a.v;

    static stdev_vector V_dev, a_dev;
    V_dev.data(V.v);
    a_dev.data(a.v);
}

void state_motion::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    state_motion_orientation::project_motion_covariance(dst, src, dt);

    if (orientation_only) return;

    for(int i = 0; i < src.rows(); ++i) {
        v4 cov_T = T.copy_cov_from_row(src, i);
        v4 cov_V = V.copy_cov_from_row(src, i);
        v4 cov_a = a.copy_cov_from_row(src, i);

        T.copy_cov_to_col(dst, i, cov_T + dt * (cov_V + f_t(.5) * dt * cov_a));
        V.copy_cov_to_col(dst, i, cov_V + dt * cov_a);
    }
}

void state_motion::cache_jacobians(f_t dt)
{
    state_motion_orientation::cache_jacobians(dt);

    if (orientation_only) return;

    dT = dt * (V.v + f_t(.5) * dt * a.v);
}

void state_motion::remove_non_orientation_states()
{
    remove_child(&T);
    T.reset();
    remove_child(&V);
    V.reset();
    remove_child(&a);
    a.reset();
}

void state_motion::add_non_orientation_states()
{
    children.push_back(&T);
    children.push_back(&V);
    children.push_back(&a);
}

void state_motion::enable_orientation_only()
{
    if(orientation_only) return;
    orientation_only = true;
    remove_non_orientation_states();
    disable_bias_estimation();
    //remap implicit in disable_bias_estimation
}

void state_motion::disable_orientation_only()
{
    if(!orientation_only) return;
    orientation_only = false;
    add_non_orientation_states();
    enable_bias_estimation();
    //remap implicit in enable_bias_estimation
}

void state_motion::disable_bias_estimation()
{
    if(!estimate_bias) return;
    estimate_bias = false;
    a_bias.set_initial_variance(a_bias.variance()[0], a_bias.variance()[1], a_bias.variance()[2]);
    remove_child(&a_bias);
    w_bias.set_initial_variance(w_bias.variance()[0], w_bias.variance()[1], w_bias.variance()[2]);
    remove_child(&w_bias);
    remap();
}

void state_motion::enable_bias_estimation()
{
    if(estimate_bias) return;
    estimate_bias = true;
    children.push_back(&a_bias);
    children.push_back(&w_bias);
    remap();
}
