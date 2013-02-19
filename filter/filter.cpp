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

/*void integrate_rk4_addition(state *s, f_t dt, int statesize)
{
    MAT_TEMP(save_state, 1, statesize);
    MAT_TEMP(save_new_state, 1, statesize);
    MAT_TEMP(state, 1, statesize);
    f->s.copy_state_to_array(save_state);

    motion_time_update(&f->s, dt, &ltu);
    f->s.copy_state_to_array(save_new_state);
    }*/

m4 compute_essential_model(state_vision *state, state_vision_group *group)
{
    m4 
        R = rodrigues(state->W, NULL),
        Rt = transpose(R),
        Rr = rodrigues(group->Wr, NULL),
        Rbc = rodrigues(state->Wc, NULL),
        Rcb = transpose(Rbc),
        Rtot = Rcb * Rt * Rr * Rbc;

    v4 Ttot = Rcb * (Rt * (Rr * state->Tc + group->Tr - state->T) - state->Tc); 
    m4 That = skew3(Ttot);
    return That * Rtot;
}

m4 compute_essential_data(state_vision *state, state_vision_group *group)
{
    assert(0 && "todo:check this");
    MAT_TEMP(chi, group->features.children.size(), 9);
    int ingroup = 0;
    for (list<state_vision_feature *>::iterator f = group->features.children.begin(); f != group->features.children.end(); ++f) {
        m4 outer = outer_product((*f)->initial, (*f)->current);
        for(int j = 0; j < 3; ++j)
            for(int k = 0; k < 3; ++k)
                chi(ingroup, j*3+k) = outer[j][k];
        ++ingroup;
    }
    chi.rows = ingroup;
    MAT_TEMP(chi_U, ingroup, ingroup);
    MAT_TEMP(chi_S, 1, ingroup);
    MAT_TEMP(chi_Vt, 9, 9);
    matrix_svd(chi, chi_U, chi_S, chi_Vt);
    matrix A(&chi_Vt(8, 0), 3, 3);
    m4 U, Vt;
    matrix Um((f_t *)&U.data[0].data, 3, 3, 4, 4);
    matrix Vtm((f_t *)&Vt.data[0].data, 3, 3, 4, 4);
    v4 S;
    matrix Sm((f_t *)&S.data, 3);
    matrix_svd(A, Um, Sm, Vtm);
    m4 Sn;
    Sn[0][0] = S[0];
    Sn[1][1] = S[1];
    return U * Sn * Vt;  
}

static void triangulate_feature(state *state, state_vision_feature *i)
{
    m4 
        R = rodrigues(state->W, NULL),
        Rt = transpose(R),
        Rbc = rodrigues(state->Wc, NULL),
        Rcb = transpose(Rbc),
        RcbRt = Rcb * Rt,
        Rr = rodrigues(i->Wr, NULL),
        Rw = Rr * Rbc,
        Rtot = RcbRt * Rw;
    v4
        Tw = Rr * state->Tc + i->Tr,
        Ttot = Rcb * (Rt * (Tw - state->T) - state->Tc);
 
    Rtot = transpose(Rtot);
    Ttot = -(Rtot * Ttot);
    v4 rot_init = Rtot * i->current;

    v4 term1 = cross(i->initial, rot_init),
        term2 = cross(Ttot, i->initial);

    v4 predict_p, predict_var;

    m4 vis_meas_cov = diag(v4(i->measurement_var));
    m4 rskew = transpose(skew3(rot_init)), //(transposed since rot_init is on rhs)
        tskew = skew3(Ttot);
    m4 term1_var = rskew * vis_meas_cov * transpose(rskew),
        term2_var = tskew * vis_meas_cov * transpose(tskew);
    v4 inv_var;
    for(int j = 0; j < 3; ++j) {
        predict_p[j] = (log(fabs(term2[j])) - log(fabs(term1[j])));
        predict_var[j] = term2_var[j][j] / (term2[j] * term2[j]) + term1_var[j][j] / (term1[j] * term1[j]);
        inv_var[j] = 1/predict_var[j];
    }
    f_t total_var = 1./(inv_var[0] + inv_var[1] + inv_var[2]);
    f_t pred_depth = (predict_p[0] * inv_var[0] + predict_p[1] * inv_var[1] + predict_p[2] * inv_var[2]) * total_var;
    /*actual variance of term1 and term2 should be (product rule) the sum of two terms
      we just use the easy one (which assumes states are all perfect), and project the
      measurement noise onto the epipolar line. basically will tell us when there is
      enough baseline. Therefore, during initialization, we have an issue with bad estimates
      during the transient. Nope! just with nans! */
    //if(1. / total_var > g->min_health) {
    //not sure whether to check if the new estimate is better here
    if(isnormal(total_var) && isnormal(pred_depth) && isnormal(exp(pred_depth)) && (total_var < i->variance)) {
        i->v = pred_depth;
        i->variance = total_var;
        assert(0 && "fail - need to update triangulate before using - max_add_vis_cov");
        if(i->status == feature_initializing)
            if(total_var < 0.5 /*i->max_add_vis_cov*/) i->status = feature_ready;
    }
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

//why doesn't this work?
// - weights seem to bias us?
void ukf_feature_initialize(struct filter *f, state_vision_feature *feat)
{
    f_t alpha, kappa, lambda, beta;
    alpha = 1.e-3;
    kappa = 0.;
    lambda = alpha * alpha * (1. + kappa) - 1.;
    beta = 2.;
    f_t W0m = lambda / (1. + lambda);
    f_t W0c = W0m + 1. - alpha * alpha + beta;
    f_t Wi = 1. / (2. * (1. + lambda));
    f_t gamma = sqrt(1. + lambda);

    W0m = 1/3.;
    W0c = 1./3.;
    Wi = 1./3.;
    gamma = sqrt(1./(3));

    m4 
        R = rodrigues(f->s.W, NULL),
        Rt = transpose(R),
        Rbc = rodrigues(f->s.Wc, NULL),
        Rcb = transpose(Rbc),
        RcbRt = Rcb * Rt;

    m4 Rr = rodrigues(feat->Wr, NULL);

    m4 
        Rw = Rr * Rbc,
        Rtot = RcbRt * Rw;
    v4
        Tw = Rr * f->s.Tc + feat->Tr,
        Ttot = Rcb * (Rt * (Tw - f->s.T) - f->s.Tc);

    f_t stdev = sqrt(feat->variance);
    f_t x[3];
    x[0] = *feat;
    x[1] = *feat + gamma * stdev;
    x[2] = *feat - gamma * stdev;
    MAT_TEMP(y, 3, 2);
    for(int i = 0; i < 3; ++i) {
        f_t rho = exp(x[i]);
        v4
            X0 = feat->initial * rho, /*not homog in v4*/
            X = Rtot * X0 + Ttot;

        f_t invZ = 1./X[2];
        v4 prediction = X * invZ;
        y(i, 0) = prediction[0];
        y(i, 1) = prediction[1];
        assert(fabs(prediction[2]-1.) < 1.e-7 && prediction[3] == 0.);
    }
    f_t meas_mean[2];
    meas_mean[0] = W0m * y(0, 0) + Wi * (y(1, 0) + y(2, 0));
    meas_mean[1] = W0m * y(0, 1) + Wi * (y(1, 1) + y(2, 1));

    f_t inn[2];
    inn[0] = feat->current[0] - meas_mean[0];
    inn[1] = feat->current[1] - meas_mean[1];
    
    f_t m_cov = feat->measurement_var;
    MAT_TEMP(Pyy, 2, 2);
    for(int i = 0; i < 2; ++i) {
        for(int j = i; j < 2; ++j) {
            Pyy(i, j) = W0c * (y(0,i) - meas_mean[i]) * (y(0,j) - meas_mean[j]);
            for(int k = 1; k < 3; ++k) {
                Pyy(i, j) += Wi * (y(k, i) - meas_mean[i]) * (y(k, j) - meas_mean[j]);
            }
            Pyy(j, i) = Pyy(i, j);
        }
        Pyy(i,i) += m_cov;
    }
    MAT_TEMP(Pxy, 1, 2);
    for(int j = 0; j < 2; ++j) {
        Pxy(0, j) = W0c * (x[0] - *feat) * (y(0,j) - meas_mean[j]);
        for(int k = 1; k < 3; ++k) {
            Pxy(0, j) += Wi * (x[k] - *feat) * (y(k, j) - meas_mean[j]);
        }
    }

    MAT_TEMP(gain, 1, 2);
    MAT_TEMP (Pyy_inverse, Pyy.rows, Pyy.cols);
    f_t invdet = 1. / (Pyy(0,0) * Pyy(1,1) - Pyy(0,1) * Pyy(1,0));
    Pyy_inverse(0,0) = invdet * Pyy(1,1);
    Pyy_inverse(1,1) = invdet * Pyy(0,0);
    Pyy_inverse(1,0) = -invdet * Pyy(0,1);
    Pyy_inverse(0,1) = -invdet * Pyy(1,0);

    gain[0] = Pxy[0] * Pyy_inverse(0, 0) + Pxy[1] * Pyy_inverse(1, 0);
    gain[1] = Pxy[0] * Pyy_inverse(0, 1) + Pxy[1] * Pyy_inverse(1, 1);

    feat->v += inn[0] * gain[0] + inn[1] * gain[1];
    f_t PKt[2];
    PKt[0] = gain[0] * Pyy(0, 0) + gain[1] * Pyy(0, 1);
    PKt[1] = gain[0] * Pyy(1, 0) + gain[1] * Pyy(1, 1);
    feat->variance -= gain[0] * PKt[0] + gain[1] * PKt[1];
    if(feat->index != -1) f->s.cov(feat->index, feat->index) = feat->variance;
    if(feat->status == feature_initializing)
        if(feat->variance < f->min_add_vis_cov) feat->status = feature_ready;

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
    f->s.total_distance += norm(f->s.T - f->s.last_position);
    f->s.last_position = f->s.T;
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

//********HERE - this is more or less implemented, but still behaves strangely, and i haven't yet updated the ios callers (accel and gyro)
//try ukf and small integration step
void process_observation_queue(struct filter *f)
{
    if(!f->observations.observations.size()) return;
    int statesize = f->s.cov.rows;
    //TODO: break apart sort and preprocess
    f->observations.lp.resize(f->observations.lp.rows, statesize);
    f->observations.preprocess(true, statesize);
    MAT_TEMP(state, 1, statesize);

    fprintf(stderr, "\nprocessing observations %d\n", f->observations.observations.size());
    vector<observation *>::iterator obs = f->observations.observations.begin();

    MAT_TEMP(lp, f->observations.meas_size, statesize);
    MAT_TEMP(inn, 1, f->observations.meas_size);
    MAT_TEMP(m_cov, 1, f->observations.meas_size);

    while(obs != f->observations.observations.end()) {
        fprintf(stderr, "new group\n", (*obs)->size, (*obs)->time_actual);
        //    for(vector<observation *>::iterator obs = f->observations.observations.begin(); /*termination is handled by trigger clause*/; obs++) {
        int count = 0;
        uint64_t obs_time = (*obs)->time_apparent;
        filter_tick(f, obs_time);
        for(list<preobservation *>::iterator pre = f->observations.preobservations.begin(); pre != f->observations.preobservations.end(); ++pre) (*pre)->process(true);

        lp.resize(f->observations.meas_size, statesize);
        inn.resize(1, f->observations.meas_size);
        m_cov.resize(1, f->observations.meas_size);
        memset(f->observations.lp_storage, 0, sizeof(f->observations.lp_storage));
        memset(lp_data, 0, sizeof(lp_data));
        f->s.copy_state_to_array(state);
        int index = count;

        //these aren't in the same order as they appear in the array - need to build up my local versions as i go
        while(obs != f->observations.observations.end()) {
            fprintf(stderr, "obs of size %d at time %lld\n", (*obs)->size, (*obs)->time_apparent);
            f_t dt = ((f_t)(*obs)->time_apparent - (f_t)obs_time) / 1000000.;
            if((*obs)->time_apparent != obs_time) {
                integrate_motion_state(&f->s, &f->s, dt, NULL);
            }
            (*obs)->predict(true);
            if((*obs)->time_apparent != obs_time) {
                f->s.copy_state_from_array(state);
                integrate_motion_pred(f, (*obs)->lp, dt);
            }
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
            ++obs;
            if(obs == f->observations.observations.end()) break;
            if((*obs)->size == 3) break;
            if((*obs)->time_apparent != obs_time) break;
        }
        lp.resize(count, lp.cols);
        inn.resize(1, count);
        m_cov.resize(1, count);
        meas_update(state, f->s.cov, inn, lp, m_cov);
        //meas_update(state, f->s.cov, f->observations.inn, f->observations.lp, f->observations.m_cov);

        f->s.copy_state_from_array(state);
    }
    f->observations.clear();
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
    packet_t *pv = mapbuffer_alloc(f->s.mapperbuf, packet_feature_variance, sizeof(packet_feature_variance_t) - 16);
    *(float*)pv->data = f->vis_cov;
    mapbuffer_enqueue(f->s.mapperbuf, pv, time);

    //fix up groups and features that have already been added
    for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        g->Wr = f->s.W;
    }
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        state_vision_feature *i = *fiter;
        i->initial = i->current;
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

static int sfm_process_features(struct filter *f, uint64_t time, feature_t *feats, int nfeats)
{
    int feat = 0;
    packet_t *fsp;
    if(f->visbuf) {
        fsp = mapbuffer_alloc(f->visbuf, packet_feature_status, nfeats);
        fsp->header.user = nfeats;
    }
    int useful_drops = 0;
    int todrop = 0;
    uint16_t drops[nfeats];
    int trackedfeats = 0;
    feature_t *uncalibrated = (feature_t*) f->last_raw_track_packet->data;
    for(list<state_vision_feature *>::iterator fi = f->s.features.begin(); fi != f->s.features.end(); ++fi) {
        state_vision_feature *i = *fi;
        if(feats[feat].x == INFINITY) {
            if(f->visbuf) fsp->data[feat] = 0;
            if(i->status == feature_normal && i->variance < i->max_variance) {
                i->status = feature_gooddrop;
                ++useful_drops;
            } else {
                i->status = feature_empty;
            }
        } else if(i->outlier > i->outlier_reject) {
            if(f->visbuf) fsp->data[feat] = 0;
            drops[todrop++] = trackedfeats;
            i->status = feature_empty;
            ++trackedfeats;
        } else {
            i->current[0] = feats[feat].x;
            i->current[1] = feats[feat].y;
            i->uncalibrated[0] = uncalibrated[feat].x;
            i->uncalibrated[1] = uncalibrated[feat].y;
            if(i->status == feature_reject) {
                new(i) state_vision_feature(feats[feat].x, feats[feat].y);
                i->Tr = f->s.T;
                i->Wr = f->s.W;
            }
            if(i->outlier >= i->outlier_reject) {
                i->make_reject();
                if(f->visbuf) fsp->data[feat] = 1;
            }
            ++trackedfeats;
        }
        ++feat;
    }
    if(todrop) {
        packet_feature_drop_t *dp = (packet_feature_drop_t *)mapbuffer_alloc(f->control, packet_feature_drop, todrop * sizeof(uint16_t));
        dp->header.user = todrop;
        memcpy(dp->indices, drops, todrop * sizeof(uint16_t));
        mapbuffer_enqueue(f->control, (packet_t *)dp, time);
    }
    assert(feat == nfeats);
    if(f->visbuf) mapbuffer_enqueue(f->visbuf, fsp, time);
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
        /*        if(i->status == feature_reject) {
            new(i) state_vision_feature(feats[feat].x, feats[feat].y);
            i->Tr = f->s.T;
            i->Wr = f->s.W;
            }*/
        if(i->status == feature_gooddrop || i->status == feature_keep) {
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
        if(i->status == feature_keep) {
            i->id = i->counter++;
            i->index = -1;
            i->status = feature_ready;
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
                    triangulate_feature(&(f->s), i);
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

extern "C" void sfm_vis_measurement(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(p->header.type != packet_feature_track) return;
    ++f->frame;

    uint64_t time = p->header.time;
    int nfeats = p->header.user;
    feature_t *feats = (feature_t *) p->data;

    filter_tick(f, time);
    int feats_used = sfm_process_features(f, time, feats, nfeats);
    if(!f->active) {
        if(f->frame > f->skip && f->gravity_init) {
            f->active = true;
            //set up plot packets
            if(f->visbuf) {
                packet_plot_setup(f->visbuf, p->header.time, packet_plot_inn_v + MAXGROUPS + 1, "Cov-T", 0.);
                packet_plot_setup(f->visbuf, p->header.time, packet_plot_inn_v + MAXGROUPS + 2, "Cov-W", 0.);
                packet_plot_setup(f->visbuf, p->header.time, packet_plot_inn_v + MAXGROUPS + 3, "Cov-a", 0.);
                packet_plot_setup(f->visbuf, p->header.time, packet_plot_inn_v + MAXGROUPS + 4, "Cov-w", 0.);
                packet_plot_setup(f->visbuf, p->header.time, packet_plot_inn_v + MAXGROUPS, "Inn-vis *", sqrt(f->vis_cov));
                for(int g = 0; g < MAXGROUPS; ++g) {
                    char name[32];
                    sprintf(name, "Inn-vis %d", g);
                    packet_plot_setup(f->visbuf, p->header.time, packet_plot_inn_v + g, name, sqrt(f->vis_cov));   
                }
            }
        } else return;
    }
    float tv[3];
    if(f->visbuf) {
        for(int i = 0; i < 3; ++i) tv[i] = f->s.T.variance[i];
        packet_plot_send(f->visbuf, p->header.time, packet_plot_inn_v + MAXGROUPS + 1, 3, tv);
        for(int i = 0; i < 3; ++i) tv[i] = f->s.W.variance[i];
        packet_plot_send(f->visbuf, p->header.time, packet_plot_inn_v + MAXGROUPS + 2, 3, tv);
        for(int i = 0; i < 3; ++i) tv[i] = f->s.a.variance[i];
        packet_plot_send(f->visbuf, p->header.time, packet_plot_inn_v + MAXGROUPS + 3, 3, tv);
        for(int i = 0; i < 3; ++i) tv[i] = f->s.w.variance[i];
        packet_plot_send(f->visbuf, p->header.time, packet_plot_inn_v + MAXGROUPS + 4, 3, tv);
    }

    if(feats_used) {
        int statesize = f->s.cov.rows;
        int meas_used = feats_used * 2;

        int fi = 0;
        preobservation_vision_base *base = f->observations.new_preobservation<preobservation_vision_base>(&f->s);
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
                //fprintf(stderr, "this feature's uncalibrated scanline is %f, translates to time delta %f\n", i->uncalibrated[1], i->uncalibrated[1]/480. * 33000.);

                obs->base = base;
                obs->group = group;
                obs->feature = i;
                obs->meas[0] = i->current[0];
                obs->meas[1] = i->current[1];

                fi += 2;
            }
        }

        process_observation_queue(f);

        /* if(f->visbuf) {
            int fi = 0;
            int gindex = 0;
            for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
                state_vision_group *g = *giter;
                if(!g->status || g->status == group_initializing) continue;
                packet_plot_t *ip;
                ip = (packet_plot_t*)mapbuffer_alloc(f->visbuf, packet_plot, g->features.children.size() *2* sizeof(float));
                if(g->status == group_reference) {
                    ip->header.user = packet_plot_inn_v + MAXGROUPS;
                } else {
                    ip->header.user = packet_plot_inn_v + gindex;
                }
                int fulli = 0;
                for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                    state_vision_feature *i = *fiter;
                    if(!i->status) {
                        ip->data[fulli*2] = 0.;
                        ip->data[fulli*2+1] = 0.;
                        ++fulli;
                        continue;
                    }
                    ip->data[fulli*2] = inn[fi*2];
                    ip->data[fulli*2+1] = inn[fi*2+1];
                    ++fi;
                    ++fulli;
                }
                ip->count = g->features.children.size();
                mapbuffer_enqueue(f->visbuf, (packet_t *)ip, p->header.time);
                ++gindex;
            }
            }*/
    }
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        state_vision_feature *i = *fiter;
        if(i->status == feature_initializing || i->status == feature_ready) {
            ukf_feature_initialize(f, i);
            if(i->v < -3.) i->status = feature_reject;
        }
    }

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
                i->found_time = p->header.time;
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
                i->initial = i->current;
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
                mapbuffer_enqueue(f->recognition_buffer, (packet_t *)pg, p->header.time);
            }
        }
    }

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

extern "C" void sfm_features_added(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(p->header.type == packet_feature_select) {
        feature_t *initial = (feature_t*) p->data;
        feature_t *uncalibrated = (feature_t*) f->last_raw_track_packet->data;
        for(int i = 0; i < p->header.user; ++i) {
            state_vision_feature *feat = f->s.add_feature(initial[i].x, initial[i].y);
            assert(initial[i].x != INFINITY);
            feat->status = feature_initializing;
            feat->uncalibrated[0] = uncalibrated[i].x;
            feat->uncalibrated[1] = uncalibrated[i].y;
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
    state_vision_group::ref_noise = f->vis_ref_noise;
    state_vision_group::min_feats = f->min_feats_per_group;
    state_vision_group::min_health = f->min_group_health;
}
