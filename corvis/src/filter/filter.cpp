// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
extern "C" {
#include "../cor/cor.h"
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
#include "stereo.h"

int state_node::statesize;
int state_node::maxstatesize;

//TODO: homogeneous coordinates.

extern "C" void filter_reset_position(struct filter *f)
{
    for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); giter++) {
        (*giter)->Tr.v -= f->s.T.v;
    }
    f->s.T.v = 0.;
    f->s.total_distance = 0.;
    f->s.last_position = f->s.T.v;
}

extern "C" void filter_reset_for_inertial(struct filter *f)
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
    
    f->s.T.v = 0.;
    f->s.V.v = 0.;
    f->s.a.v = 0.;
    f->s.da.v = 0.;
    f->s.total_distance = 0.;
    f->s.last_position = f->s.T.v;
    f->s.reference = NULL;
    f->observations.clear();
    f->s.remap();
    f->inertial_converged = false;
    
    f->s.T.reset_covariance(f->s.cov);
    f->s.V.reset_covariance(f->s.cov);
    f->s.a.reset_covariance(f->s.cov);
    f->s.da.reset_covariance(f->s.cov);
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
    filter_config(f);
    f->s.remap();

    f->gravity_init = false;
    f->last_time = 0;
    f->frame = 0;
    f->active = false;
    f->s.enable_orientation_only();
    f->want_active = false;
    f->want_start = 0;
    f->got_accelerometer = f->got_gyroscope = f->got_image = false;
    f->need_reference = true;
    f->accelerometer_max = f->gyroscope_max = 0.;
    f->detector_failed = f->tracker_failed = f->tracker_warned = false;
    f->speed_failed = f->speed_warning = f->numeric_failed = false;
    f->speed_warning_time = 0;
    f->calibration_bad = false;
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

/*
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
                if (log_enabled) fprintf(stderr, "%d\t%d\t: sign flip: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                if (log_enabled) fprintf(stderr, "%d\t%d\t: lin error: expected %e, got %e\n", i, j, ldiff, delta);
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
                if (log_enabled) fprintf(stderr, "%d\t%d\t: sign flip: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                if (log_enabled) fprintf(stderr, "%d\t%d\t: lin error: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
        }
    }
    f->s.copy_state_from_array(save_state);
}
*/

void filter_update_outputs(struct filter *f, uint64_t time)
{
    if(!f->active) return;
    if(f->output) {
        packet_t *packet = mapbuffer_alloc(f->output, packet_filter_position, 6 * sizeof(float));
        float *output = (float *)packet->data;
        output[0] = f->s.T.v[0];
        output[1] = f->s.T.v[1];
        output[2] = f->s.T.v[2];
        output[3] = f->s.W.v.raw_vector()[0];
        output[4] = f->s.W.v.raw_vector()[1];
        output[5] = f->s.W.v.raw_vector()[2];
        mapbuffer_enqueue(f->output, packet, time);
    }
    m4 
        R = to_rotation_matrix(f->s.W.v),
        Rt = transpose(R),
        Rbc = to_rotation_matrix(f->s.Wc.v),
        Rcb = transpose(Rbc),
        RcbRt = Rcb * Rt;

    f->s.camera_matrix = RcbRt;
    v4 T = Rcb * ((Rt * -f->s.T.v) - f->s.Tc.v);
    f->s.camera_matrix[0][3] = T[0];
    f->s.camera_matrix[1][3] = T[1];
    f->s.camera_matrix[2][3] = T[2];
    f->s.camera_matrix[3][3] = 1.;

    f->speed_failed = false;
    f_t speed = norm(f->s.V.v);
    if(speed > 3.) { //1.4m/s is normal walking speed
        if (log_enabled) fprintf(stderr, "Velocity %f m/s exceeds max bound\n", speed);
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(speed > 2.) {
        if (log_enabled) fprintf(stderr, "High velocity (%f m/s) warning\n", speed);
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t accel = norm(f->s.a.v);
    if(accel > 9.8) { //1g would saturate sensor anyway
        if (log_enabled) fprintf(stderr, "Acceleration exceeds max bound\n");
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(accel > 5.) { //max in mine is 6.
        if (log_enabled) fprintf(stderr, "High acceleration (%f m/s^2) warning\n", accel);
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t ang_vel = norm(f->s.w.v);
    if(ang_vel > 5.) { //sensor saturation - 250/180*pi
        if (log_enabled) fprintf(stderr, "Angular velocity exceeds max bound\n");
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(ang_vel > 2.) { // max in mine is 1.6
        if (log_enabled) fprintf(stderr, "High angular velocity warning\n");
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    //if(f->speed_warning && filter_converged(f) < 1.) f->speed_failed = true;
    if(time - f->speed_warning_time > 1000000) f->speed_warning = false;

    //if (log_enabled) fprintf(stderr, "%d [%f %f %f] [%f %f %f]\n", time, output[0], output[1], output[2], output[3], output[4], output[5]); 
}

void process_observation_queue(struct filter *f, uint64_t time)
{
    f->last_time = time;
    if(!f->observations.process(f->s, time)) {
        f->numeric_failed = true;
        f->calibration_bad = true;
    }
    filter_update_outputs(f, time);
}

void filter_compute_gravity(struct filter *f, double latitude, double altitude)
{
    //http://en.wikipedia.org/wiki/Gravity_of_Earth#Free_air_correction
    double sin_lat = sin(latitude/180. * M_PI);
    double sin_2lat = sin(2*latitude/180. * M_PI);
    f->s.g.v = 9.780327 * (1 + 0.0053024 * sin_lat*sin_lat - 0.0000058 * sin_2lat*sin_2lat) - 3.086e-6 * altitude;
}

static bool check_packet_time(struct filter *f, uint64_t t, int type)
{
    if(t < f->last_packet_time) {
        if (log_enabled) fprintf(stderr, "Warning: received packets out of order: %d at %lld came first, then %d at %lld. delta %lld\n", f->last_packet_type, f->last_packet_time, type, t, f->last_packet_time - t);
        return false;
    }
    f->last_packet_time = t;
    f->last_packet_type = type;
    return true;
}

extern "C" void filter_imu_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_imu) return;
    struct filter *f = (struct filter *)_f;
    filter_accelerometer_measurement(f, (float *)&p->data, p->header.time);
    filter_gyroscope_measurement(f, (float *)&p->data + 3, p->header.time);
}

extern "C" void filter_accelerometer_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_accelerometer) return;
    filter_accelerometer_measurement((struct filter *)_f, (float *)&p->data, p->header.time);
}

void update_static_calibration(struct filter *f)
{
    if(f->accel_stability.count < 500) return;
    v4 var = f->accel_stability.variance;
    f->a_variance = (var[0] + var[1] + var[2]) / 3.;
    var = f->gyro_stability.variance;
    f->w_variance = (var[0] + var[1] + var[2]) / 3.;
}

bool do_static_calibration(struct filter *f, stdev_vector &stdev, v4 meas, f_t variance, uint64_t time)
{
    if(stdev.count) {
        f_t sigma2 = 6*6; //4.5 sigma seems to be the right balance of not getting false positives while also capturing full stdev
        bool steady = true;
        for(int i = 0; i < 3; ++i) {
            f_t delta = meas[i] - stdev.mean[i];
            f_t d2 = delta * delta;
            if(d2 > variance * sigma2) steady = false;
        }
        if(steady) {
            update_static_calibration(f);
        } else {
            stdev = stdev_vector();
            f->stable_start = time;
        }
    } else {
        f->stable_start = time;
    }
    stdev.data(meas);
    
    if(time - f->stable_start < 100000) {
        return false;
    }

    return true;
}

void filter_accelerometer_measurement(struct filter *f, float data[3], uint64_t time)
{
    if(!check_packet_time(f, time, packet_accelerometer)) return;

    if(!f->got_accelerometer) { //skip first packet - has been crap from gyro
        f->got_accelerometer = true;
        return;
    }
    if(!f->got_gyroscope) return;

    v4 meas(data[0], data[1], data[2], 0.);
    
    if(!f->gravity_init) {
        f->gravity_init = true;
        //set up plots
        if(f->visbuf) {
            packet_plot_setup(f->visbuf, time, packet_plot_meas_a, "Meas-alpha", sqrt(f->a_variance));
            packet_plot_setup(f->visbuf, time, packet_plot_meas_w, "Meas-omega", sqrt(f->w_variance));
            packet_plot_setup(f->visbuf, time, packet_plot_inn_a, "Inn-alpha", sqrt(f->a_variance));
            packet_plot_setup(f->visbuf, time, packet_plot_inn_w, "Inn-omega", sqrt(f->w_variance));
        }
        
        //fix up groups and features that have already been added
/*        for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
            state_vision_group *g = *giter;
            g->Wr.v = f->s.W.v;
        }
        
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            i->initial = i->current;
            i->Wr = f->s.W.v;
        }*/
    }
    
    observation_spatial *obs_a;
    obs_a = new observation_accelerometer(f->s, time, time);
    
    for(int i = 0; i < 3; ++i) {
        obs_a->meas[i] = data[i];
    }
    
    f->observations.observations.push_back(obs_a);

    if(!f->active) {
        if(f->run_static_calibration && do_static_calibration(f, f->accel_stability, meas, f->a_variance, time)) {
            //we are static now, so we can use tight variance
            obs_a->variance = f->a_variance;
        } else {
            //if we are initializing, then any user-induced acceleration ends up in the measurement noise.
            obs_a->variance = 1.;
        }
    } else {
        obs_a->variance = f->a_variance;        
    }

    if(show_tuning) fprintf(stderr, "accelerometer:\n");
    process_observation_queue(f, time);
    if(show_tuning) {
        fprintf(stderr, " actual innov stdev is:\n");
        observation_accelerometer::inn_stdev.print();
        fprintf(stderr, " signal stdev is:\n");
        observation_accelerometer::stdev.print();
        fprintf(stderr, " bias is:\n");
        f->s.a_bias.v.print(); v4(f->s.a_bias.variance()).print();
    }
}

extern "C" void filter_gyroscope_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_gyroscope) return;
    filter_gyroscope_measurement((struct filter *)_f, (float *)&p->data, p->header.time);
}

void filter_gyroscope_measurement(struct filter *f, float data[3], uint64_t time)
{
    if(!check_packet_time(f, time, packet_gyroscope)) return;
    if(!f->got_gyroscope) { //skip the first piece of data as it seems to be crap
        f->got_gyroscope = true;
        return;
    }
    if(!f->gravity_init) return;

    v4 meas(data[0], data[1], data[2], 0.);

    observation_gyroscope *obs_w = new observation_gyroscope(f->s, time, time);
    for(int i = 0; i < 3; ++i) {
        obs_w->meas[i] = data[i];
    }
    obs_w->variance = f->w_variance;
    f->observations.observations.push_back(obs_w);

    if(f->run_static_calibration) do_static_calibration(f, f->gyro_stability, meas, f->w_variance, time);

    if(show_tuning) fprintf(stderr, "gyroscope:\n");
    process_observation_queue(f, time);
    if(show_tuning) {
        fprintf(stderr, " actual innov stdev is:\n");
        observation_gyroscope::inn_stdev.print();
        fprintf(stderr, " signal stdev is:\n");
        observation_gyroscope::stdev.print();
        fprintf(stderr, " bias is:\n");
        f->s.w_bias.v.print(); v4(f->s.w_bias.variance()).print();
        fprintf(stderr, "\n");
    }
}

static int filter_process_features(struct filter *f, uint64_t time)
{
    int useful_drops = 0;
    int total_feats = 0;
    int outliers = 0;
    int toobig = f->s.statesize - f->s.maxstatesize;
    //TODO: revisit this - should check after dropping other features, make this more intelligent
    if(toobig > 0) {
        int dropped = 0;
        vector<f_t> vars;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            vars.push_back((*fiter)->variance());
        }
        std::sort(vars.begin(), vars.end());
        if(vars.size() > toobig) {
            f_t min = vars[vars.size() - toobig];
            for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
                if((*fiter)->variance() >= min) {
                    (*fiter)->status = feature_empty;
                    ++dropped;
                    if(dropped >= toobig) break;
                }
            }
            if (log_enabled) fprintf(stderr, "state is %d too big, dropped %d features, min variance %f\n",toobig, dropped, min);
        }
    }
    for(list<state_vision_feature *>::iterator fi = f->s.features.begin(); fi != f->s.features.end(); ++fi) {
        state_vision_feature *i = *fi;
        if(i->current[0] == INFINITY) {
            if(i->is_good()) ++useful_drops;
            i->drop();
        } else {
            if(i->status == feature_normal || i->status == feature_reject) ++total_feats;
            if(i->outlier > i->outlier_reject || i->status == feature_reject) {
                i->status = feature_empty;
                ++outliers;
            }
        }
    }
    //    if (log_enabled) fprintf(stderr, "outliers: %d/%d (%f%%)\n", outliers, total_feats, outliers * 100. / total_feats);
    if(useful_drops && f->output) {
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
                rp->depth = i->v.depth();
                f_t var = i->measurement_var < i->variance() ? i->variance() : i->measurement_var;
                //for measurement var, the values are simply scaled by depth, so variance multiplies by depth^2
                //for depth variance, d/dx = e^x, and the variance is v*(d/dx)^2
                rp->variance = var * rp->depth * rp->depth;
                mapbuffer_enqueue(f->recognition_buffer, (packet_t *)rp, time);
            }
        }
        if(i->status == feature_gooddrop) i->status = feature_empty;
        if(i->status == feature_reject) i->status = feature_empty;
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
                    if (log_enabled) fprintf(stderr, "calling triangulate feature from process\n");
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
    return p1->variance() < p2->variance();
}

void filter_setup_next_frame(struct filter *f, uint64_t time)
{
    ++f->frame;
    size_t feats_used = f->s.features.size();

    if(!f->active) return;

    if(feats_used) {
        int fi = 0;
        for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
            state_vision_group *g = *giter;
            if(!g->status || g->status == group_initializing) continue;
            for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                state_vision_feature *i = *fiter;
                uint64_t extra_time = f->shutter_delay + i->current[1]/f->image_height * f->shutter_period;
                observation_vision_feature *obs = new observation_vision_feature(f->s, time + extra_time, time);
                obs->state_group = g;
                obs->feature = i;
                obs->meas[0] = i->current[0];
                obs->meas[1] = i->current[1];
                obs->im1 = f->track.im1;
                obs->im2 = f->track.im2;
                obs->tracker = f->track;

                f->observations.observations.push_back(obs);

                fi += 2;
            }
        }
    }
    //TODO: implement feature_single ?
}

void filter_send_output(struct filter *f, uint64_t time)
{
    size_t nfeats = f->s.features.size();
    packet_filter_current_t *cp = 0;
    if(f->output) {
        cp = (packet_filter_current_t *)mapbuffer_alloc(f->output, packet_filter_current, (uint32_t)(sizeof(packet_filter_current) - 16 + nfeats * 3 * sizeof(float)));
    }
    packet_filter_feature_id_visible_t *visible = 0;
    if(f->output) {
        visible = (packet_filter_feature_id_visible_t *)mapbuffer_alloc(f->output, packet_filter_feature_id_visible, (uint32_t)(sizeof(packet_filter_feature_id_visible_t) - 16 + nfeats * sizeof(uint64_t)));
        for(int i = 0; i < 3; ++i) {
            visible->T[i] = f->s.T.v[i];
            visible->W[i] = f->s.W.v.raw_vector()[i];
        }
    }
    int n_good_feats = 0;
    for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        if(g->status == group_initializing) continue;
        for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(!i->status || i->status == feature_reject) continue;
            if(f->output) {
                cp->points[n_good_feats][0] = i->world[0];
                cp->points[n_good_feats][1] = i->world[1];
                cp->points[n_good_feats][2] = i->world[2];
                visible->feature_id[n_good_feats] = i->id;
            }
            ++n_good_feats;
        }
    }
    if(f->output) {
        cp->header.user = n_good_feats;
        visible->header.user = n_good_feats;
    }
    if(f->output) {
        mapbuffer_enqueue(f->output, (packet_t*)cp, time);
        mapbuffer_enqueue(f->output, (packet_t*)visible, time);
    }
}

// Changing this scale factor will cause problems with the FAST detector
#define MASK_SCALE_FACTOR 8

static void mask_feature(uint8_t *scaled_mask, int scaled_width, int scaled_height, int fx, int fy)
{
    int x = fx / 8;
    int y = fy / 8;
    if(x < 0 || y < 0 || x >= scaled_width || y >= scaled_height) return;
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
        if(x < scaled_width - 1) scaled_mask[x+1 + y * scaled_width] = 0;
    }
}

static void mask_initialize(uint8_t *scaled_mask, int scaled_width, int scaled_height)
{
    //set up mask - leave a 1-block strip on border off
    //use 8 byte blocks
    memset(scaled_mask, 0, scaled_width);
    memset(scaled_mask + scaled_width, 1, (scaled_height - 2) * scaled_width);
    memset(scaled_mask + (scaled_height - 1) * scaled_width, 0, scaled_width);
    //vertical border
    for(int y = 1; y < scaled_height - 1; ++y) {
        scaled_mask[0 + y * scaled_width] = 0;
        scaled_mask[scaled_width-1 + y * scaled_width] = 0;
    }
}

//features are added to the state immediately upon detection - handled with triangulation in observation_vision_feature::predict - but what is happening with the empty row of the covariance matrix during that time?
static void addfeatures(struct filter *f, size_t newfeats, unsigned char *img, unsigned int width, int height, uint64_t time)
{
#ifdef TEST_POSDEF
    if(!test_posdef(f->s.cov.cov)) fprintf(stderr, "not pos def before adding features\n");
#endif
    // Filter out features which we already have by masking where
    // existing features are located 
    int scaled_width = width / MASK_SCALE_FACTOR;
    int scaled_height = height / MASK_SCALE_FACTOR;
    if(!f->scaled_mask) f->scaled_mask = new uint8_t[scaled_width * scaled_height];
    mask_initialize(f->scaled_mask, scaled_width, scaled_height);
    // Mark existing tracked features
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        mask_feature(f->scaled_mask, scaled_width, scaled_height, (*fiter)->current[0], (*fiter)->current[1]);
    }

    // Run detector
    vector<xy> &kp = f->track.detect(img, f->scaled_mask, (int)newfeats, 0, 0, width, height);

    // Check that the detected features don't collide with the mask
    // and add them to the filter
    if(kp.size() < newfeats) newfeats = kp.size();
    if(newfeats < f->min_feats_per_group) return;
    state_vision_group *g = f->s.add_group(time);

    int found_feats = 0;
    for(int i = 0; i < kp.size(); ++i) {
        int x = kp[i].x;
        int y = kp[i].y;
        if(x > 0 && y > 0 && x < width-1 && y < height-1 &&
           f->scaled_mask[(x/MASK_SCALE_FACTOR) + (y/MASK_SCALE_FACTOR) * scaled_width]) {
            mask_feature(f->scaled_mask, scaled_width, scaled_height, x, y);
            state_vision_feature *feat = f->s.add_feature(x, y);
            int lx = floor(x);
            int ly = floor(y);
            feat->intensity = (((unsigned int)img[lx + ly*width]) + img[lx + 1 + ly * width] + img[lx + width + ly * width] + img[lx + 1 + width + ly * width]) >> 2;
            g->features.children.push_back(feat);
            feat->groupid = g->id;
            feat->found_time = time;
            feat->Tr = g->Tr.v;
            feat->Wr = g->Wr.v;
            
            found_feats++;
            if(found_feats == newfeats) break;
        }
    }
    g->status = group_initializing;
    g->make_normal();
    f->s.remap();
#ifdef TEST_POSDEF
    if(!test_posdef(f->s.cov.cov)) fprintf(stderr, "not pos def after adding features\n");
#endif
}

void send_current_features_packet(struct filter *f, uint64_t time)
{
    if(!f->track.sink) return;
    packet_t *packet = mapbuffer_alloc(f->track.sink, packet_feature_track, (uint32_t)(f->s.features.size() * sizeof(feature_t)));
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

void filter_set_reference(struct filter *f)
{
    filter_reset_position(f);
}

extern "C" void filter_control_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_filter_control) return;
    struct filter *f = (struct filter *)_f;
    if(p->header.user == 2) {
        //full reset
        if (log_enabled) fprintf(stderr, "full filter reset\n");
        filter_reset_full(f);
    }
    if(p->header.user == 1) {
        //start measuring
        if (log_enabled) fprintf(stderr, "measurement starting\n");
        filter_set_reference(f);
    }
    if(p->header.user == 0) {
        //stop measuring
        if (log_enabled) fprintf(stderr, "measurement stopping\n");
        //ignore
    }
}

#include <mach/mach.h>

bool filter_image_measurement(struct filter *f, unsigned char *data, int width, int height, int stride, uint64_t time)
{
    if(!check_packet_time(f, time, packet_camera)) return false;
    if(!f->got_accelerometer || !f->got_gyroscope) return false;
    
    if(!f->valid_time) {
        f->first_time = time;
        f->valid_time = true;
    }

    f->image_packets++;

    f->got_image = true;
    if(f->want_active) {
        if(f->want_start == 0) f->want_start = time;
        f->inertial_converged = (f->s.W.variance()[0] < 1.e-3 && f->s.W.variance()[1] < 1.e-3);
        if(f->inertial_converged || time - f->want_start > 500000) {
            if(log_enabled) {
                if(f->inertial_converged) {
                    fprintf(stderr, "Inertial converged at time %lld\n", time - f->want_start);
                } else {
                    fprintf(stderr, "Inertial did not converge %f, %f\n", f->s.W.variance()[0], f->s.W.variance()[1]);
                }
            }
        } else return true;
    }
    if(f->run_static_calibration || (!f->active && !f->want_active)) return true; //frame was "processed" so that callbacks still get called
    if(width != f->track.width || height != f->track.height || stride != f->track.stride) {
        fprintf(stderr, "Image dimensions don't match what we expect!\n");
        abort();
    }
    
    if(!f->ignore_lateness) {
        /*thread_info_data_t thinfo;
        mach_msg_type_number_t thinfo_count;
        kern_return_t kr = thread_info(mach_thread_self(), THREAD_BASIC_INFO, thinfo, &thinfo_count);
        float cpu = ((thread_basic_info_t)thinfo)->cpu_usage / (float)TH_USAGE_SCALE;
        fprintf(stderr, "cpu usage is %f\n", cpu);*/
        
        int64_t current = cor_time();
        int64_t delta = current - (time - f->first_time);
        if(!f->valid_delta) {
            f->mindelta = delta;
            f->valid_delta = true;
        }
        if(delta < f->mindelta) {
            f->mindelta = delta;
        }
        int64_t lateness = delta - f->mindelta;
        int64_t period = time - f->last_arrival;
        f->last_arrival = time;
        
        if(lateness > period * 2) {
            if (log_enabled) fprintf(stderr, "old max_state_size was %d\n", f->s.maxstatesize);
            f->s.maxstatesize = f->s.statesize - 1;
            if(f->s.maxstatesize < MINSTATESIZE) f->s.maxstatesize = MINSTATESIZE;
            f->track.maxfeats = f->s.maxstatesize - 10;
            if (log_enabled) fprintf(stderr, "was %lld us late, new max state size is %d, current state size is %d\n", lateness, f->s.maxstatesize, f->s.statesize);
            if (log_enabled) fprintf(stderr, "dropping a frame!\n");
            return false;
        }
        if(lateness > period && f->s.maxstatesize > MINSTATESIZE && f->s.statesize < f->s.maxstatesize) {
            f->s.maxstatesize = f->s.statesize - 1;
            if(f->s.maxstatesize < MINSTATESIZE) f->s.maxstatesize = MINSTATESIZE;
            f->track.maxfeats = f->s.maxstatesize - 10;
            if (log_enabled) fprintf(stderr, "was %lld us late, new max state size is %d, current state size is %d\n", lateness, f->s.maxstatesize, f->s.statesize);
        }
        if(lateness < period / 4 && f->s.statesize > f->s.maxstatesize - f->min_group_add && f->s.maxstatesize < MAXSTATESIZE - 1) {
            ++f->s.maxstatesize;
            f->track.maxfeats = f->s.maxstatesize - 10;
            if (log_enabled) fprintf(stderr, "was %lld us late, new max state size is %d, current state size is %d\n", lateness, f->s.maxstatesize, f->s.statesize);
        }
    }

    f->track.im1 = f->track.im2;
    f->track.im2 = data;

    filter_setup_next_frame(f, time);

    if(show_tuning) {
        fprintf(stderr, "vision:\n");
    }
    process_observation_queue(f, time);
    if(show_tuning) {
        fprintf(stderr, " actual innov stdev is:\n");
        observation_vision_feature::inn_stdev[0].print();
        observation_vision_feature::inn_stdev[1].print();
    }

    filter_process_features(f, time);
    filter_send_output(f, time);
    send_current_features_packet(f, time);

    if(f->s.estimate_calibration && !f->estimating_Tc && time - f->active_time > 2000000)
    {
        //TODO: leaving Tc out of the state now. This gain scheduling is wrong (crash when adding tc back in if state is full).
        f->s.children.push_back(&f->s.Tc);
        f->s.remap();
        f->estimating_Tc = true;
    }

    int space = f->s.maxstatesize - f->s.statesize - 6;
    if(space > f->max_group_add) space = f->max_group_add;
    if(space >= f->min_group_add) {
        if(f->want_active) {
#ifdef TEST_POSDEF
            if(!test_posdef(f->s.cov.cov)) fprintf(stderr, "not pos def before disabling orient only\n");
#endif
            f->s.disable_orientation_only();
            if(f->s.estimate_calibration) {
                f->s.remove_child(&(f->s.Tc));
                f->s.remap();
                f->estimating_Tc = false;
            }
#ifdef TEST_POSDEF
            if(!test_posdef(f->s.cov.cov)) fprintf(stderr, "not pos def after disabling orient only\n");
#endif
        }
        addfeatures(f, space, data, f->track.width, f->track.height, time);
        if(f->s.features.size() < f->min_feats_per_group) {
            if (log_enabled) fprintf(stderr, "detector failure: only %ld features after add\n", f->s.features.size());
            f->detector_failed = true;
            f->calibration_bad = true;
            if(f->want_active) f->s.enable_orientation_only();
        } else {
            //don't go active until we can successfully add features
            if(f->want_active) {
                f->active = true;
                f->want_active = false;
                f->active_time = time;
            }
            f->detector_failed = false;
        }
    }


    if(f->active && f->stereo_enabled) {
        if(!f->stereo_previous_state.frame && f->s.features.size() > 15)
            f->stereo_previous_state = stereo_save_state(f, data);
        else if(f->stereo_previous_state.frame && stereo_should_save_state(f, f->stereo_previous_state)) {
            stereo_free_state(f->stereo_previous_state);
            f->stereo_previous_state = stereo_save_state(f, data);
        }
    }

    return true;
}

extern "C" void filter_image_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_camera) return;
    struct filter *f = (struct filter *)_f;
    if(!f->track.width) {
        int width, height;
        sscanf((char *)p->data, "P5 %d %d", &width, &height);
        f->track.width = width;
        f->track.height = height;
        f->track.stride = width;
    }
    filter_image_measurement(f, p->data + 16, f->track.width, f->track.height, f->track.stride, p->header.time);
}

extern "C" void filter_features_added_packet(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(p->header.type == packet_feature_select) {
        feature_t *initial = (feature_t*) p->data;
        for(int i = 0; i < p->header.user; ++i) {
            state_vision_feature *feat = f->s.add_feature(initial[i].x, initial[i].y);
            assert(initial[i].x != INFINITY);
            feat->status = feature_initializing;
            feat->current[0] = initial[i].x;
            feat->current[1] = initial[i].y;
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

/*static double a_bias_stdev = .02 * 9.8; //20 mg
static double BEGIN_ABIAS_VAR = a_bias_stdev * a_bias_stdev;
static double w_bias_stdev = 10. / 180. * M_PI; //10 dps
static double BEGIN_WBIAS_VAR = w_bias_stdev * w_bias_stdev;*/

#define BEGIN_FOCAL_VAR 10.
#define END_FOCAL_VAR .3
#define BEGIN_C_VAR 2.
#define END_C_VAR .16
#define BEGIN_ABIAS_VAR 1.e-5
#define END_ABIAS_VAR 1.e-6
#define BEGIN_WBIAS_VAR 1.e-7
#define END_WBIAS_VAR 1.e-8
#define BEGIN_K1_VAR 2.e-4
#define END_K1_VAR 1.e-5
#define BEGIN_K2_VAR 2.e-4
#define BEGIN_K3_VAR 1.e-4

void filter_config(struct filter *f)
{
    f->track.groupsize = 24;
    f->track.maxgroupsize = 40;
    f->track.maxfeats = 70;

    //This needs to be synced up with filter_reset_for_intertial
    f->s.T.set_initial_variance(1.e-7);
    //TODO: This might be wrong. changing this to 10 makes a very different (and not necessarily worse) result.
    f->s.W.set_initial_variance(10., 10., 1.e-7);
    f->s.V.set_initial_variance(1. * 1.);
    f->s.w.set_initial_variance(1.e5);
    f->s.dw.set_initial_variance(1.e5); //observed range of variances in sequences is 1-6
    f->s.a.set_initial_variance(1.e5);
    f->s.da.set_initial_variance(1.e5); //observed range of variances in sequences is 10-50
    f->s.g.set_initial_variance(1.e-7);
    //TODO: This is wrong
    f->s.Wc.set_initial_variance(f->device.Wc_var[0], f->device.Wc_var[1], f->device.Wc_var[2]);
    f->s.Tc.set_initial_variance(f->device.Tc_var[0], f->device.Tc_var[1], f->device.Tc_var[2]);
    f->s.a_bias.v = v4(f->device.a_bias[0], f->device.a_bias[1], f->device.a_bias[2], 0.);
    f_t tmp[3];
    //TODO: figure out how much drift we need to worry about between runs
    for(int i = 0; i < 3; ++i) tmp[i] = f->device.a_bias_var[i] < 1.e-5 ? 1.e-5 : f->device.a_bias_var[i];
    f->s.a_bias.set_initial_variance(tmp[0], tmp[1], tmp[2]);
    f->s.w_bias.v = v4(f->device.w_bias[0], f->device.w_bias[1], f->device.w_bias[2], 0.);
    for(int i = 0; i < 3; ++i) tmp[i] = f->device.w_bias_var[i] < 1.e-6 ? 1.e-6 : f->device.w_bias_var[i];
    f->s.w_bias.set_initial_variance(tmp[0], tmp[1], tmp[2]);
    f->s.focal_length.set_initial_variance(BEGIN_FOCAL_VAR);
    f->s.center_x.set_initial_variance(BEGIN_C_VAR);
    f->s.center_y.set_initial_variance(BEGIN_C_VAR);
    f->s.k1.set_initial_variance(BEGIN_K1_VAR);
    f->s.k2.set_initial_variance(BEGIN_K2_VAR);
    f->s.k3.set_initial_variance(BEGIN_K3_VAR);

    f->init_vis_cov = .75;
    f->max_add_vis_cov = 2.;
    f->min_add_vis_cov = .5;

    f->s.T.set_process_noise(0.);
    f->s.W.set_process_noise(0.);
    f->s.V.set_process_noise(0.);
    f->s.w.set_process_noise(0.);
    f->s.dw.set_process_noise(35. * 35.); // this stabilizes dw.stdev around 5-6
    f->s.a.set_process_noise(0.);
    f->s.da.set_process_noise(250. * 250.); //this stabilizes da.stdev around 45-50
    f->s.g.set_process_noise(1.e-30);
    f->s.Wc.set_process_noise(1.e-30);
    f->s.Tc.set_process_noise(1.e-30);
    f->s.a_bias.set_process_noise(1.e-10);
    f->s.w_bias.set_process_noise(1.e-12);
    //TODO: check this process noise
    f->s.focal_length.set_process_noise(1.e-2);
    f->s.center_x.set_process_noise(1.e-5);
    f->s.center_y.set_process_noise(1.e-5);
    f->s.k1.set_process_noise(1.e-6);
    f->s.k2.set_process_noise(1.e-6);
    f->s.k3.set_process_noise(1.e-6);

    f->vis_ref_noise = 1.e-30;
    f->vis_noise = 1.e-20;

    f->vis_cov = 2. * 2.;
    f->w_variance = f->device.w_meas_var;
    f->a_variance = f->device.a_meas_var;

    f->min_feats_per_group = 1;
    f->min_group_add = 16;
    f->max_group_add = 40;
    f->active = false;
    f->want_active = false;
    f->inertial_converged = false;
    f->s.maxstatesize = 120;
    f->frame = 0;
    f->max_feature_std_percent = .10;
    f->outlier_thresh = 1.5;
    f->outlier_reject = 30.;

    f->s.focal_length.v = f->device.Fx;
    f->s.center_x.v = f->device.Cx;
    f->s.center_y.v = f->device.Cy;
    f->s.k1.v = f->device.K[0];
    f->s.k2.v = f->device.K[1];
    f->s.k3.v = 0.; //f->device.K[2];

    f->s.Tc.v = v4(f->device.Tc[0], f->device.Tc[1], f->device.Tc[2], 0.);
    f->s.Wc.v = rotation_vector(f->device.Wc[0], f->device.Wc[1], f->device.Wc[2]);

    f->shutter_delay = f->device.shutter_delay;
    f->shutter_period = f->device.shutter_period;
    f->image_height = f->device.image_height;
    f->image_width = f->device.image_width;

    f->track.width = f->device.image_width;
    f->track.height = f->device.image_height;
    f->track.stride = f->track.width;
    f->track.init();
    
    f->s.total_distance = 0.;
}

extern "C" void filter_init(struct filter *f, struct corvis_device_parameters _device)
{
    //TODO: check init_cov stuff!!
    f->device = _device;
    filter_config(f);
    f->need_reference = true;
    state_node::statesize = 0;
    f->s.enable_orientation_only();
    f->s.remap();
    state_vision_feature::initial_depth_meters = M_E;
    state_vision_feature::initial_var = f->init_vis_cov;
    state_vision_feature::initial_process_noise = f->vis_noise;
    state_vision_feature::measurement_var = f->vis_cov;
    state_vision_feature::outlier_thresh = f->outlier_thresh;
    state_vision_feature::outlier_reject = f->outlier_reject;
    state_vision_feature::max_variance = f->max_feature_std_percent * f->max_feature_std_percent;
    state_vision_feature::min_add_vis_cov = f->min_add_vis_cov;
    state_vision_group::ref_noise = f->vis_ref_noise;
    state_vision_group::min_feats = f->min_feats_per_group;
}

float var_bounds_to_std_percent(f_t current, f_t begin, f_t end)
{
    return (sqrt(begin) - sqrt(current)) / (sqrt(begin) - sqrt(end));
}

float filter_converged(struct filter *f)
{
    if(f->run_static_calibration) {
        return f->accel_stability.count / 500.;
        /*f->s.remap();
        float min, pct;
        //return the max of the three a bias variances because we don't restrict orientation
        min = var_bounds_to_std_percent(f->s.a_bias.variance[0], BEGIN_ABIAS_VAR, END_ABIAS_VAR);
        pct = var_bounds_to_std_percent(f->s.a_bias.variance[1], BEGIN_ABIAS_VAR, END_ABIAS_VAR);
        if(pct > min) min = pct;
        pct = var_bounds_to_std_percent(f->s.a_bias.variance[2], BEGIN_ABIAS_VAR, END_ABIAS_VAR);
        if(pct > min) min = pct;
        pct = var_bounds_to_std_percent(f->s.w_bias.variance[0], BEGIN_WBIAS_VAR, END_WBIAS_VAR);
        if(pct < min) min = pct;
        pct = var_bounds_to_std_percent(f->s.w_bias.variance[1], BEGIN_WBIAS_VAR, END_WBIAS_VAR);
        if(pct < min) min = pct;
        pct = var_bounds_to_std_percent(f->s.w_bias.variance[2], BEGIN_WBIAS_VAR, END_WBIAS_VAR);
        if(pct < min) min = pct;
        return min < 0. ? 0. : min;*/
    } else return 1.;
}

bool filter_is_steady(struct filter *f)
{
    return
        norm(f->s.V.v) < .1 &&
        norm(f->s.w.v) < .1;
}

int filter_get_features(struct filter *f, struct corvis_feature_info *features, int max)
{
    int index = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        if(index >= max) break;
        features[index].id = (*fiter)->id;
        features[index].x = (*fiter)->current[0];
        features[index].y = (*fiter)->current[1];
        features[index].wx = (*fiter)->world[0];
        features[index].wy = (*fiter)->world[1];
        features[index].wz = (*fiter)->world[2];
        features[index].depth = (*fiter)->depth;
        features[index].stdev = (*fiter)->v.stdev_meters(sqrt((*fiter)->variance()));
        ++index;
    }
    return index;
}

void filter_get_camera_parameters(struct filter *f, float matrix[16], float focal_center_radial[5])
{
    focal_center_radial[0] = f->s.focal_length.v;
    focal_center_radial[1] = f->s.center_x.v;
    focal_center_radial[2] = f->s.center_y.v;
    focal_center_radial[3] = f->s.k1.v;
    focal_center_radial[4] = f->s.k2.v;

    //transpose for opengl
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            matrix[j * 4 + i] = f->s.camera_matrix[i][j];
        }
    }
}

void filter_start_static_calibration(struct filter *f)
{
    f->accel_stability = stdev_vector();
    f->gyro_stability = stdev_vector();
    f->run_static_calibration = true;
}

void filter_stop_static_calibration(struct filter *f)
{
    update_static_calibration(f);
    f->run_static_calibration = false;
}

void filter_start_processing_video(struct filter *f)
{
    f->want_active = true;
    f->want_start = f->last_time;
}

void filter_stop_processing_video(struct filter *f)
{
    f->active = false;
    f->s.enable_orientation_only();
    f->want_active = false;
    filter_reset_for_inertial(f);
}

void filter_start_processing_stereo(struct filter *f)
{
    f->stereo_enabled = true;
}

void filter_stop_processing_stereo(struct filter *f)
{
    f->stereo_enabled = false;
}

bool filter_stereo_preprocess(struct filter * f, uint8_t * current_frame)
{
    if(f->stereo_current_state.frame)
        stereo_free_state(f->stereo_current_state);
    f->stereo_current_state = stereo_save_state(f, current_frame);
    enum stereo_status_code result = stereo_preprocess(f->stereo_previous_state, f->stereo_current_state, f->stereo_F);
    if(result != stereo_status_success)
        fprintf(stderr, "stereo preprocessing failure");

    return result == stereo_status_success;
}

bool filter_stereo_triangulate(struct filter * f, int x, int y, v4 & interesection)
{
    if(!f->stereo_current_state.frame)
        return false;

    enum stereo_status_code result = stereo_triangulate(f->stereo_previous_state, f->stereo_current_state, f->stereo_F, x, y, interesection);
    return result == stereo_status_success;
}

v4 filter_stereo_baseline(struct filter *f)
{
    if(!f->stereo_previous_state.frame)
        return v4(0,0,0,0);

    return stereo_baseline(f, f->stereo_previous_state);
}

void filter_select_feature(struct filter *f, float x, float y)
{
    //first, see if we already have a feature there
    float mydist = 8; // 8 pixel radius max
    state_vision_feature *myfeat = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        state_vision_feature *feat = *fiter;
        if(feat->status != feature_normal && feat->status != feature_ready && feat->status != feature_initializing) continue;
        float dx = fabs(feat->current[0] - x);
        float dy = fabs(feat->current[1] - y);
        if(dx <= mydist && dy <= mydist) { //<= so we get full 8 pixel range and we default to younger features
            myfeat = feat;
            mydist = (dx < dy) ? dy : dx;
        }
    }
    if(!myfeat) {
        //didn't find an existing feature - select a new one
        //f->track.maxfeats is not necessarily a hard limit, so don't worry if we don't have room for a feature
        vector<xy> kp = f->track.detect(f->track.im2, NULL, 1, x - 8, y - 8, 17, 17);
        if(kp.size() > 0) {
            myfeat = f->s.add_feature(kp[0].x, kp[0].y);
            int lx = floor(kp[0].x);
            int ly = floor(kp[0].y);
            myfeat->intensity = (((unsigned int)f->track.im2[lx + ly*f->track.width]) + f->track.im2[lx + 1 + ly * f->track.width] + f->track.im2[lx + f->track.width + ly * f->track.width] + f->track.im2[lx + 1 + f->track.width + ly * f->track.width]) >> 2;
        }
    }
    if(!myfeat) return; //couldn't find anything
    myfeat->user = true;
    f->s.remap();
}
