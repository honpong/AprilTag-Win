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
    m4 dWp_dwdt;
    integrate_angular_velocity_jacobian(W.v, (w.v + dt/2. * dw.v) * dt, dWp_dW, dWp_dwdt);
    dWp_dw = dWp_dwdt * dt;
    dWp_ddw = dWp_dw * (dt/2.);
}

void state_motion_orientation::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    for(int i = 0; i < src.rows; ++i) {
        v4 cov_W = W.copy_cov_from_row(src, i);
        v4 cov_w = w.copy_cov_from_row(src, i);
        v4 cov_dw = dw.copy_cov_from_row(src, i);
        w.copy_cov_to_col(dst, i, cov_w + dt * cov_dw);
        W.copy_cov_to_col(dst, i, dWp_dW * cov_W + dWp_dw * cov_w + dWp_ddw * cov_dw);
    }
}

void state_motion_orientation::evolve_covariance(f_t dt)
{
    cache_jacobians(dt);
    
    //use the tmp cov matrix to reduce stack size
    matrix tmp(dynamic_statesize, cov.size());
    project_motion_covariance(tmp, cov.cov, dt);
    //fill in the UR and LL matrices
    for(int i = 0; i < dynamic_statesize; ++i) {
        for(int j = dynamic_statesize; j < cov.size(); ++j) {
            cov(i, j) = cov(j, i) = tmp(i, j);
        }
    }
    //compute the UL matrix
    project_motion_covariance(cov.cov, tmp, dt);
    //enforce symmetry
    for(int i = 0; i < dynamic_statesize; ++i) {
        for(int j = i + 1; j < dynamic_statesize; ++j) {
            cov(i, j) = cov(j, i);
        }
    }
    
    //cov += diag(R)*dt
    for(int i = 0; i < cov.size(); ++i) {
        cov(i, i) += cov.process_noise[i] * dt;
    }
}

void state_motion_orientation::evolve_state(f_t dt)
{
    W.v = integrate_angular_velocity(W.v, (w.v + dt/2. * dw.v) * dt);
    w.v = w.v + dw.v * dt;
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
    static stdev_vector V_dev, a_dev, w_dev, dw_dev;
    T.v = T.v + dt * (V.v + 1./2. * dt * a.v);
    V.v = V.v + dt * a.v;

    state_motion_orientation::evolve_state(dt);
    
    V_dev.data(V.v);
    a_dev.data(a.v);
    w_dev.data(w.v);
    dw_dev.data(dw.v);
}

void state_motion::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    for(int i = 0; i < src.rows; ++i) {
        v4 cov_T = T.copy_cov_from_row(src, i);
        v4 cov_V = V.copy_cov_from_row(src, i);
        v4 cov_a = a.copy_cov_from_row(src, i);
        
        T.copy_cov_to_col(dst, i, cov_T + dt * (cov_V + 1./2. * dt * cov_a));
        V.copy_cov_to_col(dst, i, cov_V + dt * cov_a);
    }
    state_motion_orientation::project_motion_covariance(dst, src, dt);
}

void state_motion::evolve_covariance_orientation_only(f_t dt)
{
    cache_jacobians(dt);
    
    //use the tmp cov matrix to reduce stack size
    matrix tmp(dynamic_statesize, cov.size());
    state_motion_orientation::project_motion_covariance(tmp, cov.cov, dt);
    for(int i = 0; i < dynamic_statesize; ++i) {
        for(int j = dynamic_statesize; j < cov.size(); ++j) {
            cov(i, j) = cov(j, i) = tmp(i, j);
        }
    }
    state_motion_orientation::project_motion_covariance(cov.cov, tmp, dt);
    //enforce symmetry
    for(int i = 0; i < dynamic_statesize; ++i) {
        for(int j = i + 1; j < dynamic_statesize; ++j) {
            cov(i, j) = cov(j, i);
        }
    }
    
    //cov += diag(R)*dt
    for(int i = 0; i < cov.size(); ++i) {
        cov(i, i) += cov.process_noise[i] * dt;
    }
}

void state_motion::evolve(f_t dt)
{
    if(orientation_only) {
        evolve_covariance_orientation_only(dt);
        state_motion_orientation::evolve_state(dt);
    } else {
        state_motion_orientation::evolve(dt);
    }
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
    remap();
}

void state_motion::disable_orientation_only()
{
    if(!orientation_only) return;
    orientation_only = false;
    add_non_orientation_states();
    remap();
}
