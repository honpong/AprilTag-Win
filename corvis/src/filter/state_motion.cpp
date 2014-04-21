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

void state_motion_orientation::orientation_init(v4 gravity, uint64_t time)
{
    //first measurement - use to determine orientation
    //cross product of this with "up": (0,0,1)
    v4 s = v4(gravity[1], -gravity[0], 0., 0.) / norm(gravity);
    v4 s2 = s * s;
    f_t sintheta = sqrt(sum(s2));
    f_t theta = asin(sintheta);
    if(gravity[2] < 0.) {
        //direction of z component tells us we're flipped - sin(x) = sin(pi - x)
        theta = M_PI - theta;
    }
    if(sintheta < 1.e-7) {
        W.v = rotation_vector(s[0], s[1], s[2]);
    } else{
        v4 snorm = s * (theta / sintheta);
        W.v = rotation_vector(snorm[0], snorm[1], snorm[2]);
    }
    current_time = time;
}

void state_motion::evolve_state(f_t dt)
{
    static stdev_vector V_dev, a_dev, da_dev, w_dev, dw_dev;
    T.v = T.v + dt * (V.v + 1./2. * dt * (a.v + 1./3. * dt * da.v));
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
        
        T.copy_cov_to_col(dst, i, cov_T + dt * (cov_V + 1./2. * dt * (cov_a + 1./3. * dt * cov_da)));
        V.copy_cov_to_col(dst, i, cov_V + dt * (cov_a + 1./2. * dt * cov_da));
        a.copy_cov_to_col(dst, i, cov_a + dt * cov_da);
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
