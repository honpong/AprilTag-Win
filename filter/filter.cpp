// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

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

int state_node::statesize;
int state_node::maxstatesize;

//TODO: homogeneous coordinates.
//TODO: reduced size for ltu

void integrate_motion_state(state_motion *state, state_motion *slope, f_t dt, matrix *ltu)
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

void motion_time_update(state *orig_state, f_t dt, matrix *ltu, int statesize)
{
    MAT_TEMP(k1, 1, statesize);
    MAT_TEMP(k2, 1, statesize);
    MAT_TEMP(k3, 1, statesize);
    MAT_TEMP(k4, 1, statesize);
    orig_state->copy_state_to_array(k1);

    state slope(*orig_state);
    integrate_motion_state(orig_state, &slope, dt * .5, 0);
    orig_state->copy_state_to_array(k2);
    slope = *orig_state;
    orig_state->copy_state_from_array(k1);
    integrate_motion_state(orig_state, &slope, dt * .5, 0);
    orig_state->copy_state_to_array(k3);
    slope = *orig_state;
    orig_state->copy_state_from_array(k1);
    integrate_motion_state(orig_state, &slope, dt, 0);
    orig_state->copy_state_to_array(k4);
    slope = *orig_state;
    orig_state->copy_state_from_array(k1);

    MAT_TEMP(ave_slope, 1, statesize);
    for(int i = 0; i < statesize; ++i) {
        ave_slope[i] = (k1[i] + 2 * (k2[i] + k3[i]) + k4[i]) / 6.;
    }
    orig_state->copy_state_from_array(ave_slope);
    slope = *orig_state;
    orig_state->copy_state_from_array(k1);
    integrate_motion_state(orig_state, &slope, dt, ltu);    
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

static void triangulate_feature(state *state, state_vision_feature *i)
{
    m4 
        R = rodrigues(state->W, NULL),
        Rt = transpose(R),
        Rr = rodrigues(i->Wr, NULL),
        Rtot = Rt * Rr;
    v4
        Ttot = Rt * (i->Tr - state->T) - state->Tc;
 
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
        if(i->status == feature_initializing)
            if(total_var < i->max_variance) i->status = feature_ready;
    }
}

//TODO: homogeneous coordinates!!!!
int vis_predict(state *state, matrix &pred, matrix *_lp)
{
    m4v4 dR_dW;
    m4 
        R = rodrigues(state->W, (_lp?&dR_dW:NULL)),
        Rt = transpose(R);

    state->camera_orientation = invrodrigues(Rt, NULL);
    
    m4v4 dRt_dW = transpose(dR_dW);

    int nummeas = 0;
    for(list<state_vision_group *>::iterator giter = state->groups.children.begin(); giter != state->groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        if(!g->status) continue;
        m4v4 dRr_dWr;
        m4 Rr = rodrigues(g->Wr, (_lp?&dRr_dWr:NULL));

        m4 
            Rtot = Rt * Rr;
        v4
            Ttot = Rt * (g->Tr - state->T);

        m4v4
            dRtot_dW = dRt_dW * Rr,
            dRtot_dWr = Rt * dRr_dWr;
        m4
            dTtot_dW = dRt_dW * (g->Tr - state->T),
            dTtot_dT = -Rt,
            dTtot_dTr = Rt;

        for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(g->status == group_initializing) {
                triangulate_feature(state, i);
                continue;
            }
            f_t rho = exp(*i);
            v4
                X0 = i->initial * rho, /*not homog in v4*/
                Xr = Rr * X0 + g->Tr,
                X = Rtot * X0 + Ttot;
            //f_t delta = fabs(X[0] - Xs[0]) + fabs(X[1] - Xs[1]) + fabs(X[2] - Xs[2]);
            i->relative = Xr;
            f_t invZ = 1./X[2];
            i->prediction = X * invZ;
            pred[nummeas+0] = i->prediction[0];
            pred[nummeas+1] = i->prediction[1];
            assert(fabs(i->prediction[2]-1.) < 1.e-7 && i->prediction[3] == 0.);
            
            if(_lp) {
                matrix &lp = *_lp;
                m4  dy_dX;
                dy_dX.data[0] = v4(invZ, 0., -X[0] * invZ * invZ, 0.);
                dy_dX.data[1] = v4(0., invZ, -X[1] * invZ * invZ, 0.);
                v4
                    dX_dp = Rtot * X0, // dX0_dp = X0
                    dy_dp = dy_dX * dX_dp;
                
                m4
                    dy_dW = dy_dX * (dRtot_dW * X0 + dTtot_dW),
                    dy_dT = dy_dX * dTtot_dT,
                    dy_dWr = dy_dX * (dRtot_dWr * X0),
                    dy_dTr = dy_dX * dTtot_dTr;
                
                for(int j = 0; j < 2; ++j) {
                    int mi = nummeas + j;
                    if(i->status) // != feature_initializing && i->status != feature_ready)
                        lp(mi, i->index) = dy_dp[j];
                    for(int k = 0; k < 3; ++k) {
                        if(g->status != group_initializing) {
                            lp(mi, state->W.index + k) = dy_dW[j][k];
                            lp(mi, state->T.index + k) = dy_dT[j][k];
                            lp(mi, g->Wr.index + k) = dy_dWr[j][k];
                            //if(g->status != group_reference) {
                                lp(mi, g->Tr.index + k) = dy_dTr[j][k];
                                //    }
                        }
                    }
                }
            }
            nummeas += 2;
        }
    }
    pred.resize(nummeas);
    if(_lp) _lp->resize(nummeas, _lp->cols);
    return nummeas;
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


void filter_tick(struct filter *f, uint64_t time)
{
    //TODO: check negative time step!
    if(time <= f->last_time) return;
    if(f->last_time) {
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
    f->last_time = time;
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
    //fprintf(stderr, "%d [%f %f %f] [%f %f %f]\n", time, output[0], output[1], output[2], output[3], output[4], output[5]); 
}

void filter_meas(struct filter *f, matrix &inn, matrix &lp, matrix &m_cov)
{
    int statesize = f->s.cov.rows;
    MAT_TEMP(state, 1, statesize);
    f->s.copy_state_to_array(state);
    meas_update(state, f->s.cov, inn, lp, m_cov);
    f->s.copy_state_from_array(state);
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
        } else {
            i->current[0] = feats[feat].x;
            i->current[1] = feats[feat].y;
            if(i->status == feature_reject) {
                new(i) state_vision_feature(feats[feat].x, feats[feat].y);
                i->Tr = f->s.T;
                i->Wr = f->s.W;
            }
            if(i->outlier > i->outlier_reject) {
                i->make_reject();
                if(f->visbuf) fsp->data[feat] = 1;
            }
        }
        ++feat;
    }
    assert(feat == nfeats);
    if(f->visbuf) mapbuffer_enqueue(f->visbuf, fsp, time);
    if(useful_drops) {
        packet_t *sp = mapbuffer_alloc(f->output, packet_filter_reconstruction, useful_drops * 3 * sizeof(float));
        sp->header.user = useful_drops;
        float (*world)[3] = (float (*)[3])sp->data;
        int nfeats = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(i->status == feature_gooddrop) {
                world[nfeats][0] = i->world[0];
                world[nfeats][1] = i->world[1];
                world[nfeats][2] = i->world[2];
                ++nfeats;
            }
        }
        mapbuffer_enqueue(f->output, sp, time);
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
        } else ++giter;
    }

    f->s.remap();

    return features_used;
}

extern "C" void sfm_vis_measurement(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(p->header.type != packet_feature_track) return;
    ++f->frame;
    
    uint64_t time = p->header.time;
    int nfeats = p->header.user;

    feature_t *feats = (feature_t *) p->data;

    int feats_used = sfm_process_features(f, time, feats, nfeats);
    if(!f->active) {
        if(f->frame > f->skip) {
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
        }
        return;
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
    filter_tick(f, time);

    if(feats_used) {
        int statesize = f->s.cov.rows;
        MAT_TEMP(lp, feats_used*2, statesize);
        memset(lp_data, 0, sizeof(lp_data));
        MAT_TEMP(pred, 1, feats_used * 2);
        int meas_used = vis_predict(&f->s, pred, &lp);
        //assert(meas_used == feats_used*2);

        if(0) {
            test_meas(f, feats_used * 2, statesize, vis_predict);
        }
        if(!meas_used) return;

        MAT_TEMP(inn, 1, meas_used);
        MAT_TEMP(m_cov, 1, meas_used);
    
        f_t ot = f->outlier_thresh * f->outlier_thresh;

        f_t residuals[meas_used];
        int fi = 0;
        int gindex = 0;
        for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
            state_vision_group *g = *giter;
            if(!g->status || g->status == group_initializing) continue;
            packet_plot_t *ip;
            if(f->visbuf) {
                ip = (packet_plot_t*)mapbuffer_alloc(f->visbuf, packet_plot, g->features.children.size() *2* sizeof(float));
                if(g->status == group_reference) {
                    ip->header.user = packet_plot_inn_v + MAXGROUPS;
                } else {
                    ip->header.user = packet_plot_inn_v + gindex;
                }
            }
            int outlier_count = 0;
            int fi_save = fi;
            int fulli = 0;
            for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                state_vision_feature *i = *fiter;
                if(!i->status) {
                    if(f->visbuf) {
                        ip->data[fulli*2] = 0.;
                        ip->data[fulli*2+1] = 0.;
                    }
                    ++fulli;
                    continue;
                }
                f_t thresh = i->measurement_var * ot;
                i->innovation = i->current - i->prediction;
                inn[fi*2  ] = i->innovation[0];
                inn[fi*2+1] = i->innovation[1];
                residuals[fi] = sum(i->innovation * i->innovation);
                if(f->visbuf) {
                    ip->data[fulli*2] = i->innovation[0];
                    ip->data[fulli*2+1] = i->innovation[1];
                }
                ++fi;
                ++fulli;
            }
            fi = fi_save;
            fulli = 0;
            for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                state_vision_feature *i = *fiter;
                if(!i->status) continue;
                f_t badness = residuals[fi]; //outlier_count <= 0  ? outlier_inn[i] : outlier_ess[i];
                f_t robust_mc;
                f_t thresh = i->measurement_var * ot;
                if(badness > thresh) {
                    f_t ratio = sqrt(badness / thresh);
                    robust_mc = ratio * i->measurement_var;
                    i->outlier += ratio;
                } else {
                    robust_mc = i->measurement_var;
                    i->outlier = 0.;
                }
                m_cov[fi*2] = robust_mc;
                m_cov[fi*2+1] = robust_mc;
                
                ++fi;
                ++fulli;
            }
            if(f->visbuf) {
                ip->count = g->features.children.size();
                mapbuffer_enqueue(f->visbuf, (packet_t *)ip, p->header.time);
            }
            ++gindex;
        }
        filter_meas(f, inn, lp, m_cov);
    }

    int space = f->s.maxstatesize - f->s.statesize - 6;
    if(space > f->max_group_add) space = f->max_group_add;
    if(space >= f->min_group_add) {
        int ready = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(i->status == feature_initializing) {
                triangulate_feature(&(f->s), i);
            }
            if(i->status == feature_ready) {
                ++ready;
            }
        }
        if(ready >= f->min_group_add || (f->s.groups.children.size() == 0 && f->s.features.size() != 0)) {
            state_vision_group * g = f->s.add_group(time);
            for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
                state_vision_feature *i = *fiter;
                if(i->status == feature_ready || (f->s.groups.children.size() == 1 && ready < f->min_group_add)) {
                    g->features.children.push_back(i);
                    //i->v = 1.; //log(i->camera_local[2]);
                    i->index = -1;
                    i->groupid = g->id;
                    i->found_time = p->header.time;
                    i->initial = i->current;
                    i->status = feature_normal;
                    i->variance = i->initial_var;
                    if(!--space) break;
                }
            }
            g->make_normal();
            f->s.remap();
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
    int n_good_feats = 0;
    for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        if(g->status == group_initializing) continue;
        for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(!i->status || i->status == feature_reject) continue;
            cp->points[n_good_feats][0] = i->world[0];
            cp->points[n_good_feats][1] = i->world[1];
            cp->points[n_good_feats][2] = i->world[2];
            ++n_good_feats;
        }
    }
    cp->header.user = n_good_feats;
    mapbuffer_enqueue(f->output, (packet_t*)cp, time);
}

extern "C" void sfm_features_added(void *_f, packet_t *p)
{
    if(p->header.type != packet_feature_select) return;
    
    struct filter *f = (struct filter *)_f;
    feature_t *initial = (feature_t*) p->data;

    for(int i = 0; i < p->header.user; ++i) {
        state_vision_feature *feat = f->s.add_feature(initial[i].x, initial[i].y);
        assert(initial[i].x != INFINITY);
        feat->status = feature_initializing;
    }
    f->s.remap();
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
    state_vision_feature::outlier_reject = f->outlier_reject;
    state_vision_feature::max_variance = f->max_feature_std_percent * f->max_feature_std_percent;
    state_vision_group::ref_noise = f->vis_ref_noise;
    state_vision_group::min_feats = f->min_feats_per_group;
    state_vision_group::min_health = f->min_group_health;
}
