// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
extern "C" {
#include "cor.h"
#include "../calibration/calibration.h"
}
#include "fast_detector/fast.h"
#include "model.h"
#include "../numerics/vec4.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "../numerics/kalman.h"
#include "../numerics/matrix.h"
#include "observation.h"
#include "filter.h"
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/video/tracking.hpp>
#include <opencv2/features2d/features2d.hpp>
int state_node::statesize;
int state_node::maxstatesize;

//TODO: homogeneous coordinates.
//TODO: reduced size for ltu

static void filter_reset_covariance(struct filter *f, int i, f_t initial)
{
    for(int j = 0; j < f->s.cov.rows; j++) {
        f->s.cov(i, j) = f->s.cov(j, i) = 0.;
    }
    f->s.cov(i, i) = initial;
}

extern "C" void filter_reset_position(struct filter *f)
{
    for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); giter++) {
        (*giter)->Tr.v -= f->s.T.v;
    }
    f->s.T.v = 0.;
    f->s.total_distance = 0.;
    f->s.last_position = f->s.T;

    for(int i = 0; i < 3; ++i) filter_reset_covariance(f, f->s.T.index + i, 1.e-7);
}

void filter_config(struct filter *f);

extern "C" void filter_reset_full(struct filter *f)
{
    //clear all features and groups
    list<state_vision_group *>::iterator giter = f->s.groups.children.begin();
    while(giter != f->s.groups.children.end()) {
        delete *giter;
        giter = f->s.groups.children.erase(giter);
    }
    list<state_vision_feature *>::iterator fiter = f->s.features.begin();
    while(fiter != f->s.features.end()) {
        delete *fiter;
        fiter = f->s.features.erase(fiter);
    }
    f->s.reset();
    f->s.total_distance = 0.;
    f->s.last_position = 0.;
    f->s.reference = NULL;
    f->s.last_reference = 0;
    filter_config(f);
    f->s.remap();

    f->gravity_init = false;
    f->last_time = 0;
    f->frame = 0;
    f->active = false;
    f->got_accelerometer = f->got_gyroscope = f->got_image = false;
    f->need_reference = true;
    f->accelerometer_max = f->gyroscope_max = 0.;
    f->measurement_running = false;
    f->detector_failed = f->tracker_failed = f->tracker_warned = false;
    f->speed_failed = f->speed_warning = f->numeric_failed = false;
    f->speed_warning_time = 0;
    f->observations.clear();

    observation_vision_feature::stdev[0] = stdev_scalar();
    observation_vision_feature::stdev[1] = stdev_scalar();
    observation_vision_feature::inn_stdev[0] = stdev_scalar();
    observation_vision_feature::inn_stdev[1] = stdev_scalar();
    observation_accelerometer::stdev = stdev_vector();
    observation_accelerometer::inn_stdev = stdev_vector();
    observation_gyroscope::stdev = stdev_vector();
    observation_gyroscope::inn_stdev = stdev_vector();
}

void integrate_motion_state_explicit(state_motion_gravity & state, f_t dt)
{
    static stdev_vector V_dev, a_dev, da_dev, w_dev, dw_dev;
    m4 
        R = rodrigues(state.W, NULL),
        rdt = rodrigues((state.w + 1./2. * state.dw * dt) * dt, NULL);

    m4 Rp = R * rdt;
    state.W = invrodrigues(Rp, NULL);
    //state.W = state.W + dt * integrate_angular_velocity(state.W, state.w + 1./2. * dt * state.dw);
    state.T = state.T + dt * (state.V + 1./2. * dt * (state.a + 2./3. * dt * state.da));
    state.V = state.V + dt * (state.a + 1./2. * dt * state.da);
    state.a = state.a + state.da * dt;

    state.w = state.w + state.dw * dt;

    V_dev.data(state.V.v);
    a_dev.data(state.a.v);
    da_dev.data(state.da.v);
    w_dev.data(state.w.v);
    dw_dev.data(state.dw.v);
}

void project_motion_covariance_explicit(state_motion_gravity &state, matrix &dst, const matrix &src, f_t dt, const m4 &dWp_dW, const m4 &dWp_dw, const m4 &dWp_ddw)
{
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < src.rows; ++j) {
            const f_t *p = &src(j, 0);
            dst(state.T.index + i, j) = p[state.T.index + i] + dt * (p[state.V.index + i] + 1./2. * dt * (p[state.a.index + i] + 2./3. * dt * p[state.da.index + i]));
            dst(state.V.index + i, j) = p[state.V.index + i] + dt * (p[state.a.index + i] + 1./2. * dt * p[state.da.index + i]);
            dst(state.a.index + i, j) = p[state.a.index + i] + dt * p[state.da.index + i];
            dst(state.w.index + i, j) = p[state.w.index + i] + dt * p[state.dw.index + i];
            dst(state.W.index + i, j) = sum(dWp_dW[i] * v4(p[state.W.index], p[state.W.index + 1], p[state.W.index + 2], 0.)) +
                sum(dWp_dw[i] * v4(p[state.w.index], p[state.w.index + 1], p[state.w.index + 2], 0.)) +
                sum(dWp_ddw[i] * v4(p[state.dw.index], p[state.dw.index + 1], p[state.dw.index + 2], 0.));
        }
    }
}

void integrate_motion_covariance_explicit(state_motion_gravity &state, f_t dt)
{
    m4v4 dR_dW, drdt_dwdt;
    v4m4 dWp_dRp;
    
    m4 
        R = rodrigues(state.W, &dR_dW),
        rdt = rodrigues((state.w + 1./2. * state.dw * dt) * dt, &drdt_dwdt);
 
    m4 Rp = R * rdt;
    v4
        Wp = invrodrigues(Rp, &dWp_dRp);
    m4v4
        dRp_dW = dR_dW * rdt,
        dRp_dw = R * (drdt_dwdt * dt);
    m4
        dWp_dW = dWp_dRp * dRp_dW,
        dWp_dw = dWp_dRp * dRp_dw,
        dWp_ddw = dWp_dw * (1./2. * dt);

    /*m4 dWp_dW, dWp_dw, dWp_ddw;
    linearize_angular_integration(state.W, (state.w + 1./2. * state.dw * dt) * dt, dWp_dW, dWp_dw);
    dWp_ddw = 1./2. * dt * dWp_dw;*/

    MAT_TEMP(tmp, MOTION_STATES, state.cov.cols);
    project_motion_covariance_explicit(state, tmp, state.cov, dt, dWp_dW, dWp_dw, dWp_ddw);
    for(int i = 0; i < MOTION_STATES; ++i) {
        for(int j = MOTION_STATES; j < state.cov.cols; ++j) {
            state.cov(i, j) = state.cov(j, i) = tmp(i, j);
        }
    }

    project_motion_covariance_explicit(state, state.cov, tmp, dt, dWp_dW, dWp_dw, dWp_ddw);
    //enforce symmetry
    for(int i = 0; i < MOTION_STATES; ++i) {
        for(int j = i + 1; j < MOTION_STATES; ++j) {
            state.cov(i, j) = state.cov(j, i);
        }
    }

    //cov += diag(R)*dt
    for(int i = 0; i < state.cov.rows; ++i) {
        state.cov(i, i) += state.p_cov[i] * dt;
    }
}

void integrate_motion_state(state_motion_gravity &state, const state_motion_derivative &slope, f_t dt)
{
    m4 
        R = rodrigues(state.W, NULL),
        rdt = rodrigues(slope.w * dt, NULL);

    m4 Rp = R * rdt;
    state.W = invrodrigues(Rp, NULL);
    state.T = state.T + slope.V * dt;
    state.V = state.V + slope.a * dt;
    state.w = state.w + slope.dw * dt;
    state.a = state.a + slope.da * dt;
}

void integrate_motion_state_euler(state &state, f_t dt)
{
    integrate_motion_state(state, state_motion_derivative(state), dt);
}

void integrate_motion_state_backward_euler(state &state, f_t dt)
{
    int statesize = state.cov.rows;
    MAT_TEMP(saved_state, 1, statesize);
    state.copy_state_to_array(saved_state);
    integrate_motion_state(state, state_motion_derivative(state), dt);
    state_motion_derivative slope(state);
    state.copy_state_from_array(saved_state);
    integrate_motion_state(state, slope, dt);
}

void integrate_motion_state_rk4(state &state, f_t dt)
{
    int statesize = state.cov.rows;
    MAT_TEMP(saved_state, 1, statesize);
    state.copy_state_to_array(saved_state);

    state_motion_derivative k1(state);

    integrate_motion_state(state, k1, dt / 2.);
    state_motion_derivative k2(state);
    state.copy_state_from_array(saved_state);

    integrate_motion_state(state, k2, dt / 2.);
    state_motion_derivative k3(state);
    state.copy_state_from_array(saved_state);

    integrate_motion_state(state, k3, dt);
    state_motion_derivative k4(state);
    state.copy_state_from_array(saved_state);

    state_motion_derivative kf;
    kf.V = 1./6. * (k1.V + 2 * (k2.V + k3.V) + k4.V);
    kf.a = 1./6. * (k1.a + 2 * (k2.a + k3.a) + k4.a);
    kf.da = 1./6. * (k1.da + 2 * (k2.da + k3.da) + k4.da);
    kf.w = 1./6. * (k1.w + 2 * (k2.w + k3.w) + k4.w);
    kf.dw = 1./6. * (k1.dw + 2 * (k2.dw + k3.dw) + k4.dw);

    integrate_motion_state(state, kf, dt);
}

void integrate_motion_pred(struct filter *f, matrix &lp, f_t dt)
{
    m4v4 dR_dW, drdt_dwdt;
    v4m4 dWp_dRp;
    
    m4 
        R = rodrigues(f->s.W, &dR_dW),
        rdt = rodrigues(f->s.w * dt, &drdt_dwdt);
    
    m4 Rp = R * rdt;
    v4
        Wp = invrodrigues(Rp, &dWp_dRp);
    m4v4 
        dRp_dW = dR_dW * rdt,
        dRp_dw = R * (drdt_dwdt * dt);
    m4
        dWp_dW = dWp_dRp * dRp_dW,
        dWp_dw = dWp_dRp * dRp_dw;

    for(int i = 0; i < lp.rows; ++i) {
        f_t tW[3], tw[3];
        for(int j = 0; j < 3; ++j) {
            tW[j] = 0.;
            tw[j] = 0.;
            for(int k = 0; k < 3; ++k) {
                tW[j] += lp(i, f->s.W.index + k) * dWp_dW[k][j];
                tw[j] += lp(i, f->s.W.index + k) * dWp_dw[k][j];
            }
        }
        for(int j = 0; j < 3; ++j) {
            lp(i, f->s.da.index + j) += lp(i, f->s.a.index + j) * dt;
            lp(i, f->s.a.index + j) += lp(i, f->s.V.index + j) * dt;
            lp(i, f->s.V.index + j) += lp(i, f->s.T.index + j) * dt;
            lp(i, f->s.dw.index + j) += lp(i, f->s.w.index + j) * dt;
            lp(i, f->s.w.index + j) += tw[j];
            lp(i, f->s.W.index + j) = tW[j];
        }
    }
}

void explicit_time_update(struct filter *f, uint64_t time)
{
    f_t dt = ((f_t)time - (f_t)f->last_time) / 1000000.;

    integrate_motion_covariance_explicit(f->s, dt);
    integrate_motion_state_explicit(f->s, dt);
}

void test_time_update(struct filter *f, f_t dt, int statesize)
{
    //test linearization
    MAT_TEMP(ltu, statesize, statesize);
    memset(ltu_data, 0, sizeof(ltu_data));
    for(int i = 0; i < statesize; ++i) {
        ltu(i, i) = 1.;
    }

    MAT_TEMP(save_state, 1, statesize);
    MAT_TEMP(save_new_state, 1, statesize);
    MAT_TEMP(state, 1, statesize);
    f->s.copy_state_to_array(save_state);

    integrate_motion_state_euler(f->s, dt);
    f->s.copy_state_to_array(save_new_state);

    f_t eps = .1;

    for(int i = 0; i < statesize; ++i) {
        memcpy(state_data, save_state_data, sizeof(state_data));
        f_t leps = state[i] * eps + 1.e-7;
        state[i] += leps;
        f->s.copy_state_from_array(state);
        integrate_motion_state_euler(f->s, dt);
        f->s.copy_state_to_array(state);
        for(int j = 0; j < statesize; ++j) {
            f_t delta = state[j] - save_new_state[j];
            f_t ldiff = leps * ltu(j, i);
            if((ldiff * delta < 0.) && (fabs(delta) > 1.e-5)) {
                fprintf(stderr, "%d\t%d\t: sign flip: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                fprintf(stderr, "%d\t%d\t: lin error: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
        }
    }
    f->s.copy_state_from_array(save_state);
}

void test_meas(struct filter *f, int pred_size, int statesize, int (*predict)(state *, matrix &, matrix *))
{
    //test linearization
    MAT_TEMP(lp, pred_size, statesize);
    MAT_TEMP(save_state, 1, statesize);
    MAT_TEMP(state, 1, statesize);
    f->s.copy_state_to_array(save_state);
    MAT_TEMP(pred, 1, pred_size);
    MAT_TEMP(save_pred, 1, pred_size);
    memset(lp_data, 0, sizeof(lp_data));
    predict(&f->s, save_pred, &lp);

    f_t eps = .1;

    for(int i = 0; i < statesize; ++i) {
        memcpy(state_data, save_state_data, sizeof(state_data));
        f_t leps = state[i] * eps + 1.e-7;
        state[i] += leps;
        f->s.copy_state_from_array(state);
        predict(&f->s, pred, NULL);
        for(int j = 0; j < pred_size; ++j) {
            f_t delta = pred[j] - save_pred[j];
            f_t ldiff = leps * lp(j, i);
            if((ldiff * delta < 0.) && (fabs(delta) > 1.e-5)) {
                fprintf(stderr, "%d\t%d\t: sign flip: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                fprintf(stderr, "%d\t%d\t: lin error: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
        }
    }
    f->s.copy_state_from_array(save_state);
}

void ekf_time_update(struct filter *f, uint64_t time)
{
    f_t dt = ((f_t)time - (f_t)f->last_time) / 1000000.;
    int statesize = f->s.cov.rows;
    MAT_TEMP(ltu, statesize, statesize);
    memset(ltu_data, 0, sizeof(ltu_data));
    for(int i = 0; i < statesize; ++i) {
        ltu(i, i) = 1.;
    }
    if(0) {
        test_time_update(f, dt, statesize);
    }
    integrate_motion_state_euler(f->s, dt);
    time_update(f->s.cov, ltu, f->s.p_cov, dt);
}

void transform_new_group(state &state, state_vision_group *g)
{
    if(g->status != group_initializing) return;
    m4 
        R = rodrigues(g->Wr, NULL),
        Rt = transpose(R),
        Rbc = rodrigues(state.Wc, NULL),
        Rcb = transpose(Rbc),
        RcbRt = Rcb * Rt;
    for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
        state_vision_feature *i = *fiter;
        m4 Rr = rodrigues(i->Wr, NULL);
        
        m4 
            Rw = Rr * Rbc,
            Rtot = RcbRt * Rw;
        v4
            Tw = Rr * state.Tc + i->Tr,
            Ttot = Rcb * (Rt * (Tw - g->Tr) - state.Tc);
        
        f_t rho = exp(*i);
        v4
            X0 = i->initial * rho, /*not homog in v4*/
            /*                Xr = Rbc * X0 + state->Tc,
                              Xw = Rw * X0 + Tw,
                              Xl = Rt * (Xw - g->Tr),*/
            X = Rtot * X0 + Ttot;
        
        f_t invZ = 1./X[2];
        v4 prediction = X * invZ;
        if(fabs(prediction[2]-1.) > 1.e-7 || prediction[3] != 0.) {
            fprintf(stderr, "FAILURE in feature projection in transform_new_group\n");
        }
    
        i->v = X[2] > .01 ? log(X[2]) : log(.01);
    }
}

void ukf_time_update(struct filter *f, uint64_t time, void (* do_time_update)(state &state, f_t dt) = integrate_motion_state_euler)
{
    //TODO: optimize for static vision portions
    f_t dt = ((f_t)time - (f_t)f->last_time) / 1000000.;
    int statesize = f->s.cov.rows;

    f_t alpha, kappa, lambda, beta;
    alpha = 1.e-3;
    kappa = 0.;
    lambda = alpha * alpha * (statesize + kappa) - statesize;
    beta = 2.;
    f_t W0m = lambda / (statesize + lambda);
    f_t W0c = W0m + 1. - alpha * alpha + beta;
    f_t Wi = 1. / (2. * (statesize + lambda));
    f_t gamma = sqrt(statesize + lambda);

    if(!matrix_cholesky(f->s.cov)) {
        f->numeric_failed = true;
    }

    MAT_TEMP(x, 1 + 2 * statesize, statesize);
    matrix state(x.data, statesize);
    f->s.copy_state_to_array(state);
    for(int i = 0; i < statesize; ++i) {
        for(int j = 0; j < statesize; ++j) {
            x(i + 1, j) = x(0, j) + gamma * f->s.cov(j, i);
            x(i + 1 + statesize, j) = x(0, j) - gamma * f->s.cov(j, i);
        }
    }
    for(int i = 0; i < 1 + statesize * 2; ++i) {
        matrix state2(x.data + i * x.stride, statesize);
        f->s.copy_state_from_array(state2);
        integrate_motion_state_euler(f->s, dt);
        f->s.copy_state_to_array(state2);
    }
    MAT_TEMP(new_state, 1, statesize);
    for(int j = 0; j < statesize; j++) {
        new_state[j] = x(0,j) * W0m;
    }
    f_t wmtot = W0m;
    for(int i = 1; i < 1 + statesize * 2; ++i) {
        wmtot += Wi;
        for(int j = 0; j < statesize; ++j) {
            new_state[j] += Wi * x(i, j);
        }
    }
    f->s.copy_state_from_array(new_state);

    //outer product
    for(int i = 0; i < statesize; ++i) {
        for(int j = i; j < statesize; ++j) {
            f->s.cov(i, j) = W0c * (x(0,i) - new_state[i]) * (x(0,j) - new_state[j]);
            for(int k = 1; k < 1 + statesize * 2; ++k) {
                f->s.cov(i, j) += Wi * (x(k, i) - new_state[i]) * (x(k, j) - new_state[j]);
            }
            f->s.cov(j, i) = f->s.cov(i, j);
        }
        f->s.cov(i,i) += f->s.p_cov[i] * dt;
    }
}

void ukf_meas_update(struct filter *f, int (* predict)(state *, matrix &, matrix *), void (*robustify)(struct filter *, matrix &, matrix &, void *), matrix &meas, matrix &inn, matrix &lp, matrix &m_cov, void *flag)
{
    //re-draw sigma points. TODO: integrate this with time update to maintain higher order moments
    int statesize = f->s.cov.rows;
    int meas_size = meas.cols;

    f_t alpha, kappa, lambda, beta;
    alpha = 1.e-3;
    kappa = 0.;
    lambda = alpha * alpha * (statesize + kappa) - statesize;
    beta = 2.;
    f_t W0m = lambda / (statesize + lambda);
    f_t W0c = W0m + 1. - alpha * alpha + beta;
    f_t Wi = 1. / (2. * (statesize + lambda));
    f_t gamma = sqrt(statesize + lambda);

    MAT_TEMP(cov_save, statesize, statesize);
    for(int i = 0; i < statesize; ++i) {
        for(int j = 0; j < statesize; ++j) {
            cov_save(i,j) = f->s.cov(i,j);
        }
    }

    if(!matrix_cholesky(f->s.cov)) {
        f->numeric_failed = true;
    }

    MAT_TEMP(x, 1 + 2 * statesize, statesize);
    matrix state(x.data, statesize);
    f->s.copy_state_to_array(state);
    for(int i = 0; i < statesize; ++i) {
        for(int j = 0; j < statesize; ++j) {
            x(i + 1, j) = x(0, j) + gamma * f->s.cov(j, i);
            x(i + 1 + statesize, j) = x(0, j) - gamma * f->s.cov(j, i);
        }
    }

    MAT_TEMP(y, 1 + 2 * statesize, meas_size);
    for(int i = 0; i < 1 + statesize * 2; ++i) {
        matrix state2(x.data + i * x.stride, statesize);
        f->s.copy_state_from_array(state2);
        matrix pred(y.data + i * y.stride, meas_size);
        predict(&f->s, pred, NULL);
    }

    //restore original state and cov
    f->s.copy_state_from_array(state);
    for(int i = 0; i < statesize; ++i) {
        for(int j = 0; j < statesize; ++j) {
            f->s.cov(i,j) = cov_save(i,j);
        }
    }

    MAT_TEMP(meas_mean, 1, meas_size);
    for(int i = 0; i < meas_size; ++i) {
        meas_mean[i] = W0m * y(0, i);
    }
    for(int i = 1; i < 1 + statesize*2; ++i) {
        for(int j = 0; j < meas_size; ++j) {
            meas_mean[j] += Wi * y(i, j);
        }
    }

    //calculate innovation
    for(int i = 0; i < meas_size; ++i) {
        inn[i] = meas[i] - meas_mean[i];
    }

    if(robustify) robustify(f, inn, m_cov, flag);

    //outer product to calculate Pyy
    MAT_TEMP(Pyy, meas_size, meas_size);
    for(int i = 0; i < meas_size; ++i) {
        for(int j = i; j < meas_size; ++j) {
            Pyy(i, j) = W0c * (y(0,i) - meas_mean[i]) * (y(0,j) - meas_mean[j]);
            for(int k = 1; k < 1 + statesize * 2; ++k) {
                Pyy(i, j) += Wi * (y(k, i) - meas_mean[i]) * (y(k, j) - meas_mean[j]);
            }
            Pyy(j, i) = Pyy(i, j);
        }
        Pyy(i,i) += m_cov[i];
    }

    //outer product to calculate Pxy
    MAT_TEMP(Pxy, statesize, meas_size);
    for(int i = 0; i < statesize; ++i) {
        for(int j = 0; j < meas_size; ++j) {
            Pxy(i, j) = W0c * (x(0,i) - state[i]) * (y(0,j) - meas_mean[j]);
            for(int k = 1; k < 1 + statesize * 2; ++k) {
                Pxy(i, j) += Wi * (x(k, i) - state[i]) * (y(k, j) - meas_mean[j]);
            }
        }
    }

    MAT_TEMP(gain, statesize, meas_size);
    MAT_TEMP (Pyy_inverse, Pyy.rows, Pyy.cols);
    for(int i = 0; i < Pyy.rows; ++i) {
        for (int j = 0; j < Pyy.cols; ++j) {
            Pyy_inverse(i, j) = Pyy(i, j);
        }
    }
    if(!matrix_invert(Pyy_inverse)) {
        f->numeric_failed = true;
    }

    matrix_product(gain, Pxy, Pyy_inverse);
    matrix_product(state, inn, gain, false, true, 1.0);
    f->s.copy_state_from_array(state);
    MAT_TEMP(PKt, gain.cols, gain.rows);
    matrix_product(PKt, Pyy, gain, false, true);
    matrix_product(f->s.cov, gain, PKt, false, false, 1.0, -1.0);
    
    /*   
    int statesize = f->s.cov.rows;
    int meas_size = meas.cols;
    MAT_TEMP(pred, 1, meas_size);
    predict(&f->s, pred, &lp);
    for(int i = 0; i < meas_size; ++i) {
        inn[i] = meas[i] - pred[i];
    }

    MAT_TEMP(state, 1, statesize);
    f->s.copy_state_to_array(state);
    meas_update(state, f->s.cov, inn, lp, m_cov);
    f->s.copy_state_from_array(state);*/
}

void debug_filter(struct filter *f, uint64_t time)
{
    fprintf(stderr, "orig cov is: \n");
    f->s.cov.print();
    
    int statesize = f->s.cov.rows;
    MAT_TEMP(mean, 1, f->s.cov.rows);
    MAT_TEMP(cov, f->s.cov.rows, f->s.cov.rows);
    f->s.copy_state_to_array(mean);
    for(int i = 0; i < statesize; ++i) {
        for(int j = 0; j < statesize; ++j) {
            cov(i, j) = f->s.cov(i, j);
        }
    }
    ukf_time_update(f, time);
    MAT_TEMP(ukf_state, 1, statesize);
    MAT_TEMP(ukf_cov, statesize, statesize);
    f->s.copy_state_to_array(ukf_state);
    f->s.copy_state_from_array(mean);
    for(int i = 0; i < statesize; ++i) {
        for(int j = 0; j < statesize; ++j) {
            ukf_cov(i, j) = f->s.cov(i, j);
            f->s.cov(i, j) = cov(i, j);
        }
    }
    
    ekf_time_update(f, time);
    f->s.copy_state_to_array(mean);
    f->s.copy_state_from_array(ukf_state);
    
    /*        fprintf(stderr, "ukf state is: \n");
              ukf_state.print();
              fprintf(stderr, "ekf state is: \n");
              mean.print();
              MAT_TEMP(resid, 1, statesize);
              for(int i = 0; i < statesize; ++i) {
              resid[i] = mean[i] - ukf_state[i];
              }
              fprintf(stderr, "residual is: \n");
              resid.print();*/
    fprintf(stderr, "ukf cov is: \n");
    ukf_cov.print();
    fprintf(stderr, "ekf state is: \n");
    f->s.cov.print();
    MAT_TEMP(resid, statesize, statesize);
    for(int i = 0; i < statesize; ++i) {
        for(int j = 0; j < statesize; ++j) {
            resid(i, j) = f->s.cov(i, j) - ukf_cov(i, j);
        }
    }
    fprintf(stderr, "residual is: \n");
    resid.print();
    
    debug_stop();
}

void filter_tick(struct filter *f, uint64_t time)
{
    //TODO: check negative time step!
    if(time <= f->last_time) return;
    if(f->last_time && f->active) {
        explicit_time_update(f, time);
    }
    f->last_time = time;
    if(!f->active) return;
    packet_t *packet = mapbuffer_alloc(f->output, packet_filter_position, 6 * sizeof(float));
    float *output = (float *)packet->data;
    output[0] = f->s.T.v[0];
    output[1] = f->s.T.v[1];
    output[2] = f->s.T.v[2];
    output[3] = f->s.W.v[0];
    output[4] = f->s.W.v[1];
    output[5] = f->s.W.v[2];
    mapbuffer_enqueue(f->output, packet, f->last_time);

    if(!f->measurement_running) f->s.initial_orientation = f->s.W.v;
    m4 
        R = rodrigues(f->s.W, NULL),
        Rt = transpose(R),
        Rbc = rodrigues(f->s.Wc, NULL),
        Rcb = transpose(Rbc),
        RcbRt = Rcb * Rt,
        initial_R = rodrigues(f->s.initial_orientation, NULL);

    f->s.camera_orientation = invrodrigues(RcbRt, NULL);
    v4 pt = Rcb * (Rt * (initial_R * (Rbc * v4(0., 0., 1., 0.))));
    if(pt[2] < 0.) pt[2] = -pt[2];
    if(pt[2] < .0001) pt[2] = .0001;
    float x = pt[0] / pt[2], y = pt[1] / pt[2];
    f->s.projected_orientation_marker = (feature_t) {x, y};
    //transform gravity into the local frame
    v4 local_gravity = RcbRt * v4(0., 0., f->s.g, 0.);
    //roll (in the image plane) is x/-y
    //TODO: verify sign
    f->s.orientation = atan2(local_gravity[0], -local_gravity[1]);

    f_t speed = norm(f->s.V.v);
    if(speed > 3.) { //1.4m/s is normal walking speed
        fprintf(stderr, "Velocity exceeds max bound\n");
        f->speed_failed = true;
    } else if(speed > 2.) {
        fprintf(stderr, "High velocity warning\n");
        f->speed_warning = true;
        f->speed_warning_time = f->last_time;
    }
    f_t accel = norm(f->s.a.v);
    if(accel > 9.8) { //1g would saturate sensor anyway
        fprintf(stderr, "Acceleration exceeds max bound\n");
        f->speed_failed = true;
    } else if(accel > 5.) { //max in mine is 6.
        fprintf(stderr, "High acceleration warning\n");
        f->speed_warning = true;
        f->speed_warning_time = f->last_time;
    }
    f_t ang_vel = norm(f->s.w.v);
    if(ang_vel > 3.) { //sensor saturation - 250/180*pi
        fprintf(stderr, "Angular velocity exceeds max bound\n");
        f->speed_failed = true;
    } else if(ang_vel > 1.) { // max in mine is 1.6
        fprintf(stderr, "High angular velocity warning\n");
        f->speed_warning = true;
        f->speed_warning_time = f->last_time;
    }
    //if(f->speed_warning && filter_converged(f) < 1.) f->speed_failed = true;
    if(f->last_time - f->speed_warning_time > 1000000) f->speed_warning = false;

    //fprintf(stderr, "%d [%f %f %f] [%f %f %f]\n", time, output[0], output[1], output[2], output[3], output[4], output[5]); 
}

void run_tracking(struct filter *f, feature_t *trackedfeats);
void run_local_tracking(struct filter *f, feature_t *trackedfeats);

f_t feature_distance(feature_t f1, feature_t f2)
{
    f_t dx = f1.x - f2.x, dy = f1.y - f2.y;
    return sqrt(dx*dx + dy*dy);
}

//********HERE - this is more or less implemented, but still behaves strangely, and i haven't yet updated the ios callers (accel and gyro)
//try ukf and small integration step
void process_observation_queue(struct filter *f)
{
    if(!f->observations.observations.size()) return;
    int statesize = f->s.cov.rows;
    //TODO: break apart sort and preprocess
    f->observations.preprocess(true, statesize);
    MAT_TEMP(state, 1, statesize);

    vector<observation *>::iterator obs = f->observations.observations.begin();

    MAT_TEMP(inn, 1, f->observations.meas_size);
    MAT_TEMP(m_cov, 1, f->observations.meas_size);
    while(obs != f->observations.observations.end()) {
        int count = 0;
        uint64_t obs_time = (*obs)->time_apparent;
        filter_tick(f, obs_time);
        for(list<preobservation *>::iterator pre = f->observations.preobservations.begin(); pre != f->observations.preobservations.end(); ++pre) (*pre)->process(true);

        //compile the next group of measurements to be processed together
        int meas_size = 0;
        vector<observation *>::iterator start = obs;        
        while(obs != f->observations.observations.end()) {
            meas_size += (*obs)->size;
            ++obs;
            if(obs == f->observations.observations.end()) break;
            if((*obs)->size == 3) break;
            //if((*obs)->size == 0) break;
            if((*obs)->time_apparent != obs_time) break;
        }
        vector<observation *>::iterator end = obs;
        inn.resize(1, meas_size);
        m_cov.resize(1, meas_size);
        f->s.copy_state_to_array(state);

        bool is_vis = false;
        bool is_init = false;
        //these aren't in the same order as they appear in the array - need to build up my local versions as i go
        //do prediction and linearization
        for(obs = start; obs != end; ++obs) {
            f_t dt = ((f_t)(*obs)->time_apparent - (f_t)obs_time) / 1000000.;
            if((*obs)->time_apparent != obs_time) {
                integrate_motion_state_explicit(f->s, dt);
            }
            (*obs)->predict(true);
            //(*obs)->project_covariance(f->s.cov);
            if((*obs)->time_apparent != obs_time) {
                f->s.copy_state_from_array(state);
                assert(0); //integrate_motion_pred(f, (*obs)->lp, dt);
            }
            if((*obs)->size == 2) is_vis = true;
            if((*obs)->size == 0) is_init = true;
        }

        //measure; calculate innovation and covariance
        for(obs = start; obs != end; ++obs) {
            (*obs)->measure();
            if((*obs)->valid) {
                for(int i = 0; i < (*obs)->size; ++i) {
                    (*obs)->inn[i] = (*obs)->meas[i] - (*obs)->pred[i];
                }
                (*obs)->compute_measurement_covariance();
                for(int i = 0; i < (*obs)->size; ++i) {
                    inn[count + i] = (*obs)->inn[i];
                    m_cov[count + i] = (*obs)->m_cov[i];
                }
                count += (*obs)->size;
            }
        }
        inn.resize(1, count);
        m_cov.resize(1, count);
        if(count) { //meas_update(state, f->s.cov, inn, lp, m_cov)
            //project state cov onto measurement to get cov(meas, state)
            // matrix_product(LC, lp, A, false, false);
            MAT_TEMP(LC, count, statesize);
            matrix A(f->s.cov.data, statesize, statesize, f->s.cov.maxrows, f->s.cov.stride);
            int index = 0;
            for(obs = start; obs != end; ++obs) {
                if((*obs)->valid && (*obs)->size) {
                    matrix dst(&LC(index, 0), (*obs)->size, statesize, LC.maxrows, LC.stride);
                    (*obs)->project_covariance(dst, f->s.cov);
                    index += (*obs)->size;
                }
            }
            
            //project cov(state, meas)=(LC)' onto meas to get cov(meas, meas), and add measurement cov to get residual covariance
            MAT_TEMP(res_cov, count, count);
            //matrix_product(res_cov, lp, LC, false, true);
            index = 0;
            for(obs = start; obs != end; ++obs) {
                if((*obs)->valid && (*obs)->size) {
                    matrix dst(&res_cov(index, 0), (*obs)->size, count, res_cov.maxrows, res_cov.stride);
                    (*obs)->project_covariance(dst, LC);
                    for(int i = 0; i < (*obs)->size; ++i) {
                        res_cov(index + i, index + i) += m_cov[index + i];
                        (*obs)->inn_cov[i] = res_cov(index + i, index + i);
                    }
                    index += (*obs)->size;
                }
            }

            MAT_TEMP(K, statesize, count);
            //lambda K = CL'
            matrix_transpose(K, LC);
            if(!matrix_solve(res_cov, K)) {
                f->numeric_failed = true;
            }
            //state.T += innov.T * K.T
            matrix_product(state, inn, K, false, true, 1.0);
            //cov -= KHP
            matrix_product(A, K, LC, false, false, 1.0, -1.0);
        }
        //meas_update(state, f->s.cov, f->observations.inn, f->observations.lp, f->observations.m_cov);
        f->s.copy_state_from_array(state);
    }
    f->observations.clear();
    f_t delta_T = norm(f->s.T - f->s.last_position);
    if(delta_T > .01) {
        f->s.total_distance += norm(f->s.T - f->s.last_position);
        f->s.last_position = f->s.T;
    }
    if(f->measurement_callback) f->measurement_callback(f->measurement_callback_object);
}

void filter_meas(struct filter *f, matrix &inn, matrix &lp, matrix &m_cov)
{
    int statesize = f->s.cov.rows;
    MAT_TEMP(state, 1, statesize);
    f->s.copy_state_to_array(state);
    meas_update(state, f->s.cov, inn, lp, m_cov);
    f->s.copy_state_from_array(state);
}

void ekf_meas_update(struct filter *f, int (* predict)(state *, matrix &, matrix *, void *), void (*robustify)(struct filter *, matrix &, matrix &, void *), matrix &meas, matrix &inn, matrix &lp, matrix &m_cov, void *flag)
{
    int statesize = f->s.cov.rows;
    int meas_size = meas.cols;
    MAT_TEMP(pred, 1, meas_size);
    predict(&f->s, pred, &lp, flag);
    for(int i = 0; i < meas_size; ++i) {
        inn[i] = meas[i] - pred[i];
    }
    if(robustify) robustify(f, inn, m_cov, flag);

    MAT_TEMP(state, 1, statesize);
    f->s.copy_state_to_array(state);
    meas_update(state, f->s.cov, inn, lp, m_cov);
    f->s.copy_state_from_array(state);
}

void filter_meas2(struct filter *f, int (* predict)(state *, matrix &, matrix *, void *), void (*robustify)(struct filter *, matrix &, matrix &, void *), matrix &meas, matrix &inn, matrix &lp, matrix &m_cov, void *flag)
{
    ekf_meas_update(f, predict, robustify, meas, inn, lp, m_cov, flag);
}

static struct camera_calibration get_camera_calibration(struct filter *f)
{
    struct camera_calibration calibration;
    calibration.F.x = f->s.focal_length;
    calibration.F.y = f->s.focal_length;
    calibration.C.x = f->s.center_x;
    calibration.C.y = f->s.center_y;
    calibration.K[0] = f->s.k1;
    calibration.K[1] = f->s.k2;
    calibration.K[2] = f->s.k3;
    calibration.niter = 10;
    calibration.maxerr = .1 / f->s.focal_length;
    return calibration;
}

static double compute_gravity(double latitude, double altitude)
{
    //http://en.wikipedia.org/wiki/Gravity_of_Earth#Free_air_correction
    double sin_lat = sin(latitude/180. * M_PI);
    double sin_2lat = sin(2*latitude/180. * M_PI);
    return 9.780327 * (1 + 0.0053024 * sin_lat*sin_lat - 0.0000058 * sin_2lat*sin_2lat) - 3.086e-6 * altitude;
}

void do_gravity_init(struct filter *f, float *data, uint64_t time)
{
    if(f->location_valid) {
        f->s.g = compute_gravity(f->latitude, f->altitude);
    }
    else f->s.g = 9.8;
    //first measurement - use to determine orientation
    v4 gravity(data[0], data[1], data[2], 0.);
    gravity.print();
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
        f->s.W = s;
    } else{
        f->s.W = s * (theta / sintheta);
    }

    //set up plots
    if(f->visbuf) {
        packet_plot_setup(f->visbuf, time, packet_plot_meas_a, "Meas-alpha", sqrt(f->a_variance));
        packet_plot_setup(f->visbuf, time, packet_plot_meas_w, "Meas-omega", sqrt(f->w_variance));
        packet_plot_setup(f->visbuf, time, packet_plot_inn_a, "Inn-alpha", sqrt(f->a_variance));
        packet_plot_setup(f->visbuf, time, packet_plot_inn_w, "Inn-omega", sqrt(f->w_variance));
    }
    f->gravity_init = true;

    //let the map know what the vision measurement cov is
    if(f->s.mapperbuf) {
        packet_t *pv = mapbuffer_alloc(f->s.mapperbuf, packet_feature_variance, sizeof(packet_feature_variance_t) - 16);
        *(float*)pv->data = f->vis_cov;
        mapbuffer_enqueue(f->s.mapperbuf, pv, time);
    }

    //fix up groups and features that have already been added
    for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        g->Wr = f->s.W;
    }

    struct camera_calibration calibration = get_camera_calibration(f);

    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        state_vision_feature *i = *fiter;
        feature_t uncalibrated = { (float)i->current[0], (float)i->current[1] };
        feature_t calib;
        calibration_normalize(&calibration, &uncalibrated, &calib, 1);
        i->initial[0] = calib.x;
        i->initial[1] = calib.y;
        //i->initial = i->current;
        i->Wr = f->s.W;
    }
}

static bool check_packet_time(struct filter *f, uint64_t t)
{
    if(t < f->last_packet_time) {
        fprintf(stderr, "Warning: received packets out of order: %lld came first, then %lld\n", f->last_packet_time, t);
        return false;
    }
    f->last_packet_time = t;
    return true;
}

extern "C" void sfm_imu_measurement(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(!check_packet_time(f, p->header.time)) return;
    if(p->header.type != packet_imu) return;
    float *data = (float *)&p->data;
    
    if(!f->gravity_init) {
        do_gravity_init(f, data, p->header.time);
    }
    
    observation_accelerometer *obs_a = f->observations.new_observation_accelerometer(&f->s, p->header.time, p->header.time);
    observation_gyroscope *obs_w = f->observations.new_observation_gyroscope(&f->s, p->header.time, p->header.time);

    for(int i = 0; i < 3; ++i) {
        obs_a->meas[i] = data[i];
        obs_w->meas[i] = data[i+3];
    }
    obs_a->variance = f->a_variance;
    obs_w->variance = f->w_variance;
    obs_a->initializing = !f->active;
    obs_w->initializing = !f->active;
    process_observation_queue(f);
    /*
    float am_float[3];
    float wm_float[3];
    float ai_float[3];
    float wi_float[3];
    for(int i = 0; i < 3; ++i) {
        am_float[i] = data[i];
        wm_float[i] = data[i+3];
        ai_float[i] = obs_a->inn[i];
        wi_float[i] = obs_w->inn[i+3];
    }
    if(f->visbuf) {
        packet_plot_send(f->visbuf, p->header.time, packet_plot_inn_a, 3, ai_float);
        packet_plot_send(f->visbuf, p->header.time, packet_plot_inn_w, 3, wi_float);
        packet_plot_send(f->visbuf, p->header.time, packet_plot_meas_a, 3, am_float);
        packet_plot_send(f->visbuf, p->header.time, packet_plot_meas_w, 3, wm_float);
    }
    */
}

extern "C" void sfm_accelerometer_measurement(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(!check_packet_time(f, p->header.time)) return;
    if(p->header.type != packet_accelerometer) return;
    f->got_accelerometer = true;
    if(!f->got_gyroscope || !f->got_image) return;
    float *data = (float *)&p->data;

    for(int i = 0; i < 3; ++i) {
        if(fabs(data[i]) > f->accelerometer_max) f->accelerometer_max = fabs(data[i]);
    }

    if(!f->gravity_init) do_gravity_init(f, data, p->header.time);

    observation_accelerometer *obs_a = f->observations.new_observation_accelerometer(&f->s, p->header.time, p->header.time);
    for(int i = 0; i < 3; ++i) {
        obs_a->meas[i] = data[i];
    }
    obs_a->variance = f->a_variance;
    obs_a->initializing = false;
    process_observation_queue(f);
    /*
    float am_float[3];
    float ai_float[3];
    for(int i = 0; i < 3; ++i) {
        am_float[i] = data[i];
        ai_float[i] = obs_a->inn[i];
    }
    if(f->visbuf) {
        packet_plot_send(f->visbuf, p->header.time, packet_plot_inn_a, 3, ai_float);
        packet_plot_send(f->visbuf, p->header.time, packet_plot_meas_a, 3, am_float);
    }
    */
}

extern "C" void sfm_gyroscope_measurement(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(!check_packet_time(f, p->header.time)) return;
    if(p->header.type != packet_gyroscope) return;
    float *data = (float *)&p->data;
    f->got_gyroscope = true;
    if(!f->got_accelerometer || !f->got_image || !f->gravity_init) return;

    for(int i = 0; i < 3; ++i) {
        if(fabs(data[i]) > f->gyroscope_max) f->gyroscope_max = fabs(data[i]);
    }

    observation_gyroscope *obs_w = f->observations.new_observation_gyroscope(&f->s, p->header.time, p->header.time);
    for(int i = 0; i < 3; ++i) {
        obs_w->meas[i] = data[i];
    }
    obs_w->variance = f->w_variance;
    obs_w->initializing = false;
    process_observation_queue(f);
    /*
    float wm_float[3];
    float wi_float[3];
    for(int i = 0; i < 3; ++i) {
        wm_float[i] = data[i];
        wi_float[i] = obs_w->inn[i];
    }
    if(f->visbuf) {
        packet_plot_send(f->visbuf, p->header.time, packet_plot_inn_w, 3, wi_float);
        packet_plot_send(f->visbuf, p->header.time, packet_plot_meas_w, 3, wm_float);
    }
    */
}

static int sfm_process_features(struct filter *f, uint64_t time)
{
    int useful_drops = 0;
    for(list<state_vision_feature *>::iterator fi = f->s.features.begin(); fi != f->s.features.end(); ++fi) {
        state_vision_feature *i = *fi;
        if(i->current[0] == INFINITY) {
            if(i->status == feature_normal && i->variance < i->max_variance) {
                i->status = feature_gooddrop;
                ++useful_drops;
            } else {
                i->status = feature_empty;
            }
        } else if(i->outlier > i->outlier_reject || i->status == feature_reject) {
            i->status = feature_empty;
        }
    }
    if(useful_drops) {
        packet_t *sp = mapbuffer_alloc(f->output, packet_filter_reconstruction, useful_drops * 3 * sizeof(float));
        sp->header.user = useful_drops;
        packet_filter_feature_id_association_t *association = (packet_filter_feature_id_association_t *)mapbuffer_alloc(f->output, packet_filter_feature_id_association, useful_drops * sizeof(uint64_t));
        association->header.user = useful_drops;
        packet_feature_intensity_t *intensity = (packet_feature_intensity_t *)mapbuffer_alloc(f->output, packet_feature_intensity, useful_drops);
        intensity->header.user = useful_drops;
        float (*world)[3] = (float (*)[3])sp->data;
        int nfeats = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(i->status == feature_gooddrop) {
                world[nfeats][0] = i->world[0];
                world[nfeats][1] = i->world[1];
                world[nfeats][2] = i->world[2];
                association->feature_id[nfeats] = i->id;
                intensity->intensity[nfeats] = i->intensity;
                ++nfeats;
            }
        }
        mapbuffer_enqueue(f->output, sp, time);
        mapbuffer_enqueue(f->output, (packet_t *)association, time);
        mapbuffer_enqueue(f->output, (packet_t *)intensity, time);
    }

    int features_used = f->s.process_features(time);

    //clean up dropped features and groups
    list<state_vision_feature *>::iterator fi = f->s.features.begin();
    while(fi != f->s.features.end()) {
        state_vision_feature *i = *fi;
        if(i->status == feature_gooddrop) {
            if(f->recognition_buffer) {
                packet_recognition_feature_t *rp = (packet_recognition_feature_t *)mapbuffer_alloc(f->recognition_buffer, packet_recognition_feature, sizeof(packet_recognition_feature_t));
                rp->groupid = i->groupid;
                rp->id = i->id;
                rp->ix = i->initial[0];
                rp->iy = i->initial[1];
                rp->x = i->relative[0];
                rp->y = i->relative[1];
                rp->z = i->relative[2];
                rp->depth = exp(i->v);
                f_t var = i->measurement_var < i->variance ? i->variance : i->measurement_var;
                //for measurement var, the values are simply scaled by depth, so variance multiplies by depth^2
                //for depth variance, d/dx = e^x, and the variance is v*(d/dx)^2
                rp->variance = var * rp->depth * rp->depth;
                mapbuffer_enqueue(f->recognition_buffer, (packet_t *)rp, time);
            }
        }
        if(i->status == feature_gooddrop) i->status = feature_empty;
        if(i->status == feature_empty) {
            delete i;
            fi = f->s.features.erase(fi);
        } else ++fi;
    }

    list<state_vision_group *>::iterator giter = f->s.groups.children.begin();
    while(giter != f->s.groups.children.end()) {
        state_vision_group *g = *giter;
        if(g->status == group_empty) {
            if(f->recognition_buffer) {
                packet_recognition_group_t *pg = (packet_recognition_group_t *)mapbuffer_alloc(f->recognition_buffer, packet_recognition_group, sizeof(packet_recognition_group_t));
                pg->id = g->id;
                pg->header.user = 1;
                mapbuffer_enqueue(f->recognition_buffer, (packet_t *)pg, time);
            }
            delete g;
            giter = f->s.groups.children.erase(giter);
        } else {
            if(g->status == group_initializing) {
                for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                    state_vision_feature *i = *fiter;
                    fprintf(stderr, "calling triangulate feature from process\n");
                    assert(0);
                    //triangulate_feature(&(f->s), i);
                }
            }
            ++giter;
        }
    }

    f->s.remap();

    return features_used;
}

bool feature_variance_comp(state_vision_feature *p1, state_vision_feature *p2) {
    return p1->variance < p2->variance;
}

void sfm_setup_next_frame(struct filter *f, uint64_t time)
{
    ++f->frame;
    int feats_used = f->s.features.size();

    if(!f->active) {
        if(f->frame >= f->skip && f->gravity_init) {
            f->active = true;
            filter_reset_position(f);
            for(int i = 0; i < 3; ++i) filter_reset_covariance(f, f->s.V.index + i, 1.);
            f->s.V.v = 0.;
            //set up plot packets
            if(f->visbuf) {
                packet_plot_setup(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 1, "Cov-T", 0.);
                packet_plot_setup(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 2, "Cov-W", 0.);
                packet_plot_setup(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 3, "Cov-a", 0.);
                packet_plot_setup(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 4, "Cov-w", 0.);
                packet_plot_setup(f->visbuf, time, packet_plot_inn_v + MAXGROUPS, "Inn-vis *", sqrt(f->vis_cov));
                for(int g = 0; g < MAXGROUPS; ++g) {
                    char name[32];
                    sprintf(name, "Inn-vis %d", g);
                    packet_plot_setup(f->visbuf, time, packet_plot_inn_v + g, name, sqrt(f->vis_cov));   
                }
            }
        } else return;
    }
    float tv[3];
    if(f->visbuf) {
        for(int i = 0; i < 3; ++i) tv[i] = f->s.T.variance[i];
        packet_plot_send(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 1, 3, tv);
        for(int i = 0; i < 3; ++i) tv[i] = f->s.W.variance[i];
        packet_plot_send(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 2, 3, tv);
        for(int i = 0; i < 3; ++i) tv[i] = f->s.a.variance[i];
        packet_plot_send(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 3, 3, tv);
        for(int i = 0; i < 3; ++i) tv[i] = f->s.w.variance[i];
        packet_plot_send(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 4, 3, tv);
    }
    preobservation_vision_base *base = f->observations.new_preobservation_vision_base(&f->s, f->track.width, f->track.height);
    base->im1 = f->track.im1;
    base->im2 = f->track.im2;
    if(feats_used) {
        int statesize = f->s.cov.rows;
        int meas_used = feats_used * 2;

        int fi = 0;
        for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
            state_vision_group *g = *giter;
            if(!g->status || g->status == group_initializing) continue;
            preobservation_vision_group *group = f->observations.new_preobservation_vision_group(&f->s);
            group->group = g;
            group->base = base;
            for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                state_vision_feature *i = *fiter;
                if(!i->status) continue;

                uint64_t extra_time = f->shutter_delay + i->uncalibrated[1]/f->image_height * f->shutter_period;
                observation_vision_feature *obs = f->observations.new_observation_vision_feature(&f->s, time + extra_time, time);
                obs->state_group = g;
                obs->base = base;
                obs->group = group;
                obs->feature = i;
                obs->meas[0] = i->current[0];
                obs->meas[1] = i->current[1];

                fi += 2;
            }
        }
    }
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        state_vision_feature *i = *fiter;
        if(i->status == feature_initializing || i->status == feature_ready) {
            uint64_t extra_time = f->shutter_delay + i->uncalibrated[1]/f->image_height * f->shutter_period;
            observation_vision_feature_initializing *obs = f->observations.new_observation_vision_feature_initializing(&f->s, time + extra_time, time);
            obs->base = base;
            obs->feature = i;
            i->prediction.x = i->current[0];
            i->prediction.y = i->current[1];
        }
    }
}

void add_new_groups(struct filter *f, uint64_t time)
{
    int space = f->s.maxstatesize - f->s.statesize - 6;
    if(space > f->max_group_add) space = f->max_group_add;
    if(space >= f->min_group_add) {
        f_t max_add = f->max_add_vis_cov;
        f_t min_add = f->min_add_vis_cov;
        if(f->s.groups.children.size() == 0) {
            max_add = f->init_vis_cov;
            min_add = f->init_vis_cov;
        }
        int ready = 0;
        vector<state_vision_feature *> availfeats;
        availfeats.reserve(f->s.features.size());
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(i->status == feature_ready) {
                ++ready;
            }
            if((i->status == feature_ready || i->status == feature_initializing) && i->variance <= max_add) {
                availfeats.push_back(i);
            }
        }
        if(ready >= f->min_group_add || (f->s.groups.children.size() == 0 && availfeats.size() != 0)) {
            make_heap(availfeats.begin(), availfeats.end(), feature_variance_comp);
            while(availfeats.size() > space || (availfeats[0]->variance > min_add && availfeats.size() > f->min_group_add)) {
                pop_heap(availfeats.begin(), availfeats.end(), feature_variance_comp);
                availfeats.pop_back();
            }
            state_vision_group * g = f->s.add_group(time);
            for(vector<state_vision_feature *>::iterator fiter = availfeats.begin(); fiter != availfeats.end(); ++fiter) {
                state_vision_feature *i = *fiter;
                g->features.children.push_back(i);
                i->index = -1;
                i->groupid = g->id;
                i->found_time = time;
                i->status = feature_normal;
                if(i->variance > f->max_add_vis_cov) {
                    //feature wasn't initialized well enough - will break the filter if added
                    i->v = i->initial_rho;
                    i->variance = i->initial_var;
                }
            }

            g->make_normal();
            f->s.remap();
            g->status = group_initializing;
            transform_new_group(f->s, g);
            g->status = group_normal;
            struct camera_calibration calibration = get_camera_calibration(f);
            for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                state_vision_feature *i = *fiter;
                feature_t uncalibrated = { (float)i->current[0], (float)i->current[1] };
                feature_t calib;
                calibration_normalize(&calibration, &uncalibrated, &calib, 1);
                i->initial[0] = calib.x;
                i->initial[1] = calib.y;
                //i->initial = i->current;
                i->Tr = g->Tr;
                i->Wr = g->Wr;
                f->s.cov(i->index, i->index) *= 2.;
            }

            //********* NOW: DO THIS FOR OTHER PACKETS
            if(f->recognition_buffer) {
                fprintf(stderr, "Relative orientation updated in gravity_init: apply to recognition\n");
                #warning "Relative orientation updated in gravity_init: apply to recognition\n"
                packet_recognition_group_t *pg = (packet_recognition_group_t *)mapbuffer_alloc(f->recognition_buffer, packet_recognition_group, sizeof(packet_recognition_group_t));
                pg->id = g->id;
                m4 R = rodrigues(g->Wr.v, NULL);
                v4 local_down = transpose(R) * v4(0., 0., 1., 0.);
                v4 relative_rot = relative_rotation(local_down, v4(0., 0., 1., 0.));
                for(int i = 0; i < 3; ++i) {
                    pg->W[i] = relative_rot[i];
                    pg->W_var[i] = g->Wr.variance[i];
                }
                //TODO: fix this: get correct variance
                pg->W_var[2] = 0.;
                mapbuffer_enqueue(f->recognition_buffer, (packet_t *)pg, time);
            }
        }
    }
}

void filter_send_output(struct filter *f, uint64_t time)
{
    float tv[3];
    /*    if(f->visbuf) {
        for(int i = 0; i < 3; ++i) tv[i] = f->s.T.variance[i];
        packet_plot_send(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 1, 3, tv);
        for(int i = 0; i < 3; ++i) tv[i] = f->s.W.variance[i];
        packet_plot_send(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 2, 3, tv);
        for(int i = 0; i < 3; ++i) tv[i] = f->s.a.variance[i];
        packet_plot_send(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 3, 3, tv);
        for(int i = 0; i < 3; ++i) tv[i] = f->s.w.variance[i];
        packet_plot_send(f->visbuf, time, packet_plot_inn_v + MAXGROUPS + 4, 3, tv);
        }*/
    int nfeats = f->s.features.size();
    packet_filter_current_t *cp = (packet_filter_current_t *)mapbuffer_alloc(f->output, packet_filter_current, sizeof(packet_filter_current) - 16 + nfeats * 3 * sizeof(float));
    packet_filter_current_t *sp;
    if(f->s.mapperbuf) {
        //get world
        sp = (packet_filter_current_t *)mapbuffer_alloc(f->s.mapperbuf, packet_filter_current, sizeof(packet_filter_current) - 16 + nfeats * 3 * sizeof(float));
    }
    packet_filter_feature_id_visible_t *visible = (packet_filter_feature_id_visible_t *)mapbuffer_alloc(f->output, packet_filter_feature_id_visible, sizeof(packet_filter_feature_id_visible_t) - 16 + nfeats * sizeof(uint64_t));
    for(int i = 0; i < 3; ++i) {
        visible->T[i] = f->s.T.v[i];
        visible->W[i] = f->s.W.v[i];
    }
    int n_good_feats = 0;
    for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        if(g->status == group_initializing) continue;
        for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(!i->status || i->status == feature_reject) continue;
            if(f->s.mapperbuf) {
                sp->points[n_good_feats][0] = i->local[0];
                sp->points[n_good_feats][1] = i->local[1];
                sp->points[n_good_feats][2] = i->local[2];
            }
            cp->points[n_good_feats][0] = i->world[0];
            cp->points[n_good_feats][1] = i->world[1];
            cp->points[n_good_feats][2] = i->world[2];
            visible->feature_id[n_good_feats] = i->id;
            ++n_good_feats;
        }
    }
    cp->header.user = n_good_feats;
    visible->header.user = n_good_feats;
    if(f->s.mapperbuf) {
        sp->header.user = n_good_feats;
        sp->reference = f->s.reference ? f->s.reference->id : f->s.last_reference;
        v4 rel_T, rel_W;
        f->s.get_relative_transformation(f->s.T, f->s.W, rel_T, rel_W);
        for(int i = 0; i < 3; ++i) {
            sp->T[i] = rel_T[i];
            sp->W[i] = rel_W[i];
        }
        mapbuffer_enqueue(f->s.mapperbuf, (packet_t *)sp, time);
    }
    mapbuffer_enqueue(f->output, (packet_t*)cp, time);
    mapbuffer_enqueue(f->output, (packet_t*)visible, time);
}

/*
int temp_track(struct filter *f)
{
    feature_t feats[f->s.features.size()];
    int nfeats = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        feats[nfeats].x = (*fiter)->current[0];
        feats[nfeats].y = (*fiter)->current[1];
        ++nfeats;
    }
    feature_t trackedfeats[f->s.features.size()];
    char found_features[f->s.features.size()];
    float errors[f->s.features.size()];
    cvCalcOpticalFlowPyrLK(f->track->header1, f->track->header2,
                           f->track->pyramid1, f->track->pyramid2,
                           (CvPoint2D32f *)feats, (CvPoint2D32f *)trackedfeats, f->s.features.size(),
                           f->track->optical_flow_window, f->track->levels,
                           found_features, errors, f->track->optical_flow_termination_criteria,
                           f->track->pyramidgood?CV_LKFLOW_PYR_A_READY:0);
    int feat = 0;
    int area = (f->track->spacing * 2 + 3);
    area = area * area;

    int goodfeats = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        f_t x,y;
        if(found_features[feat] &&
           trackedfeats[feat].x > 0.0 &&
           trackedfeats[feat].y > 0.0 &&
           trackedfeats[feat].x < f->track->width-1 &&
           trackedfeats[feat].y < f->track->height-1 &&
           errors[feat] / area < f->track->max_tracking_error) {
            x = trackedfeats[feat].x;
            y = trackedfeats[feat].y;
            ++goodfeats;
        } else {
            x = y = INFINITY;
        }
        state_vision_feature *i = *fiter;
        i->current[0] = x;
        i->current[1] = y;
        i->uncalibrated[0] = x;
        i->uncalibrated[1] = y;
        ++feat;
    }
    return goodfeats;
}
*/

static void mask_feature(struct tracker *t, uint8_t *scaled_mask, int fx, int fy)
{
    int scaled_width = t->width / 8;
    int scaled_height = t->height / 8;
    int x = fx / 8;
    int y = fy / 8;
    scaled_mask[x + y * scaled_width] = 0;
    if(y > 1) {
        //don't worry about horizontal overdraw as this just is the border on the previous row
        for(int i = 0; i < 3; ++i) scaled_mask[x-1+i + (y-1)*scaled_width] = 0;
        scaled_mask[x-1 + y*scaled_width] = 0;
    } else {
        //don't draw previous row, but need to check pixel to left
        if(x > 1) scaled_mask[x-1 + y * scaled_width] = 0;
    }
    if(y < scaled_height - 1) {
        for(int i = 0; i < 3; ++i) scaled_mask[x-1+i + (y+1)*scaled_width] = 0;
        scaled_mask[x+1 + y*scaled_width] = 0;
    } else {
        if(x < scaled_height - 1) scaled_mask[x+1 + y * scaled_width] = 0;
    }
}

static void addfeatures(struct filter *f, struct tracker *t, int newfeats, unsigned char *img, unsigned int width)
{
    //set up mask - leave a 1-block strip on border off
    //use 8 byte blocks
    int scaled_width = t->width / 8;
    int scaled_height = t->height / 8;
    uint8_t scaled_mask[scaled_width * scaled_height];
    memset(scaled_mask, 0, scaled_width);
    memset(scaled_mask + scaled_width, 1, (scaled_height - 2) * scaled_width);
    memset(scaled_mask + (scaled_height - 1) * scaled_width, 0, scaled_width);
    //vertical border
    for(int y = 1; y < scaled_height - 1; ++y) {
        scaled_mask[0 + y * scaled_width] = 0;
        scaled_mask[scaled_width-1 + y * scaled_width] = 0;
    }
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        mask_feature(t, scaled_mask, (*fiter)->current[0], (*fiter)->current[1]);
    }

    feature_t newfeatures[newfeats];
   
    int b = 20;
    unsigned char *im = img;
    int xsize = t->width;
    int ysize = t->height;
    int stride = t->width;

    fast_detector detector(xsize, ysize, stride);
    vector<xy> &keypoints = detector.detect(im, scaled_mask, newfeats, b);

    if(keypoints.size() < newfeats) newfeats = keypoints.size();
    int found_feats = 0;
    for(int i = 0; i < keypoints.size(); ++i) {
        int x = keypoints[i].x;
        int y = keypoints[i].y;
        if(scaled_mask[(x/8) + (y/8) * (width/8)]) {
            newfeatures[found_feats].x = x;
            newfeatures[found_feats].y = y;
            mask_feature(t, scaled_mask, x, y);
            found_feats++;
            if(found_feats == newfeats) break;
        }
    }
    newfeats = found_feats;

    //cvFindCornerSubPix(t->header1, (CvPoint2D32f *)newfeatures, newfeats, t->optical_flow_window, cvSize(-1,-1), t->optical_flow_termination_criteria);

    int goodfeats = 0;
    struct camera_calibration calibration = get_camera_calibration(f);
    for(int i = 0; i < newfeats; ++i) {
        if(newfeatures[i].x > 0.0 &&
           newfeatures[i].y > 0.0 &&
           newfeatures[i].x < t->width-1 &&
           newfeatures[i].y < t->height-1) {
            feature_t calib;
            calibration_normalize(&calibration, &newfeatures[i], &calib, 1);
            state_vision_feature *feat = f->s.add_feature(calib.x, calib.y);
            feat->status = feature_initializing;
            feat->current[0] = feat->uncalibrated[0] = newfeatures[i].x;
            feat->current[1] = feat->uncalibrated[1] = newfeatures[i].y;
            int lx = floor(newfeatures[i].x);
            int ly = floor(newfeatures[i].y);
            feat->intensity = (((unsigned int)img[lx + ly*width]) + img[lx + 1 + ly * width] + img[lx + width + ly * width] + img[lx + 1 + width + ly * width]) >> 2;
        }
    }
    f->s.remap();
}

/*
void run_tracking(struct filter *f, feature_t *trackedfeats)
{
    struct tracker *t = f->track;
    //feature_t *trackedfeats[f->s.features.size()];
    //are we tracking anything?
    int newindex = 0;
    if(f->s.features.size()) {
        feature_t feats[f->s.features.size()];
        int nfeats = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            feats[nfeats].x = (*fiter)->current[0];
            feats[nfeats].y = (*fiter)->current[1];
            ++nfeats;
        }
        float errors[nfeats];
        char found_features[nfeats];
        
        //track
        cvCalcOpticalFlowPyrLK(t->header1, t->header2, 
                               t->pyramid1, t->pyramid2, 
                               (CvPoint2D32f *)feats, (CvPoint2D32f *)trackedfeats, f->s.features.size(),
                               t->optical_flow_window, t->levels, 
                               found_features, 
                               errors, t->optical_flow_termination_criteria, 
                               (t->pyramidgood?CV_LKFLOW_PYR_A_READY:0) | CV_LKFLOW_INITIAL_GUESSES );
        t->pyramidgood = 1;

        int area = (t->spacing * 2 + 3);
        area = area * area;

        int i = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            if(found_features[i] &&
               trackedfeats[i].x > 0.0 &&
               trackedfeats[i].y > 0.0 &&
               trackedfeats[i].x < t->width-1 &&
               trackedfeats[i].y < t->height-1 &&
               errors[i] / area < t->max_tracking_error) {
                (*fiter)->current[0] = (*fiter)->uncalibrated[0] = trackedfeats[i].x;
                (*fiter)->current[1] = (*fiter)->uncalibrated[2] = trackedfeats[i].y;
            } else {
                trackedfeats[i].x = trackedfeats[i].y = INFINITY;
                (*fiter)->current[0] = (*fiter)->uncalibrated[0] = trackedfeats[i].x;
                (*fiter)->current[1] = (*fiter)->uncalibrated[2] = trackedfeats[i].y;
            }
            ++i;            
        }
    }
}
*/

 /*
void run_local_tracking(struct filter *f, feature_t *trackedfeats)
{
    struct tracker *t = f->track;
    int newindex = 0;
    if(f->s.features.size()) {
        int b = 20;
        int xsize = t->width;
        int ysize = t->height;
        int stride = t->width;
        fast_detector detector(xsize, ysize, stride);
        int index = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            xy bestkp = detector.track(t->im1, t->im2, (*fiter)->current[0], (*fiter)->current[1], 15, 15, 20);

            if(bestkp.x < 0.0 ||
               bestkp.y < 0.0 ||
               bestkp.x >= t->width ||
               bestkp.y >= t->height) {
                bestkp.x = bestkp.y = INFINITY;
            }

            (*fiter)->current[0] = (*fiter)->uncalibrated[0] = trackedfeats[index].x = bestkp.x;
            (*fiter)->current[1] = (*fiter)->uncalibrated[2] = trackedfeats[index].y = bestkp.y;
            ++index;
        }
    }
}
 */

void send_current_features_packet(struct filter *f, uint64_t time)
{
    packet_t *packet = mapbuffer_alloc(f->track.sink, packet_feature_track, f->s.features.size() * sizeof(feature_t));
    feature_t *trackedfeats = (feature_t *)packet->data;
    int nfeats = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        trackedfeats[nfeats].x = (*fiter)->current[0];
        trackedfeats[nfeats].y = (*fiter)->current[1];
        ++nfeats;
    }
    packet->header.user = f->s.features.size();
    mapbuffer_enqueue(f->track.sink, packet, time);
}

extern "C" void sfm_control(void *_f, packet_t *p)
{
    if(p->header.type != packet_filter_control) return;
    struct filter *f = (struct filter *)_f;
    if(p->header.user == 2) {
        //full reset
        fprintf(stderr, "full filter reset\n");
        filter_reset_full(f);
    }
    if(p->header.user == 1) {
        //start measuring
        fprintf(stderr, "measurement starting\n");
        f->measurement_running = true;
        filter_reset_position(f);
        f->s.initial_orientation = f->s.W.v;
    }
    if(p->header.user == 0) {
        //stop measuring
        fprintf(stderr, "measurement stopping\n");
        f->measurement_running = false;
    }
}

extern "C" void sfm_image_measurement(void *_f, packet_t *p)
{
    if(p->header.type != packet_camera) return;
    struct filter *f = (struct filter *)_f;
    if(!check_packet_time(f, p->header.time)) return;
    f->got_image = true;
    if(!f->got_accelerometer || !f->got_gyroscope) return;

    uint64_t time = p->header.time;
    if(!f->track.width) {
        int width, height;
        sscanf((char *)p->data, "P5 %d %d", &width, &height);
        f->track.width = width;
        f->track.height = height;
    }
    f->track.im1 = f->track.im2;
    f->track.im2 = p->data + 16;
    filter_tick(f, time);
    sfm_setup_next_frame(f, time);

    if(f->active) process_observation_queue(f);

    int feats_used = sfm_process_features(f, time);

    if(f->active) {
        add_new_groups(f, time);
        filter_send_output(f, time);
    }

    send_current_features_packet(f, time);
    int space = f->track.maxfeats - f->s.features.size();
    if(space >= f->track.groupsize) {
        if(space > f->track.maxgroupsize) space = f->track.maxgroupsize;
        addfeatures(f, &f->track, space, p->data + 16, f->track.width);
        if(f->s.features.size() < 16 && !f->measurement_running) {
            fprintf(stderr, "detector failure: only %d features after add\n", f->s.features.size());
            f->detector_failed = true;
        }
    }

    if(f->active) {
        int normal = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            if((*fiter)->status == feature_normal) ++normal;
        }
        if(normal == 0 && !f->measurement_running) { //only throw error if the measurement hasn't started yet
            fprintf(stderr, "Tracker failure: 0 normal features\n");
            f->tracker_failed = true;
        } else if(normal < f->min_feats_per_group && f->measurement_running) {
            fprintf(stderr, "Tracker warning: only %d normal features\n", normal);
            f->tracker_warned = true;
        }
    }
}

extern "C" void sfm_features_added(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(p->header.type == packet_feature_select) {
        feature_t *initial = (feature_t*) p->data;
        struct camera_calibration calibration = get_camera_calibration(f);
        for(int i = 0; i < p->header.user; ++i) {
            feature_t calib;
            calibration_normalize(&calibration, &initial[i], &calib, 1);
            state_vision_feature *feat = f->s.add_feature(calib.x, calib.y);
            assert(initial[i].x != INFINITY);
            feat->status = feature_initializing;
            feat->current[0] = feat->uncalibrated[0] = initial[i].x;
            feat->current[1] = feat->uncalibrated[1] = initial[i].y;
        }
        f->s.remap();
    }
    if(p->header.type == packet_feature_intensity) {
        uint8_t *intensity = (uint8_t *)p->data;
        list<state_vision_feature *>::iterator fiter = f->s.features.end();
        --fiter;
        for(int i = p->header.user; i > 0; --i) {
            (*fiter)->intensity = intensity[i];
        }
        /*  
        int feature_base = f->s.features.size() - p->header.user;
        //        list<state_vision_feature *>::iterator fiter = f->s.featuresf->s.features.end()-p->header.user;
        for(int i = 0; i < p->header.user; ++i) {
            f->s.features[feature_base + i]->intensity = intensity[i];
            }*/
    }
}

#define BEGIN_FOCAL_VAR 10.
#define END_FOCAL_VAR .5
#define BEGIN_C_VAR 4.
#define END_C_VAR .25
#define BEGIN_ABIAS_VAR 1.e-4
#define END_ABIAS_VAR 2.e-5
#define BEGIN_WBIAS_VAR 1.e-4
#define END_WBIAS_VAR 2.e-5
#define BEGIN_K1_VAR .1
#define END_K1_VAR .005
#define BEGIN_K2_VAR .1
#define END_K2_VAR .01
#define BEGIN_K3_VAR .1
#define END_K3_VAR .08

void filter_config(struct filter *f)
{
    f->track.groupsize = 24;
    f->track.maxgroupsize = 40;
    f->track.maxfeats = 90;

    f->s.T.variance = 1.e-7;
    f->s.W.variance = v4(10., 10., 1.e-7, 0.);
    f->s.V.variance = 1. * 1.;
    f->s.w.variance = .5 * .5;
    f->s.dw.variance = 5. * 5.; //observed range of variances in sequences is 1-6
    f->s.a.variance = .1 * .1;
    f->s.da.variance = 50. * 50.; //observed range of variances in sequences is 10-50
    f->s.g.variance = 1.e-7;
    f->s.Wc.variance = v4(f->device.Wc_var[0], f->device.Wc_var[1], f->device.Wc_var[2], 0.);
    f->s.Tc.variance = v4(f->device.Tc_var[0], f->device.Tc_var[1], f->device.Tc_var[2], 0.);
    f->s.a_bias.v = v4(f->device.a_bias[0], f->device.a_bias[1], f->device.a_bias[2], 0.);
    f->s.a_bias.variance = v4(f->device.a_bias_var[0], f->device.a_bias_var[1], f->device.a_bias_var[2], 0.);
    f->s.w_bias.v = v4(f->device.w_bias[0], f->device.w_bias[1], f->device.w_bias[2], 0.);
    f->s.w_bias.variance = v4(f->device.w_bias_var[0], f->device.w_bias_var[1], f->device.w_bias_var[2], 0.);
    f->s.focal_length.variance = BEGIN_FOCAL_VAR;
    f->s.center_x.variance = BEGIN_C_VAR;
    f->s.center_y.variance = BEGIN_C_VAR;
    f->s.k1.variance = BEGIN_K1_VAR;
    f->s.k2.variance = BEGIN_K2_VAR;
    f->s.k3.variance = BEGIN_K3_VAR;

    f->init_vis_cov = 4.;
    f->max_add_vis_cov = 2.;
    f->min_add_vis_cov = .5;

    f->s.T.process_noise = 0.;
    f->s.W.process_noise = 0.;
    f->s.V.process_noise = 0.;
    f->s.w.process_noise = 0.;
    f->s.dw.process_noise = 40. * 40.; // this stabilizes dw.stdev around 5-6
    f->s.a.process_noise = 0.;
    f->s.da.process_noise = 400. * 400.; //this stabilizes da.stdev around 45-50
    f->s.g.process_noise = 1.e-7;
    f->s.Wc.process_noise = 1.e-7;
    f->s.Tc.process_noise = 1.e-7;
    f->s.a_bias.process_noise = 1.e-7;
    f->s.w_bias.process_noise = 1.e-7;
    f->s.focal_length.process_noise = 1.e-7;
    f->s.center_x.process_noise = 1.e-7;
    f->s.center_y.process_noise = 1.e-7;
    f->s.k1.process_noise = 1.e-7;
    f->s.k2.process_noise = 1.e-7;
    f->s.k3.process_noise = 1.e-7;

    f->vis_ref_noise = 1.e-7;
    f->vis_noise = 1.e-7;

    f->vis_cov = 2. * 2.;
    f->w_variance = f->device.w_meas_var;
    f->a_variance = f->device.a_meas_var;

    f->min_feats_per_group = 6;
    f->min_group_add = 16;
    f->max_group_add = 40;
    f->max_features = 80;
    f->active = false;
    f->max_state_size = 128;
    f->frame = 0;
    f->skip = 1;
    f->min_group_health = 10.;
    f->max_feature_std_percent = .10;
    f->outlier_thresh = 1.5;
    f->outlier_reject = 10.;

    f->s.focal_length.v = f->device.Fx;
    f->s.center_x.v = f->device.Cx;
    f->s.center_y.v = f->device.Cy;
    f->s.k1.v = f->device.K[0];
    f->s.k2.v = f->device.K[1];
    f->s.k3.v = f->device.K[2];

    f->s.Tc.v = v4(f->device.Tc[0], f->device.Tc[1], f->device.Tc[2], 0.);
    f->s.Wc.v = v4(f->device.Wc[0], f->device.Wc[1], f->device.Wc[2], 0.);

    f->shutter_delay = f->device.shutter_delay;
    f->shutter_period = f->device.shutter_period;
    f->image_height = f->device.image_height;
}

extern "C" void filter_init(struct filter *f, struct corvis_device_parameters _device)
{
    //TODO: check init_cov stuff!!
    f->device = _device;
    filter_config(f);
    f->need_reference = true;
    state_node::statesize = 0;
    state_node::maxstatesize = f->max_state_size;
    f->s.remap();
    state_vision_feature::initial_rho = 1.;
    state_vision_feature::initial_var = f->init_vis_cov;
    state_vision_feature::initial_process_noise = f->vis_noise;
    state_vision_feature::measurement_var = f->vis_cov;
    state_vision_feature::outlier_thresh = f->outlier_thresh;
    state_vision_feature::outlier_reject = f->outlier_reject;
    state_vision_feature::max_variance = f->max_feature_std_percent * f->max_feature_std_percent;
    state_vision_feature::min_add_vis_cov = f->min_add_vis_cov;
    state_vision_group::ref_noise = f->vis_ref_noise;
    state_vision_group::min_feats = f->min_feats_per_group;
    state_vision_group::min_health = f->min_group_health;
}

float var_bounds_to_std_percent(f_t current, f_t begin, f_t end)
{
    return (sqrt(begin) - sqrt(current)) / (sqrt(begin) - sqrt(end));
}

float filter_converged(struct filter *f)
{
    float min, pct;
    min = var_bounds_to_std_percent(f->s.focal_length.variance, BEGIN_FOCAL_VAR, END_FOCAL_VAR);
    pct = var_bounds_to_std_percent(f->s.center_x.variance, BEGIN_C_VAR, END_C_VAR);
    if(pct < min) min = pct;
    pct = var_bounds_to_std_percent(f->s.center_y.variance, BEGIN_C_VAR, END_C_VAR);
    if(pct < min) min = pct;
    pct = var_bounds_to_std_percent(f->s.a_bias.variance.absmax(), BEGIN_ABIAS_VAR, END_ABIAS_VAR);
    if(pct < min) min = pct;
    pct = var_bounds_to_std_percent(f->s.w_bias.variance.absmax(), BEGIN_WBIAS_VAR, END_WBIAS_VAR);
    if(pct < min) min = pct;
    pct = var_bounds_to_std_percent(f->s.k1.variance, BEGIN_K1_VAR, END_K1_VAR);
    if(pct < min) min = pct;
    pct = var_bounds_to_std_percent(f->s.k2.variance, BEGIN_K2_VAR, END_K2_VAR);
    if(pct < min) min = pct;
    pct = var_bounds_to_std_percent(f->s.k3.variance, BEGIN_K3_VAR, END_K3_VAR);
    if(pct < min) min = pct;
    return min < 0. ? 0. : min;
}

bool filter_is_steady(struct filter *f)
{
    return
        norm(f->s.V.v) < .1 &&
        norm(f->s.w.v) < .1;
}

bool filter_is_aligned(struct filter *f)
{
    return 
        fabs(f->s.projected_orientation_marker.x) < .03 &&
        fabs(f->s.projected_orientation_marker.y) < .03;
}
