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
#include "matrix.h"
#include "observation.h"
#include "filter.h"
#include <memory>
#include "fast_tracker.h"
#include "descriptor.h"
#ifdef HAVE_IPP
#include "ipp_tracker.h"
#endif
#include "Trace.h"
#include "rc_compat.h"

#ifdef MYRIAD2
#include "shave_tracker.h"
#include "platform_defines.h"
#endif // MYRIAD2

using rc::map;

const static sensor_clock::duration camera_wait_time = std::chrono::milliseconds(500); //time we'll wait for all cameras before attempting to detect features
const static sensor_clock::duration max_detector_failed_time = std::chrono::milliseconds(500); //time we'll go with no features before dropping to inertial only mode
const static f_t accelerometer_inertial_var = 2.33*2.33; //variance when in inertial only mode
const static f_t dynamic_W_thresh_variance = 5.e-2; // variance of W must be less than this to initialize from dynamic mode
//a_bias_var for best results on benchmarks is 6.4e-3
const static f_t max_accel_delta = 20.; //This is biggest jump seen in hard shaking of device
const static f_t max_gyro_delta = 5.; //This is biggest jump seen in hard shaking of device
const static f_t convergence_minimum_velocity = 0.3; //Minimum speed (m/s) that the user must have traveled to consider the filter converged
const static f_t convergence_maximum_depth_variance = .001; //Median feature depth must have been under this to consider the filter converged
const static f_t recovered_feature_initial_variance = .05; //When features are recovered, set their initial variance to this

void filter_update_outputs(struct filter *f, sensor_clock::time_point time, bool failed)
{
    if(f->run_state != RCSensorFusionRunStateRunning) return;

    if(failed) { // if we lost all features - reset convergence
        f->has_converged = false;
        f->max_velocity = 0;
    }

    f->median_depth_variance = f->s.median_depth_variance();
    if(f->max_velocity > convergence_minimum_velocity && f->median_depth_variance < convergence_maximum_depth_variance)
        f->has_converged = true;

    if(f->s.V.v.norm() > f->max_velocity)
        f->max_velocity = f->s.V.v.norm();

    bool old_speedfail = f->speed_failed;
    f->speed_failed = false;
    f_t speed = f->s.V.v.norm();
    if(speed > 3.f) { //1.4m/s is normal walking speed
        if (!old_speedfail) f->log->info("Velocity {} m/s exceeds max bound at {}", speed, sensor_clock::tp_to_micros(time));
        f->speed_failed = true;
    } else if(speed > 2.f) {
        if (!f->speed_warning) f->log->info("High velocity ({} m/s) at {}", speed, sensor_clock::tp_to_micros(time));
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t accel = f->s.a.v.norm();
    if(accel > 9.8f) { //1g would saturate sensor anyway
        if (!old_speedfail) f->log->info("Acceleration exceeds max bound");
        f->speed_failed = true;
    } else if(accel > 5.f) { //max in mine is 6.
        if (!f->speed_warning) f->log->info("High acceleration ({} m/s^2) at {}", accel, sensor_clock::tp_to_micros(time));
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t ang_vel = f->s.w.v.norm();
    if(ang_vel > 5.f) { //sensor saturation - 250/180*pi
        if (!old_speedfail) f->log->info("Angular velocity exceeds max bound at {}", sensor_clock::tp_to_micros(time));
        f->speed_failed = true;
    } else if(ang_vel > 2.f) { // max in mine is 1.6
        if (!f->speed_warning) f->log->info("High angular velocity at {}", sensor_clock::tp_to_micros(time));
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
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
    START_EVENT(SF_MINI_ACCEL_MEAS, 0);
    if(data.id >= f->accelerometers.size() || data.id >= f->s.imus.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();
    auto &accelerometer = *f->accelerometers[data.id];
    auto &imu = *state.imus.children[data.id];
    v3 meas = map(accelerometer.intrinsics.scale_and_alignment.v) * map(data.acceleration_m__s2.v);

    //TODO: if out of order, project forward in time

    auto obs_a = std::make_unique<observation_accelerometer>(accelerometer, state, state, imu);
    obs_a->meas = meas;
    obs_a->variance = accelerometer.measurement_variance;

    queue.observations.push_back(std::move(obs_a));
    bool ok = filter_mini_process_observation_queue(f, queue, state, data.timestamp);
    auto stop = std::chrono::steady_clock::now();
    accelerometer.fast_path_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    END_EVENT(SF_MINI_ACCEL_MEAS, data.time_us / 1000);
    return ok;
}

bool filter_mini_gyroscope_measurement(struct filter * f, observation_queue &queue, state_motion &state, const sensor_data &data)
{
    START_EVENT(SF_MINI_GYRO_MEAS, 0);
    if(data.id >= f->gyroscopes.size() || data.id >= f->s.imus.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();
    
    auto &gyroscope = *f->gyroscopes[data.id];
    auto &imu = *state.imus.children[data.id];
    v3 meas = map(gyroscope.intrinsics.scale_and_alignment.v) * map(data.angular_velocity_rad__s.v);

    //TODO: if out of order, project forward in time
    
    auto obs_w = std::make_unique<observation_gyroscope>(gyroscope, state, imu);
    obs_w->meas = meas;
    obs_w->variance = gyroscope.measurement_variance;

    queue.observations.push_back(std::move(obs_w));
    bool ok = filter_mini_process_observation_queue(f, queue, state, data.timestamp);

    auto stop = std::chrono::steady_clock::now();
    gyroscope.fast_path_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    END_EVENT(SF_MINI_GYRO_MEAS, data.time_us / 1000);

    return ok;
}

static void preprocess_observation_queue(struct filter *f, sensor_clock::time_point time)
{
    f->last_time = time;
    f->observations.preprocess(f->s, time);
}

static void process_observation_queue(struct filter *f)
{
    bool run_on_shave = true;
    if(!f->observations.process(f->s, run_on_shave)) {
        f->numeric_failed = true;
    }
    f->s.integrate_distance();
}

void filter_compute_gravity(struct filter *f, double latitude, double altitude)
{
    assert(f); f->s.compute_gravity(latitude, altitude);
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

static f_t get_accelerometer_variance_for_run_state(struct filter *f, sensor_accelerometer &accelerometer)
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
    }
    assert(0); //should never fall through to here;
    return accelerometer.measurement_variance;
}

bool filter_accelerometer_measurement(struct filter *f, const sensor_data &data_)
{
    sensor_data data(data_, sensor_data::stack_copy());
    START_EVENT(SF_ACCEL_MEAS, 0);
    if(data.id >= f->accelerometers.size() || data.id >= f->s.imus.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();
    auto timestamp = data.timestamp;
    auto &accelerometer = *f->accelerometers[data.id];
    auto &imu = *f->s.imus.children[data.id];

    if (!accelerometer.decimate(data.time_us, data.acceleration_m__s2.v))
        return false;

    v3 meas = map(accelerometer.intrinsics.scale_and_alignment.v) * map(data.acceleration_m__s2.v);
    if (!accelerometer.got)
        accelerometer.got = true;
    else {
        v3 accel_delta = (meas - accelerometer.last_meas);
        if (fabs(accel_delta[0]) > max_accel_delta || fabs(accel_delta[1]) > max_accel_delta || fabs(accel_delta[2]) > max_accel_delta)
            f->log->warn("Extreme jump in accelerometer {} {} {} at {}", accel_delta[0], accel_delta[1], accel_delta[2], sensor_clock::tp_to_micros(timestamp));
    }
    accelerometer.last_meas = meas;

    //This will throw away both the outlier measurement and the next measurement, because we update last every time. This prevents setting last to an outlier and never recovering.
    if(f->run_state == RCSensorFusionRunStateInactive) return false;
    if (!f->got_any_gyroscopes()) return false;

    if(!f->s.orientation_initialized) {
        f->s.orientation_initialized = true;
        f->s.Q.v = initial_orientation_from_gravity_facing(f->s.world.up, imu.extrinsics.Q.v * meas,
                                                           f->s.world.initial_forward, f->s.body_forward);
        if(f->origin_set)
            f->origin.Q = f->origin.Q * f->s.Q.v.conjugate();

        preprocess_observation_queue(f, timestamp);
        return true;
    }

    auto obs_a = std::make_unique<observation_accelerometer>(accelerometer, f->s, f->s, imu);
    obs_a->meas = meas;
    obs_a->variance = get_accelerometer_variance_for_run_state(f, accelerometer) / accelerometer.decimate_by;

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
    END_EVENT(SF_ACCEL_MEAS, data.time_us / 1000);
    return true;
}

bool filter_gyroscope_measurement(struct filter *f, const sensor_data & data_)
{
    sensor_data data(data_, sensor_data::stack_copy());
    START_EVENT(SF_GYRO_MEAS, 0);
    if(data.id >= f->gyroscopes.size() || data.id >= f->s.imus.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();
    auto timestamp = data.timestamp;
    auto &gyroscope = *f->gyroscopes[data.id];
    auto &imu = *std::next(f->s.imus.children.begin(), data.id)->get();

    if (!gyroscope.decimate(data.time_us, data.angular_velocity_rad__s.v))
        return false;

    v3 meas = map(gyroscope.intrinsics.scale_and_alignment.v) * map(data.angular_velocity_rad__s.v);
    if (!gyroscope.got)
        gyroscope.got = true;
    else {
        v3 gyro_delta = meas - gyroscope.last_meas;
        if(fabs(gyro_delta[0]) > max_gyro_delta || fabs(gyro_delta[1]) > max_gyro_delta || fabs(gyro_delta[2]) > max_gyro_delta)
            f->log->warn("Extreme jump in gyro {} {} {} at {}", gyro_delta[0], gyro_delta[1], gyro_delta[2], sensor_clock::tp_to_micros(timestamp));
    }
    gyroscope.last_meas = meas;

    //This will throw away both the outlier measurement and the next measurement, because we update last every time. This prevents setting last to an outlier and never recovering.
    if(f->run_state == RCSensorFusionRunStateInactive) return false;

    if(!f->s.orientation_initialized) return false;

    auto obs_w = std::make_unique<observation_gyroscope>(gyroscope, f->s, imu);
    obs_w->meas = meas;
    obs_w->variance = gyroscope.measurement_variance / gyroscope.decimate_by;

    f->observations.observations.push_back(std::move(obs_w));

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
    END_EVENT(SF_GYRO_MEAS, data.time_us / 1000);
    return true;
}


bool filter_velocimeter_measurement(struct filter *f, const sensor_data & data_)
{
    sensor_data data(data_, sensor_data::stack_copy());
    if(data.id >= f->velocimeters.size() || data.id >= f->s.velocimeters.children.size())  // f->velocimeters sensor and f->s.velocimeters state
        return false;

    auto start = std::chrono::steady_clock::now();
    auto timestamp = data.timestamp;
    auto &velocimeter = *f->velocimeters[data.id];
    auto &velocimeter_s = *std::next(f->s.velocimeters.children.begin(), data.id)->get();

    if (!velocimeter.decimate(data.time_us, data.translational_velocity_m__s.v))
        return true;

    v3 meas = map(velocimeter.intrinsics.scale_and_alignment.v) * map(data.translational_velocity_m__s.v);

    if(f->run_state == RCSensorFusionRunStateInactive) return false;
    if(!f->s.orientation_initialized) return false;

    auto obs_v = std::make_unique<observation_velocimeter>(velocimeter, f->s, velocimeter_s.extrinsics, velocimeter_s.intrinsics);
    obs_v->meas = meas;
    obs_v->variance = velocimeter.measurement_variance;

    f->observations.observations.push_back(std::move(obs_v));

    if(show_tuning) fprintf(stderr, "\nvelocimeter:\n");
    preprocess_observation_queue(f, timestamp);
    process_observation_queue(f);
    if(show_tuning) {
        std::cerr << " meas   " << meas << "\n"
                  << " innov  " << velocimeter.inn_stdev << "\n"
                  << " signal " << velocimeter.meas_stdev << "\n";
    }

    auto stop = std::chrono::steady_clock::now();
    velocimeter.measure_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    return true;
}


static void filter_setup_next_frame(struct filter *f, const sensor_data &data)
{
    auto &camera_sensor = *f->cameras[data.id];
    auto &camera_state = *f->s.cameras.children[data.id];

    for(auto &track : camera_state.tracks) {
        auto obs = std::make_unique<observation_vision_feature>(camera_sensor, camera_state, track.feature, track);
        f->observations.observations.push_back(std::move(obs));
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

    float i_Cx = d_center_x       * i_height + (i_width  - 1) / 2.f,
          i_Cy = d_center_y       * i_height + (i_height - 1) / 2.f,
          i_Fx = d_focal_length_x * i_height,
          i_Fy = d_focal_length_y * i_height;

    float o_Cx = camera_state.intrinsics.center.v.x()   * o_height + (o_width  - 1) / 2.f,
          o_Cy = camera_state.intrinsics.center.v.y()   * o_height + (o_height - 1) / 2.f,
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

static int filter_add_detected_features(struct filter * f, state_camera &camera, sensor_grey &camera_sensor, size_t newfeats, int image_height, sensor_clock::time_point time)
{
    auto &kp = camera.standby_tracks;
    auto g = f->s.add_group(camera_sensor.id, f->map.get());

    std::unique_ptr<sensor_data> aligned_undistorted_depth;

    size_t found_feats = 0;
    f_t image_to_depth = 1;
    if(f->has_depth)
        image_to_depth = f_t(f->recent_depth->image.height)/image_height;

    for(auto i = f->s.stereo_matches.begin(); i != f->s.stereo_matches.end() && found_feats < newfeats;) {
        auto &m = i->second;
        auto view = std::find_if(m.views.begin(), m.views.end(), [&](const stereo_match::view &v) { return &v.camera == &camera; });
        if (view != m.views.end()) {
            auto feat = std::make_unique<state_vision_feature>(*view->track, *g);
            feat->v->set_depth_meters(view->depth_m);
            auto std_pct = sqrt(state_vision_feature::initial_var/10); // consider using error
            feat->set_initial_variance(std_pct *std_pct); // assumes log depth
            feat->status = feature_normal;
            feat->depth_measured = true;
            for (auto &v : m.views) {
                v.camera.tracks.emplace_back(v.camera.id, *feat, std::move(*v.track));
                v.camera.standby_tracks.erase(v.track);
            }
            if(f->map)
                f->map->triangulated_tracks.erase(feat->feature->id);
            g->features.children.push_back(std::move(feat));
            i = f->s.stereo_matches.erase(i);
            ++found_feats;
        } else ++i;
    }

    for(auto i = kp.begin(); i != kp.end() && found_feats < newfeats; found_feats++, i = kp.erase(i)) {
        auto feat = std::make_unique<state_vision_feature>(*i, *g);
        if(f->has_depth) {
            if (!aligned_undistorted_depth)
                aligned_undistorted_depth = filter_aligned_depth_to_camera(*f->recent_depth, *f->depths[f->recent_depth->id], camera, camera_sensor);
            
            float depth_m = 0.001f * get_depth_for_point_mm(aligned_undistorted_depth->depth, image_to_depth*camera.intrinsics.unnormalize_feature(camera.intrinsics.undistort_feature(camera.intrinsics.normalize_feature({i->x, i->y}))));
            if(depth_m) {
                feat->v->set_depth_meters(depth_m);
                float std_pct = get_stdev_pct_for_depth(depth_m);
                //fprintf(stderr, "percent %f\n", std_pct);
                feat->set_initial_variance(std_pct * std_pct); // assumes log depth
                feat->status = feature_normal;
                feat->depth_measured = true;
            }
        }

        if(f->map)
            f->map->triangulated_tracks.erase(feat->feature->id);
        camera.tracks.emplace_back(camera.id, *feat, std::move(*i));
        g->features.children.push_back(std::move(feat));
    }

    if(f->map) {
        for (auto &t: camera.standby_tracks) {
            f->map->initialize_track_triangulation(t, g->id);
        }
    }

    f->s.remap();
#ifdef TEST_POSDEF
    if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def after adding features");
#endif
    return found_feats;
}

static size_t filter_available_feature_space(struct filter *f)
{
    auto space = f->store.maxstatesize - f->s.statesize;
    //leave space for the group
    space -= 6;
    if(space < 0) space = 0;
    if((size_t)space > f->max_group_add)
        space = f->max_group_add;
    return space;
}

std::unique_ptr<camera_frame_t> filter_create_camera_frame(const struct filter *f, const sensor_data& data)
{
    std::unique_ptr<camera_frame_t> camera_frame;
    groupid closest_group_id;
    transformation G_Bclosest_Bnow;
    if(f->s.get_closest_group_transformation(closest_group_id, G_Bclosest_Bnow)) {
        camera_frame.reset(new camera_frame_t);
        camera_frame->camera_id = data.id;
        camera_frame->G_closestnode_frame = std::move(G_Bclosest_Bnow);
        camera_frame->closest_node = closest_group_id;
        camera_frame->frame.reset(new frame_t);
        camera_frame->frame->timestamp = data.timestamp;
#ifdef RELOCALIZATION_DEBUG
        camera_frame->frame->image = cv::Mat(data.image.height, data.image.width, CV_8UC1, (uint8_t*)data.image.image, data.image.stride).clone();
#endif
    }

    return camera_frame;
}

size_t filter_detect(struct filter *f, const sensor_data &data, const std::unique_ptr<camera_frame_t>& camera_frame)
{
    sensor_grey &camera_sensor = *f->cameras[data.id];
    state_camera &camera = *f->s.cameras.children[data.id];
    auto start = std::chrono::steady_clock::now();
    const rc_ImageData &image = data.image;
    camera.feature_tracker->tracks.clear();
    int standby_count = camera.standby_tracks.size(),
        detect_count = camera.detecting_space,
        track_count = camera.track_count();
    camera.detecting_space = 0;

    auto space = std::max(0, detect_count - standby_count);
    if(!space && !camera_frame) return 0; // FIXME: what min number is worth detecting?

    camera.feature_tracker->tracks.reserve(track_count + space);

    for(auto &i : camera.tracks)
        if(i.track.found()) camera.feature_tracker->tracks.emplace_back(&i.track);

    for(auto &t: camera.standby_tracks)
        camera.feature_tracker->tracks.emplace_back(&t);

    // Run detector
    tracker::image timage;
    timage.image = (uint8_t *)image.image;
    timage.width_px = image.width;
    timage.height_px = image.height;
    timage.stride_px = image.stride;

    START_EVENT(SF_DETECT, 0);
    std::vector<tracker::feature_track> &kp = camera.feature_tracker->detect(timage, camera.feature_tracker->tracks, space);
    END_EVENT(SF_DETECT, kp.size());

    for (auto &p : kp)
        camera.feature_tracker->tracks.push_back(&p);

    if (camera_frame) {
        auto& frame = camera_frame->frame;
        frame->keypoints.reserve(camera.feature_tracker->tracks.size());
        frame->keypoints_xy.reserve(camera.feature_tracker->tracks.size());
        for (auto &p : camera.feature_tracker->tracks) {
            auto feature = std::static_pointer_cast<fast_tracker::fast_feature<patch_orb_descriptor>>(p->feature);
            if (feature->descriptor.orb_computed || fast_tracker::is_trackable<orb_descriptor::border_size>((int)p->x, (int)p->y, timage.width_px, timage.height_px)) {
                frame->keypoints.emplace_back(std::move(feature));
                frame->keypoints_xy.emplace_back(p->x, p->y);
            }
        }
    }

    // insert (newest w/highest score first) up to detect_count features (so as to not let mapping affect tracking)
    camera.standby_tracks.insert(camera.standby_tracks.begin(),
                                 std::make_move_iterator(kp.begin()),
                                 std::make_move_iterator(kp.end()));

    auto stop = std::chrono::steady_clock::now();
    camera_sensor.detect_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    return kp.size();
}

bool filter_compute_orb(struct filter *f, const sensor_data& data, camera_frame_t& camera_frame)
{
    if (!std::is_same<DESCRIPTOR, orb_descriptor>::value) {
        const rc_ImageData &image = data.image;
        tracker::image timage;
        timage.image = (uint8_t *)image.image;
        timage.width_px = image.width;
        timage.height_px = image.height;
        timage.stride_px = image.stride;

        START_EVENT(SF_ORB, 0);
        for (size_t i = 0; i < camera_frame.frame->keypoints.size(); ++i) {
            const v2& p = camera_frame.frame->keypoints_xy[i];
            auto& feature = camera_frame.frame->keypoints[i];
            if(!feature->descriptor.orb_computed) {
                feature->descriptor.orb = orb_descriptor(p.x(), p.y(), timage);
                feature->descriptor.orb_computed = true;
            }
        }
        END_EVENT(SF_ORB, camera_frame.frame->keypoints.size());
    }
    return true;
}

void filter_compute_dbow(struct filter *f, camera_frame_t& camera_frame)
{
    START_EVENT(SF_DBOW_TRANSFORM, 0);
    camera_frame.frame->calculate_dbow(f->map->orb_voc.get());
    END_EVENT(SF_DBOW_TRANSFORM, camera_frame.frame->keypoints.size());
}

void filter_assign_frame(struct filter *f, const camera_frame_t& camera_frame)
{
    f->map->set_node_frame(camera_frame.closest_node, camera_frame.frame);
}

void filter_update_map_index(struct filter *f)
{
    f->map->index_finished_nodes();
}

map_relocalization_result filter_relocalize(struct filter *f, const camera_frame_t& camera_frame)
{
    auto start = std::chrono::steady_clock::now();
    auto &camera_sensor = *f->cameras[camera_frame.camera_id];
    START_EVENT(SF_RELOCALIZE, camera_frame.camera_id);
    map_relocalization_result reloc_result = f->map->relocalize(camera_frame);
    END_EVENT(SF_RELOCALIZE, reloc_result.info.is_relocalized);
    auto stop  = std::chrono::steady_clock::now();
    float elapsed_time = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count());
    camera_sensor.reloc_time_stats.data(v<1> {elapsed_time});
    if (reloc_result.info.rstatus > relocalization_status::match_descriptors)
        f->log->info("      status {:2}, elapsed time [us]: {:.3f}", (int)reloc_result.info.rstatus, elapsed_time);
    else
        f->log->debug("      status {:2}, elapsed time [us]: {:.3f}", (int)reloc_result.info.rstatus, elapsed_time);
    if (reloc_result.info.is_relocalized)
        f->log->info("relocalized");
    return reloc_result;
}

void filter_add_relocalization_edges(struct filter *f, const aligned_vector<map_relocalization_edge>& edges)
{
    f->map->add_relocalization_edges(edges);
    if (f->map->is_map_unlinked()) {
        for (auto& edge : edges) {
            if (edge.id1 < f->map->get_node_id_offset()) {
                if (f->map->link_map(edge))
                    break;
            }
        }
    }
}

bool filter_depth_measurement(struct filter *f, const sensor_data & data)
{
    if(data.id != 0) return true;

    f->recent_depth = data.make_copy();
    f->has_depth = true;
    return true;
}

// From http://paulbourke.net/geometry/pointlineplane/lineline.c
// line 1 is p1 to p2, line 2 is p3 to p4
static bool l_l_intersect(const v3& p1, const v3& p2, const v3& p3, const v3& p4, v3 & pa, v3 & pb)
{
    v3 p13,p43,p21;
    f_t d1343,d4321,d1321,d4343,d2121;
    f_t numer,denom;

    p13 = p1 - p3;
    p43 = p4 - p3;
    if (fabs(p43[0]) < F_T_EPS && fabs(p43[1]) < F_T_EPS && fabs(p43[2]) < F_T_EPS)
      return false;

    p21 = p2 - p1;
    if (fabs(p21[0]) < F_T_EPS && fabs(p21[1]) < F_T_EPS && fabs(p21[2]) < F_T_EPS)
      return false;

    d1343 = p13.dot(p43); //p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
    d4321 = p43.dot(p21); //p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
    d1321 = p13.dot(p21); //p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
    d4343 = p43.dot(p43); //p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
    d2121 = p21.dot(p21); //p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

    denom = d2121 * d4343 - d4321 * d4321;
    if (fabs(denom) < F_T_EPS)
      return false;
    numer = d1343 * d4321 - d1321 * d4343;

    float mua = numer / denom;
    float mub = (d1343 + d4321 * mua) / d4343;

    pa = p1 + mua*p21;
    pb = p3 + mub*p43;

    return true;
}

struct kp_pre_data{
    v3 p_cal_transformed, o_transformed;
};

// Triangulates a point in the body reference frame from two views
static kp_pre_data preprocess_keypoint_intersect(const state_camera & camera, const feature_t& f,const m3& Rw)
{
    v3 p_calibrated = camera.intrinsics.undistort_feature(camera.intrinsics.normalize_feature(f)).homogeneous();
    return { Rw*p_calibrated + camera.extrinsics.T.v, camera.extrinsics.T.v };
}

// Triangulates a point in the body reference frame from two views
static bool keypoint_intersect(state_camera & camera1, kp_pre_data& pre_data1, const m3& Rw1T, f_t &depth1,
                               state_camera & camera2, kp_pre_data& pre_data2, const m3& Rw2T, f_t &depth2, float &intersection_error_percent)
{
    v3 pa, pb; // pa (pb) is the point on the first (second) line closest to the intersection
    bool success = l_l_intersect(pre_data1.o_transformed, pre_data1.p_cal_transformed, pre_data2.o_transformed, pre_data2.p_cal_transformed, pa, pb);
    if(!success)
        return false;

    v3 cam1_intersect = Rw1T * (pa - camera1.extrinsics.T.v);
    v3 cam2_intersect = Rw2T * (pb - camera2.extrinsics.T.v);
    if(cam1_intersect[2] < 0 || cam2_intersect[2] < 0)
        return false;

    // TODO: set minz and maxz or at least bound error when close to / far away from camera
    float error = (pa - pb).norm();
    intersection_error_percent = error/cam1_intersect[2];
    if(error/cam1_intersect[2] > .05f)
        return false;

    depth1 = cam1_intersect[2];
    depth2 = cam2_intersect[2];
    return true;
}

static float keypoint_compare(const tracker::feature_track & t1, const tracker::feature_track & t2)
{
    auto f1 = std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(t1.feature);
    auto f2 = std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(t2.feature);
    return DESCRIPTOR::distance_stereo(f1->descriptor, f2->descriptor);
}

bool filter_stereo_initialize(struct filter *f, rc_Sensor camera1_id, rc_Sensor camera2_id)
{
    {
        START_EVENT(SF_STEREO_MATCH, camera1_id)
        state_camera &camera_state1 = *f->s.cameras.children[camera1_id];
        state_camera &camera_state2 = *f->s.cameras.children[camera2_id];
        std::list<tracker::feature_track> &kp1 = f->s.cameras.children[camera1_id]->standby_tracks;
        std::list<tracker::feature_track> &kp2 = f->s.cameras.children[camera2_id]->standby_tracks;

#ifdef ENABLE_SHAVE_STEREO_MATCHING
        shave_tracker shave_stereo_o;
        shave_stereo_o.stereo_matching_full_shave(f, camera1_id, camera2_id);
#else
        // preprocess data for kp1
        m3 Rw1 = camera_state1.extrinsics.Q.v.toRotationMatrix();
        m3 Rw1T = Rw1.transpose();
        // preprocess data for kp2
        m3 Rw2 = camera_state2.extrinsics.Q.v.toRotationMatrix();
        m3 Rw2T = Rw2.transpose();
        std::vector<kp_pre_data> prkpv2;
        for(auto & k2 : kp2)
            if (f->s.stereo_matches.count(k2.feature->id)) // already stereo
                prkpv2.emplace_back();
            else
                prkpv2.emplace_back(preprocess_keypoint_intersect(camera_state2, feature_t{k2.x, k2.y},Rw2));
        for(auto k1 = kp1.begin(); k1 != kp1.end(); ++k1) {
            float second_best_distance = INFINITY;
            float best_distance = INFINITY;
            if (f->s.stereo_matches.count(k1->feature->id)) // already stereo
                continue;
            float best_depth1 = 0;
            float best_depth2 = 0;
            float best_error = 0;
            auto best_k2 = kp2.end();
            kp_pre_data pre1 = preprocess_keypoint_intersect(camera_state1, feature_t{k1->x, k1->y}, Rw1);
            // try to find a match in im2
            auto prk2 = prkpv2.begin();
            for(auto k2 = kp2.begin(); k2 != kp2.end(); ++k2, ++prk2){
                if (f->s.stereo_matches.count(k2->feature->id)) // already stereo
                    continue;
                float depth1, depth2, error_percent;
                if (keypoint_intersect(camera_state1,  pre1, Rw1T, depth1,
                                       camera_state2, *prk2, Rw2T, depth2, error_percent)
                    && error_percent < 0.02f) {
                    float distance = keypoint_compare(*k1, *k2);
                    if(distance < best_distance) {
                        second_best_distance = best_distance;
                        best_distance = distance;
                        best_depth1 = depth1;
                        best_depth2 = depth2;
                        best_error = error_percent;
                        best_k2 = k2;
                    } else if(distance < second_best_distance){
                        second_best_distance = distance;
                    }
                }
            }
            // Only match if we have exactly one candidate
            if(best_distance < DESCRIPTOR::good_track_distance && second_best_distance > DESCRIPTOR::good_track_distance) {
                if (f->map)
                    f->map->triangulated_tracks.erase(best_k2->feature->id); // FIXME: check if triangulated_tracks is more accurate than stereo match
                best_k2->feature = k1->feature;
                f->s.stereo_matches.emplace(k1->feature->id,
                                            stereo_match(camera_state1,      k1, best_depth1,
                                                         camera_state2, best_k2, best_depth2, best_error));
            }
        }

#endif
        END_EVENT(SF_STEREO_MATCH, 0)
    }
    return true;
}

bool filter_image_measurement(struct filter *f, const sensor_data & data)
{
    START_EVENT(SF_IMAGE_MEAS, 0);
    if(data.id >= f->cameras.size() || data.id >= f->s.cameras.children.size())
        return false;

    auto start = std::chrono::steady_clock::now();

    auto &camera_sensor = *f->cameras[data.id];
    auto &camera_state = *f->s.cameras.children[data.id];
    camera_sensor.got = true;

    camera_state.intrinsics.image_width = data.image.width;
    camera_state.intrinsics.image_height = data.image.height;

    sensor_clock::time_point time = data.timestamp;

    if(f->run_state == RCSensorFusionRunStateInactive) return false;
    if(!f->got_any_accelerometers() || !f->got_any_gyroscopes()) return false;
    if(!f->stereo_enabled && time - f->want_start < camera_wait_time) {
        for(auto &camera : f->cameras) if(!camera->got) return false;
    }

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

    if(f->run_state == RCSensorFusionRunStateRunning && f->detector_failed && time - f->detector_failed_time > max_detector_failed_time) {
        f->log->error("No features for 500ms; switching to orientation only.");
        f->run_state = RCSensorFusionRunStateDynamicInitialization;
        f->s.enable_orientation_only(true);
        f->s.disable_bias_estimation(true);
        if(f->map) f->map->triangulated_tracks.clear();
    }

    if(f->run_state == RCSensorFusionRunStateDynamicInitialization) {
        if(f->want_start == sensor_clock::micros_to_tp(0)) f->want_start = time;
        f->last_time = time;
        v3 non_up_var = f->s.Q.variance() - f->s.world.up * f->s.world.up.dot(f->s.Q.variance());
        bool inertial_converged = non_up_var[0] < dynamic_W_thresh_variance && non_up_var[1] < dynamic_W_thresh_variance && non_up_var[2] < dynamic_W_thresh_variance;
        if(inertial_converged) {
            f->log->debug("Inertial converged at time {}", std::chrono::duration_cast<std::chrono::microseconds>(time - f->want_start).count());
        } else return true;
    }
    if(f->run_state != RCSensorFusionRunStateRunning && f->run_state != RCSensorFusionRunStateDynamicInitialization) return true; //frame was "processed" so that callbacks still get called

    for(auto &camera : f->s.cameras.children) {
        if(camera->detection_future.valid()) camera->detection_future.wait();
    }

    if (camera_state.detection_future.valid()) {
        size_t detected = camera_state.detection_future.get();
        if (detected > 0) {
            auto active_features = camera_state.track_count();
            if(active_features < state_vision_group::min_feats) {
                f->log->info("detector failure: only {} features after add on camera {}", active_features, camera_state.id);
                camera_state.detector_failed = true;
                bool all_failed = true; for (const auto &c : f->s.cameras.children) all_failed &= c->detector_failed;
                if(all_failed) {
                    f->log->info("failed to detect in all cameras\n");
                    if(!f->detector_failed) f->detector_failed_time = time;
                    f->detector_failed = true;
                }
            } else if(active_features >= f->min_group_add) {
                camera_state.detector_failed = false;
                f->detector_failed = false;
            }
        }
    }

    if(f->run_state == RCSensorFusionRunStateRunning)
        filter_setup_next_frame(f, data); // put current features into observation queue as potential things to measure

    if(show_tuning) {
        fprintf(stderr, "\nvision:\n");
    }

    preprocess_observation_queue(f, time); // time update filter, then predict locations of current features in the observation queue
    camera_state.update_feature_tracks(data); // track the current features near their predicted locations
    process_observation_queue(f); // update state and covariance based on current location of tracked features

    for(auto &g : f->s.groups.children) {
        for(auto &i : g->features.children)
            i->tracks_found = 0;
        for(auto &i : g->lost_features)
            i->tracks_found = 0;
    }

    for(auto &c : f->s.cameras.children)
        for(auto &i : c->tracks)
            if(i.track.found()) ++i.feature.tracks_found;

    auto space = filter_available_feature_space(f);
    if(space) {
        for(auto &g : f->s.groups.children)
        {
            if(!space) break;
            g->lost_features.remove_if([&](std::unique_ptr<state_vision_feature> &f) {
                if(!space || !f->tracks_found)
                    return false;
                --space;
                f->set_initial_variance(recovered_feature_initial_variance);
                f->status = feature_normal;
                g->features.children.push_back(std::move(f));
                return true;
            });
        }
        f->s.remap();
    }

    for(auto &g : f->s.groups.children)
    {
        for(auto &feat : g->features.children)
        {
            if(!feat->is_good()) continue;
            feat->tracks.resize(f->s.cameras.children.size());
            for(size_t i = 0; i < feat->tracks.size(); ++i)
            {
                if(feat->tracks[i] == nullptr)
                    f->s.cameras.children[i]->tracks.emplace_back(i, *feat, tracker::feature_track(feat->feature, INFINITY, INFINITY, 0));
            }
        }
    }

    if(show_tuning) {
        for (auto &c : f->cameras)
            std::cerr << " innov  " << c->inn_stdev << "\n";
    }

    for (auto i = f->s.stereo_matches.begin(); i != f->s.stereo_matches.end(); )
        if (!i->second.views[0].track->found() || !i->second.views[1].track->found())
            i = f->s.stereo_matches.erase(i);
        else
            ++i;

    camera_state.process_tracks(f->map.get(), *f->log);
    auto normal_groups = f->s.process_features(f->map.get());
    filter_update_outputs(f, time, normal_groups == 0);
    f->s.remap();

    filter_update_triangulated_tracks(f, data.id);

    space = filter_available_feature_space(f);
    // bring groups back
    if(f->map && space >= f->min_group_map_add) {
        groupid closest_group_id;
        transformation G_Bclosest_Bnow;
        if(f->s.get_closest_group_transformation(closest_group_id, G_Bclosest_Bnow)) {
            camera_state.update_map_tracks(data, f->map.get(), f->min_group_map_add, closest_group_id, G_Bclosest_Bnow);
            if(f->map->map_feature_tracks.size()) {
                filter_bring_groups_back(f, data.id);
                space = filter_available_feature_space(f);
            }
        }
    }

    if(space >= f->min_group_add && camera_state.standby_tracks.size() >= f->min_group_add)
    {
#ifdef TEST_POSDEF
        if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def before adding group");
#endif
        if(f->run_state == RCSensorFusionRunStateDynamicInitialization) {
            f->s.disable_orientation_only();
#ifdef TEST_POSDEF
            if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def after disabling orient only");
#endif
            f->run_state = RCSensorFusionRunStateRunning;
            f->log->trace("When moving from steady init to running:");
            print_calibration(f);
            space = filter_available_feature_space(f);
        }
        filter_add_detected_features(f, camera_state, camera_sensor, space, data.image.height, time);
    }

    f->s.update_map(f->map.get());

    space = filter_available_feature_space(f);
    if(space >= f->min_group_add && camera_state.standby_tracks.size() < f->max_group_add) {
        camera_state.detecting_space = f->max_group_add;
    }
    END_EVENT(SF_IMAGE_MEAS, data.time_us / 1000);

    auto stop = std::chrono::steady_clock::now();
    camera_sensor.measure_time_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });

    return true;
}

//This should be called every time we want to initialize or reset the filter
void filter_initialize(struct filter *f)
{
    //changing these two doesn't affect much.
    f->min_group_add = 16;
    f->min_group_map_add = 11;
    f->max_group_add = std::max<int>(80 / f->cameras.size(), f->min_group_add);
    f->has_depth = false;
    f->stereo_enabled = false;
    f->relocalization_info.is_relocalized = false;

#ifdef INITIAL_DEPTH
    state_vision_feature::initial_depth_meters = INITIAL_DEPTH;
#else
    state_vision_feature::initial_depth_meters = M_E;
#endif
    state_vision_feature::initial_var = .75;
    state_vision_feature::initial_process_noise = 1.e-20;
    state_vision_track::outlier_thresh = 1;
    state_vision_track::outlier_reject = 30.;
    state_vision_track::outlier_lost_reject = 5.;
    state_vision_feature::max_variance = .10 * .10; //because of log-depth, the standard deviation is approximately a percentage (so .10 * .10 = 10%)
    state_vision_group::ref_noise = 1.e-30;
    state_vision_group::min_feats = 4;

    for (auto &g : f->gyroscopes)     g->init_with_variance(g->intrinsics.measurement_variance_rad2__s2, g->intrinsics.decimate_by);
    for (auto &a : f->accelerometers) a->init_with_variance(a->intrinsics.measurement_variance_m2__s4,   a->intrinsics.decimate_by);
    for (auto &v : f->velocimeters)   v->init_with_variance(v->intrinsics.measurement_variance_m2__s2,   v->intrinsics.decimate_by);
    for (auto &c : f->cameras)        c->init_with_variance(1 * 1);
    for (auto &d : f->depths)         d->init_with_variance(0);
    for (auto &t : f->thermometers)   t->init_with_variance(t->intrinsics.measurement_variance_C2);

    f->last_time = sensor_clock::time_point(sensor_clock::duration(0));
    f->want_start = sensor_clock::time_point(sensor_clock::duration(0));
    f->run_state = RCSensorFusionRunStateInactive;

    for (auto &c : f->s.cameras.children)
        c->detector_failed = false;
    f->detector_failed = false;
    f->tracker_failed = false;
    f->tracker_warned = false;
    f->speed_failed = false;
    f->speed_warning = false;
    f->numeric_failed = false;
    f->detector_failed_time = sensor_clock::time_point(sensor_clock::duration(0));
    f->speed_warning_time = sensor_clock::time_point(sensor_clock::duration(0));

    f->observations.observations.clear();
    f->mini->observations.observations.clear();
    f->catchup->observations.observations.clear();

    f->s.reset();
    if (f->map) f->map->reset();

    for (size_t i=0; i<f->s.cameras.children.size() && i<f->cameras.size(); i++) {
        state_camera &camera_state = *f->s.cameras.children[i];
        sensor_grey &camera_sensor = *f->cameras[i];

        camera_state.extrinsics.T.v = camera_sensor.extrinsics.mean.T;
        camera_state.extrinsics.Q.v = camera_sensor.extrinsics.mean.Q;

        camera_state.extrinsics.Q.set_initial_variance(camera_sensor.extrinsics.variance.Q);
        camera_state.extrinsics.T.set_initial_variance(camera_sensor.extrinsics.variance.T);

        camera_state.intrinsics.focal_length.v = (camera_sensor.intrinsics.f_x_px + camera_sensor.intrinsics.f_y_px)   / 2        / camera_sensor.intrinsics.height_px;
        camera_state.intrinsics.center.v.x()   = (camera_sensor.intrinsics.c_x_px - camera_sensor.intrinsics.width_px  / 2. + .5) / camera_sensor.intrinsics.height_px;
        camera_state.intrinsics.center.v.y()   = (camera_sensor.intrinsics.c_y_px - camera_sensor.intrinsics.height_px / 2. + .5) / camera_sensor.intrinsics.height_px;
        if (camera_sensor.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE)
            camera_state.intrinsics.k.v[0] = camera_sensor.intrinsics.w;
        else
            camera_state.intrinsics.k.v[0] = camera_sensor.intrinsics.k1;
        camera_state.intrinsics.k.v[1] = camera_sensor.intrinsics.k2;
        camera_state.intrinsics.k.v[2] = camera_sensor.intrinsics.k3;
        camera_state.intrinsics.k.v[3] = camera_sensor.intrinsics.k4;
        camera_state.intrinsics.type = camera_sensor.intrinsics.type;

        camera_state.extrinsics.Q.set_process_noise(1.e-30);
        camera_state.extrinsics.T.set_process_noise(1.e-30);

        camera_state.intrinsics.focal_length.set_process_noise(2.3e-3 / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);
        camera_state.intrinsics.center.set_process_noise(      2.3e-3 / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);
        camera_state.intrinsics.k.set_process_noise(2.3e-7);

        camera_state.intrinsics.focal_length.set_initial_variance(10. / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);
        camera_state.intrinsics.center.set_initial_variance(       2. / camera_sensor.intrinsics.height_px / camera_sensor.intrinsics.height_px);

        camera_state.intrinsics.k.set_initial_variance(camera_sensor.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE ? .01*.01 : 2.e-4);

        camera_state.intrinsics.image_width  = camera_sensor.intrinsics.width_px;
        camera_state.intrinsics.image_height = camera_sensor.intrinsics.height_px;

#ifdef MYRIAD2
#ifdef ENABLE_SHAVE_TRACKER
        camera_state.feature_tracker = std::make_unique<shave_tracker>();
#else
        camera_state.feature_tracker = std::make_unique<fast_tracker>();
#endif
#else // MYRIAD2
        if (/* DISABLES CODE */ (1))
            camera_state.feature_tracker = std::make_unique<fast_tracker>();
#ifdef HAVE_IPP
        else
            camera_state.feature_tracker = std::make_unique<ipp_tracker>();
#endif
#endif // MYRIAD2

        // send intrinsics and extrinsics to map
        if (f->map) {
            f->map->camera_intrinsics.push_back(&camera_state.intrinsics);
            f->map->camera_extrinsics.push_back(&camera_state.extrinsics);
        }
    }

    for (size_t i=f->s.imus.children.size(); i<f->gyroscopes.size(); i++)
        f->s.imus.children.emplace_back(std::make_unique<state_imu>());

    for (size_t i = 0; i < f->s.imus.children.size() && i < f->gyroscopes.size(); i++) {
        auto &imu = *f->s.imus.children[i];
        const auto &gyro = *f->gyroscopes[i];
        imu.intrinsics.w_bias.v = map(gyro.intrinsics.bias_rad__s.v);
        imu.intrinsics.w_bias.set_initial_variance(map(gyro.intrinsics.bias_variance_rad2__s2.v));
        imu.intrinsics.w_bias.set_process_noise(2.3e-10);

        imu.extrinsics.Q.v = gyro.extrinsics.mean.Q;
        imu.extrinsics.T.v = gyro.extrinsics.mean.T;

        imu.extrinsics.Q.set_process_noise(0);
        imu.extrinsics.T.set_process_noise(0);

        imu.extrinsics.Q.set_initial_variance(gyro.extrinsics.variance.Q);
        imu.extrinsics.T.set_initial_variance(gyro.extrinsics.variance.T);
    }

    for (size_t i=f->s.imus.children.size(); i<f->accelerometers.size(); i++)
        f->s.imus.children.emplace_back(std::make_unique<state_imu>());

    for (size_t i = 0; i < f->s.imus.children.size() && i < f->accelerometers.size(); i++) {
        auto &imu = *f->s.imus.children[i];
        const auto &accel = *f->accelerometers[i];
        imu.intrinsics.a_bias.v = map(accel.intrinsics.bias_m__s2.v);
        imu.intrinsics.a_bias.set_initial_variance(map(accel.intrinsics.bias_variance_m2__s4.v));
        imu.intrinsics.a_bias.set_process_noise(2.3e-8);

        imu.extrinsics.Q.v = accel.extrinsics.mean.Q;
        imu.extrinsics.T.v = accel.extrinsics.mean.T;

        imu.extrinsics.Q.set_process_noise(0);
        imu.extrinsics.T.set_process_noise(0);

        imu.extrinsics.Q.set_initial_variance(accel.extrinsics.variance.Q);
        imu.extrinsics.T.set_initial_variance(accel.extrinsics.variance.T);
    }


    for (size_t i=f->s.velocimeters.children.size(); i<f->velocimeters.size(); i++)
        f->s.velocimeters.children.emplace_back(std::make_unique<state_velocimeter>());

    for (size_t i = 0; i < f->s.velocimeters.children.size() && i < f->velocimeters.size(); i++) {
        auto &s_velocimeter = *f->s.velocimeters.children[i];
        const auto &velocimeter = *f->velocimeters[i];

        s_velocimeter.extrinsics.Q.v = velocimeter.extrinsics.mean.Q;
        s_velocimeter.extrinsics.T.v = velocimeter.extrinsics.mean.T;

        s_velocimeter.extrinsics.Q.set_process_noise(0);
        s_velocimeter.extrinsics.T.set_process_noise(0);

        s_velocimeter.extrinsics.Q.set_initial_variance(velocimeter.extrinsics.variance.Q);
        s_velocimeter.extrinsics.T.set_initial_variance(velocimeter.extrinsics.variance.T);
    }


    f->s.T.set_process_noise(0.);
    f->s.Q.set_process_noise(0.);
    f->s.V.set_process_noise(0.);
    f->s.w.set_process_noise(0.);
    f->s.dw.set_process_noise(0);
    f->s.a.set_process_noise(0);
    f->s.g.set_process_noise(0);

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

    f->mini->state.copy_from(f->s);
    f->catchup->state.copy_from(f->s);
}

//TODO: change the calibration parameters to floats
void filter_deinitialize(struct filter *f)
{
    for (size_t i = 0; i < f->s.cameras.children.size() && i < f->cameras.size(); i++) {
        const state_camera &camera_state = *f->s.cameras.children[i];
        sensor_grey &camera_sensor = *f->cameras[i];

        camera_sensor.intrinsics.width_px  = camera_state.intrinsics.image_width;
        camera_sensor.intrinsics.height_px = camera_state.intrinsics.image_height;
        camera_sensor.intrinsics.f_x_px    = camera_state.intrinsics.focal_length.v * camera_state.intrinsics.image_height;
        camera_sensor.intrinsics.f_y_px    = camera_state.intrinsics.focal_length.v * camera_state.intrinsics.image_height;
        camera_sensor.intrinsics.c_x_px    = camera_state.intrinsics.center.v.x()   * camera_state.intrinsics.image_height + camera_state.intrinsics.image_width / 2. - .5;
        camera_sensor.intrinsics.c_y_px    = camera_state.intrinsics.center.v.y()   * camera_state.intrinsics.image_height + camera_state.intrinsics.image_height / 2. - .5;

        switch((camera_sensor.intrinsics.type = camera_state.intrinsics.type)) {
        case rc_CALIBRATION_TYPE_FISHEYE:
            camera_sensor.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
            camera_sensor.intrinsics.w = camera_state.intrinsics.k.v[0];
            break;
        case rc_CALIBRATION_TYPE_POLYNOMIAL3:
            camera_sensor.intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
            camera_sensor.intrinsics.k1 = camera_state.intrinsics.k.v[0];
            camera_sensor.intrinsics.k2 = camera_state.intrinsics.k.v[1];
            camera_sensor.intrinsics.k3 = camera_state.intrinsics.k.v[2];
            break;
        case rc_CALIBRATION_TYPE_KANNALA_BRANDT4:
            camera_sensor.intrinsics.type = rc_CALIBRATION_TYPE_KANNALA_BRANDT4;
            camera_sensor.intrinsics.k1 = camera_state.intrinsics.k.v[0];
            camera_sensor.intrinsics.k2 = camera_state.intrinsics.k.v[1];
            camera_sensor.intrinsics.k3 = camera_state.intrinsics.k.v[2];
            camera_sensor.intrinsics.k4 = camera_state.intrinsics.k.v[3];
            break;
        case rc_CALIBRATION_TYPE_UNDISTORTED:
            break;
        case rc_CALIBRATION_TYPE_UNKNOWN:
            assert(0);
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

    for (size_t i = 0; i < f->s.imus.children.size() && i < f->accelerometers.size(); i++) {
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

    for (size_t i = 0; i < f->s.velocimeters.children.size(); i++) {
        const state_velocimeter &s_velocimeter = *f->s.velocimeters.children[i];
        sensor_velocimeter &velo = *f->velocimeters[i];
        struct sensor::extrinsics &velocimeter_extrinsics = velo.extrinsics;

        velocimeter_extrinsics.mean.T =     s_velocimeter.extrinsics.T.v;
        velocimeter_extrinsics.mean.Q =     s_velocimeter.extrinsics.Q.v;
        velocimeter_extrinsics.variance.Q = s_velocimeter.extrinsics.Q.variance();
        velocimeter_extrinsics.variance.T = s_velocimeter.extrinsics.T.variance();
    }
}

void filter_start(struct filter *f)
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
        statstr << i->name << "\t (detect):  " << i->detect_time_stats << "\n";
        statstr << i->name << "\t (reloc):   " << i->reloc_time_stats << "\n";
    }
    for(auto &i:f->accelerometers)
    {
        statstr << i->name << "\t (measure): " << i->measure_time_stats << "\n";
        statstr << i->name << "\t (fast):    " << i->fast_path_time_stats << "\n";
    }
    for(auto &i:f->gyroscopes)
    {
        statstr << i->name << "\t (measure): " << i->measure_time_stats << "\n";
        statstr << i->name << "\t (fast):    " << i->fast_path_time_stats << "\n";
    }
    statstr << "\n";
    return statstr.str();
}

void filter_update_triangulated_tracks(const filter *f, const rc_Sensor camera_id)
{
    // update triangulated 3d feature with new observation
    if(f->map) {
        groupid closest_group_id;
        transformation G_Bclosest_Bnow;
        bool valid_transformation = f->s.get_closest_group_transformation(closest_group_id, G_Bclosest_Bnow);
        auto &c = f->s.cameras.children[camera_id];
        for(auto &sbt : c->standby_tracks) {
            auto tp = f->map->triangulated_tracks.find(sbt.feature->id);
            if(tp != f->map->triangulated_tracks.end()) {
                if(!f->map->node_in_map(tp->second.reference_nodeid)) {
                    f->map->triangulated_tracks.erase(sbt.feature->id); //if reference node removed, remove triangulated feature too
                } else if (valid_transformation && tp->second.reference_nodeid != std::numeric_limits<nodeid>::max()) {
                    f->map->update_3d_feature(sbt, closest_group_id, invert(G_Bclosest_Bnow), camera_id);
                }
            }
        }
    }
}

void filter_bring_groups_back(filter *f, const rc_Sensor camera_id)
{
    if (f->map) {
        for(auto &mft : f->map->map_feature_tracks) {
            map_node &node = f->map->get_node(mft.group_id);
            auto &camera_node_state = *f->s.cameras.children[node.camera_id];

            auto space = filter_available_feature_space(f);
            if(space >= f->min_group_map_add) {
                if(mft.found >= f->min_group_map_add) {
                    TRACE_EVENT(SF_ADD_MAP_GROUP, mft.found);
                    auto g = std::make_unique<state_vision_group>(camera_node_state, mft.group_id);
                    g->Tr.v = mft.G_neighbor_now.T;
                    g->Qr.v = mft.G_neighbor_now.Q;
                    g->reused = true;
                    // g->Tr.set_initial_variance({0.1,0.1,0.1});
                    // g->Qr.set_initial_variance({0.1,0.1,0.1});
                    node.status = node_status::normal;

                    for(auto &ft : mft.tracks) {
                        auto feat = std::make_unique<state_vision_feature>(ft.track, *g);
                        feat->v = ft.v;
                        float std_pct = get_stdev_pct_for_depth(feat->v->depth());
                        feat->set_initial_variance(std_pct * std_pct); // assumes log depth
                        feat->depth_measured = false;
                        feat->recovered = true;

                        auto &camera_state = *f->s.cameras.children[camera_id];
                        if(space && ft.track.found()) {
                            --space;
                            feat->status = feature_normal;
                            camera_state.tracks.emplace_back(camera_state.id, *feat, std::move(ft.track));
                            g->features.children.push_back(std::move(feat));
                        } else {
                            // if there is no space or feature not found add to feature_lost
                            feat->status = feature_lost;
                            camera_state.tracks.emplace_back(camera_state.id, *feat, std::move(ft.track));
                            g->lost_features.push_back(std::move(feat));
                        }
                    }
                    for(auto &neighbor : f->s.groups.children) {
                        if(neighbor->status == group_reference)
                            f->map->add_edge(g->id, neighbor->id, (*g->Gr)*invert(*neighbor->Gr), edge_type::filter);
                        f->map->add_covisibility_edge(g->id, neighbor->id);
                    }
                    f->s.groups.children.push_back(std::move(g));
                    f->s.remap();
                } else {
                    break;
                }
            }
            else {
                break;
            }
        }
    }
}
