#define RCTRACKER_API_EXPORTS
#include "rc_tracker.h"
#include "rc_compat.h"
#include "sensor_fusion.h"
#include "capture.h"
#include "calibration.h"
#include <fstream>

#define RC_STR_(x) #x
#define RC_STR(x) RC_STR_(x)
RCTRACKER_API const char *rc_copyright_ = "COPYRIGHT: Intel(r) RealSense(tm)";
RCTRACKER_API const char *rc_build_     = "BUILD: "   RC_STR(RC_BUILD);
#ifdef RC_VERSION
RCTRACKER_API const char *rc_version_   = "VERSION: " RC_STR(RC_VERSION);
#endif

std::unique_ptr<spdlog::logger> trace_log = std::make_unique<spdlog::logger>("rc_trace", make_shared<spdlog::sinks::null_sink_st> ());
static const bool trace = false;

const char *rc_version()
{
    if(trace) trace_log->info("rc_version: {}", rc_build_);
    return rc_build_;
}

static void rc_trace(const rc_Vector p)
{
    trace_log->info("{} {} {}", p.x, p.y, p.z);
}

static void rc_trace(const rc_Matrix p)
{
    trace_log->info("{} {} {}; {} {} {}; {} {} {}", p.v[0][0], p.v[0][1], p.v[0][2],  p.v[1][0], p.v[1][1], p.v[1][2],  p.v[2][0], p.v[2][1], p.v[2][2]);
}

static void rc_trace(const rc_Pose p)
{
    quaternion Q(v_map(p.Q.v).cast<f_t>()); m3 R = m_map(p.R.v).cast<f_t>(); v3 T = v_map(p.T.v).cast<f_t>();
    if (std::fabs(R.determinant() - 1) < std::fabs(Q.norm() - 1))
        trace_log->info("{} {} {}  {}; {} {} {}  {}; {} {} {}  {}",
                        p.R.v[0][0], p.R.v[0][1], p.R.v[0][2], p.T.v[0],
                        p.R.v[1][0], p.R.v[1][1], p.R.v[1][2], p.T.v[1],
                        p.R.v[2][0], p.R.v[2][1], p.R.v[2][2], p.T.v[2]);
    else
        trace_log->info("{} {} {} {}; {} {} {}", p.Q.w, p.Q.x, p.Q.y, p.Q.z, p.T.x, p.T.y, p.T.z);
}

static void rc_trace(const rc_Extrinsics e)
{
    rc_trace(e.pose_m); // FIXME: make rc_Pose, rc_Vector, rc_Matrix types you can serialize natively to spdlog, then all of this can be cleaned up
    trace_log->info("var({} {} {}; {} {} {})", e.variance_m2.W.x, e.variance_m2.W.y, e.variance_m2.W.z, e.variance_m2.T.x, e.variance_m2.T.y, e.variance_m2.T.z);
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

static void rc_trace(rc_Sensor camera_id, rc_ImageFormat format, rc_Timestamp time_us, rc_Timestamp shutter_time_us, int width, int height, int stride, const void *image) {
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
    rc_Tracker(bool immediate_dispatch): sensor_fusion(immediate_dispatch ? fusion_queue::latency_strategy::MINIMIZE_LATENCY : fusion_queue::latency_strategy::IMAGE_TRIGGER) {}
    std::unique_ptr<image_depth16> last_depth;
    std::string jsonString;
    std::vector<rc_Feature> gottenFeatures;
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

void rc_reset(rc_Tracker * tracker, rc_Timestamp initial_time_us)
{
    if(trace) trace_log->info("rc_reset {}", initial_time_us);
    tracker->reset(sensor_clock::micros_to_tp(initial_time_us));
}

bool rc_configureCamera(rc_Tracker *tracker, rc_Sensor camera_id, rc_ImageFormat format, const rc_Extrinsics *extrinsics_wrt_origin_m, const rc_CameraIntrinsics * intrinsics)
{
    if(trace) trace_log->info("rc_configureCamera {} format {}", camera_id, format);

    if(!extrinsics_wrt_origin_m || !intrinsics) return false;

    if(trace) {
        rc_trace(*extrinsics_wrt_origin_m);
        rc_trace(*intrinsics);
    }

    if (format == rc_FORMAT_GRAY8) {
        if(camera_id > tracker->sfm.cameras.size()) return false;
        if(camera_id == tracker->sfm.cameras.size()) {
            // new camera
            if(trace) trace_log->info(" configuring new camera");
            auto new_camera = std::make_unique<sensor_grey>(camera_id);
            tracker->sfm.cameras.push_back(std::move(new_camera));
        }

        tracker->sfm.cameras[camera_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
        tracker->sfm.cameras[camera_id]->intrinsics = *intrinsics;

        return true;
    }
    else if(format == rc_FORMAT_DEPTH16) {
        if(camera_id > tracker->sfm.depths.size()) return false;
        if(camera_id == tracker->sfm.depths.size()) {
            // new depth camera
            if(trace) trace_log->info(" configuring new camera");
            auto new_camera = std::make_unique<sensor_depth>(camera_id);
            tracker->sfm.depths.push_back(std::move(new_camera));
        }

        tracker->sfm.depths[camera_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
        tracker->sfm.depths[camera_id]->intrinsics = *intrinsics;

        return true;
    }
    return false;
}

bool rc_describeCamera(rc_Tracker *tracker,  rc_Sensor camera_id, rc_ImageFormat format, rc_Extrinsics *extrinsics_wrt_origin_m, rc_CameraIntrinsics *intrinsics)
{
    if(trace)
        trace_log->info("rc_describeCamera {}", camera_id);

    if(format == rc_FORMAT_GRAY8 && camera_id < tracker->sfm.cameras.size()) {
        if (extrinsics_wrt_origin_m)
            *extrinsics_wrt_origin_m = rc_Extrinsics_from_sensor_extrinsics(tracker->sfm.cameras[camera_id]->extrinsics);
        if (intrinsics)
            *intrinsics = tracker->sfm.cameras[camera_id]->intrinsics;
    } else if (format == rc_FORMAT_DEPTH16 && camera_id < tracker->sfm.depths.size()) {
        if (extrinsics_wrt_origin_m)
            *extrinsics_wrt_origin_m = rc_Extrinsics_from_sensor_extrinsics(tracker->sfm.depths[camera_id]->extrinsics);
        if (intrinsics)
            *intrinsics = tracker->sfm.depths[camera_id]->intrinsics;
    } else
        return false;

    if(trace) {
        rc_trace(*extrinsics_wrt_origin_m);
        rc_trace(*intrinsics);
    }
    return true;
}

bool rc_configureAccelerometer(rc_Tracker *tracker, rc_Sensor accel_id, const rc_Extrinsics * extrinsics_wrt_origin_m, const rc_AccelerometerIntrinsics *intrinsics)
{
    if(trace)
        trace_log->info("rc_configureAccelerometer {} noise {}", accel_id, intrinsics->measurement_variance_m2__s4);

    if(!extrinsics_wrt_origin_m || !intrinsics) return false;

    if(trace) {
        rc_trace(*extrinsics_wrt_origin_m);
        rc_trace(intrinsics->scale_and_alignment);
    }

    if(accel_id > tracker->sfm.accelerometers.size()) return false;
    if(accel_id == tracker->sfm.accelerometers.size()) {
        // new depth camera
        if(trace) trace_log->info(" configuring new accel");
        auto new_accel = std::make_unique<sensor_accelerometer>(accel_id);
        tracker->sfm.accelerometers.push_back(std::move(new_accel));
    }

    tracker->sfm.accelerometers[accel_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
    tracker->sfm.accelerometers[accel_id]->intrinsics = *intrinsics;

    return true;
}

bool rc_describeAccelerometer(rc_Tracker *tracker, rc_Sensor accel_id, rc_Extrinsics *extrinsics_wrt_origin_m, rc_AccelerometerIntrinsics *intrinsics)
{
    if(accel_id >= tracker->sfm.accelerometers.size())
        return false;

    if (extrinsics_wrt_origin_m)
        *extrinsics_wrt_origin_m = rc_Extrinsics_from_sensor_extrinsics(tracker->sfm.accelerometers[accel_id]->extrinsics);

    if (intrinsics)
        *intrinsics = tracker->sfm.accelerometers[accel_id]->intrinsics;

    if(trace) {
        trace_log->info("rc_describeAccelerometer {} noise {}", accel_id, intrinsics->measurement_variance_m2__s4);

        if (extrinsics_wrt_origin_m)
            rc_trace(*extrinsics_wrt_origin_m);
        if (intrinsics)
            rc_trace(intrinsics->scale_and_alignment);
    }
    return true;
}

bool rc_configureGyroscope(rc_Tracker *tracker, rc_Sensor gyro_id, const rc_Extrinsics * extrinsics_wrt_origin_m, const rc_GyroscopeIntrinsics * intrinsics)
{
    if(trace)
        trace_log->info("rc_configureGyroscope {} noise {}", gyro_id, intrinsics->measurement_variance_rad2__s2);

    if(!extrinsics_wrt_origin_m || !intrinsics) return false;

    if(trace) {
        rc_trace(*extrinsics_wrt_origin_m);
        rc_trace(intrinsics->scale_and_alignment);
    }

    if(gyro_id > tracker->sfm.gyroscopes.size()) return false;
    if(gyro_id == tracker->sfm.gyroscopes.size()) {
        // new depth camera
        if(trace) trace_log->info(" configuring new gyro");
        auto new_gyro = std::make_unique<sensor_gyroscope>(gyro_id);
        tracker->sfm.gyroscopes.push_back(std::move(new_gyro));
    }

    tracker->sfm.gyroscopes[gyro_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
    tracker->sfm.gyroscopes[gyro_id]->intrinsics = *intrinsics;

    return true;
}

bool rc_describeGyroscope(rc_Tracker *tracker, rc_Sensor gyro_id, rc_Extrinsics *extrinsics_wrt_origin_m, rc_GyroscopeIntrinsics *intrinsics)
{
    if(gyro_id >= tracker->sfm.gyroscopes.size())
        return false;

    if (extrinsics_wrt_origin_m)
        *extrinsics_wrt_origin_m = rc_Extrinsics_from_sensor_extrinsics(tracker->sfm.gyroscopes[gyro_id]->extrinsics);

    if (intrinsics)
        *intrinsics = tracker->sfm.gyroscopes[gyro_id]->intrinsics;

    if(trace) {
        trace_log->info("rc_describeGyroscope {} noise {}", gyro_id, intrinsics->measurement_variance_rad2__s2);

        if (extrinsics_wrt_origin_m)
            rc_trace(*extrinsics_wrt_origin_m);
        if (intrinsics)
            rc_trace(intrinsics->scale_and_alignment);
    }
    return true;
}

void rc_configureLocation(rc_Tracker * tracker, double latitude_deg, double longitude_deg, double altitude_m)
{
    if(trace) trace_log->info("rc_configureLocation {} {} {}", latitude_deg, longitude_deg, altitude_m);
    tracker->set_location(latitude_deg, longitude_deg, altitude_m);
}

void rc_configureWorld(rc_Tracker *tracker, const rc_Vector world_up, const rc_Vector world_initial_forward, const rc_Vector body_forward)
{
    tracker->sfm.s.world_up              = v_map(world_up.v).normalized();
    tracker->sfm.s.world_initial_forward = v_map(world_initial_forward.v).normalized();
    tracker->sfm.s.body_forward          = v_map(body_forward.v).normalized();
}

void rc_describeWorld(rc_Tracker *tracker, rc_Vector *world_up, rc_Vector *world_initial_forward, rc_Vector *body_forward)
{
    v_map(world_up->v)              = tracker->sfm.s.world_up;
    v_map(world_initial_forward->v) = tracker->sfm.s.world_initial_forward;
    v_map(body_forward->v)          = tracker->sfm.s.body_forward;
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
    if(callback) tracker->data_callback = [callback, handle, tracker](rc_SensorType type, rc_Sensor id) {
        uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(tracker->sfm.last_time.time_since_epoch()).count();
        callback(handle, micros, type, id);
    };
    else tracker->data_callback = nullptr;
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
    if (tracker->started())
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

bool rc_receiveImage(rc_Tracker *tracker, rc_Sensor camera_id, rc_ImageFormat format, rc_Timestamp time_us, rc_Timestamp shutter_time_us,
                     int width, int height, int stride, const void *image,
                     void(*completion_callback)(void *callback_handle), void *callback_handle)
{
    if(trace)
        rc_trace(camera_id, format, time_us, shutter_time_us, width, height, stride, image);

    if(!image) {
        tracker->sfm.log->error("Null image provided with sensor id {} format {} time {} shutter {} width {} height {} stride {}", camera_id, format, time_us, shutter_time_us, width, height, stride);
        return false;
    }
    if(width <= 0 || height <= 0 || stride < width * (format == rc_FORMAT_DEPTH16 ? 2 : 1)) {
        tracker->sfm.log->error("Incorrectly configured image sensor id {} format {} time {} shutter {} width {} height {} stride {}", camera_id, format, time_us, shutter_time_us, width, height, stride);
        return false;
    }
    if (format == rc_FORMAT_DEPTH16 && camera_id < tracker->sfm.depths.size()) {
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
            tracker->output.write_camera(camera_id, d.make_copy());
        if (tracker->started())
            tracker->receive_image(std::move(d));
    } else if (format == rc_FORMAT_GRAY8 && camera_id < tracker->sfm.cameras.size()) {
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
            tracker->output.write_camera(camera_id, d.make_copy());
        if (tracker->started())
            tracker->receive_image(std::move(d));
    }
    else
        return false;

    return true;
}

bool rc_receiveAccelerometer(rc_Tracker * tracker, rc_Sensor accelerometer_id, rc_Timestamp time_us, const rc_Vector acceleration_m__s2)
{
    if(trace) trace_log->info("rc_receiveAccelerometer {} {}: {} {} {}", accelerometer_id, time_us, acceleration_m__s2.x, acceleration_m__s2.y, acceleration_m__s2.z);
    if (accelerometer_id >= tracker->sfm.accelerometers.size())
        return false;
    accelerometer_data d;
    d.source = tracker->sfm.accelerometers[accelerometer_id].get();
    v_map(d.acceleration_m__s2) = v_map(acceleration_m__s2.v).cast<float>();
    d.timestamp = sensor_clock::micros_to_tp(time_us);
    if(tracker->output.started())
        tracker->output.write_accelerometer(accelerometer_id, std::move(d));
    if (tracker->started())
        tracker->receive_accelerometer(std::move(d));
    return true;
}

bool rc_receiveGyro(rc_Tracker * tracker, rc_Sensor gyroscope_id, rc_Timestamp time_us, const rc_Vector angular_velocity_rad__s)
{
    if(trace) trace_log->info("rc_receiveGyro {} {}: {} {} {}", gyroscope_id, time_us, angular_velocity_rad__s.x, angular_velocity_rad__s.y, angular_velocity_rad__s.z);
    if (gyroscope_id >= tracker->sfm.gyroscopes.size())
        return false;
    gyro_data d;
    d.source = tracker->sfm.gyroscopes[gyroscope_id].get();
    v_map(d.angular_velocity_rad__s) = v_map(angular_velocity_rad__s.v).cast<float>();
    d.timestamp = sensor_clock::micros_to_tp(time_us);
    if(tracker->output.started())
        tracker->output.write_gyro(gyroscope_id, std::move(d));
    if (tracker->started())
        tracker->receive_gyro(std::move(d));
    return true;
}

rc_Pose rc_getPose(const rc_Tracker * tracker, rc_PoseVelocity *v, rc_PoseAcceleration *a)
{
    if(trace) trace_log->info("rc_getPose");
    transformation total = tracker->sfm.origin * tracker->sfm.s.loop_offset;
    if (v) v_map(v->W.v) = total.Q * tracker->sfm.s.Q.v * tracker->sfm.s.w.v; // we use body rotational velocity, but we export spatial
    if (a) v_map(a->W.v) = total.Q * tracker->sfm.s.Q.v * tracker->sfm.s.dw.v;
    if (v) v_map(v->T.v) = total.Q * tracker->sfm.s.V.v;
    if (a) v_map(a->T.v) = total.Q * tracker->sfm.s.a.v;
    rc_Pose pose_m = to_rc_Pose(total * transformation(tracker->sfm.s.Q.v, tracker->sfm.s.T.v));
    // assert(pose_m == tracker->get_transformation()); // FIXME: this depends on the specific implementation of ->get_transformation()
    if(trace) rc_trace(pose_m);
    return pose_m;
}

void rc_setPose(rc_Tracker * tracker, const rc_Pose pose_m)
{
    if(trace) {
        trace_log->info("rc_setPose");
        rc_trace(pose_m);
    }
    tracker->set_transformation(to_transformation(pose_m));
}

int rc_getFeatures(rc_Tracker * tracker, rc_Feature **features_px)
{
    if(trace) trace_log->info("rc_getFeatures");
    copy_features_from_sensor_fusion(tracker->gottenFeatures, tracker->get_features());
    if (features_px) *features_px = tracker->gottenFeatures.data();
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
    calibration cal;

    size_t size = std::min(tracker->sfm.accelerometers.size(), tracker->sfm.gyroscopes.size());
    cal.imus.resize(size);
    for (int id = 0; id < size; id++) {
        rc_describeGyroscope(tracker, id, &cal.imus[id].extrinsics, &cal.imus[id].intrinsics.gyroscope);
        rc_describeAccelerometer(tracker, id, &cal.imus[id].extrinsics, &cal.imus[id].intrinsics.accelerometer);
    }

    cal.cameras.resize(tracker->sfm.cameras.size());
    for (int id = 0; id < tracker->sfm.cameras.size(); id++)
        rc_describeCamera(tracker, id, rc_FORMAT_GRAY8, &cal.cameras[id].extrinsics, &cal.cameras[id].intrinsics);

    cal.depths.resize(tracker->sfm.depths.size());
    for (int id = 0; id < tracker->sfm.depths.size(); id++)
        rc_describeCamera(tracker, id, rc_FORMAT_DEPTH16, &cal.depths[id].extrinsics, &cal.depths[id].intrinsics);

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
    calibration cal;
    if(!calibration_deserialize(buffer, cal))
        return false;

    tracker->sfm.cameras.clear();
    tracker->sfm.depths.clear();
    tracker->sfm.accelerometers.clear();
    tracker->sfm.gyroscopes.clear();

    int id = 0;
    for(auto imu : cal.imus) {
        rc_configureAccelerometer(tracker, id, &imu.extrinsics, &imu.intrinsics.accelerometer);
        rc_configureGyroscope(tracker, id, &imu.extrinsics, &imu.intrinsics.gyroscope);
        id++;
    }

    id = 0;
    for(auto camera : cal.cameras)
        rc_configureCamera(tracker, id++, rc_FORMAT_GRAY8, &camera.extrinsics, &camera.intrinsics);

    id = 0;
    for(auto depth : cal.depths)
        rc_configureCamera(tracker, id++, rc_FORMAT_DEPTH16, &depth.extrinsics, &depth.intrinsics);

    return true;
}
