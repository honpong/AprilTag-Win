#ifdef _WIN32
#define RCTRACKER_API_EXPORTS
#endif
#include "rc_tracker.h"
#include "rc_compat.h"
#include "rc_internal.h"
#include "sensor_fusion.h"
#include "capture.h"
#include "calibration.h"
#include "sensor_data.h"
#include <fstream>

using rc::map;

//We want these strings to be in the build but not in the header
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations"
#define RC_STR_(x) #x
#define RC_STR(x) RC_STR_(x)
RCTRACKER_API const char *rc_copyright_ = "COPYRIGHT: Intel(r) RealSense(tm)";
RCTRACKER_API const char *rc_build_     = "BUILD: "   RC_STR(RC_BUILD);
#ifdef RC_VERSION
RCTRACKER_API const char *rc_version_   = "VERSION: " RC_STR(RC_VERSION);
#endif
#pragma GCC diagnostic pop

const char *rc_version()
{
    return rc_build_;
}

struct rc_Tracker: public sensor_fusion
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    rc_Tracker(bool placement_ = false): sensor_fusion(fusion_queue::latency_strategy::MINIMIZE_DROPS), placement(placement_) {}
    bool placement;
    std::string jsonString;
    std::vector<std::vector<rc_Feature> > stored_features;
    std::vector<rc_Pose> relocalization_G_world_body;
    std::vector<rc_RelocEdge> relocalization_edges;
    std::vector<rc_MapNode> map_nodes;
    std::string timingStats;
    capture output;
    struct status {
        rc_TrackerState run_state = rc_E_INACTIVE;
        rc_TrackerError error = rc_E_ERROR_NONE;
        rc_TrackerConfidence confidence = rc_E_CONFIDENCE_NONE;
        status() {}
        status(rc_Tracker *tracker) : run_state(rc_getState(tracker)), error(rc_getError(tracker)), confidence(rc_getConfidence(tracker)) {}
        bool operator!=(const struct status &o) { return run_state != o.run_state || error != o.error || confidence != o.confidence; }
    } last;
};

static bool is_configured(const rc_Tracker * tracker)
{
    return tracker->sfm.cameras.size() && tracker->sfm.accelerometers.size() && tracker->sfm.gyroscopes.size();
}

rc_Tracker * rc_create()
{
    return new rc_Tracker();
}

rc_Tracker *rc_create_at(void *mem, size_t *size)
{
    if (size) *size = sizeof(rc_Tracker);
    return mem ? new (mem) rc_Tracker(true) : nullptr;
}

void rc_destroy(rc_Tracker * tracker)
{
    if (tracker->placement)
        tracker->~rc_Tracker();
    else
        delete tracker;
}

void rc_reset(rc_Tracker * tracker, rc_Timestamp initial_time_us)
{
    if(!is_configured(tracker)) return;
    tracker->reset(sensor_clock::micros_to_tp(initial_time_us));
}

bool rc_configureCamera(rc_Tracker *tracker, rc_Sensor camera_id, rc_ImageFormat format, const rc_Extrinsics *extrinsics_wrt_origin_m, const rc_CameraIntrinsics * intrinsics)
{
    if(!extrinsics_wrt_origin_m || !intrinsics) return false;

    if (format == rc_FORMAT_GRAY8) {
        if(camera_id > tracker->sfm.cameras.size()) return false;
        if(camera_id == tracker->sfm.cameras.size()) {
            auto new_camera = std::make_unique<sensor_grey>(camera_id);
            tracker->sfm.cameras.push_back(std::move(new_camera));
            tracker->queue.require_sensor(rc_SENSOR_TYPE_IMAGE, camera_id, std::chrono::milliseconds(10));
            if (camera_id == tracker->sfm.s.cameras.children.size())
                tracker->sfm.s.cameras.children.emplace_back(std::make_unique<state_camera>(camera_id));
        }

        tracker->sfm.cameras[camera_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
        tracker->sfm.cameras[camera_id]->intrinsics = *intrinsics;

        return true;
    }
    else if(format == rc_FORMAT_DEPTH16) {
        if(camera_id > tracker->sfm.depths.size()) return false;
        if(camera_id == tracker->sfm.depths.size()) {
            auto new_camera = std::make_unique<sensor_depth>(camera_id);
            tracker->sfm.depths.push_back(std::move(new_camera));
            tracker->queue.require_sensor(rc_SENSOR_TYPE_DEPTH, camera_id, std::chrono::milliseconds(25));
        }

        tracker->sfm.depths[camera_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
        tracker->sfm.depths[camera_id]->intrinsics = *intrinsics;

        return true;
    }
    return false;
}

bool rc_describeCamera(rc_Tracker *tracker,  rc_Sensor camera_id, rc_ImageFormat format, rc_Extrinsics *extrinsics_wrt_origin_m, rc_CameraIntrinsics *intrinsics)
{
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

    return true;
}

bool rc_configureStereo(rc_Tracker *tracker, rc_Sensor camera_id_0, rc_Sensor camera_id_1)
{
    tracker->sfm.stereo_enabled = true;
    tracker->queue.require_sensor(rc_SENSOR_TYPE_STEREO, camera_id_0, std::chrono::milliseconds(25));
    return true;
}

bool rc_configureAccelerometer(rc_Tracker *tracker, rc_Sensor accel_id, const rc_Extrinsics * extrinsics_wrt_origin_m, const rc_AccelerometerIntrinsics *intrinsics)
{
    if(!extrinsics_wrt_origin_m || !intrinsics) return false;

    auto &accelerometers = tracker->sfm.accelerometers;

    if(accel_id > accelerometers.size()) return false;
    if(accel_id == accelerometers.size()) {
        tracker->queue.require_sensor(rc_SENSOR_TYPE_ACCELEROMETER, accel_id, std::chrono::microseconds(
            /* accel group delay @ 62.5Hz */     13800
            /* gyro group delay @ 200Hz */      - 4500
            /* typical system latency on TM2 */ +  600));
        accelerometers.push_back(std::make_unique<sensor_accelerometer>(accel_id));
    }

    accelerometers[accel_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
    accelerometers[accel_id]->intrinsics = *intrinsics;

    return true;
}

bool rc_describeAccelerometer(rc_Tracker *tracker, rc_Sensor accel_id, rc_Extrinsics *extrinsics_wrt_origin_m, rc_AccelerometerIntrinsics *intrinsics)
{
    if(accel_id >= tracker->sfm.accelerometers.size())
        return false;

    const auto &accelerometer = tracker->sfm.accelerometers[accel_id];

    if (extrinsics_wrt_origin_m)
        *extrinsics_wrt_origin_m = rc_Extrinsics_from_sensor_extrinsics(accelerometer->extrinsics);

    if (intrinsics)
        *intrinsics = accelerometer->intrinsics;

    return true;
}

bool rc_configureGyroscope(rc_Tracker *tracker, rc_Sensor gyro_id, const rc_Extrinsics * extrinsics_wrt_origin_m, const rc_GyroscopeIntrinsics * intrinsics)
{
    if(!extrinsics_wrt_origin_m || !intrinsics) return false;

    auto &gyroscopes = tracker->sfm.gyroscopes;

    if(gyro_id > gyroscopes.size()) return false;
    if(gyro_id == gyroscopes.size()) {
        tracker->queue.require_sensor(rc_SENSOR_TYPE_GYROSCOPE, gyro_id, std::chrono::microseconds(600));
        gyroscopes.push_back(std::make_unique<sensor_gyroscope>(gyro_id));
    }

    gyroscopes[gyro_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
    gyroscopes[gyro_id]->intrinsics = *intrinsics;

    return true;
}

bool rc_describeGyroscope(rc_Tracker *tracker, rc_Sensor gyro_id, rc_Extrinsics *extrinsics_wrt_origin_m, rc_GyroscopeIntrinsics *intrinsics)
{
    if(gyro_id >= tracker->sfm.gyroscopes.size())
        return false;

    const auto &gyroscope = tracker->sfm.gyroscopes[gyro_id];

    if (extrinsics_wrt_origin_m)
        *extrinsics_wrt_origin_m = rc_Extrinsics_from_sensor_extrinsics(gyroscope->extrinsics);

    if (intrinsics)
        *intrinsics = gyroscope->intrinsics;

    return true;
}

bool rc_configureThermometer(rc_Tracker *tracker, rc_Sensor therm_id, const rc_Extrinsics *extrinsics_wrt_origin_m, const rc_ThermometerIntrinsics *intrinsics)
{
    if(!extrinsics_wrt_origin_m || !intrinsics) return false;

    auto &thermometers = tracker->sfm.thermometers;


    if(therm_id > thermometers.size()) return false;
    if(therm_id == thermometers.size()) {
        //tracker->queue.require_sensor(rc_SENSOR_TYPE_THERMOMETER, therm_id, std::chrono::microseconds(600));
        thermometers.push_back(std::make_unique<sensor_thermometer>(therm_id));
    }

    thermometers[therm_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
    thermometers[therm_id]->intrinsics = *intrinsics;

    return true;
}

bool rc_describeThermometer(rc_Tracker *tracker, rc_Sensor therm_id, rc_Extrinsics *extrinsics_wrt_origin_m, rc_ThermometerIntrinsics *intrinsics)
{
    if(therm_id >= tracker->sfm.thermometers.size())
        return false;

    const auto &thermometer = tracker->sfm.thermometers[therm_id];

    if (extrinsics_wrt_origin_m)
        *extrinsics_wrt_origin_m = rc_Extrinsics_from_sensor_extrinsics(thermometer->extrinsics);

    if (intrinsics)
        *intrinsics = thermometer->intrinsics;

    return true;
}

bool rc_configureVelocimeter(rc_Tracker *tracker, rc_Sensor velo_id, const rc_Extrinsics * extrinsics_wrt_origin_m, const rc_VelocimeterIntrinsics * intrinsics)
{
    if(!extrinsics_wrt_origin_m || !intrinsics) return false;

    if(velo_id > tracker->sfm.velocimeters.size()) return false;
    if(velo_id == tracker->sfm.velocimeters.size()) {
        tracker->queue.require_sensor(rc_SENSOR_TYPE_VELOCIMETER, velo_id, std::chrono::microseconds(600));
        auto new_velo = std::make_unique<sensor_velocimeter>(velo_id);
        tracker->sfm.velocimeters.push_back(std::move(new_velo));
    }

    tracker->sfm.velocimeters[velo_id]->extrinsics = rc_Extrinsics_to_sensor_extrinsics(*extrinsics_wrt_origin_m);
    tracker->sfm.velocimeters[velo_id]->intrinsics = *intrinsics;

    return true;
}

bool rc_describeVelocimeter(rc_Tracker *tracker, rc_Sensor velo_id, rc_Extrinsics *extrinsics_wrt_origin_m, rc_VelocimeterIntrinsics *intrinsics)
{
    if(velo_id >= tracker->sfm.velocimeters.size())
        return false;

    if (extrinsics_wrt_origin_m)
        *extrinsics_wrt_origin_m = rc_Extrinsics_from_sensor_extrinsics(tracker->sfm.velocimeters[velo_id]->extrinsics);

    if (intrinsics)
        *intrinsics = tracker->sfm.velocimeters[velo_id]->intrinsics;

    return true;
}

void rc_configureLocation(rc_Tracker * tracker, double latitude_deg, double longitude_deg, double altitude_m)
{
    tracker->set_location(latitude_deg, longitude_deg, altitude_m);
}

void rc_configureWorld(rc_Tracker *tracker, const rc_Vector world_up, const rc_Vector world_initial_forward, const rc_Vector body_forward)
{
    tracker->sfm.s.world.up              = map(world_up.v).normalized();
    tracker->sfm.s.world.initial_forward = map(world_initial_forward.v).normalized();
    tracker->sfm.s.world.initial_left    = tracker->sfm.s.world.up.cross(tracker->sfm.s.world.initial_forward);
    tracker->sfm.s.body_forward          = map(body_forward.v).normalized();
}

void rc_describeWorld(rc_Tracker *tracker, rc_Vector *world_up, rc_Vector *world_initial_forward, rc_Vector *body_forward)
{
    map(world_up->v)              = tracker->sfm.s.world.up;
    map(world_initial_forward->v) = tracker->sfm.s.world.initial_forward;
    map(body_forward->v)          = tracker->sfm.s.body_forward;
}

bool rc_configureQueueStrategy(rc_Tracker *tracker, rc_TrackerQueueStrategy strategy)
{
    switch(strategy) {
        case rc_QUEUE_MINIMIZE_DROPS:
            tracker->queue.strategy = fusion_queue::latency_strategy::MINIMIZE_DROPS;
            return true;

        case rc_QUEUE_MINIMIZE_LATENCY:
            tracker->queue.strategy = fusion_queue::latency_strategy::MINIMIZE_LATENCY;
            return true;
    }

    return false;
}

bool rc_describeQueueStrategy(rc_Tracker *tracker, rc_TrackerQueueStrategy * strategy)
{
    *strategy = (rc_TrackerQueueStrategy)tracker->queue.strategy;
    return true;
}

class rc_callback_sink_st : public spdlog::sinks::base_sink < spdlog::details::null_mutex >
{
private:
    void *handle;
    void (*callback)(void *, rc_MessageLevel, const char *, size_t);
    static const std::array<rc_MessageLevel, 7> rc_levels;
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

const std::array<rc_MessageLevel, 7> rc_callback_sink_st::rc_levels {{
    rc_MESSAGE_TRACE, // trace    = 0,
    rc_MESSAGE_DEBUG, // debug    = 1,
    rc_MESSAGE_INFO,  // info     = 2,
    rc_MESSAGE_WARN,  // warn     = 3,
    rc_MESSAGE_ERROR, // err      = 4,
    rc_MESSAGE_ERROR, // critical = 5,
    rc_MESSAGE_NONE,  // off      = 6
}};

RCTRACKER_API void rc_setMessageCallback(rc_Tracker *tracker, rc_MessageCallback callback, void *handle, rc_MessageLevel maximum_log_level)
{
#if defined(RC_VERSION) && !defined(DEBUG)
    bool debug_unavailable = false;
    if ((debug_unavailable = maximum_log_level > rc_MESSAGE_WARN))
        maximum_log_level = rc_MESSAGE_WARN;
#endif

    auto rc_sink = std::make_shared<rc_callback_sink_st>(callback, handle);
    tracker->sfm.s.log = std::make_unique<spdlog::logger>("rc_tracker", rc_sink);
    tracker->sfm.s.log->set_level(rc_sink->level(maximum_log_level));
    if (tracker->sfm.map) {
        tracker->sfm.map->log = std::make_unique<spdlog::logger>("rc_tracker", rc_sink);
        tracker->sfm.map->log->set_level(rc_sink->level(maximum_log_level));
    }
#if defined(RC_VERSION) && !defined(DEBUG)
    if (debug_unavailable)
        tracker->sfm.s.log->warn("INFO, DEBUG, TRACE message levels are unavailable.");
#endif
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
    if(callback) tracker->data_callback = [callback, handle, tracker](const rc_Data * data) {
        callback(handle, tracker, data);
    };
    else tracker->data_callback = nullptr;
}

RCTRACKER_API void rc_setStatusCallback(rc_Tracker *tracker, rc_StatusCallback callback, void *handle)
{
    tracker->status_callback = [callback, handle, tracker]() -> bool {
        rc_Tracker::status current(tracker);
        bool diff = tracker->last != current;
        tracker->last = current;
        if (diff && callback)
            callback(handle, current.run_state, current.error, current.confidence);
        return diff;
    };
}

RCTRACKER_API void rc_setStageCallback(rc_Tracker *tracker, rc_StageCallback callback, void *handle)
{
    if(callback) tracker->stage_callback = [callback, handle, tracker]() {
        for (rc_Stage stage = {}; rc_getStage(tracker, nullptr, &stage); )
            callback(handle, tracker, &stage);
    };
    else tracker->stage_callback = nullptr;
}

void rc_pauseAndResetPosition(rc_Tracker * tracker)
{
    tracker->pause_and_reset_position();
}

void rc_unpause(rc_Tracker *tracker)
{
    tracker->unpause();
}

bool rc_startBuffering(rc_Tracker * tracker)
{
    if(!is_configured(tracker)) return false;
    tracker->start_buffering();
    return true;
}

bool rc_startTracker(rc_Tracker * tracker, rc_TrackerRunFlags run_flags)
{
    if(!is_configured(tracker)) return false;
    tracker->start(run_flags & rc_RUN_ASYNCHRONOUS, run_flags & rc_RUN_FAST_PATH, run_flags & rc_RUN_DYNAMIC_CALIBRATION);
    return true;
}

void rc_stopTracker(rc_Tracker * tracker)
{
    if (tracker->started())
        tracker->stop();
    if(tracker->output.started())
        tracker->output.stop();
}

void rc_startMapping(rc_Tracker *tracker, bool relocalize, bool save_map)
{
    tracker->start_mapping(relocalize, save_map);
}

void rc_stopMapping(rc_Tracker *tracker)
{
    tracker->stop_mapping();
}

bool rc_loadMap(rc_Tracker *tracker, rc_LoadCallback read, void *handle)
{
    return tracker->load_map(read, handle);
}

void rc_saveMap(rc_Tracker *tracker, rc_SaveCallback write, void *handle)
{
    tracker->save_map(write, handle);
}

bool rc_receiveStereo(rc_Tracker *tracker, rc_Sensor stereo_id, rc_ImageFormat format, rc_Timestamp time_us, rc_Timestamp shutter_time_us,
                     int width, int height, int stride1, int stride2, const void *image1, const void * image2,
                     void(*completion_callback)(void *callback_handle), void *callback_handle)
{
    std::unique_ptr<void, void(*)(void *)> stereo_handle(callback_handle, completion_callback);

    sensor_data data(time_us, rc_SENSOR_TYPE_STEREO, stereo_id,
            shutter_time_us, width, height, stride1, stride2, rc_FORMAT_GRAY8, image1, image2, std::move(stereo_handle));

    if(tracker->output.started())
        tracker->output.push(data.make_copy());
    if (tracker->started())
        tracker->receive_data(std::move(data));

    return true;
}

bool rc_receiveImage(rc_Tracker *tracker, rc_Sensor camera_id, rc_ImageFormat format, rc_Timestamp time_us, rc_Timestamp shutter_time_us,
                     int width, int height, int stride, const void *image,
                     void(*completion_callback)(void *callback_handle), void *callback_handle)
{
    std::unique_ptr<void, void(*)(void *)> image_handle(callback_handle, completion_callback);

    if(!image) {
        tracker->sfm.log->error("Null image provided with sensor id {} format {} time {} shutter {} width {} height {} stride {}", camera_id, format, time_us, shutter_time_us, width, height, stride);
        return false;
    }
    if(width <= 0 || height <= 0 || stride < width * (format == rc_FORMAT_DEPTH16 ? 2 : 1)) {
        tracker->sfm.log->error("Incorrectly configured image sensor id {} format {} time {} shutter {} width {} height {} stride {}", camera_id, format, time_us, shutter_time_us, width, height, stride);
        return false;
    }
    if (format == rc_FORMAT_DEPTH16 && camera_id < tracker->sfm.depths.size()) {
        sensor_data data(time_us, rc_SENSOR_TYPE_DEPTH, camera_id,
                shutter_time_us, width, height, stride, rc_FORMAT_DEPTH16, image, std::move(image_handle));

        if(tracker->output.started())
            tracker->output.push(data.make_copy());
        if (tracker->started())
            tracker->receive_data(std::move(data));
    } else if (format == rc_FORMAT_GRAY8 && camera_id < tracker->sfm.cameras.size()) {
        sensor_data data(time_us, rc_SENSOR_TYPE_IMAGE, camera_id,
                shutter_time_us, width, height, stride, rc_FORMAT_GRAY8, image, std::move(image_handle));

        if(tracker->output.started())
            tracker->output.push(data.make_copy());
        if (tracker->started())
            tracker->receive_data(std::move(data));
    }
    else
        return false;

    return true;
}


bool rc_receiveVelocimeter(rc_Tracker *tracker, rc_Sensor velocimeter_id, rc_Timestamp time_us, rc_Vector translational_velocity_m__s)
{
    if (velocimeter_id >= tracker->sfm.velocimeters.size())
        return false;

    sensor_data data(time_us, rc_SENSOR_TYPE_VELOCIMETER, velocimeter_id, translational_velocity_m__s);

    if(tracker->output.started())
        tracker->output.push(data.make_copy());
    if (tracker->started())
        tracker->receive_data(std::move(data));

    return true;
}


bool rc_receiveAccelerometer(rc_Tracker * tracker, rc_Sensor accelerometer_id, rc_Timestamp time_us, rc_Vector acceleration_m__s2)
{
    if (accelerometer_id >= tracker->sfm.accelerometers.size())
        return false;

    sensor_data data(time_us, rc_SENSOR_TYPE_ACCELEROMETER, accelerometer_id, acceleration_m__s2);

    if(tracker->output.started())
        tracker->output.push(data.make_copy());
    if (tracker->started())
        tracker->receive_data(std::move(data));
    return true;
}

bool rc_receiveGyro(rc_Tracker * tracker, rc_Sensor gyroscope_id, rc_Timestamp time_us, rc_Vector angular_velocity_rad__s)
{
    if (gyroscope_id >= tracker->sfm.gyroscopes.size())
        return false;

    sensor_data data(time_us, rc_SENSOR_TYPE_GYROSCOPE, gyroscope_id, angular_velocity_rad__s);

    if(tracker->output.started())
        tracker->output.push(data.make_copy());
    if (tracker->started())
        tracker->receive_data(std::move(data));
    return true;
}

bool rc_receiveTemperature(rc_Tracker * tracker, rc_Sensor therm_id, rc_Timestamp time_us, float temperature_C)
{
    if (therm_id >= tracker->sfm.thermometers.size())
        return false;

    sensor_data data(time_us, rc_SENSOR_TYPE_THERMOMETER, therm_id, temperature_C);

    if(tracker->output.started())
        tracker->output.push(data.make_copy());
    if (tracker->started())
        tracker->receive_data(std::move(data));
    return true;
}

bool rc_setStage(rc_Tracker *tracker, const char *name, const rc_Pose pose_m)
{
    return tracker && rc_getConfidence(tracker) == rc_E_CONFIDENCE_HIGH && tracker->set_stage(name, pose_m);
}

bool rc_getStage(rc_Tracker *tracker, const char *name, rc_Stage *stage)
{
    if (!tracker || !tracker->sfm.map)
        return false;

    bool ok = false; mapper::stage::output current_stage;
    if (name)
        ok = tracker->get_stage(false, name, current_stage);
    else if (stage)
        ok = tracker->get_stage(true, stage->name, current_stage);

    if (stage)
        *stage = { current_stage.name, to_rc_Pose(current_stage.G_world_stage) };

    return ok;
}

int rc_getRelocalizationPoses(rc_Tracker* tracker, rc_Pose **reloc_edges)
{
    if (tracker && tracker->sfm.map && tracker->sfm.relocalization_info.is_relocalized) {
        const auto& info = tracker->sfm.relocalization_info;
        tracker->relocalization_G_world_body.clear();
        tracker->relocalization_G_world_body.reserve(info.size());
        for (auto& c : info.candidates) {
            tracker->relocalization_G_world_body.emplace_back(
                        to_rc_Pose(c.G_world_node * c.G_node_frame));
        }
        if (reloc_edges) *reloc_edges = tracker->relocalization_G_world_body.data();
        return tracker->relocalization_G_world_body.size();
    } else {
        if (reloc_edges) *reloc_edges = nullptr;
        return 0;
    }
}

int rc_getRelocalizationEdges(rc_Tracker *tracker, rc_Timestamp *source, rc_RelocEdge **edges) {
    if (tracker && tracker->sfm.map) {
        const auto& info = tracker->sfm.relocalization_info;
        tracker->relocalization_edges.clear();
        tracker->relocalization_edges.resize(info.size());
        for (size_t i = 0; i < info.size(); ++i) {
            const transformation& G_node_frame = info.candidates[i].G_node_frame;
            rc_RelocEdge& edge = tracker->relocalization_edges[i];
            edge.pose_m = to_rc_Pose(G_node_frame);
            edge.time_destination.time_us = sensor_clock::tp_to_micros(info.candidates[i].node_timestamp);
            edge.time_destination.session_id = tracker->sfm.map->get_node_session(info.candidates[i].node_id);
        }
        if (source) *source = sensor_clock::tp_to_micros(info.frame_timestamp);
        if (edges) *edges = (info.is_relocalized ? tracker->relocalization_edges.data() : nullptr);
        return tracker->relocalization_edges.size();
    } else {
        if (source) *source = 0;
        if (edges) *edges = nullptr;
        return 0;
    }
}

int rc_getMapNodes(rc_Tracker *tracker, rc_MapNode **map_nodes)
{
    if (tracker && tracker->sfm.map) {
        const auto& nodes = tracker->sfm.map->get_nodes();
        tracker->map_nodes.clear();
        tracker->map_nodes.reserve(nodes.size());
        rc_MapNode map_node;
        for (auto& it : nodes) {
            auto& node = it.second;
            if (node.status == node_status::finished && node.frame) {
                map_node.time.time_us = sensor_clock::tp_to_micros(node.frame->timestamp);
                map_node.time.session_id = tracker->sfm.map->get_node_session(node.id);
                tracker->map_nodes.push_back(map_node);
            }
        }
        std::sort(tracker->map_nodes.begin(), tracker->map_nodes.end(),
                  [](const rc_MapNode& lhs, const rc_MapNode& rhs) {
            return lhs.time < rhs.time;
        });
        if (map_nodes) *map_nodes = tracker->map_nodes.data();
        return tracker->map_nodes.size();
    } else {
        if (map_nodes) *map_nodes = nullptr;
        return 0;
    }
}

rc_StorageStats rc_getStorageStats(rc_Tracker* tracker)
{
    rc_StorageStats ret {};
    if (tracker && tracker->sfm.map) {
        auto& nodes = tracker->sfm.map->get_nodes();
        std::unordered_set<void*> unique_features;
        ret.nodes = nodes.size();
        for (auto& it : nodes) {
            auto& node = it.second;
            ret.edges += node.edges.size() + node.covisibility_edges.size();
            ret.features += node.features.size();
            if (node.frame) {
                ret.relocalization_bins += node.frame->dbow_histogram.size();
                for (auto& f : node.frame->keypoints)
                    unique_features.insert(f.get());
                ret.unique_features += node.frame->keypoints.size();
            }
        }
        ret.unique_features = unique_features.size();
    }
    return ret;
}

rc_PoseTime rc_getPose(rc_Tracker * tracker, rc_PoseVelocity *v, rc_PoseAcceleration *a, rc_DataPath path)
{
    const state_motion &s = path == rc_DATA_PATH_FAST ? tracker->sfm.mini->state : tracker->sfm.s;

    transformation total = tracker->sfm.origin * s.loop_offset;
    if (v) map(v->W.v) = total.Q * s.Q.v * s.w.v; // we use body rotational velocity, but we export spatial
    if (a) map(a->W.v) = total.Q * s.Q.v * s.dw.v;
    if (v) map(v->T.v) = total.Q * s.V.v;
    if (a) map(a->T.v) = total.Q * s.a.v;
    rc_PoseTime pose_time;
    pose_time.pose_m = to_rc_Pose(total * transformation(s.Q.v, s.T.v));
    pose_time.time_us = sensor_clock::tp_to_micros(s.get_current_time());
    // assert(pose_m == tracker->get_transformation()); // FIXME: this depends on the specific implementation of ->get_transformation()
    return pose_time;
}

void rc_setPose(rc_Tracker * tracker, const rc_Pose pose_m)
{
    tracker->set_transformation(to_transformation(pose_m));
}

int rc_getFeatures(rc_Tracker * tracker, rc_Sensor camera_id, rc_Feature **features_px)
{
    if(camera_id > tracker->sfm.cameras.size()) return 0;
    tracker->stored_features.resize(tracker->sfm.cameras.size());

    std::vector<rc_Feature> & features = tracker->stored_features[camera_id];
    features.clear();

    if(camera_id < tracker->sfm.s.cameras.children.size()) {
        transformation G = tracker->get_transformation();
        for(auto &t: tracker->sfm.s.cameras.children[camera_id]->tracks) {
            auto i = &t.feature;
            if(!i->is_valid() || !t.track.found()) continue;
            rc_Feature feat;
            feat.id = i->feature->id;
            feat.camera_id = i->group.camera.id;
            feat.image_x = static_cast<decltype(feat.image_x)>(t.track.x);
            feat.image_y = static_cast<decltype(feat.image_y)>(t.track.y);
            feat.image_prediction_x = static_cast<decltype(feat.image_prediction_x)>(t.track.pred_x);
            feat.image_prediction_y = static_cast<decltype(feat.image_prediction_y)>(t.track.pred_y);
            v3 ext_pos = G * i->body;
            feat.world.x = static_cast<decltype(feat.world.x)>(ext_pos[0]);
            feat.world.y = static_cast<decltype(feat.world.y)>(ext_pos[1]);
            feat.world.z = static_cast<decltype(feat.world.z)>(ext_pos[2]);
            feat.stdev   = static_cast<decltype(feat.stdev)>(i->v->stdev_meters(sqrt(i->variance())));
            feat.innovation_variance_x = t.innovation_variance_x;
            feat.innovation_variance_y = t.innovation_variance_y;
            feat.innovation_variance_xy = t.innovation_variance_xy;
            feat.depth_measured = i->depth_measured;
            feat.recovered = i->recovered;
            feat.initialized =  i->is_initialized();
            feat.depth = i->v->depth();
            features.push_back(feat);
        }
    }

    if (features_px) *features_px = features.data();
    return features.size();
}

rc_TrackerState rc_getState(const rc_Tracker *tracker)
{
    switch(tracker->sfm.run_state)
    {
        case RCSensorFusionRunStateDynamicInitialization:
            return rc_E_DYNAMIC_INITIALIZATION;
        case RCSensorFusionRunStateInactive:
            return rc_E_INACTIVE;
        case RCSensorFusionRunStateRunning:
            return rc_E_RUNNING;
        case RCSensorFusionRunStateInertialOnly:
            return rc_E_INERTIAL_ONLY;
    }
    assert(false && "Invalid tracker state");
    return rc_E_INACTIVE;
}

rc_TrackerConfidence rc_getConfidence(const rc_Tracker *tracker)
{
    rc_TrackerConfidence confidence = rc_E_CONFIDENCE_NONE;
    if(tracker->sfm.run_state == RCSensorFusionRunStateRunning)
    {
        if(tracker->sfm.numeric_failed ||
           Eigen::isnan(tracker->sfm.s.Q.v.coeffs().array()).any() || Eigen::isnan(tracker->sfm.s.T.v.array()).any() ||
           Eigen::isinf(tracker->sfm.s.Q.v.coeffs().array()).any() || Eigen::isinf(tracker->sfm.s.T.v.array()).any())
            confidence = rc_E_CONFIDENCE_NONE;
        else if(tracker->sfm.detector_failed) confidence = rc_E_CONFIDENCE_LOW;
        else if(tracker->sfm.has_converged) confidence = rc_E_CONFIDENCE_HIGH;
        else confidence = rc_E_CONFIDENCE_MEDIUM;
    }
    return confidence;
}

rc_TrackerError rc_getError(const rc_Tracker *tracker)
{
    rc_TrackerError error = rc_E_ERROR_NONE;
    if(tracker->sfm.numeric_failed) error = rc_E_ERROR_OTHER;
    else if(tracker->sfm.speed_failed) error = rc_E_ERROR_SPEED;
    else if(tracker->sfm.detector_failed) error = rc_E_ERROR_VISION;
    return error;
}

bool rc_setOutputLog(rc_Tracker * tracker, const char *filename, rc_TrackerRunFlags run_flags)
{
    return tracker->output.start(filename, run_flags & rc_RUN_ASYNCHRONOUS);
}

const char *rc_getTimingStats(rc_Tracker *tracker)
{
    tracker->timingStats = tracker->get_timing_stats();
    return tracker->timingStats.c_str();
}

size_t rc_getCalibration(rc_Tracker *tracker, const char **buffer)
{
    calibration cal;

    size_t size = std::min(tracker->sfm.accelerometers.size(), tracker->sfm.gyroscopes.size());
    cal.imus.resize(size);
    for (size_t id = 0; id < size; id++) {
        rc_describeGyroscope(tracker, id, &cal.imus[id].extrinsics, &cal.imus[id].intrinsics.gyroscope);
        rc_describeAccelerometer(tracker, id, &cal.imus[id].extrinsics, &cal.imus[id].intrinsics.accelerometer);
    }

    size = tracker->sfm.velocimeters.size();
    cal.velocimeters.resize(size);
    for (size_t id = 0; id < size; id++) {
        rc_describeVelocimeter(tracker, id, &cal.velocimeters[id].extrinsics, &cal.velocimeters[id].intrinsics);
    }

    cal.cameras.resize(tracker->sfm.cameras.size());
    for (size_t id = 0; id < tracker->sfm.cameras.size(); id++)
        rc_describeCamera(tracker, id, rc_FORMAT_GRAY8, &cal.cameras[id].extrinsics, &cal.cameras[id].intrinsics);

    cal.depths.resize(tracker->sfm.depths.size());
    for (size_t id = 0; id < tracker->sfm.depths.size(); id++)
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
    calibration cal;
    if(!calibration_deserialize(buffer, cal))
        return false;

    tracker->sfm.cameras.clear();
    tracker->sfm.depths.clear();
    tracker->sfm.accelerometers.clear();
    tracker->sfm.gyroscopes.clear();
    tracker->sfm.thermometers.clear();
    tracker->sfm.velocimeters.clear();

    int id = 0;
    for(auto imu : cal.imus) {
        rc_configureAccelerometer(tracker, id, &imu.extrinsics, &imu.intrinsics.accelerometer);
        rc_configureGyroscope(tracker, id, &imu.extrinsics, &imu.intrinsics.gyroscope);
        if (imu.intrinsics.thermometer.measurement_variance_C2)
            rc_configureThermometer(tracker, id++, &imu.extrinsics, &imu.intrinsics.thermometer);
        id++;
    }

    id = 0;
    for(auto velocimeter : cal.velocimeters) {
        rc_configureVelocimeter(tracker, id, &velocimeter.extrinsics, &velocimeter.intrinsics);
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

bool rc_appendCalibration(rc_Tracker *tracker, const char *buffer)
{
    calibration cal;
    if(!calibration_append_deserialize(buffer, cal))
        return false;

    int id = 0;
    for(auto velocimeter : cal.velocimeters) {
        rc_configureVelocimeter(tracker, id, &velocimeter.extrinsics, &velocimeter.intrinsics);
        id++;
    }

    return true;
}

float rc_getPathLength(rc_Tracker* tracker) {
    return ((sensor_fusion *)tracker)->sfm.s.total_distance;
}
