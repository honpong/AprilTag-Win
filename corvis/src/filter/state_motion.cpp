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
    matrix &tmp = cov.temp_matrix(dynamic_statesize, cov.size());
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

void state_motion_orientation::evolve(f_t dt)
{
    evolve_covariance(dt);
    evolve_state(dt);
}

void state_motion::evolve_state_orientation_only(f_t dt)
{
    state_motion_orientation::evolve_state(dt);
}

void state_motion::project_motion_covariance_orientation_only(matrix &dst, const matrix &src, f_t dt)
{
    for(int i = 0; i < src.rows; ++i) {
        v4 cov_T = T.copy_cov_from_row(src, i);
        v4 cov_V = V.copy_cov_from_row(src, i);
        v4 cov_a = a.copy_cov_from_row(src, i);
        
        T.copy_cov_to_col(dst, i, cov_T);
        V.copy_cov_to_col(dst, i, cov_V);
        a.copy_cov_to_col(dst, i, cov_a);
    }
    state_motion_orientation::project_motion_covariance(dst, src, dt);
}

void state_motion::evolve_covariance_orientation_only(f_t dt)
{
    cache_jacobians(dt);
    
    //use the tmp cov matrix to reduce stack size
    matrix &tmp = cov.temp_matrix(dynamic_statesize, cov.size());
    project_motion_covariance_orientation_only(tmp, cov.cov, dt);
    for(int i = 0; i < dynamic_statesize; ++i) {
        for(int j = dynamic_statesize; j < cov.size(); ++j) {
            cov(i, j) = cov(j, i) = tmp(i, j);
        }
    }
    
    project_motion_covariance_orientation_only(cov.cov, tmp, dt);
    //enforce symmetry
    for(int i = 0; i < dynamic_statesize; ++i) {
        for(int j = i + 1; j < dynamic_statesize; ++j) {
            cov(i, j) = cov(j, i);
        }
    }
    
    //cov += diag(R)*dt
    for(int i = 0; i < 3; ++i) {
        cov(W.index + i, W.index + i) += cov.process_noise[W.index + i] * dt;
        cov(w.index + i, w.index + i) += cov.process_noise[w.index + i] * dt;
        cov(dw.index + i, dw.index + i) += cov.process_noise[dw.index + i] * dt;
        cov(w_bias.index + i, w_bias.index + i) += cov.process_noise[w_bias.index + i] * dt;
        cov(a_bias.index + i, a_bias.index + i) += cov.process_noise[w_bias.index + i] * dt;
    }
}

void state_motion::evolve_orientation_only(f_t dt)
{
    evolve_covariance_orientation_only(dt);
    evolve_state_orientation_only(dt);
}

void state_motion::evolve(f_t dt)
{
    evolve_covariance(dt);
    evolve_state(dt);
}

void state_motion::evolve_state(f_t dt)
{
    static stdev_vector V_dev, a_dev, da_dev, w_dev, dw_dev;
    T.v = T.v + dt * (V.v + 1./2. * dt * (a.v + 2./3. * dt * da.v));
    V.v = V.v + dt * (a.v + 1./2. * dt * da.v);
    a.v = a.v + da.v * dt;

    state_motion_orientation::evolve_state(dt);
    
    V_dev.data(V.v);
    a_dev.data(a.v);
    da_dev.data(da.v);
    w_dev.data(w.v);
    dw_dev.data(dw.v);
}

void state_motion::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    for(int i = 0; i < src.rows; ++i) {
        v4 cov_T = T.copy_cov_from_row(src, i);
        v4 cov_V = V.copy_cov_from_row(src, i);
        v4 cov_a = a.copy_cov_from_row(src, i);
        v4 cov_da = da.copy_cov_from_row(src, i);
        
        T.copy_cov_to_col(dst, i, cov_T + dt * (cov_V + 1./2. * dt * (cov_a + 2./3. * dt * cov_da)));
        V.copy_cov_to_col(dst, i, cov_V + dt * (cov_a + 1./2. * dt * cov_da));
        a.copy_cov_to_col(dst, i, cov_a + dt * cov_da);
    }
    state_motion_orientation::project_motion_covariance(dst, src, dt);
}

void state_motion::evolve_covariance(f_t dt)
{
    cache_jacobians(dt);
    
    //use the tmp cov matrix to reduce stack size
    matrix &tmp = cov.temp_matrix(dynamic_statesize, cov.size());
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
