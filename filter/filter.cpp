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
}
#include "model.h"
#include "../numerics/vec4.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "../numerics/kalman.h"
#include "../numerics/matrix.h"
#include "observation.h"
#include "filter.h"
#include "tracker.h"
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/video/tracking.hpp>
#include <opencv2/features2d/features2d.hpp>
int state_node::statesize;
int state_node::maxstatesize;

//TODO: homogeneous coordinates.
//TODO: reduced size for ltu

void integrate_motion_state(state_motion_gravity *state, state_motion_gravity *slope, f_t dt, matrix *ltu)
{
    m4v4 dR_dW, drdt_dwdt;
    v4m4 dWp_dRp;
    
    m4 
        R = rodrigues(state->W, (ltu?&dR_dW:NULL)),
        rdt = rodrigues(slope->w * dt, (ltu?&drdt_dwdt:NULL));

    m4 Rp = R * rdt;
    v4
        Wp = invrodrigues(Rp, (ltu?&dWp_dRp:NULL)),
        Tp = state->T + slope->V * dt,
        Vp = state->V + slope->a * dt,
        wp = state->w + slope->dw * dt,
        ap = state->a + slope->da * dt;

    state->W = Wp;
    state->T = Tp;
    state->V = Vp;
    state->w = wp;
    state->a = ap;

    if(ltu) {
        m4v4 
            dRp_dW = dR_dW * rdt,
            dRp_dw = R * (drdt_dwdt * dt);
        m4
            dWp_dW = dWp_dRp * dRp_dW,
            dWp_dw = dWp_dRp * dRp_dw;

        for(int i = 0; i < 3; ++i) {
            (*ltu)(state->T.index + i, state->V.index + i) = dt;
            (*ltu)(state->V.index + i, state->a.index + i) = dt;
            (*ltu)(state->w.index + i, state->dw.index + i) = dt;
            (*ltu)(state->a.index + i, state->da.index + i) = dt;
            for(int j = 0; j < 3; ++j) {
                (*ltu)(state->W.index + i, state->W.index + j) = dWp_dW[i][j];
                (*ltu)(state->W.index + i, state->w.index + j) = dWp_dw[i][j];
            }
        }
    }
}

void update_additive_cov(matrix &cov, int dest, int src, f_t dt)
{
    cov(dest, dest) += 2 * dt * cov(dest, src) + dt * dt * cov(src, src);
    for(int i = 0; i < cov.rows; ++i) {
        if(i == dest) continue;
        cov(dest, i) += dt * cov(src, i);
        cov(i, dest) = cov(dest, i);
    }
}

void update_additive_triple_cov(matrix &cov, int dest, int src, f_t dt)
{
    for(int i = 0; i < 3; ++i) {
        update_additive_cov(cov, dest + i, src + i, dt);
    }
}

void motion_time_update(state *orig_state, f_t dt, matrix *ltu, int statesize)
{
    MAT_TEMP(saved_state, 1, statesize);
    MAT_TEMP(state_base, 1, statesize);
    MAT_TEMP(k1, 1, statesize);
    MAT_TEMP(k2, 1, statesize);
    MAT_TEMP(k3, 1, statesize);
    MAT_TEMP(k4, 1, statesize);

    orig_state->copy_state_to_array(saved_state);
    orig_state->copy_state_to_array(state_base);

    integrate_motion_state(orig_state, orig_state, dt, 0);
    orig_state->copy_state_to_array(k1);
    for(int i = 0; i < statesize; ++i) {
        k1[i] -= state_base[i];
        state_base[i] = saved_state[i] + k1[i] * .5;
    }
    
    orig_state->copy_state_from_array(state_base);
    integrate_motion_state(orig_state, orig_state, dt, 0);
    orig_state->copy_state_to_array(k2);
    for(int i = 0; i < statesize; ++i) {
        k2[i] -= state_base[i];
        state_base[i] = saved_state[i] + k2[i] * .5;
    }

    orig_state->copy_state_from_array(state_base);
    integrate_motion_state(orig_state, orig_state, dt, 0);
    orig_state->copy_state_to_array(k3);
    for(int i = 0; i < statesize; ++i) {
        k3[i] -= state_base[i];
        state_base[i] = saved_state[i] + k3[i];
    }

    orig_state->copy_state_from_array(state_base);
    integrate_motion_state(orig_state, orig_state, dt, 0);
    orig_state->copy_state_to_array(k4);
    for(int i = 0; i < statesize; ++i) {
        k4[i] -= state_base[i];
        state_base[i] = saved_state[i] + (k1[i] + 2 * (k2[i] + k3[i]) + k4[i]) / 6.;
    }

    orig_state->copy_state_from_array(state_base);
}

void integrate_motion_cov(struct filter *f, f_t dt)
{
    int statesize = f->s.cov.rows;
    
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

    assert(f->s.w.index == f->s.W.index + 3);
    MAT_TEMP(lu, 6, 6);
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            lu(i, j) = dWp_dW[i][j];
            lu(i, j + 3) = dWp_dw[i][j];
            lu(i + 3, j) = 0;
            lu(i + 3, j + 3) = (i==j)?1:0;
        }
    }

    update_additive_triple_cov(f->s.cov, f->s.T.index, f->s.V.index, dt);
    update_additive_triple_cov(f->s.cov, f->s.V.index, f->s.a.index, dt);
    update_additive_triple_cov(f->s.cov, f->s.a.index, f->s.da.index, dt);

    block_update(f->s.cov, lu, f->s.W.index);

    update_additive_triple_cov(f->s.cov, f->s.w.index, f->s.dw.index, dt);
    //cov += diag(R)*dt
    for(int i = 0; i < statesize; ++i) {
        f->s.cov(i, i) += f->s.p_cov[i] * dt;
    }
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
    integrate_motion_cov(f, dt);
    integrate_motion_state(&f->s, &f->s, dt, NULL);
    //motion_time_update(&f->s, dt, NULL, statesize);
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

    motion_time_update(&f->s, dt, &ltu, statesize);
    f->s.copy_state_to_array(save_new_state);

    f_t eps = .1;

    for(int i = 0; i < statesize; ++i) {
        memcpy(state_data, save_state_data, sizeof(state_data));
        f_t leps = state[i] * eps + 1.e-7;
        state[i] += leps;
        f->s.copy_state_from_array(state);
        motion_time_update(&f->s, dt, NULL, statesize);
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
    motion_time_update(&f->s, dt, &ltu, statesize);
    time_update(f->s.cov, ltu, f->s.p_cov, dt);
}

void transform_new_group(state *state, f_t dt, matrix *ltu, int statesize)
{
    for(list<state_vision_group *>::iterator giter = state->groups.children.begin(); giter != state->groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        if(g->status != group_initializing) continue;
        m4 
            R = rodrigues(g->Wr, NULL),
            Rt = transpose(R),
            Rbc = rodrigues(state->Wc, NULL),
            Rcb = transpose(Rbc),
            RcbRt = Rcb * Rt;
        for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            m4 Rr = rodrigues(i->Wr, NULL);

            m4 
                Rw = Rr * Rbc,
                Rtot = RcbRt * Rw;
            v4
                Tw = Rr * state->Tc + i->Tr,
                Ttot = Rcb * (Rt * (Tw - g->Tr) - state->Tc);

            f_t rho = exp(*i);
            v4
                X0 = i->initial * rho, /*not homog in v4*/
                /*                Xr = Rbc * X0 + state->Tc,
                Xw = Rw * X0 + Tw,
                Xl = Rt * (Xw - g->Tr),*/
                X = Rtot * X0 + Ttot;
            
            f_t invZ = 1./X[2];
            v4 prediction = X * invZ;
            assert(fabs(prediction[2]-1.) < 1.e-7 && prediction[3] == 0.);
            i->v = X[2] > .01 ? log(X[2]) : log(.01);
        }
    }
}

void ukf_time_update(struct filter *f, uint64_t time, void (* do_time_update)(state *state, f_t dt, matrix *ltu, int statesize) = motion_time_update)
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

    matrix_cholesky(f->s.cov);

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
        do_time_update(&f->s, dt, NULL, statesize);
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

    matrix_cholesky(f->s.cov);

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
    matrix_invert(Pyy_inverse);

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
    m4 
        R = rodrigues(f->s.W, NULL),
        Rt = transpose(R),
        Rbc = rodrigues(f->s.Wc, NULL),
        Rcb = transpose(Rbc),
        RcbRt = Rcb * Rt;

    f->s.camera_orientation = invrodrigues(RcbRt, NULL);
    //transform gravity into the local frame
    v4 local_gravity = RcbRt * v4(0., 0., f->s.g, 0.);
    //roll (in the image plane) is x/-y
    //TODO: verify sign
    f->s.orientation = atan2(local_gravity[0], -local_gravity[1]);

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
    //static f_t fd_bin[24];
    //static int fd_bin_count[24];
    if(!f->observations.observations.size()) return;
    int statesize = f->s.cov.rows;
    //TODO: break apart sort and preprocess
    f->observations.lp.resize(f->observations.lp.rows, statesize);
    f->observations.preprocess(true, statesize);
    MAT_TEMP(state, 1, statesize);

    vector<observation *>::iterator obs = f->observations.observations.begin();

    MAT_TEMP(lp, f->observations.meas_size, statesize);
    MAT_TEMP(inn, 1, f->observations.meas_size);
    MAT_TEMP(m_cov, 1, f->observations.meas_size);

    bool ranvis = false;
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
            if((*obs)->size == 0) break;
            if((*obs)->time_apparent != obs_time) break;
        }
        vector<observation *>::iterator end = obs;
        lp.resize(meas_size, statesize);
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
                integrate_motion_state(&f->s, &f->s, dt, NULL);
            }
            (*obs)->lp.clear();
            (*obs)->predict(true);
            if((*obs)->time_apparent != obs_time) {
                f->s.copy_state_from_array(state);
                integrate_motion_pred(f, (*obs)->lp, dt);
            }
            if((*obs)->size == 2) is_vis = true;
            if((*obs)->size == 0) is_init = true;
        }
        //vis measurement
        if(is_vis || (!ranvis && is_init)) {
            feature_t trackedfeats[f->s.features.size()];
            int nfeats = 0;
            feature_t ol[f->s.features.size()], nl[f->s.features.size()], ml[f->s.features.size()];
            f_t fd_sum = 0, od_sum = 0;
            int outside = 0;
            int count = 0;
       
            for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
                trackedfeats[nfeats].x = (*fiter)->prediction.x;
                trackedfeats[nfeats].y = (*fiter)->prediction.y;
                ol[nfeats].x = (*fiter)->current[0];
                ol[nfeats].y = (*fiter)->current[1];
                nl[nfeats].x = (*fiter)->prediction.x;
                nl[nfeats].y = (*fiter)->prediction.y;
                ++nfeats;
            }
            run_tracking(f, trackedfeats);
            nfeats = 0;
            for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
                ml[nfeats].x = (*fiter)->current[0];
                ml[nfeats].y = (*fiter)->current[1];
                if(ml[nfeats].x != INFINITY && (*fiter)->status == feature_normal) {
                    f_t fd = feature_distance(nl[nfeats], ml[nfeats]);
                    f_t od = feature_distance(ol[nfeats], ml[nfeats]);
                    if(fd > 10.) ++outside;
                    fd_sum +=fd;
                    od_sum +=od;
                    //fd_bin[(int)floor(ml[nfeats].y/20)] += fd;
                    //fd_bin_count[(int)floor(ml[nfeats].y/20)]++;
                    ++count;
                }
                ++nfeats;
            }
            fprintf(stderr, "avg feature distance is %f, from orig is %f, outside is %d\n", fd_sum/count, od_sum/count, outside);
            /*fprintf(stderr,"row binning:\n");
            for(int i = 0; i < 24; ++i) {
                fprintf(stderr,"%f\n", fd_bin[i] / fd_bin_count[i]);
                }*/
            ranvis = true;
        }
        

        //measure; calculate innovation and covariance
        for(obs = start; obs != end; ++obs) {
            bool valid = (*obs)->measure();
            if(valid) {
                for(int i = 0; i < (*obs)->size; ++i) {
                    (*obs)->inn[i] = (*obs)->meas[i] - (*obs)->pred[i];
                }
                (*obs)->compute_covariance();
                for(int i = 0; i < (*obs)->size; ++i) {
                    inn[count + i] = (*obs)->inn[i];
                    m_cov[count + i] = (*obs)->m_cov[i];
                    for(int j = 0; j < statesize; ++j) {
                        lp(count + i, j) = (*obs)->lp(i,j);
                    }
                }
                count += (*obs)->size;
            }
        }
        lp.resize(count, lp.cols);
        inn.resize(1, count);
        m_cov.resize(1, count);
        if(count) meas_update(state, f->s.cov, inn, lp, m_cov);
        //meas_update(state, f->s.cov, f->observations.inn, f->observations.lp, f->observations.m_cov);
        f->s.copy_state_from_array(state);
    }
    f->observations.clear();
    f->s.total_distance += norm(f->s.T - f->s.last_position);
    f->s.last_position = f->s.T;
    if(f->measurement_callback) f->measurement_callback(f->measurement_callback_object, f->s.T.v[0], sqrt(f->s.T.variance[0]), f->s.T.v[1], sqrt(f->s.T.variance[1]), f->s.T.v[2], sqrt(f->s.T.variance[2]), f->s.total_distance, 0.);
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

void do_gravity_init(struct filter *f, float *data, uint64_t time)
{
    //first measurement - use as g
    v4 local_down(data[0], data[1], data[2], 0.);
    f->s.g = 9.8;
    f->s.W = relative_rotation(local_down, v4(0., 0., 1., 0.));
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
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        state_vision_feature *i = *fiter;
        feature_t uncalibrated = { (float)i->current[0], (float)i->current[1] };
        feature_t calib;
        calibration_normalize(f->calibration, &uncalibrated, &calib, 1);
        i->initial[0] = calib.x;
        i->initial[1] = calib.y;
        //i->initial = i->current;
        i->Wr = f->s.W;
    }    
}

extern "C" void sfm_imu_measurement(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
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
    if(p->header.type != packet_accelerometer) return;
    float *data = (float *)&p->data;

    if(!f->gravity_init) do_gravity_init(f, data, p->header.time);

    observation_accelerometer *obs_a = f->observations.new_observation_accelerometer(&f->s, p->header.time, p->header.time);
    for(int i = 0; i < 3; ++i) {
        obs_a->meas[i] = data[i];
    }
    obs_a->variance = f->a_variance;
    obs_a->initializing = !f->active;
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
    if(p->header.type != packet_gyroscope) return;
    float *data = (float *)&p->data;

    observation_gyroscope *obs_w = f->observations.new_observation_gyroscope(&f->s, p->header.time, p->header.time);
    for(int i = 0; i < 3; ++i) {
        obs_w->meas[i] = data[i];
    }
    obs_w->variance = f->w_variance;
    obs_w->initializing = !f->active;
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
    feature_t *uncalibrated = (feature_t*) f->last_raw_track_packet->data;
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
        if(f->frame > f->skip && f->gravity_init) {
            f->active = true;
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
    preobservation_vision_base *base = f->observations.new_preobservation<preobservation_vision_base>(&f->s);
    base->cal = f->calibration;
    base->track = f->track;
    if(feats_used) {
        int statesize = f->s.cov.rows;
        int meas_used = feats_used * 2;

        int fi = 0;
        for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
            state_vision_group *g = *giter;
            if(!g->status || g->status == group_initializing) continue;
            preobservation_vision_group *group = f->observations.new_preobservation<preobservation_vision_group>(&f->s);
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
            ukf_time_update(f, f->last_time, transform_new_group);
            g->status = group_normal;
            for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                state_vision_feature *i = *fiter;
                feature_t uncalibrated = { (float)i->current[0], (float)i->current[1] };
                feature_t calib;
                calibration_normalize(f->calibration, &uncalibrated, &calib, 1);
                i->initial[0] = calib.x;
                i->initial[1] = calib.y;
                //i->initial = i->current;
                i->Tr = g->Tr;
                i->Wr = g->Wr;
            }

            //********* NOW: DO THIS FOR OTHER PACKETS
            if(f->recognition_buffer) {
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

void tracker_finish_frame(struct tracker *t, packet_t *p)
{
    CvMat *tmp = t->pyramid1;
    t->pyramid1 = t->pyramid2;
    t->pyramid2 = tmp;
    t->oldframe = (packet_camera_t *)p;
    cvSetData(t->header1, p->data + 16, t->width);
    ++t->framecount;
}

static bool keypoint_score_comp(cv::KeyPoint kp1, cv::KeyPoint kp2)
{
    return kp1.response > kp2.response;
}

static void addfeatures(struct filter *f, struct tracker *t, int newfeats, unsigned char *img, unsigned int width)
{
    //don't select near old features
    //turn on everything away from the border
    cvRectangle(t->mask, cvPoint(t->spacing, t->spacing), cvPoint(t->width-t->spacing, t->height-t->spacing), cvScalarAll(255), CV_FILLED, 4, 0);
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        cvCircle(t->mask, cvPoint((*fiter)->current[0],(*fiter)->current[1]), t->spacing, CV_RGB(0,0,0), -1, 8, 0);
    }

    feature_t newfeatures[newfeats];

    //cvGoodFeaturesToTrack(t->header1, t->eig_image, t->temp_image, (CvPoint2D32f *)newfeatures, &newfeats, t->thresh, t->spacing, t->mask, t->blocksize, t->harris, t->harrisk);
    vector<cv::KeyPoint> keypoints;
    //    cv::FASTX(cv::Mat(t->header1), keypoints, 20, true, cv::FastFeatureDetector::TYPE_9_16);
    cv::FastFeatureDetector detect(10);
    detect.detect(cv::Mat(t->header1), keypoints, t->mask);
    std::sort(keypoints.begin(), keypoints.end(), keypoint_score_comp);
    if(keypoints.size() < newfeats) newfeats = keypoints.size();
    for(int i = 0; i < newfeats; ++i) {
        newfeatures[i].x = keypoints[i].pt.x;
        newfeatures[i].y = keypoints[i].pt.y;
    }
    cvFindCornerSubPix(t->header1, (CvPoint2D32f *)newfeatures, newfeats, t->optical_flow_window, cvSize(-1,-1), t->optical_flow_termination_criteria);

    int goodfeats = 0;
    for(int i = 0; i < newfeats; ++i) {
        if(newfeatures[i].x > 0.0 &&
           newfeatures[i].y > 0.0 &&
           newfeatures[i].x < t->width-1 &&
           newfeatures[i].y < t->height-1) {
            feature_t calib;
            calibration_normalize(f->calibration, &newfeatures[i], &calib, 1);
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

f_t calc_track_error(struct tracker *t, f_t ox, f_t oy, f_t nx, f_t ny, f_t max_error)
{
    int x1 = (int)ox, y1 = (int)oy, x2 = (int)nx, y2 = (int)ny;
    int window = 3;
    int area = 7 * 7;
    int total_error = max_error * area;
    
    if(x1 < window || y1 < window || x2 < window || y2 < window || x1 >= t->width - window || x2 >= t->width - window || y1 >= t->height - window || y2 >= t->height - window) return max_error + 1.;
    int error = 0;
    for(int dx = -window; dx <= window; ++dx) {
        for(int dy = -window; dy <= window; ++dy) {
            int p1 = ((uchar*)(t->header1->imageData + t->header1->widthStep*(y1+dy)))[(x1+dx)];
            int p2 = ((uchar*)(t->header2->imageData + t->header2->widthStep*(y2+dy)))[(x2+dx)];
            error += abs(p1-p2);
            if(error >= total_error) return max_error + 1;
        }
    }
    return (f_t)error/(f_t)area;
}

f_t kpdist(cv::KeyPoint &kp, state_vision_feature &feat)
{
    f_t dx = kp.pt.x - feat.prediction.x, dy = kp.pt.y - feat.prediction.y;
    return sqrt(dx * dx + dy * dy);
}

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

void run_local_tracking(struct filter *f, feature_t *trackedfeats)
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

        //detect all keypoints
        vector<cv::KeyPoint> keypoints;
        cv::FastFeatureDetector detect(10, false);
        detect.detect(cv::Mat(t->header2), keypoints);
        int index = 0;
        //        std::sort(keypoints.begin(), keypoints.end(), keypoint_score_comp);
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            f_t best_error = 30.;
            feature_t bestkp = {INFINITY, INFINITY};
            for(vector<cv::KeyPoint>::iterator kiter = keypoints.begin(); kiter != keypoints.end(); ++kiter) {
                f_t dist = kpdist(*kiter, **fiter);
                if(dist > 15.) continue;
                f_t error = calc_track_error(t, (*fiter)->current[0], (*fiter)->current[1], kiter->pt.x, kiter->pt.y, best_error);
                if(error < best_error) {
                    best_error=error;
                    bestkp.x = kiter->pt.x;
                    bestkp.y = kiter->pt.y;
                }
            }
            if(bestkp.x != INFINITY) {
                cvFindCornerSubPix(t->header2, (CvPoint2D32f *)&bestkp, 1, t->optical_flow_window, cvSize(-1,-1), t->optical_flow_termination_criteria);
                found_features[index] = true;
                fprintf(stderr, "feature at %f %f tracked to %f %f, error %f\n", i->current[0], i->current[1], bestkp.x, bestkp.y, best_error );
            } else {
                found_features[index] = false;
                fprintf(stderr, "feature at %f %f not found\n", i->current[0], i->current[1]);
            }
            trackedfeats[index] = bestkp;
            ++index;
        }
        int i = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            if(found_features[i] &&
               trackedfeats[i].x > 0.0 &&
               trackedfeats[i].y > 0.0 &&
               trackedfeats[i].x < t->width-1 &&
               trackedfeats[i].y < t->height-1) {
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


void send_current_features_packet(struct filter *f, uint64_t time)
{
    packet_t *packet = mapbuffer_alloc(f->track->sink, packet_feature_track, f->s.features.size() * sizeof(feature_t));
    feature_t *trackedfeats = (feature_t *)packet->data;
    int nfeats = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        trackedfeats[nfeats].x = (*fiter)->current[0];
        trackedfeats[nfeats].y = (*fiter)->current[1];
        ++nfeats;
    }
    packet->header.user = f->s.features.size();
    mapbuffer_enqueue(f->track->sink, packet, time);
}

extern "C" void sfm_image_measurement(void *_f, packet_t *p)
{
    if(p->header.type != packet_camera) return;
    struct filter *f = (struct filter *)_f;

    uint64_t time = p->header.time;
    tracker_setup_next_frame(f->track, p);
    filter_tick(f, time);
    sfm_setup_next_frame(f, time);

    if(!f->active) {
    feature_t trackedfeats[f->s.features.size()];
    int nfeats = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        trackedfeats[nfeats].x = (*fiter)->current[0];
        trackedfeats[nfeats].y = (*fiter)->current[1];
        ++nfeats;
    }
    run_tracking(f, trackedfeats);
    }

    if(f->active) process_observation_queue(f);
    int feats_used = sfm_process_features(f, time);

    if(f->active) {
        add_new_groups(f, time);
        filter_send_output(f, time);
    }

    send_current_features_packet(f, time);

    tracker_finish_frame(f->track, p);

    int space = f->track->maxfeats - f->s.features.size();
    if(space >= f->track->groupsize) {
        if(space > f->track->maxgroupsize) space = f->track->maxgroupsize;
        addfeatures(f, f->track, space, p->data + 16, f->track->width);
    }
}

extern "C" void sfm_features_added(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(p->header.type == packet_feature_select) {
        feature_t *initial = (feature_t*) p->data;
        feature_t *uncalibrated = (feature_t*) f->last_raw_track_packet->data;
        for(int i = 0; i < p->header.user; ++i) {
            feature_t calib;
            calibration_normalize(f->calibration, &initial[i], &calib, 1);
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

extern "C" void sfm_raw_trackdata(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(p->header.type == packet_feature_select || p->header.type == packet_feature_track) {
        f->last_raw_track_packet = p;
    }
}

extern "C" void filter_init(struct filter *f)
{
    //TODO: check init_cov stuff!!
    f->need_reference = true;
    state_node::statesize = 0;
    state_node::maxstatesize = 128;
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
