// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <spdlog/fmt/ostr.h> // must be included to use our operator<<
#include "state_vision.h"
#include "vec4.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "kalman.h"
#include "matrix.h"
#include "observation.h"
#include "filter.h"
#include <memory>
#include "fast_tracker.h"
#ifdef HAVE_IPP
#include "ipp_tracker.h"
#endif

const static sensor_clock::duration min_steady_time = std::chrono::microseconds(100000); //time held steady before we start treating it as steady
const static sensor_clock::duration steady_converge_time = std::chrono::microseconds(200000); //time that user needs to hold steady (us)
const static int calibration_converge_samples = 200; //number of accelerometer readings needed to converge in calibration mode
const static f_t accelerometer_steady_var = .15*.15; //variance when held steady, based on std dev measurement of iphone 5s held in hand
const static f_t velocity_steady_var = .1 * .1; //initial var of state.V when steady
const static f_t accelerometer_inertial_var = 2.33*2.33; //variance when in inertial only mode
const static f_t static_sigma = 6.; //how close to mean measurements in static mode need to be
const static f_t steady_sigma = 3.; //how close to mean measurements in steady mode need to be - lower because it is handheld motion, not gaussian noise
const static f_t dynamic_W_thresh_variance = 5.e-2; // variance of W must be less than this to initialize from dynamic mode
//a_bias_var for best results on benchmarks is 6.4e-3
const static f_t min_a_bias_var = 1.e-6; // calibration will finish immediately when variance of a_bias is less than this, and it is reset to this between each run
const static f_t min_w_bias_var = 1.e-8; // variance of w_bias is reset to this between each run
const static f_t max_accel_delta = 20.; //This is biggest jump seen in hard shaking of device
const static f_t max_gyro_delta = 5.; //This is biggest jump seen in hard shaking of device
const static sensor_clock::duration qr_detect_period = std::chrono::microseconds(100000); //Time between checking frames for QR codes to reduce CPU usage
const static f_t convergence_minimum_velocity = 0.3; //Minimum speed (m/s) that the user must have traveled to consider the filter converged
const static f_t convergence_maximum_depth_variance = .001; //Median feature depth must have been under this to consider the filter converged
//TODO: homogeneous coordinates.

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
                f->log->warn("{}\t{}\t: sign flip: expected {}, got {}", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                f->log->warn("{}\t{}\t: lin error: expected {}, got {}", i, j, ldiff, delta);
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
                f->log->warn("{}\t{}\t: sign flip: expected {}, got {}", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                f->log->warn("{}\t{}\t: lin error: expected {}, got {}", i, j, ldiff, delta);
                continue;
            }
        }
    }
    f->s.copy_state_from_array(save_state);
}
*/

void filter_update_outputs(struct filter *f, sensor_clock::time_point time)
{
    if(f->run_state != RCSensorFusionRunStateRunning) return;

    bool old_speedfail = f->speed_failed;
    f->speed_failed = false;
    f_t speed = f->s.V.v.norm();
    if(speed > 3.) { //1.4m/s is normal walking speed
        if (!old_speedfail) f->log->info("Velocity {} m/s exceeds max bound", speed);
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(speed > 2.) {
        if (!f->speed_warning) f->log->info("High velocity ({} m/s)", speed);
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t accel = f->s.a.v.norm();
    if(accel > 9.8) { //1g would saturate sensor anyway
        if (!old_speedfail) f->log->info("Acceleration exceeds max bound");
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(accel > 5.) { //max in mine is 6.
        if (!f->speed_warning) f->log->info("High acceleration ({} m/s^2)", accel);
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t ang_vel = f->s.w.v.norm();
    if(ang_vel > 5.) { //sensor saturation - 250/180*pi
        if (!old_speedfail) f->log->info("Angular velocity exceeds max bound");
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(ang_vel > 2.) { // max in mine is 1.6
        if (!f->speed_warning) f->log->info("High angular velocity");
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    //if(f->speed_warning && filter_converged(f) < 1.) f->speed_failed = true;
    if(time - f->speed_warning_time > std::chrono::microseconds(1000000)) f->speed_warning = false;

    //f->log->trace("{} [{} {} {}] [{} {} {}]", time, output[0], output[1], output[2], output[3], output[4], output[5]);
}

static bool filter_mini_process_observation_queue(struct filter * f, observation_queue &queue, state_root &state, const sensor_clock::time_point & time)
{
    queue.preprocess(state, time);
    if(!queue.process(state)) {
        f->log->error("mini state observation failed\n");
        return false;
    }
    return true;
}

bool filter_mini_accelerometer_measurement(struct filter * f, observation_queue &queue, state_motion &state, const sensor_data &data)
{
    if(data.id >= f->accelerometers.size() || data.id >= f->s.imus.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();
    auto &accelerometer = *f->accelerometers[data.id];
    auto &imu = *state.imus.children[data.id];
    v3 meas = map(accelerometer.intrinsics.scale_and_alignment.v) * map(data.acceleration_m__s2.v);

    //TODO: if out of order, project forward in time
    
    auto obs_a = std::make_unique<observation_accelerometer>(accelerometer, state, imu.extrinsics, imu.intrinsics);
    obs_a->meas = meas;
    obs_a->variance = accelerometer.measurement_variance;

    queue.observations.push_back(std::move(obs_a));
    bool ok = filter_mini_process_observation_queue(f, queue, state, data.timestamp);
    auto stop = std::chrono::steady_clock::now();
    accelerometer.other_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    return ok;
}

bool filter_mini_gyroscope_measurement(struct filter * f, observation_queue &queue, state_motion &state, const sensor_data &data)
{
    if(data.id >= f->gyroscopes.size() || data.id >= f->s.imus.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();
    
    auto &gyroscope = *f->gyroscopes[data.id];
    auto &imu = *state.imus.children[data.id];
    v3 meas = map(gyroscope.intrinsics.scale_and_alignment.v) * map(data.angular_velocity_rad__s.v);

    //TODO: if out of order, project forward in time
    
    auto obs_w = std::make_unique<observation_gyroscope>(gyroscope, state, imu.extrinsics, imu.intrinsics);
    obs_w->meas = meas;
    obs_w->variance = gyroscope.measurement_variance;

    queue.observations.push_back(std::move(obs_w));
    bool ok = filter_mini_process_observation_queue(f, queue, state, data.timestamp);

    auto stop = std::chrono::steady_clock::now();
    gyroscope.other_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    return ok;
}

static void preprocess_observation_queue(struct filter *f, sensor_clock::time_point time)
{
    f->last_time = time;
    f->observations.preprocess(f->s, time);
}

static void process_observation_queue(struct filter *f)
{
    if(!f->observations.process(f->s)) {
        f->numeric_failed = true;
        f->calibration_bad = true;
    }
    f_t delta_T = (f->s.T.v - f->s.last_position).norm();
    if(delta_T > .01) {
        f->s.total_distance += (float)delta_T;
        f->s.last_position = f->s.T.v;
    }
}

void filter_compute_gravity(struct filter *f, double latitude, double altitude)
{
    assert(f); f->s.compute_gravity(latitude, altitude);
}

static void update_static_calibration(struct filter *f, state_imu &imu, sensor_accelerometer &a, sensor_gyroscope &g)
{
    if(g.stability.count < calibration_converge_samples) return;
    a.measurement_variance = a.stability.variance.array().mean();
    g.measurement_variance = g.stability.variance.array().mean();
#ifdef _WIN32
    //WARNING HACK - floor set at milhone values
    if (a.measurement_variance <   .005) a.measurement_variance =   .005;
    if (g.measurement_variance < .00002) g.measurement_variance = .00002;
    //TODO: get rid of this (figure out how to deal with quantized sensor data)
#endif
    //this updates even the one dof that can't converge in the filter for this orientation (since we were static)
    imu.intrinsics.w_bias.v = g.stability.mean;
    imu.intrinsics.w_bias.set_initial_variance(g.stability.variance); //Even though the one dof won't have converged in the filter, we know that this is a good value (average across stable meas).
    imu.intrinsics.w_bias.reset_covariance(f->s.cov);
}

static void reset_stability(struct filter *f)
{
    for (auto &g : f->gyroscopes)     g->stability = stdev<3>();
    for (auto &a : f->accelerometers) a->stability = stdev<3>();
    f->stable_start = sensor_clock::time_point(sensor_clock::duration(0));
}

//TODOMSM - this could be done per-sensor, or just using a single accelerometer (simpler), which should be fine
sensor_clock::duration steady_time(struct filter *f, stdev<3> &stdev, const v3 &meas, f_t variance, f_t sigma, sensor_clock::time_point time, const v3 &orientation, bool use_orientation)
{
    bool steady = false;
    if(stdev.count) {
        //hysteresis - tighter tolerance for getting into calibration, but looser for staying in
        f_t sigma2 = sigma * sigma;
        if(time - f->stable_start < min_steady_time) sigma2 *= .5*.5;
        steady = true;
        for(int i = 0; i < 3; ++i) {
            f_t delta = meas[i] - stdev.mean[i];
            f_t d2 = delta * delta;
            if(d2 > variance * sigma2) steady = false;
        }
    }
    if(!steady) {
        reset_stability(f);
        f->stable_start = time;
    }
    if(!stdev.count && use_orientation) {
        if(!f->s.orientation_initialized) return sensor_clock::duration(0);
        v3 local_up = f->s.Q.v.conjugate() * f->s.world.up;
        //face up -> (0, 0, 1)
        //portrait -> (1, 0, 0)
        //landscape -> (0, 1, 0)
        f_t costheta = orientation.dot(local_up);
        if(fabs(costheta) < .71) return sensor_clock::duration(0); //don't start since we aren't in orientation +/- 6 deg
    }
    stdev.data(meas);
    
    return time - f->stable_start;
}

static void print_calibration(struct filter *f)
{
    for (const auto &imu : f->s.imus.children) {
        f->log->trace("w bias is: {}, {}, {}",     imu->intrinsics.w_bias.v[0],          imu->intrinsics.w_bias.v[1],          imu->intrinsics.w_bias.v[2]);
        f->log->trace("w bias var is: {}, {}, {}", imu->intrinsics.w_bias.variance()[0], imu->intrinsics.w_bias.variance()[1], imu->intrinsics.w_bias.variance()[2]);
        f->log->trace("a bias is: {}, {}, {}",     imu->intrinsics.a_bias.v[0],          imu->intrinsics.a_bias.v[1],          imu->intrinsics.a_bias.v[2]);
        f->log->trace("a bias var is: {}, {}, {}", imu->intrinsics.a_bias.variance()[0], imu->intrinsics.a_bias.variance()[1], imu->intrinsics.a_bias.variance()[2]);
    }
}

static float var_bounds_to_std_percent(f_t current, f_t begin, f_t end)
{
    return current < end ? 1.f : (float) ((log(begin) - log(current)) / (log(begin) - log(end))); //log here seems to give smoother progress
}

//TODOMSM - this should check all sensors and return the least converged one
static float get_bias_convergence(const struct filter *f, const state_imu &imu, const sensor_accelerometer &a, int dir)
{
    float max_pct = (float)var_bounds_to_std_percent(imu.intrinsics.a_bias.variance()[dir], a.start_variance[dir], min_a_bias_var);
    float pct = (float)a.stability.count / (float)calibration_converge_samples;
    if(f->last_time - f->stable_start < min_steady_time) pct = 0.f;
    if(pct > max_pct) max_pct = pct;
    if(max_pct < 0.f) max_pct = 0.f;
    if(max_pct > 1.f) max_pct = 1.f;
    return max_pct;
}

//TODOMSM - this should be per-sensor
static f_t get_accelerometer_variance_for_run_state(struct filter *f, state_imu &imu, sensor_accelerometer &accelerometer, sensor_gyroscope &gyroscope, const v3 &meas, sensor_clock::time_point time)
{
    if(!f->s.orientation_initialized) return accelerometer_inertial_var; //first measurement is not used, so this doesn't actually matter
    switch(f->run_state)
    {
        case RCSensorFusionRunStateRunning:
        case RCSensorFusionRunStateInactive: //shouldn't happen
            return accelerometer.measurement_variance;
        case RCSensorFusionRunStateDynamicInitialization:
            return accelerometer_inertial_var;
        case RCSensorFusionRunStateInertialOnly:
            return accelerometer_inertial_var;
        case RCSensorFusionRunStateStaticCalibration:
            if(steady_time(f, accelerometer.stability, meas, accelerometer.measurement_variance, static_sigma, time, v3(0, 0, 1), true) > min_steady_time)
            {
                f->s.enable_bias_estimation();
                //base this on # samples instead of variance because we are also estimating a, w variance here
                if(accelerometer.stability.count >= calibration_converge_samples && get_bias_convergence(f, imu, accelerometer, 2) >= 1.)
                {
                    update_static_calibration(f, imu, accelerometer, gyroscope);
                    f->run_state = RCSensorFusionRunStatePortraitCalibration;
                    accelerometer.start_variance = imu.intrinsics.a_bias.variance();
                    gyroscope.start_variance = imu.intrinsics.w_bias.variance();
                    reset_stability(f);
                    f->s.disable_bias_estimation();
                    f->log->trace("When finishing static calibration:");
                    print_calibration(f);
                }
                return accelerometer.measurement_variance * 3 * 3; //pump up this variance because we aren't really perfect here
            }
            else
            {
                f->s.disable_bias_estimation();
                return accelerometer_inertial_var;
            }
        case RCSensorFusionRunStatePortraitCalibration:
        {
            if(steady_time(f, accelerometer.stability, meas, accelerometer_steady_var, steady_sigma, time, v3(1, 0, 0), true) > min_steady_time)
            {
                f->s.enable_bias_estimation();
                if(get_bias_convergence(f, imu, accelerometer, 0) >= 1.)
                {
                    f->run_state = RCSensorFusionRunStateLandscapeCalibration;
                    accelerometer.start_variance = imu.intrinsics.a_bias.variance();
                    gyroscope.start_variance = imu.intrinsics.w_bias.variance();
                    reset_stability(f);
                    f->s.disable_bias_estimation();
                    f->log->trace("When finishing portrait calibration:");
                    print_calibration(f);
                }
                return accelerometer_steady_var;
            }
            else
            {
                f->s.disable_bias_estimation();
                return accelerometer_inertial_var;
            }
        }
        case RCSensorFusionRunStateLandscapeCalibration:
        {
            if(steady_time(f, accelerometer.stability, meas, accelerometer_steady_var, steady_sigma, time, v3(0, 1, 0), true) > min_steady_time)
            {
                f->s.enable_bias_estimation();
                if(get_bias_convergence(f, imu, accelerometer, 1) >= 1.)
                {
                    f->run_state = RCSensorFusionRunStateInactive;
                    reset_stability(f);
                    f->s.disable_bias_estimation();
                    f->log->trace("When finishing landscape calibration:");
                    print_calibration(f);
                }
                return accelerometer_steady_var;
            }
            else
            {
                f->s.disable_bias_estimation();
                return accelerometer_inertial_var;
            }
        }
        case RCSensorFusionRunStateSteadyInitialization:
        {
            auto steady = steady_time(f, accelerometer.stability, meas, accelerometer_steady_var, steady_sigma, time, v3(), false);
            if(steady > min_steady_time)
            {
                f->s.enable_bias_estimation();
                if(steady > steady_converge_time) {
                    f->s.V.set_initial_variance(velocity_steady_var);
                    f->s.a.set_initial_variance(accelerometer_steady_var);
                }
                return accelerometer_steady_var;
            }
            else
            {
                f->s.disable_bias_estimation();
                return accelerometer_inertial_var;
            }
        }
    }
    assert(0); //should never fall through to here;
    return accelerometer.measurement_variance;
}

bool filter_accelerometer_measurement(struct filter *f, const sensor_data &data)
{
    if(data.id >= f->accelerometers.size() || data.id >= f->s.imus.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();
    auto timestamp = data.timestamp;
    auto &accelerometer = *f->accelerometers[data.id];
    auto &gyroscope     = *f->gyroscopes[data.id];
    auto &imu = *f->s.imus.children[data.id];
    v3 meas = map(accelerometer.intrinsics.scale_and_alignment.v) * map(data.acceleration_m__s2.v);
    v3 accel_delta = meas - accelerometer.last_meas;
    accelerometer.last_meas = meas;
    //This will throw away both the outlier measurement and the next measurement, because we update last every time. This prevents setting last to an outlier and never recovering.
    if(f->run_state == RCSensorFusionRunStateInactive) return false;
    if(!accelerometer.got) { //skip first packet - has been crap from gyro
        accelerometer.got = true;
        return false;
    }
    if (!f->got_any_gyroscopes()) return false;

    if(fabs(accel_delta[0]) > max_accel_delta || fabs(accel_delta[1]) > max_accel_delta || fabs(accel_delta[2]) > max_accel_delta)
    {
        f->log->warn("Extreme jump in accelerometer {} {} {}", accel_delta[0], accel_delta[1], accel_delta[2]);
    }

    if(!f->s.orientation_initialized) {
        f->s.orientation_initialized = true;
        f->s.Q.v = initial_orientation_from_gravity_facing(f->s.world.up, imu.extrinsics.Q.v * meas,
                                                           f->s.world.initial_forward, f->s.body_forward);
        if(f->origin_set)
            f->origin.Q = f->origin.Q * f->s.Q.v.conjugate();

        preprocess_observation_queue(f, timestamp);
        return true;
    }

    auto obs_a = std::make_unique<observation_accelerometer>(accelerometer, f->s, imu.extrinsics, imu.intrinsics);
    obs_a->meas = meas;
    obs_a->variance = get_accelerometer_variance_for_run_state(f, imu, accelerometer, gyroscope, meas, timestamp);

    f->observations.observations.push_back(std::move(obs_a));

    if(show_tuning) fprintf(stderr, "\naccelerometer:\n");
    preprocess_observation_queue(f, timestamp);
    process_observation_queue(f);
    if(show_tuning) {
        std::cerr << " meas   " << meas << "\n"
                  << " innov  " << accelerometer.inn_stdev
                  << " signal "       << accelerometer.meas_stdev
                  // FIXME: these should be for this accelerometer->
                  << " bias is: " << imu.intrinsics.a_bias.v << " stdev is: " << imu.intrinsics.a_bias.variance().array().sqrt() << "\n";
    }

    auto stop = std::chrono::steady_clock::now();
    accelerometer.measure_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    return true;
}

bool filter_gyroscope_measurement(struct filter *f, const sensor_data & data)
{
    if(data.id >= f->gyroscopes.size() || data.id >= f->s.imus.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();
    auto timestamp = data.timestamp;
    auto &gyroscope = *f->gyroscopes[data.id];
    auto &imu = *std::next(f->s.imus.children.begin(), data.id)->get();
    v3 meas = map(gyroscope.intrinsics.scale_and_alignment.v) * map(data.angular_velocity_rad__s.v);
    v3 gyro_delta = meas - gyroscope.last_meas;
    gyroscope.last_meas = meas;
    //This will throw away both the outlier measurement and the next measurement, because we update last every time. This prevents setting last to an outlier and never recovering.
    if(f->run_state == RCSensorFusionRunStateInactive) return false;
    if(!gyroscope.got) { //skip the first piece of data as it seems to be crap
        gyroscope.got = true;
        return false;
    }
    if(!f->s.orientation_initialized) return false;

    if(fabs(gyro_delta[0]) > max_gyro_delta || fabs(gyro_delta[1]) > max_gyro_delta || fabs(gyro_delta[2]) > max_gyro_delta)
    {
        f->log->warn("Extreme jump in gyro {} {} {}", gyro_delta[0], gyro_delta[1], gyro_delta[2]);
    }

    auto obs_w = std::make_unique<observation_gyroscope>(gyroscope, f->s, imu.extrinsics, imu.intrinsics);
    obs_w->meas = meas;
    obs_w->variance = gyroscope.measurement_variance;

    f->observations.observations.push_back(std::move(obs_w));

    if(f->run_state == RCSensorFusionRunStateStaticCalibration)
        gyroscope.stability.data(meas);

    if(show_tuning) fprintf(stderr, "\ngyroscope:\n");
    preprocess_observation_queue(f, timestamp);
    process_observation_queue(f);
    if(show_tuning) {
        std::cerr << " meas   " << meas << "\n"
                  << " innov  " << gyroscope.inn_stdev
                  << " signal " << gyroscope.meas_stdev
                  // FIXME: these should be for this gyroscope->
                  << " bias " << imu.intrinsics.w_bias.v << " stdev is: " << imu.intrinsics.w_bias.variance().array().sqrt() << "\n";
    }

    auto stop = std::chrono::steady_clock::now();
    gyroscope.measure_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    return true;
}

static void filter_setup_next_frame(struct filter *f, const sensor_data &data)
{
    auto &camera_sensor = *f->cameras[data.id];
    auto &camera_state = *f->s.cameras.children[data.id];

    auto timestamp = data.timestamp;

    for(state_vision_group *g : camera_state.groups.children) {
        if(!g->status || g->status == group_initializing) continue;
        for(state_vision_feature *i : g->features.children) {
            auto obs = std::make_unique<observation_vision_feature>(camera_sensor, camera_state.extrinsics, camera_state.intrinsics);
            obs->feature = i;
            f->observations.observations.push_back(std::move(obs));
        }
    }
}

static uint16_t get_depth_for_point_mm(const rc_ImageData &depth, const feature_t & p)
{
    auto x = (int)p.x(), y = (int)p.y();
    if (x >=0 && x < depth.width && y >= 0 && y < depth.height)
        return ((uint16_t *)depth.image)[depth.stride / sizeof(uint16_t) * y + x];
    else
        return 0;
}

static float get_stdev_pct_for_depth(float depth_m)
{
    return 0.0023638192164147698f + (0.0015072367800769945f + 0.00044245048102432134f * depth_m) * depth_m;
}

static std::unique_ptr<sensor_data> filter_aligned_depth_to_camera(const sensor_data &depth, const sensor_depth &depth_sensor, state_camera &camera_state, sensor_grey &camera_sensor)
{
    const auto &intrinsics = depth_sensor.intrinsics;
    const auto &extrinsics = depth_sensor.extrinsics.mean;
    if (intrinsics.type == rc_CALIBRATION_TYPE_UNDISTORTED && extrinsics == camera_sensor.extrinsics.mean)
        return depth.make_copy();

    auto aligned_depth = depth.make_copy();

    int i_width =         depth.depth.width, i_height =           depth.depth.height, i_stride =         depth.depth.stride / sizeof(uint16_t);
    int o_width = aligned_depth->depth.width, o_height = aligned_depth->depth.height, o_stride = aligned_depth->depth.stride / sizeof(uint16_t);
    uint16_t *in  = (uint16_t *)depth.depth.image;
    uint16_t *out = (uint16_t *)aligned_depth->depth.image;

    for (int y = 0; y < i_height; y++)
        for (int x = 0; x < i_width; x++)
            out[y * o_stride + x] = std::numeric_limits<uint16_t>::max();

    float d_focal_length_x =  intrinsics.f_x_px                                   / intrinsics.height_px,
          d_focal_length_y =  intrinsics.f_y_px                                   / intrinsics.height_px,
          d_center_x       = (intrinsics.c_x_px - intrinsics.width_px  / 2. + .5) / intrinsics.height_px,
          d_center_y       = (intrinsics.c_y_px - intrinsics.height_px / 2. + .5) / intrinsics.height_px;

    float i_Cx = d_center_x       * i_height + (i_width  - 1) / 2.,
          i_Cy = d_center_y       * i_height + (i_height - 1) / 2.,
          i_Fx = d_focal_length_x * i_height,
          i_Fy = d_focal_length_y * i_height;

    float o_Cx = camera_state.intrinsics.center_x.v     * o_height + (o_width  - 1) / 2.,
          o_Cy = camera_state.intrinsics.center_y.v     * o_height + (o_height - 1) / 2.,
          o_Fx = camera_state.intrinsics.focal_length.v * o_height,
          o_Fy = camera_state.intrinsics.focal_length.v * o_height;

    transformation depth_to_color_m = invert(transformation(camera_state.extrinsics.Q.v,camera_state.extrinsics.T.v)) * extrinsics;
    auto x_T_mm = (depth_to_color_m.T * 1000).cast<float>();

    for (int y = 0; y < i_height; y++)
        for (int x = 0; x < i_width; x++) {
            uint16_t z = in[y * i_stride + x];
            if (!z) continue;
            float xf = x, yf = y, zf = z;

            // normalize, undistort(?), unproject and transform
            float ix = zf * (xf - i_Cx) / i_Fx + x_T_mm[0];
            float iy = zf * (yf - i_Cy) / i_Fy + x_T_mm[1];
            float iz = zf                      + x_T_mm[2];

            // project, distort(?), unnormalize
            float ox = o_Fx * ix / iz + o_Cx;
            float oy = o_Fy * iy / iz + o_Cy;
            float oz = iz;

            // ceil() and -1s give the 4 closest grid points
            Eigen::Array4i X = static_cast<int>(std::ceil(ox)) - Eigen::Array4i{0,1,0,1};
            Eigen::Array4i Y = static_cast<int>(std::ceil(oy)) - Eigen::Array4i{0,0,1,1};
            //auto Z = static_cast<int>(roundf(oz));
            auto I = Y * o_stride + X;
            auto within = X >= 0 && X < o_width && Y >= 0 && Y < o_height;
            if (within[0] && oz < static_cast<float>(out[I[0]])) out[I[0]] = static_cast<uint16_t>(oz);
            if (within[1] && oz < static_cast<float>(out[I[1]])) out[I[1]] = static_cast<uint16_t>(oz);
            if (within[2] && oz < static_cast<float>(out[I[2]])) out[I[2]] = static_cast<uint16_t>(oz);
            if (within[3] && oz < static_cast<float>(out[I[3]])) out[I[3]] = static_cast<uint16_t>(oz);
        }

    for (int Y = 0; Y < o_height; Y++)
        for (int X = 0; X < o_width; X++)
            if (out[Y * o_stride + X] == std::numeric_limits<uint16_t>::max())
                out[Y * o_stride + X] = 0;

    return aligned_depth;
}

/*
static std::unique_ptr<image_depth16> filter_aligned_depth_overlay(const struct filter *f, const image_depth16 &depth, const image_gray8 & image)
{
    state_camera &grey = f->s.cameras[image.id];
    sensor_depth &depth_sensor = f->depths[depth.id];
    std::unique_ptr<image_depth16> aligned_depth = filter_aligned_depth_to_camera(depth, depth_sensor, grey, grey_sensor);

    auto aligned_distorted_depth = make_unique<image_depth16>(image.width, image.height, 0);
    auto out = aligned_distorted_depth->image;
    int out_stride = aligned_distorted_depth->stride / sizeof(*out);
    // This assumes depth and image have the same aspect ratio
    f_t image_to_depth = f_t(depth.height)/image.height;
    for(int y_image = 0; y_image < image.height; y_image++) {
        for(int x_image = 0; x_image < image.width; x_image++) {
            feature_t kp_i = {(f_t)x_image, (f_t)y_image};
            feature_t kp_d = image_to_depth*grey.intrinsics.unnormalize_feature(grey.intrinsics.undistort_feature(grey.intrinsics.normalize_feature(kp_i)));
            uint16_t depth_mm = get_depth_for_point_mm(*aligned_depth.get(), kp_d);
            out[y_image * out_stride + x_image] = depth_mm;
        }
    }

    return aligned_distorted_depth;
}
*/

static int filter_add_detected_features(struct filter * f, state_vision_group *g, sensor_grey &camera_sensor, const std::vector<tracker::point> &kp, size_t newfeats, int image_height)
{
    state_camera &camera = g->camera;
    // give up if we didn't get enough features
    if(kp.size() < state_vision_group::min_feats) {
        camera.remove_group(g, f->map.get());
        f->s.remap();
        for(const auto &p : kp)
            camera.feature_tracker->drop_feature(p.id);
        int active_features = camera.feature_count();
        if(active_features < state_vision_group::min_feats) {
            f->log->info("detector failure: only {} features after add", active_features);
            f->detector_failed = true;
            f->calibration_bad = true;
        }
        return 0;
    }

    f->detector_failed = false;

    std::unique_ptr<sensor_data> aligned_undistorted_depth;

    int found_feats = 0;
    int i;
    f_t image_to_depth = 1;
    if(f->has_depth)
        image_to_depth = f_t(f->recent_depth->image.height)/image_height;
    for(i = 0; i < (int)kp.size(); ++i) {
        feature_t kp_i = {kp[i].x, kp[i].y};
        {
            state_vision_feature *feat = f->s.add_feature(*g, kp_i);

            float depth_m = 0;
            if(f->has_depth) {
                if (!aligned_undistorted_depth)
                    aligned_undistorted_depth = filter_aligned_depth_to_camera(*f->recent_depth, *f->depths[f->recent_depth->id], camera, camera_sensor);

                depth_m = 0.001f * get_depth_for_point_mm(aligned_undistorted_depth->depth, image_to_depth*camera.intrinsics.unnormalize_feature(camera.intrinsics.undistort_feature(camera.intrinsics.normalize_feature(kp_i))));
            }
            if(depth_m)
            {
                feat->v.set_depth_meters(depth_m);
                float std_pct = get_stdev_pct_for_depth(depth_m);
                feat->set_initial_variance(std_pct * std_pct); // assumes log depth
                feat->status = feature_normal;
                feat->depth_measured = true;
            }
            
            g->features.children.push_back(feat);
            feat->tracker_id = kp[i].id;
            
            found_feats++;
            if(found_feats == newfeats) break;
        }
    }
    for(i = i+1; i < (int)kp.size(); ++i)
        camera.feature_tracker->drop_feature(kp[i].id);

    g->status = group_initializing;
    g->make_normal();
    f->s.remap();
#ifdef TEST_POSDEF
    if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def after adding features");
#endif
    return found_feats;
}

static int filter_available_feature_space(struct filter *f, state_camera &camera)
{
    int space = f->s.maxstatesize - f->s.fake_statesize - f->s.statesize;
    int empty = 0;
    for (auto &c : f->s.cameras.children) {
        if (!c->detecting_group)
            space -= 6;
        empty += c->groups.children.size() == 0;
    }
    if (empty)
        space /= empty;
    if(space < 0) space = 0;
    if(space > f->max_group_add)
        space = f->max_group_add;
    return space;
}

const std::vector<tracker::point> & filter_detect(struct filter *f, const sensor_data &data)
{
    sensor_grey &camera_sensor = *f->cameras[data.id];
    state_camera &camera = *f->s.cameras.children[data.id];
    auto start = std::chrono::steady_clock::now();
    int space = filter_available_feature_space(f, camera);
    const rc_ImageData &image = data.image;
    camera.feature_tracker->current_features.clear();
    camera.feature_tracker->current_features.reserve(camera.feature_count());
    if (!space)
        return camera.feature_tracker->current_features;
    for(auto &g : camera.groups.children)
        for(auto &i : g->features.children)
            camera.feature_tracker->current_features.emplace_back(i->tracker_id, (float)i->current[0], (float)i->current[1], 0);

    // Run detector
    tracker::image timage;
    timage.image = (uint8_t *)image.image;
    timage.width_px = image.width;
    timage.height_px = image.height;
    timage.stride_px = image.stride;
    std::vector<tracker::point> &kp = camera.feature_tracker->detect(timage, camera.feature_tracker->current_features, space);

    auto stop = std::chrono::steady_clock::now();
    camera_sensor.other_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    return kp;
}

bool filter_depth_measurement(struct filter *f, const sensor_data & data)
{
    if(data.id != 0) return true;

    f->recent_depth = data.make_copy();
    f->has_depth = true;
    return true;
}

bool filter_image_measurement(struct filter *f, const sensor_data & data)
{
    if(data.id >= f->cameras.size() || data.id >= f->s.cameras.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();

    auto &camera_sensor = *f->cameras[data.id];
    auto &camera_state = *f->s.cameras.children[data.id];

    sensor_clock::time_point time = data.timestamp;

    if(f->run_state == RCSensorFusionRunStateInactive) return false;
    if(!f->got_any_accelerometers() || !f->got_any_gyroscopes()) return false;

#ifdef ENABLE_QR
    if (f->qr.running || f->qr_bench.enabled) {
        transformation world(f->s.Q.v, f->s.T.v);
        auto calibrate = [&](feature_t un) { return camera_state.intrinsics.undistort_feature(camera_state.intrinsics.normalize_feature(un)); };
        if(f->qr.running && (time - f->last_qr_time > qr_detect_period)) {
            f->last_qr_time = time;
            f->qr.process_frame(world, calibrate, data.image.image, data.image.width, data.image.height);
            if(f->qr.valid)
                filter_set_qr_origin(f, f->qr.origin, f->qr_origin_gravity_aligned);
        }
        if(f->qr_bench.enabled)
            f->qr_bench.process_frame(f, data.image.image, data.image.width, data.image.height);
    }
#endif

    camera_sensor.got = true;

    if(f->run_state == RCSensorFusionRunStateDynamicInitialization) {
        if(f->want_start == sensor_clock::micros_to_tp(0)) f->want_start = time;
        f->last_time = time;
        v3 non_up_var = f->s.Q.variance() - f->s.world.up * f->s.world.up.dot(f->s.Q.variance());
        bool inertial_converged = non_up_var[0] < dynamic_W_thresh_variance && non_up_var[1] < dynamic_W_thresh_variance && non_up_var[2] < dynamic_W_thresh_variance;
        if(inertial_converged) {
            if(inertial_converged) {
                f->log->debug("Inertial converged at time {}", std::chrono::duration_cast<std::chrono::microseconds>(time - f->want_start).count());
            } else {
                f->log->warn("Inertial did not converge {}", non_up_var);
            }
        } else return true;
    }
    if(f->run_state == RCSensorFusionRunStateSteadyInitialization) {
        if(f->stable_start == sensor_clock::micros_to_tp(0)) return true;
        if(time - f->stable_start < steady_converge_time) return true;
    }
    if(f->run_state != RCSensorFusionRunStateRunning && f->run_state != RCSensorFusionRunStateDynamicInitialization && f->run_state != RCSensorFusionRunStateSteadyInitialization) return true; //frame was "processed" so that callbacks still get called

    camera_state.intrinsics.image_width = data.image.width;
    camera_state.intrinsics.image_height = data.image.height;
    
    if(camera_state.detecting_group)
    {
#ifdef TEST_POSDEF
        if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def before adding features");
#endif
        if(camera_state.detection_future.valid()) {
            const auto & kp = camera_state.detection_future.get();
            int space = filter_available_feature_space(f, camera_state);
            filter_add_detected_features(f, camera_state.detecting_group, camera_sensor, kp, space, data.image.height);
        } else {
            camera_state.remove_group(camera_state.detecting_group, f->map.get());
            f->s.remap();
        }
        camera_state.detecting_group = nullptr;
    }

    if(f->run_state == RCSensorFusionRunStateRunning)
        filter_setup_next_frame(f, data); // put current features into observation queue as potential things to measure

    if(show_tuning) {
        fprintf(stderr, "\nvision:\n");
    }
    preprocess_observation_queue(f, time); // time update filter, then predict locations of current features in the observation queue
    camera_state.update_feature_tracks(data.image); // track the current features near their predicted locations
    process_observation_queue(f); // update state and covariance based on current location of tracked features
    if(show_tuning) {
        for (auto &c : f->cameras)
            std::cerr << " innov  " << c->inn_stdev << "\n";
    }

    int features_used = f->s.process_features(camera_state, data.image, f->map.get());
    if(!features_used)
    {
        //Lost all features - reset convergence
        f->has_converged = false;
        f->max_velocity = 0.;
        f->median_depth_variance = 1.;
    }
    
    f->median_depth_variance = f->s.median_depth_variance();
    
    float velocity = (float)f->s.V.v.norm();
    if(velocity > f->max_velocity) f->max_velocity = velocity;
    
    if(f->max_velocity > convergence_minimum_velocity && f->median_depth_variance < convergence_maximum_depth_variance) f->has_converged = true;
    
    filter_update_outputs(f, time);
    
    auto stop = std::chrono::steady_clock::now();
    camera_sensor.measure_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });

    int space = filter_available_feature_space(f, camera_state);
    if(space >= f->min_group_add)
    {
        if(f->run_state == RCSensorFusionRunStateDynamicInitialization || f->run_state == RCSensorFusionRunStateSteadyInitialization) {
            auto & detection = filter_detect(f, data);
            if(detection.size() >= state_vision_group::min_feats) {
#ifdef TEST_POSDEF
                if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def before disabling orient only");
#endif
                f->s.disable_orientation_only();
#ifdef TEST_POSDEF
                if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def after disabling orient only");
#endif
                f->run_state = RCSensorFusionRunStateRunning;
                f->log->trace("When moving from steady init to running:");
                print_calibration(f);
                state_vision_group *g = f->s.add_group(camera_state, f->map.get());
                // we remap here to update f->s.statesize to account
                // for the new group and to update f->s.fake_statesize
                // to account for disable_orientation_only
                f->s.remap();
                space = filter_available_feature_space(f, camera_state);
                filter_add_detected_features(f, g, camera_sensor, detection, space, data.image.height);
            }
        } else {
#ifdef TEST_POSDEF
            if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def before adding group");
#endif
            camera_state.detecting_group = f->s.add_group(camera_state, f->map.get());
        }
    }
    return true;
}

//This should be called every time we want to initialize or reset the filter
void filter_initialize(struct filter *f)
{
    //changing these two doesn't affect much.
    f->min_group_add = 16;
    f->max_group_add = std::max<int>(40 / f->cameras.size(), f->min_group_add);
    f->has_depth = false;

#ifdef INITIAL_DEPTH
    state_vision_feature::initial_depth_meters = INITIAL_DEPTH;
#else
    state_vision_feature::initial_depth_meters = M_E;
#endif
    state_vision_feature::initial_var = .75;
    state_vision_feature::initial_process_noise = 1.e-20;
    state_vision_feature::outlier_thresh = 1;
    state_vision_feature::outlier_reject = 30.;
    state_vision_feature::max_variance = .10 * .10; //because of log-depth, the standard deviation is approximately a percentage (so .10 * .10 = 10%)
    state_vision_group::ref_noise = 1.e-30;
    state_vision_group::min_feats = 4;

    for (auto &g : f->gyroscopes)     g->init_with_variance(g->intrinsics.measurement_variance_rad2__s2);
    for (auto &a : f->accelerometers) a->init_with_variance(a->intrinsics.measurement_variance_m2__s4);
    for (auto &c : f->cameras)        c->init_with_variance(2 * 2);
    for (auto &d : f->depths)         d->init_with_variance(0);

    f->last_time = sensor_clock::time_point(sensor_clock::duration(0));
    f->want_start = sensor_clock::time_point(sensor_clock::duration(0));
    f->run_state = RCSensorFusionRunStateInactive;

    f->detector_failed = false;
    f->tracker_failed = false;
    f->tracker_warned = false;
    f->speed_failed = false;
    f->speed_warning = false;
    f->numeric_failed = false;
    f->speed_warning_time = sensor_clock::time_point(sensor_clock::duration(0));
    
    f->stable_start = sensor_clock::time_point(sensor_clock::duration(0));
    f->calibration_bad = false;
    
    f->observations.observations.clear();
    f->mini->observations.observations.clear();
    f->catchup->observations.observations.clear();

    f->s.reset();
    if (f->map) f->map->reset();

    for (int i=0; i<f->s.cameras.children.size() && i<f->cameras.size(); i++) {
        state_camera &camera_state = *f->s.cameras.children[i];
        sensor_grey &camera_sensor = *f->cameras[i];

        camera_state.extrinsics.T.v = camera_sensor.extrinsics.mean.T;
        camera_state.extrinsics.Q.v = camera_sensor.extrinsics.mean.Q;

        camera_state.extrinsics.Q.set_initial_variance(camera_sensor.extrinsics.variance.Q);
        camera_state.extrinsics.T.set_initial_variance(camera_sensor.extrinsics.variance.T);

        camera_state.intrinsics.focal_length.v = (camera_sensor.intrinsics.f_x_px + camera_sensor.intrinsics.f_y_px)   / 2        / camera_sensor.intrinsics.height_px;
        camera_state.intrinsics.center_x.v     = (camera_sensor.intrinsics.c_x_px - camera_sensor.intrinsics.width_px  / 2. + .5) / camera_sensor.intrinsics.height_px;
        camera_state.intrinsics.center_y.v     = (camera_sensor.intrinsics.c_y_px - camera_sensor.intrinsics.height_px / 2. + .5) / camera_sensor.intrinsics.height_px;
        if (camera_sensor.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE)
            camera_state.intrinsics.k1.v = camera_sensor.intrinsics.w;
        else
            camera_state.intrinsics.k1.v = camera_sensor.intrinsics.k1;
        camera_state.intrinsics.k2.v = camera_sensor.intrinsics.k2;
        camera_state.intrinsics.k3.v = camera_sensor.intrinsics.k3;
        camera_state.intrinsics.type = camera_sensor.intrinsics.type;

        camera_state.extrinsics.Q.set_process_noise(1.e-30);
        camera_state.extrinsics.T.set_process_noise(1.e-30);

        camera_state.intrinsics.focal_length.set_process_noise(2.3e-3 / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);
        camera_state.intrinsics.center_x.set_process_noise(    2.3e-3 / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);
        camera_state.intrinsics.center_y.set_process_noise(    2.3e-3 / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);
        camera_state.intrinsics.k1.set_process_noise(2.3e-7);
        camera_state.intrinsics.k2.set_process_noise(2.3e-7);
        camera_state.intrinsics.k3.set_process_noise(2.3e-7);

        camera_state.intrinsics.focal_length.set_initial_variance(10. / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);
        camera_state.intrinsics.center_x.set_initial_variance(     2. / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);
        camera_state.intrinsics.center_y.set_initial_variance(     2. / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);

        camera_state.intrinsics.k1.set_initial_variance(camera_sensor.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE ? .01*.01 : 2.e-4);
        camera_state.intrinsics.k2.set_initial_variance(camera_sensor.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE ? .01*.01 : 2.e-4);
        camera_state.intrinsics.k3.set_initial_variance(camera_sensor.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE ? .01*.01 : 2.e-4);

        camera_state.intrinsics.image_width  = camera_sensor.intrinsics.width_px;
        camera_state.intrinsics.image_height = camera_sensor.intrinsics.height_px;

        if (1)
            camera_state.feature_tracker = std::make_unique<fast_tracker>();
#ifdef HAVE_IPP
        else
            camera_state.feature_tracker = std::make_unique<ipp_tracker>();
#endif
    }

    for (size_t i = 0; i < f->s.imus.children.size() && i < f->gyroscopes.size(); i++) {
        auto &imu = *f->s.imus.children[i];
        const auto &gyro = *f->gyroscopes[i];
        imu.intrinsics.w_bias.v = map(gyro.intrinsics.bias_rad__s.v);
        imu.intrinsics.w_bias.set_initial_variance(map(gyro.intrinsics.bias_variance_rad2__s2.v));

        imu.extrinsics.Q.v = gyro.extrinsics.mean.Q;
        imu.extrinsics.T.v = gyro.extrinsics.mean.T;

        imu.extrinsics.Q.set_initial_variance(gyro.extrinsics.variance.Q);
        imu.extrinsics.T.set_initial_variance(gyro.extrinsics.variance.T);
    }

    for (size_t i = 0; i < f->s.imus.children.size() && i < f->accelerometers.size(); i++) {
        auto &imu = *f->s.imus.children[i];
        const auto &accel = *f->accelerometers[i];
        imu.intrinsics.a_bias.v = map(accel.intrinsics.bias_m__s2.v);
        imu.intrinsics.a_bias.set_initial_variance(map(accel.intrinsics.bias_variance_m2__s4.v));

        imu.extrinsics.Q.v = accel.extrinsics.mean.Q;
        imu.extrinsics.T.v = accel.extrinsics.mean.T;

        imu.extrinsics.Q.set_initial_variance(accel.extrinsics.variance.Q);
        imu.extrinsics.T.set_initial_variance(accel.extrinsics.variance.T);
    }

    for (const auto &imu : f->s.imus.children) {
        imu->intrinsics.w_bias.set_process_noise(2.3e-10);
        imu->intrinsics.a_bias.set_process_noise(2.3e-8);
        imu->extrinsics.Q.set_process_noise(1.e-30);
        imu->extrinsics.T.set_process_noise(1.e-30);
    }

    f->s.T.set_process_noise(0.);
    f->s.Q.set_process_noise(0.);
    f->s.V.set_process_noise(0.);
    f->s.w.set_process_noise(0.);
    f->s.dw.set_process_noise(0);
    f->s.a.set_process_noise(0);
    f->s.g.set_process_noise(1.e-30);

    f->s.g.set_initial_variance(1.e-7);
    f->s.T.set_initial_variance(1.e-7); // to avoid not being positive definite
    // FIXME: set_initial_variance should support passing the full covariance if we want to support "odd" world coordinates
    f->s.Q.set_initial_variance((f->s.world_up_initial_forward_left.transpose()
                                 * v3{f_t(1e-7),10,10}.asDiagonal() * // to avoid not being positive definite
                                 f->s.world_up_initial_forward_left).diagonal());
    f->s.V.set_initial_variance(1. * 1.);
    f->s.w.set_initial_variance(10);
    f->s.dw.set_initial_variance(10);
    f->s.ddw.set_initial_variance(466*466);
    f->s.a.set_initial_variance(10);
    f->s.da.set_initial_variance(9*9);

#ifdef ENABLE_QR
    f->last_qr_time = sensor_clock::micros_to_tp(0);
    f->qr_origin_gravity_aligned = true;
#endif
    f->max_velocity = 0.;
    f->median_depth_variance = 1.;
    f->has_converged = false;
    
    f->origin = transformation();
    f->origin_set = false;
    
    f->s.statesize = 0;
    f->s.enable_orientation_only(false);
    f->s.remap();
    f->s.maxstatesize = MAXSTATESIZE;

    f->mini->state.copy_from(f->s);
    f->catchup->state.copy_from(f->s);
}

void filter_deinitialize(struct filter *f)
{
    for (size_t i = 0; i < f->s.cameras.children.size() && i < f->cameras.size(); i++) {
        const state_camera &camera_state = *f->s.cameras.children[i];
        sensor_grey &camera_sensor = *f->cameras[i];

        camera_sensor.intrinsics.width_px  = camera_state.intrinsics.image_width;
        camera_sensor.intrinsics.height_px = camera_state.intrinsics.image_height;
        camera_sensor.intrinsics.f_x_px    = camera_state.intrinsics.focal_length.v * camera_state.intrinsics.image_height;
        camera_sensor.intrinsics.f_y_px    = camera_state.intrinsics.focal_length.v * camera_state.intrinsics.image_height;
        camera_sensor.intrinsics.c_x_px    = camera_state.intrinsics.center_x.v * camera_state.intrinsics.image_height + camera_state.intrinsics.image_width / 2. - .5;
        camera_sensor.intrinsics.c_y_px    = camera_state.intrinsics.center_y.v * camera_state.intrinsics.image_height + camera_state.intrinsics.image_height / 2. - .5;

        switch((camera_sensor.intrinsics.type = camera_state.intrinsics.type)) {
        case rc_CALIBRATION_TYPE_FISHEYE:
            camera_sensor.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
            camera_sensor.intrinsics.w = camera_state.intrinsics.k1.v;
            break;
        case rc_CALIBRATION_TYPE_POLYNOMIAL3:
            camera_sensor.intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
            camera_sensor.intrinsics.k1 = camera_state.intrinsics.k1.v;
            camera_sensor.intrinsics.k2 = camera_state.intrinsics.k2.v;
            camera_sensor.intrinsics.k3 = camera_state.intrinsics.k3.v;
            break;
        default:
        case rc_CALIBRATION_TYPE_UNKNOWN:
        case rc_CALIBRATION_TYPE_UNDISTORTED:
            break;
        }

        camera_sensor.extrinsics.mean.T = camera_state.extrinsics.T.v;
        camera_sensor.extrinsics.mean.Q = camera_state.extrinsics.Q.v;
        camera_sensor.extrinsics.variance.Q = camera_state.extrinsics.Q.variance();
        camera_sensor.extrinsics.variance.T = camera_state.extrinsics.T.variance();
    }

    for (size_t i = 0; i < f->s.imus.children.size() && i < f->gyroscopes.size(); i++) {
        const state_imu &imu = *f->s.imus.children[i];
        sensor_gyroscope &gyro  = *f->gyroscopes[i];

        map(gyro.intrinsics.bias_rad__s.v)            = imu.intrinsics.w_bias.v;
        map(gyro.intrinsics.bias_variance_rad2__s2.v) = imu.intrinsics.w_bias.variance();
    }

    for (size_t i = 0; i < f->s.imus.children.size() && i < f->gyroscopes.size(); i++) {
        const state_imu &imu = *f->s.imus.children[i];
        sensor_accelerometer &accel = *f->accelerometers[i];
        struct sensor::extrinsics &imu_extrinsics = accel.extrinsics;

        map(accel.intrinsics.bias_m__s2.v)            = imu.intrinsics.a_bias.v;
        map(accel.intrinsics.bias_variance_m2__s4.v)  = imu.intrinsics.a_bias.variance();

        imu_extrinsics.mean.T =     imu.extrinsics.T.v;
        imu_extrinsics.mean.Q =     imu.extrinsics.Q.v;
        imu_extrinsics.variance.Q = imu.extrinsics.Q.variance();
        imu_extrinsics.variance.T = imu.extrinsics.T.variance();
    }
}

float filter_converged(const struct filter *f)
{
    // FIXME: remove the hardcoded use of the first IMU
    if (f->s.imus.children.size() < 1 || f->accelerometers.size() < 1)
        return 0;
    const auto &imu = *f->s.imus.children[0]; const auto &a = *f->accelerometers[0];
    if(f->run_state == RCSensorFusionRunStateSteadyInitialization) {
        if(f->stable_start == sensor_clock::micros_to_tp(0)) return 0.;
        float progress = (f->last_time - f->stable_start) / std::chrono::duration_cast<std::chrono::duration<float>>(steady_converge_time);
        if(progress >= .99f) return 0.99f; //If focus takes a long time, we won't know how long it will take
        return progress;
    } else if(f->run_state == RCSensorFusionRunStatePortraitCalibration) {
        return get_bias_convergence(f, imu, a, 0);
    } else if(f->run_state == RCSensorFusionRunStateLandscapeCalibration) {
        return get_bias_convergence(f, imu, a, 1);
    } else if(f->run_state == RCSensorFusionRunStateStaticCalibration) {
        return fmin(a.stability.count / (float)calibration_converge_samples, get_bias_convergence(f, imu, a, 2));
    } else if(f->run_state == RCSensorFusionRunStateRunning || f->run_state == RCSensorFusionRunStateDynamicInitialization) { // TODO: proper progress for dynamic init, if needed.
        return 1.;
    } else return 0.;
}

bool filter_is_steady(const struct filter *f)
{
    return
        f->s.V.v.norm() < .1 &&
        f->s.w.v.norm() < .1;
}

void filter_start_static_calibration(struct filter *f)
{
    reset_stability(f);
    { auto i = f->s.imus.children.begin(); for (auto &g : f->gyroscopes)     { auto &imu = *i; g->start_variance = imu->intrinsics.w_bias.variance(); i++; } }
    { auto i = f->s.imus.children.begin(); for (auto &a : f->accelerometers) { auto &imu = *i; a->start_variance = imu->intrinsics.a_bias.variance(); i++; } }
    f->run_state = RCSensorFusionRunStateStaticCalibration;
}

void filter_start_hold_steady(struct filter *f)
{
    reset_stability(f);
    f->run_state = RCSensorFusionRunStateSteadyInitialization;
}

void filter_start_dynamic(struct filter *f)
{
    f->want_start = f->last_time;
    f->run_state = RCSensorFusionRunStateDynamicInitialization;
}

void filter_start_inertial_only(struct filter *f)
{
    f->run_state = RCSensorFusionRunStateInertialOnly;
    f->s.enable_orientation_only();
}

void filter_set_origin(struct filter *f, const transformation &origin, bool gravity_aligned)
{
    if(gravity_aligned) {
        v3 z_old = f->s.world.up;
        v3 z_new = origin.Q * z_old;
        quaternion Qd = rotation_between_two_vectors_normalized(z_new, z_old);
        f->origin.Q = Qd * origin.Q;
    } else f->origin.Q = origin.Q;
    f->origin.T = origin.T;
    f->origin_set = true;
}

#ifdef ENABLE_QR
void filter_start_qr_detection(struct filter *f, const std::string& data, float dimension, bool use_gravity)
{
    f->qr_origin_gravity_aligned = use_gravity;
    f->qr.start(data, dimension);
    f->last_qr_time = sensor_clock::micros_to_tp(0);
}

void filter_stop_qr_detection(struct filter *f)
{
    f->qr.stop();
}

void filter_start_qr_benchmark(struct filter * f, float qr_size_m)
{
    f->qr_bench.start(qr_size_m);
}
#endif

std::string filter_get_stats(const struct filter *f)
{
    std::ostringstream statstr;
    for(auto &i:f->cameras)
    {
        statstr << i->name << "\t (measure): " << i->measure_time_stats << "\n";
        statstr << i->name << "\t (detect):  " << i->other_time_stats << "\n";
    }
    for(auto &i:f->accelerometers)
    {
        statstr << i->name << "\t (measure): " << i->measure_time_stats << "\n";
        statstr << i->name << "\t (fast):    " << i->other_time_stats << "\n";
    }
    for(auto &i:f->gyroscopes)
    {
        statstr << i->name << "\t (measure): " << i->measure_time_stats << "\n";
        statstr << i->name << "\t (fast):    " << i->other_time_stats << "\n";
    }
    statstr << "\n";
    return statstr.str();
}
