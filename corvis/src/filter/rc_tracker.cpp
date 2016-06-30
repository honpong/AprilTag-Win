#define RCTRACKER_API_EXPORTS
#include "rc_tracker.h"
#include "sensor_fusion.h"
#include "device_parameters.h"
#include "capture.h"
#include <fstream>

#define RC_STR_(x) #x
#define RC_STR(x) RC_STR_(x)
RCTRACKER_API const char *rc_copyright_ = "COPYRIGHT: Intel(r) RealSense(tm)";
RCTRACKER_API const char *rc_build_     = "BUILD: "   RC_STR(RC_BUILD);
#ifdef RC_VERSION
RCTRACKER_API const char *rc_version_   = "VERSION: " RC_STR(RC_VERSION);
#endif

const char *rc_version()
{
    return rc_build_;
}

std::unique_ptr<spdlog::logger> trace_log = std::make_unique<spdlog::logger>("rc_trace", make_shared<spdlog::sinks::null_sink_st> ());
static const bool trace = false;

static void rc_trace(const rc_Extrinsics e)
{
    trace_log->info("{} {} {} var({} {} {});\n {} {} {} (var {} {} {})", e.T.x, e.T.y, e.T.z, e.T_variance.x, e.T_variance.z, e.T_variance.z, e.W.x, e.W.y, e.W.z, e.W_variance.x, e.W_variance.y, e.W_variance.z);
}

static void rc_trace(const rc_Vector p)
{
    trace_log->info("{} {} {}", p.x, p.y, p.z);
}

static void rc_trace(const rc_Pose p)
{
    trace_log->info("{} {} {} {}; {} {} {} {}; {} {} {} {}", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11]);;
}

static void rc_trace(const rc_CameraIntrinsics c)
{
    trace_log->info("camera type {} w,h {} {} fx,fy {} {} cx,cy {} {} d[3] {} {} {}",
            c.type,
            c.width_px, c.height_px,
            c.f_x_px, c.f_y_px,
            c.c_x_px, c.c_y_px,
            c.distortion[0], c.distortion[1], c.distortion[2]);
}

static void transformation_to_rc_Pose(const transformation &g, rc_Pose p)
{
    m3 R = g.Q.toRotationMatrix();
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            p[i * 4 + j] = (float)R(i, j);
        }
        p[i * 4 + 3] = (float)g.T[i];
    }
}

static transformation rc_Extrinsics_to_transformation(const rc_Extrinsics e)
{
    return transformation(rotation_vector(e.W.x, e.W.y, e.W.z), v3(e.T.x, e.T.y, e.T.z));
}

static transformation rc_Pose_to_transformation(const rc_Pose p)
{
    transformation g;
    m3 R = m3::Zero();
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            R(i,j) = p[i * 4 + j];
        }
        g.T[i] = p[i * 4 + 3];
    }
    g.Q = to_quaternion(R);
    return g;
}

static rc_TrackerState tracker_state_from_run_state(RCSensorFusionRunState run_state)
{
    switch(run_state)
    {
        case RCSensorFusionRunStateDynamicInitialization:
            return rc_E_DYNAMIC_INITIALIZATION;
        case RCSensorFusionRunStateInactive:
            return rc_E_INACTIVE;
        case RCSensorFusionRunStateLandscapeCalibration:
            return rc_E_LANDSCAPE_CALIBRATION;
        case RCSensorFusionRunStatePortraitCalibration:
            return rc_E_PORTRAIT_CALIBRATION;
        case RCSensorFusionRunStateRunning:
            return rc_E_RUNNING;
        case RCSensorFusionRunStateStaticCalibration:
            return rc_E_STATIC_CALIBRATION;
        case RCSensorFusionRunStateSteadyInitialization:
            return rc_E_STEADY_INITIALIZATION;
        default: // This case should never be reached
            return rc_E_INACTIVE;
    }
}

static rc_TrackerError tracker_error_from_error(RCSensorFusionErrorCode error)
{
    switch(error)
    {
        case RCSensorFusionErrorCodeNone:
            return rc_E_ERROR_NONE;
        case RCSensorFusionErrorCodeOther:
            return rc_E_ERROR_OTHER;
        case RCSensorFusionErrorCodeVision:
            return rc_E_ERROR_VISION;
        case RCSensorFusionErrorCodeTooFast:
            return rc_E_ERROR_SPEED;
        default:
            return rc_E_ERROR_OTHER;
    }
}

static rc_TrackerConfidence tracker_confidence_from_confidence(RCSensorFusionConfidence confidence)
{
    switch(confidence)
    {
        case RCSensorFusionConfidenceHigh:
            return rc_E_CONFIDENCE_HIGH;
        case RCSensorFusionConfidenceMedium:
            return rc_E_CONFIDENCE_MEDIUM;
        case RCSensorFusionConfidenceLow:
            return rc_E_CONFIDENCE_LOW;
        case RCSensorFusionConfidenceNone:
        default:
            return rc_E_CONFIDENCE_NONE;
    }
}

struct rc_Tracker: public sensor_fusion
{
    rc_Tracker(bool immediate_dispatch): sensor_fusion(immediate_dispatch ? fusion_queue::latency_strategy::ELIMINATE_LATENCY : fusion_queue::latency_strategy::IMAGE_TRIGGER) {}
    std::unique_ptr<image_depth16> last_depth;
    std::string jsonString;
    std::vector<rc_Feature> gottenFeatures;
    std::vector<rc_Feature> dataFeatures;
    std::string timingStats;
    capture output;
};

static void copy_features_from_sensor_fusion(std::vector<rc_Feature> &features, const std::vector<sensor_fusion::feature_point> &in_feats)
{
    features.clear();
    features.reserve(in_feats.size());
    for (auto fp : in_feats)
    {
        rc_Feature feat;
        feat.image_x = static_cast<decltype(feat.image_x)>(fp.x);
        feat.image_y = static_cast<decltype(feat.image_y)>(fp.y);
        feat.world.x = static_cast<decltype(feat.world.x)>(fp.worldx);
        feat.world.y = static_cast<decltype(feat.world.y)>(fp.worldy);
        feat.world.z = static_cast<decltype(feat.world.z)>(fp.worldz);
        feat.stdev   = static_cast<decltype(feat.stdev)>(fp.stdev);
        feat.id = fp.id;
        feat.initialized = fp.initialized;
        features.push_back(feat);
    }
}

rc_Tracker * rc_create()
{
    rc_Tracker * tracker = new rc_Tracker(false); //don't dispatch immediately - intel doesn't really make any data interleaving guarantees
    tracker->sfm.ignore_lateness = true; //and don't drop frames to keep up
    if(trace) trace_log->info("rc_create");

    return tracker;
}

void rc_destroy(rc_Tracker * tracker)
{
    if(trace) trace_log->info("rc_destroy");
    delete tracker;
}

void rc_reset(rc_Tracker * tracker, rc_Timestamp initialTime_us, const rc_Pose initialPose_m)
{
    if(trace) trace_log->info("rc_reset {}", initialTime_us);
    if (initialPose_m)
        tracker->reset(sensor_clock::micros_to_tp(initialTime_us), rc_Pose_to_transformation(initialPose_m), false);
    else
        tracker->reset(sensor_clock::micros_to_tp(initialTime_us), transformation(), true);
}


void rc_configureCamera(rc_Tracker *tracker, rc_Sensor camera_id, const rc_Extrinsics * extrinsics_wrt_origin_m, const rc_CameraIntrinsics * intrinsics)
{
    //TODO: extrinsics
    //TODO: camera_id
    if(trace) {
        trace_log->info("rc_configureCamera {}", camera_id);
        rc_trace(*extrinsics_wrt_origin_m);
        rc_trace(*intrinsics);
    }
    // Make this given camera the current camera
    calibration_xml::camera *cam =
        (intrinsics->format == rc_FORMAT_GRAY8)   ? &tracker->device.color :
        (intrinsics->format == rc_FORMAT_DEPTH16) ? &tracker->device.depth : nullptr;
    if (cam && extrinsics_wrt_origin_m)
        cam->extrinsics_wrt_imu_m = rc_Extrinsics_to_transformation(*extrinsics_wrt_origin_m);
    if (cam && intrinsics)
        cam->intrinsics = *intrinsics;
}

bool rc_describeCamera(rc_Tracker *tracker,  rc_Sensor camera_id, rc_Extrinsics * extrinsics_wrt_origin_m,       rc_CameraIntrinsics *intrinsics)
{
    // TODO: camera id
    return false;
    /*
    // When you query a currently configure camera, you get the info from the current 'device' struct
    const calibration_xml::camera *cam =
        (camera_id == rc_CAMERA_ID_DEPTH   && tracker->device.depth.intrinsics.type != rc_CALIBRATION_TYPE_UNKNOWN) ? &tracker->device.depth :
        (camera_id == rc_CAMERA_ID_COLOR   && tracker->device.color.intrinsics.type != rc_CALIBRATION_TYPE_FISHEYE) ||
        (camera_id == rc_CAMERA_ID_FISHEYE && tracker->device.color.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE) ? &tracker->device.color :
    // When you query a currently unused camera, you get the data from the milti-camera calibration struct
         camera_id == rc_CAMERA_ID_FISHEYE ? &tracker->calibration.fisheye :
         camera_id == rc_CAMERA_ID_COLOR   ? &tracker->calibration.color :
         camera_id == rc_CAMERA_ID_DEPTH   ? &tracker->calibration.depth :
         camera_id == rc_CAMERA_ID_IR      ? &tracker->calibration.ir : nullptr;
    if (cam && extrinsics_wrt_accel_m)
        transformation_to_rc_Pose(cam->extrinsics_wrt_imu_m, extrinsics_wrt_accel_m);
    if (cam && intrinsics)
        *intrinsics = cam->intrinsics;

    if(trace) {
        trace_log->info("rc_describeCamera {}", camera_id);
        rc_trace(extrinsics_wrt_accel_m);
        rc_trace(*intrinsics);
    }

    return true;
    */
}

void rc_configureAccelerometer(rc_Tracker *tracker, rc_Sensor accel_id, const rc_Extrinsics * extrinsics_wrt_origin_m, const rc_AccelerometerIntrinsics * intrinsics)
{
    //TODO: multi-sensor
    //TODO: extrinsics
    if(trace) {
        trace_log->info("rc_configureAccelerometer {} noise {}", accel_id, intrinsics->measurement_variance_m2__s4);
        rc_trace(*extrinsics_wrt_origin_m);
        rc_trace(intrinsics->scale_and_alignment);
    }
    Eigen::Map<const Eigen::Matrix<float,3,4>>    a_alignment_bias_m__s2(intrinsics->scale_and_alignment);
    tracker->device.imu.a_alignment        = tracker->calibration.imu.a_alignment        = a_alignment_bias_m__s2.block<3,3>(0,0).cast<f_t>();
    tracker->device.imu.a_bias_m__s2       = tracker->calibration.imu.a_bias_m__s2       = a_alignment_bias_m__s2.block<3,1>(0,3).cast<f_t>();
    tracker->device.imu.a_noise_var_m2__s4 = tracker->calibration.imu.a_noise_var_m2__s4 = intrinsics->measurement_variance_m2__s4;
}

void rc_configureGyroscope(rc_Tracker *tracker, rc_Sensor gyro_id, const rc_Extrinsics * extrinsics_wrt_origin_m, const rc_GyroscopeIntrinsics * intrinsics)
{
    //TODO: multi-sensor
    //TODO: extrinsics
    if(trace) {
        trace_log->info("rc_configureGyroscope {} noise {}", gyro_id, intrinsics->measurement_variance_rad2__s2);
        rc_trace(*extrinsics_wrt_origin_m);
        rc_trace(intrinsics->scale_and_alignment);
    }
    Eigen::Map<const Eigen::Matrix<float,3,4>> w_alignment_bias_rad__s(intrinsics->scale_and_alignment);
    tracker->device.imu.w_alignment          = tracker->calibration.imu.w_alignment          = w_alignment_bias_rad__s.block<3,3>(0,0).cast<f_t>();;
    tracker->device.imu.w_bias_rad__s        = tracker->calibration.imu.w_bias_rad__s        = w_alignment_bias_rad__s.block<3,1>(0,3).cast<f_t>();;
    tracker->device.imu.w_noise_var_rad2__s2 = tracker->calibration.imu.w_noise_var_rad2__s2 = intrinsics->measurement_variance_rad2__s2;
}

void rc_configureLocation(rc_Tracker * tracker, double latitude_deg, double longitude_deg, double altitude_m)
{
    if(trace) trace_log->info("rc_configureLocation {} {} {}", latitude_deg, longitude_deg, altitude_m);
    tracker->set_location(latitude_deg, longitude_deg, altitude_m);
}

class rc_callback_sink_st : public spdlog::sinks::base_sink < spdlog::details::null_mutex >
{
private:
    void *handle;
    void (*callback)(void *, rc_MessageLevel, const char *, size_t);
    static const std::array<rc_MessageLevel, 10> rc_levels;
    static_assert(sizeof(spdlog::level::level_names)/sizeof(*spdlog::level::level_names) == rc_levels.max_size(), "rc_levels is consistent with spdlog::level::level_enum");

public:
    rc_callback_sink_st(rc_MessageCallback callback_, void *handle_) : handle(handle_), callback(callback_) {}
    spdlog::level::level_enum level(rc_MessageLevel rc_level) const {
        int i=0;
        for (rc_MessageLevel rc_l : rc_levels)
            if (rc_level == rc_l) return static_cast<spdlog::level::level_enum>(i); else i++;
        return spdlog::level::trace;
    }

protected:
    void _sink_it(const spdlog::details::log_msg& msg) override {
        if (callback)
            callback(handle, rc_levels[msg.level], msg.formatted.c_str(), msg.formatted.size());
    }

    void flush() override {}
};

const std::array<rc_MessageLevel, 10> rc_callback_sink_st::rc_levels = {
    rc_MESSAGE_TRACE, // trace    = 0,
    rc_MESSAGE_DEBUG, // debug    = 1,
    rc_MESSAGE_INFO,  // info     = 2,
    rc_MESSAGE_WARN,  // notice   = 3,
    rc_MESSAGE_WARN,  // warn     = 4,
    rc_MESSAGE_ERROR, // err      = 5,
    rc_MESSAGE_ERROR, // critical = 6,
    rc_MESSAGE_ERROR, // alert    = 7,
    rc_MESSAGE_ERROR, // emerg    = 8,
    rc_MESSAGE_NONE,  // off      = 9
};

RCTRACKER_API void rc_setMessageCallback(rc_Tracker *tracker, rc_MessageCallback callback, void *handle, rc_MessageLevel maximum_log_level)
{
    if(maximum_log_level > rc_MESSAGE_WARN)
        maximum_log_level = rc_MESSAGE_WARN;

    auto rc_sink = std::make_shared<rc_callback_sink_st>(callback, handle);
    tracker->sfm.s.log = std::make_unique<spdlog::logger>("rc_tracker", rc_sink);
    tracker->sfm.s.log->set_level(rc_sink->level(maximum_log_level));
    tracker->sfm.s.map.log = std::make_unique<spdlog::logger>("rc_tracker", rc_sink);
    tracker->sfm.s.map.log->set_level(rc_sink->level(maximum_log_level));
    if(trace) {
        trace_log = std::make_unique<spdlog::logger>("rc_trace", rc_sink);
        trace_log->set_pattern("%n: %v");
    }
}

RCTRACKER_API void rc_debug(rc_Tracker *tracker, rc_MessageLevel log_level, const char *msg)
{
    switch(log_level) {
    case rc_MESSAGE_TRACE: tracker->sfm.log->trace(msg); break;
    case rc_MESSAGE_DEBUG: tracker->sfm.log->debug(msg); break;
    case rc_MESSAGE_INFO:  tracker->sfm.log->info(msg);  break;
    case rc_MESSAGE_WARN:  tracker->sfm.log->warn(msg);  break;
    case rc_MESSAGE_ERROR: tracker->sfm.log->error(msg); break;
    case rc_MESSAGE_NONE:                                break;
    }
}

RCTRACKER_API void rc_setDataCallback(rc_Tracker *tracker, rc_DataCallback callback, void *handle)
{
    if(trace) trace_log->info("rc_setDataCallback");
    if(callback) tracker->camera_callback = [callback, handle, tracker](std::unique_ptr<sensor_fusion::data> d, image_gray8 &&cam) {
        uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(d->time.time_since_epoch()).count();

        rc_Pose p;
        transformation_to_rc_Pose(d->transform, p);
        copy_features_from_sensor_fusion(tracker->dataFeatures, d->features);
        callback(handle, micros, p, tracker->dataFeatures.data(), tracker->dataFeatures.size());
    };
    else tracker->camera_callback = nullptr;
}

RCTRACKER_API void rc_setStatusCallback(rc_Tracker *tracker, rc_StatusCallback callback, void *handle)
{
    if(trace) trace_log->info("rc_setStatusCallback");
    if(callback) tracker->status_callback = [callback, handle](sensor_fusion::status s) {
        callback(handle, tracker_state_from_run_state(s.run_state), tracker_error_from_error(s.error), tracker_confidence_from_confidence(s.confidence), static_cast<float>(s.progress));
    };
}

void rc_startCalibration(rc_Tracker * tracker, rc_TrackerRunFlags run_flags)
{
    if(trace) trace_log->info("rc_startCalibration {}", run_flags);
    tracker->start_calibration(run_flags == rc_E_ASYNCHRONOUS);
}

void rc_pauseAndResetPosition(rc_Tracker * tracker)
{
    if(trace) trace_log->info("rc_pauseAndResetPosition");
    tracker->pause_and_reset_position();
}

void rc_unpause(rc_Tracker *tracker)
{
    if(trace) trace_log->info("rc_unpause");
    tracker->unpause();
}

void rc_startBuffering(rc_Tracker * tracker)
{
    if(trace) trace_log->info("rc_startBuffering");
    tracker->start_buffering();
}

void rc_startTracker(rc_Tracker * tracker, rc_TrackerRunFlags run_flags)
{
    if(trace) trace_log->info("rc_startTracker");
    if (run_flags == rc_E_ASYNCHRONOUS)
        tracker->start_unstable(true);
    else
        tracker->start_offline();
}

void rc_stopTracker(rc_Tracker * tracker)
{
    if(trace) trace_log->info("rc_stopTracker");
    tracker->stop();
    if(tracker->output.started())
        tracker->output.stop();
}

void rc_startMapping(rc_Tracker *tracker)
{
    if(trace) trace_log->info("rc_startMapping");
    tracker->start_mapping();
}

void rc_stopMapping(rc_Tracker *tracker)
{
    if(trace) trace_log->info("rc_stopMapping");
    tracker->stop_mapping();
}

bool rc_loadMap(rc_Tracker *tracker, size_t (*read)(void *handle, void *buffer, size_t length), void *handle)
{
    if(trace) trace_log->info("rc_loadMap");
    return tracker->load_map(read, handle);
}

void rc_saveMap(rc_Tracker *tracker,  void (*write)(void *handle, const void *buffer, size_t length), void *handle)
{
    if(trace) trace_log->info("rc_saveMap");
    tracker->save_map(write, handle);
}

void rc_receiveImage(rc_Tracker *tracker, rc_Sensor camera_id, rc_ImageFormat format, rc_Timestamp time_us, rc_Timestamp shutter_time_us,
                     int width, int height, int stride, const void *image,
                     void(*completion_callback)(void *callback_handle), void *callback_handle)
{
    if(trace) {
        int pixel1 = -1;
        int pixel2 = -1;
        int pixel3 = -1;
        if(format == rc_FORMAT_DEPTH16) {
            pixel1 = ((uint16_t*)image)[width/2 + width * height/2 - 1];
            pixel2 = ((uint16_t*)image)[width/2 + width * height/2    ];
            pixel3 = ((uint16_t*)image)[width/2 + width * height/2 + 1];
        }
        else {
            pixel1 = ((uint8_t*)image)[width/2 + width * height/2 - 1];
            pixel2 = ((uint8_t*)image)[width/2 + width * height/2    ];
            pixel3 = ((uint8_t*)image)[width/2 + width * height/2 + 1];
        }
        trace_log->info("rc_receiveImage id,t,s,f {} {} {} {} w,h,s {} {} {} px {} {} {}", camera_id, time_us, shutter_time_us, format, width, height, stride, pixel1, pixel2, pixel3);
    }
    if (format == rc_FORMAT_DEPTH16) {
        image_depth16 d;
        d.source = tracker->sfm.depths[camera_id].get();
        d.image_handle = std::unique_ptr<void, void(*)(void *)>(callback_handle, completion_callback);
        d.image = (uint16_t *)image;
        d.width = width;
        d.height = height;
        d.stride = stride;
        d.timestamp = sensor_clock::micros_to_tp(time_us);
        d.exposure_time = std::chrono::microseconds(shutter_time_us);

        if(tracker->output.started())
            tracker->output.write_camera(camera_id, std::move(d));
        else
            tracker->receive_image(std::move(d));
    } else if (format == rc_FORMAT_GRAY8) {
        image_gray8 d;
        d.source = tracker->sfm.cameras[camera_id].get();
        d.image_handle = std::unique_ptr<void, void(*)(void *)>(callback_handle, completion_callback);
        d.image = (uint8_t *)image;
        d.width = width;
        d.height = height;
        d.stride = stride;
        d.timestamp = sensor_clock::micros_to_tp(time_us);
        d.exposure_time = std::chrono::microseconds(shutter_time_us);

        if(tracker->output.started())
            tracker->output.write_camera(camera_id, std::move(d));
        else
            tracker->receive_image(std::move(d));
    }
    tracker->trigger_log();
}

void rc_receiveAccelerometer(rc_Tracker * tracker, rc_Sensor accelerometer_id, rc_Timestamp time_us, const rc_Vector acceleration_m__s2)
{
    if(trace) trace_log->info("rc_receiveAccelerometer {} {}: {} {} {}", accelerometer_id, time_us, acceleration_m__s2.x, acceleration_m__s2.y, acceleration_m__s2.z);
    accelerometer_data d;
    d.source = tracker->sfm.accelerometers[accelerometer_id].get();
    d.accel_m__s2[0] = acceleration_m__s2.x;
    d.accel_m__s2[1] = acceleration_m__s2.y;
    d.accel_m__s2[2] = acceleration_m__s2.z;
    d.timestamp = sensor_clock::micros_to_tp(time_us);
    if(tracker->output.started())
        tracker->output.write_accelerometer(accelerometer_id, std::move(d));
    else
        tracker->receive_accelerometer(std::move(d));
}

void rc_receiveGyro(rc_Tracker * tracker, rc_Sensor gyroscope_id, rc_Timestamp time_us, const rc_Vector angular_velocity_rad__s)
{
    if(trace) trace_log->info("rc_receiveGyro {} {}: {} {} {}", gyroscope_id, time_us, angular_velocity_rad__s.x, angular_velocity_rad__s.y, angular_velocity_rad__s.z);
    gyro_data d;
    d.source = tracker->sfm.gyros[gyroscope_id].get();
    d.angvel_rad__s[0] = angular_velocity_rad__s.x;
    d.angvel_rad__s[1] = angular_velocity_rad__s.y;
    d.angvel_rad__s[2] = angular_velocity_rad__s.z;
    d.timestamp = sensor_clock::micros_to_tp(time_us);
    if(tracker->output.started())
        tracker->output.write_gyro(gyroscope_id, std::move(d));
    else
        tracker->receive_gyro(std::move(d));
}

void rc_getPose(const rc_Tracker * tracker, rc_Pose pose_m)
{
    if(trace) trace_log->info("rc_getPose");
    transformation_to_rc_Pose(tracker->get_transformation(), pose_m);
    if(trace) rc_trace(pose_m);
}

void rc_setPose(rc_Tracker * tracker, const rc_Pose pose_m)
{
    if(trace) {
        trace_log->info("rc_setPose");
        rc_trace(pose_m);
    }
    tracker->set_transformation(rc_Pose_to_transformation(pose_m));
}

int rc_getFeatures(rc_Tracker * tracker, rc_Feature **features_px)
{
    if(trace) trace_log->info("rc_getFeatures");
    copy_features_from_sensor_fusion(tracker->gottenFeatures, tracker->get_features());
    *features_px = tracker->gottenFeatures.data();
    return tracker->gottenFeatures.size();
}

rc_TrackerState rc_getState(const rc_Tracker *tracker)
{
    if(trace) trace_log->info("rc_getState");
    return tracker_state_from_run_state(tracker->sfm.run_state);
}

rc_TrackerConfidence rc_getConfidence(const rc_Tracker *tracker)
{
    if(trace) trace_log->info("rc_getConfidence");
    rc_TrackerConfidence confidence = rc_E_CONFIDENCE_NONE;
    if(tracker->sfm.run_state == RCSensorFusionRunStateRunning)
    {
        if(tracker->sfm.detector_failed) confidence = rc_E_CONFIDENCE_LOW;
        else if(tracker->sfm.has_converged) confidence = rc_E_CONFIDENCE_HIGH;
        else confidence = rc_E_CONFIDENCE_MEDIUM;
    }
    return confidence;
}

rc_TrackerError rc_getError(const rc_Tracker *tracker)
{
    if(trace) trace_log->info("rc_getError");
    rc_TrackerError error = rc_E_ERROR_NONE;
    if(tracker->sfm.numeric_failed) error = rc_E_ERROR_OTHER;
    else if(tracker->sfm.speed_failed) error = rc_E_ERROR_SPEED;
    else if(tracker->sfm.detector_failed) error = rc_E_ERROR_VISION;
    return error;
}

float rc_getProgress(const rc_Tracker *tracker)
{
    if(trace) trace_log->info("rc_getProgress");
    return filter_converged(&tracker->sfm);
}

void rc_setLog(rc_Tracker * tracker, void (*log)(void *handle, const char *buffer_utf8, size_t length), bool stream, rc_Timestamp period_us, void *handle)
{
    if(trace) trace_log->info("rc_setLog");
    tracker->set_log_function(log, stream, std::chrono::duration_cast<sensor_clock::duration>(std::chrono::microseconds(period_us)), handle);
}

void rc_triggerLog(const rc_Tracker * tracker)
{
    if(trace) trace_log->info("rc_triggerLog");
    tracker->trigger_log();
}

bool rc_setOutputLog(rc_Tracker * tracker, const char *filename, rc_TrackerRunFlags run_flags)
{
    if(trace) trace_log->info("rc_setOutputLog");
    return tracker->output.start(filename, run_flags == rc_E_ASYNCHRONOUS);
}

const char *rc_getTimingStats(rc_Tracker *tracker)
{
    if(trace) trace_log->info("rc_getTimingStats");
    tracker->timingStats = tracker->get_timing_stats();
    return tracker->timingStats.c_str();
}

size_t rc_getCalibration(rc_Tracker *tracker, const char **buffer)
{
    if(trace) trace_log->info("rc_getCalibration");
    device_parameters cal = tracker->get_device();

    std::string json;
    if (!calibration_serialize(cal, json))
        return 0;
    tracker->jsonString = json;
    *buffer = tracker->jsonString.c_str();
    return tracker->jsonString.length();
}

bool rc_setCalibration(rc_Tracker *tracker, const char *buffer)
{
    if(trace) trace_log->info("rc_setCalibration {}", buffer);
    std::string str(buffer);
    if (str.find("<") != std::string::npos && (str.find("{") == std::string::npos || str.find("<") < str.find("{"))) {
        struct calibration_xml multi_camera_calibration;
        if (!calibration_deserialize_xml(str, multi_camera_calibration))
            return false;
        // Store the multi-camera calibration for rc_describeCamera() and in case we want to write it back out
        tracker->calibration = multi_camera_calibration;
        // Pick the imu,depth,color combo from multi-camera calibration defaulting to fisheye if it's available
        device_parameters device;
        device.device_id = multi_camera_calibration.device_id;
        device.depth = multi_camera_calibration.depth;
        device.color = multi_camera_calibration.fisheye.intrinsics.type ?
            multi_camera_calibration.fisheye :
            multi_camera_calibration.color;
        device.imu   = multi_camera_calibration.imu;
        tracker->set_device(device);
    } else {
        device_parameters device;
        if (!calibration_deserialize(buffer, device))
            return false;
        if (tracker->calibration.device_id != "") // prefer the XML device_id (which usually has the serial number)
            device.device_id = tracker->calibration.device_id;
        tracker->set_device(device);
    }
    return true;
}
