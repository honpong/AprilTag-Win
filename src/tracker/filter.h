#ifndef __FILTER_H
#define __FILTER_H

#include <future>
#include "state_vision.h"
#include "observation.h"
#include "transformation.h"
#ifdef ENABLE_QR
#include "qr.h"
#endif
#include "RCSensorFusionInternals.h"
#include "platform/sensor_clock.h"
#include "sensor_data.h"
#include "spdlog/spdlog.h"
#include "mapper.h"
#include "storage.h"
#include "state_size.h"
#include "tpose.h"

struct filter {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    filter() { }
    RCSensorFusionRunState run_state;
    size_t min_group_add;
    size_t max_group_add;
    size_t min_group_map_add;

    sensor_clock::time_point last_time;

    kalman_storage<MAXSTATESIZE, MAXOBSERVATIONSIZE, FAKESTATESIZE> store;
    observation_queue observations{store.x, store.y, store.R, store.HP, store.S};
    covariance cov{store.maxstatesize, store.P, store.Q, store.iP, store.iQ};

    struct {
        // maxobservationsize is 3, but due to a bug in BLIS we need
        // matrices to be resizable to boundaries divisible by 4
        kalman_storage<MAX_FAST_PATH_STATESIZE, 4, FAKESTATESIZE> store;
        observation_queue observations{store.x, store.y, store.R, store.HP, store.S};
        covariance cov{store.maxstatesize, store.P, store.Q, store.iP, store.iQ};
        state_motion state{cov, store.FP};
        sensor_clock::time_point slow_path_timestamp;
        bool valid{false};
    } _mini[2], *mini = &_mini[0], *catchup = &_mini[1];

    std::unique_ptr<spdlog::logger> &log = s.log;

    sensor_clock::time_point want_start;
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    sensor_clock::time_point detector_failed_time;
    sensor_clock::time_point speed_warning_time;

    float extrinsic_Q_var_thresh;
    float max_velocity;
    float median_depth_variance;
    bool has_converged;
    bool stereo_enabled;
    bool relocalize; /*!< usage: value may be set during mapping session .*/
    bool save_map; /*!< usage: value may be set during mapping session .*/
    bool allow_jumps; /*!< usage: value may be set during mapping session .*/
    map_relocalization_info relocalization_info;
    std::vector<nodeid> brought_back_groups;

    template<int N>
    class every_n {
        int index_ = 0;
     public:
        bool ready() {
            bool yes = (index_ == 0);
            index_ = (index_ + 1 == N ? 0 : index_ + 1);
            return yes;
        }
        void reset() {
            index_ = 0;
        }
    };

    std::unique_ptr<mapper> map;
    std::future<map_relocalization_result> relocalization_future;
    every_n<60> relocalization_every_n;

#ifdef ENABLE_QR
    qr_detector qr;
    sensor_clock::time_point last_qr_time;
    qr_benchmark qr_bench;
    bool qr_origin_gravity_aligned;
#endif

    transformation origin;
    bool origin_set;

    std::unique_ptr<sensor_data> recent_depth; //TODOMSM - per depth
    bool has_depth; //TODOMSM - per depth

    stdev<1> stereo_stats;

    std::vector<std::unique_ptr<sensor_grey>> cameras;
    std::vector<std::unique_ptr<sensor_depth>> depths;
    std::vector<std::unique_ptr<sensor_accelerometer>> accelerometers;
    std::vector<std::unique_ptr<sensor_gyroscope>> gyroscopes;
    std::vector<std::unique_ptr<sensor_thermometer>> thermometers;
    std::vector<std::unique_ptr<sensor_velocimeter>> velocimeters;

    state s{cov, store.FP};

    bool got_any_gyroscopes()     const { for (const auto &gyro  :     gyroscopes) if (gyro->got)  return true; return false;}
    bool got_any_accelerometers() const { for (const auto &accel : accelerometers) if (accel->got) return true; return false; }
    bool got_any_velocimeters() const { for (const auto &velo : velocimeters) if (velo->got) return true; return false; }
};

bool filter_depth_measurement(struct filter *f, const sensor_data & data);
bool filter_image_measurement(struct filter *f, const sensor_data & data);
bool filter_stereo_initialize(struct filter *f, rc_Sensor camera1_id, rc_Sensor camera2_id);
std::unique_ptr<camera_frame_t> filter_create_camera_frame(const filter *f, const sensor_data& data);
std::vector<tracker::feature_track> &filter_detect(struct filter *f, const sensor_data &data, const std::vector<tracker::feature_position> &avoid, size_t detect);
bool filter_compute_orb(struct filter *f, const sensor_data &data, camera_frame_t& camera_frame);
void filter_compute_dbow(struct filter *f, camera_frame_t& camera_frame);
void filter_assign_frame(struct filter *f, const camera_frame_t& camera_frame);
map_relocalization_result filter_relocalize(struct filter *f, const camera_frame_t& camera_frame);
void filter_add_relocalization_edges(struct filter *f, const aligned_vector<map_relocalization_edge>& edges);
bool filter_accelerometer_measurement(struct filter *f, const sensor_data & data);
bool filter_gyroscope_measurement(struct filter *f, const sensor_data & data);
bool filter_velocimeter_measurement(struct filter *f, const sensor_data & data);
bool filter_mini_accelerometer_measurement(struct filter * f, observation_queue &queue, state_motion &state, const sensor_data &data);
bool filter_mini_gyroscope_measurement(struct filter * f, observation_queue &queue, state_motion &state, const sensor_data &data);
void filter_compute_gravity(struct filter *f, double latitude, double altitude);
void filter_start(struct filter *f);
void filter_start_inertial_only(struct filter *f);
void filter_update_detection_status(struct filter *f, state_camera &camera_state, int detected, sensor_clock::time_point detection_time);
void filter_update_triangulated_tracks(const filter *f, const rc_Sensor camera_id);
std::vector<nodeid> filter_bring_groups_back(struct filter *f, const rc_Sensor camera_id);
#ifdef ENABLE_QR
void filter_start_qr_detection(struct filter *f, const std::string& data, float dimension, bool use_gravity);
void filter_stop_qr_detection(struct filter *f);
void filter_start_qr_benchmark(struct filter *f, float dimension);
#endif
void filter_set_origin(struct filter *f, const transformation &origin, bool gravity_aligned);
void filter_start_dynamic_calibration(struct filter *f);

void filter_initialize(struct filter *f);
void filter_deinitialize(struct filter *f);
std::string filter_get_stats(const struct filter *f);

#endif
