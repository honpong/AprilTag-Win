#include "gtest/gtest.h"
#include "replay.h"
#include "stream_object.h"
#include "file_stream.h"
#include "rc_compat.h"
#include "sensor_fusion.h"

#include <iostream>
#include <memory>
#include <string>
#include <future>

static constexpr const char* dataset_filename = "data/other/minimal_test_suite/stereo/R01_W3_mapping_T1_X90-25.stereo.rc";
static constexpr const char* map_filename = "test.map";

// rc_Stage with managed memory
struct stage_t : rc_Stage {
    std::string name_buffer;

    stage_t() {
        name = nullptr;
    }

    stage_t(const std::string& name, const rc_Pose& pose)
        : name_buffer(name) {
        this->pose_m = pose;
        this->name = name_buffer.c_str();
    }

    stage_t(const stage_t& rhs) {
        *this = rhs;
    }

    stage_t(stage_t&& rhs) {
        *this = std::move(rhs);
    }

    stage_t& operator=(const stage_t& rhs) {
        if (this != &rhs) {
            name_buffer = rhs.name_buffer;
            pose_m = rhs.pose_m;
            name = (name_buffer.empty() ? nullptr : name_buffer.c_str());
        }
        return *this;
    }

    stage_t& operator=(stage_t&& rhs) {
        if (this != &rhs) {
            name_buffer = std::move(rhs.name_buffer);
            pose_m = std::move(rhs.pose_m);
            name = (name_buffer.empty() ? nullptr : name_buffer.c_str());
        }
        return *this;
    }
};

TEST(Stages, stage_t)
{
    stage_t st { "stage A", rc_Pose{} };
    EXPECT_EQ(std::string(st.name), std::string("stage A"));
    EXPECT_EQ(st.name, st.name_buffer.c_str());

    std::vector<stage_t> stages;
    stages.push_back(st);
    EXPECT_EQ(std::string(stages[0].name), std::string("stage A"));
    EXPECT_EQ(stages[0].name, stages[0].name_buffer.c_str());

    stages.reserve(stages.capacity() + 1);
    EXPECT_EQ(std::string(stages[0].name), std::string("stage A"));
    EXPECT_EQ(stages[0].name, stages[0].name_buffer.c_str());

    stage_t st2;
    st2 = st;
    EXPECT_EQ(std::string(st2.name), std::string("stage A"));
    EXPECT_EQ(st2.name, st2.name_buffer.c_str());

    stage_t st3(std::move(st));
    EXPECT_EQ(std::string(st3.name), std::string("stage A"));
    EXPECT_EQ(st3.name, st3.name_buffer.c_str());

    stages.push_back(st2);
    stages.push_back(st3);
    EXPECT_EQ(std::string(stages[0].name), std::string("stage A"));
    EXPECT_EQ(std::string(stages[1].name), std::string("stage A"));
    EXPECT_EQ(std::string(stages[2].name), std::string("stage A"));
    EXPECT_EQ(stages[0].name, stages[0].name_buffer.c_str());
    EXPECT_EQ(stages[1].name, stages[1].name_buffer.c_str());
    EXPECT_EQ(stages[2].name, stages[2].name_buffer.c_str());
}

enum replay_options {
    DEFAULT = 0,  // sync, no relocalize, no save, no load
    ASYNC = 1,
    RELOCALIZE = 2,
    SAVE_MAP = 4,
    LOAD_MAP = 8
};

std::unique_ptr<replay> create_replay(int options = DEFAULT, const char* dataset = dataset_filename, const char* map_name = map_filename) {
    auto rp = std::make_unique<replay>((host_stream *)(new file_stream(dataset)));

    if (!rp->init()) {
        std::cerr << "Warning: file " << dataset << " could not be replayed. Skipping test" << std::endl;
        return nullptr;
    }

    rp->set_calibration_from_filename(dataset);
    rp->set_message_level(rc_MESSAGE_ERROR);
    uint32_t run_flags = rc_RUN_FAST_PATH;
    rp->start_mapping(options & RELOCALIZE, options & SAVE_MAP);
    if (options & ASYNC) run_flags |= rc_RUN_ASYNCHRONOUS;
    if (options & LOAD_MAP)
        EXPECT_TRUE(rp->load_map(map_name));

    rp->set_run_flags((rc_TrackerRunFlags)run_flags);
    return rp;
}

void run_CreateStages_test(bool sync_replay, bool sync_stages) {
    auto rp = create_replay(sync_replay ? DEFAULT : ASYNC);
    if (!rp) return;

    std::future<void> worker;
    std::vector<stage_t> stages;
    rc_Tracker* tracker = nullptr;
    rp->set_data_callback([sync_stages, &worker, &stages, &tracker, event_time_us = (rc_Timestamp)0, idx = 0]
                          (const replay_output *output, const rc_Data * data) mutable {
        constexpr rc_Timestamp period_us = 5 * 1000000;
        if (data->path != rc_DATA_PATH_SLOW) return;
        if (event_time_us == 0) {
            event_time_us = data->time_us + period_us;
            return;
        }
        if (data->time_us > event_time_us) {
            event_time_us += period_us;

            tracker = output->tracker;
            ASSERT_TRUE(tracker);

            std::string name = "stage" + std::to_string(idx++);
            rc_Pose pose_m = rc_getPose(tracker, nullptr, nullptr, rc_DATA_PATH_SLOW).pose_m;

            if (worker.valid()) worker.get();
            worker = std::async(sync_stages ? std::launch::deferred : std::launch::async, [=, &stages]() {
                if (rc_setStage(tracker, name.c_str(), pose_m))
                    stages.emplace_back(name, pose_m);
            });
        }
    });

    rp->start();

    // setting a stage is an asynchronous operation. There is a small chance
    // the last stage wasn't set if it was done in one of the last packets.
    if (!stages.empty()) stages.pop_back();

    ASSERT_TRUE(tracker);
    for (auto& st : stages) {
        rc_Stage stage;
        ASSERT_TRUE(rc_getStage(tracker, st.name, &stage));
        EXPECT_EQ(0, std::strcmp(st.name, stage.name));
    }

    rp->end();
}

TEST(Stages, CreateStages_SyncReplay_SyncStages)
{
    run_CreateStages_test(true, true);
}

TEST(Stages, CreateStages_SyncReplay_AsyncStages)
{
    run_CreateStages_test(true, false);
}

TEST(Stages, CreateStages_AsyncReplay_SyncStages)
{
    run_CreateStages_test(false, true);
}

TEST(Stages, CreateStages_AsyncReplay_AsyncStages)
{
    run_CreateStages_test(false, false);
}

static std::vector<stage_t> persistent_stages_;

TEST(Stages, CreateAndCheckStages)
{
    auto rp = create_replay(SAVE_MAP);
    if (!rp) return;

    std::vector<stage_t> stages;
    rc_Tracker* tracker = nullptr;
    rp->set_data_callback([&stages, &tracker, event_time_us = (rc_Timestamp)0](const replay_output *output, const rc_Data * data) mutable {
        constexpr rc_Timestamp period_us = 5 * 1000000;
        if (data->path != rc_DATA_PATH_SLOW) return;
        if (event_time_us == 0) {
            event_time_us = data->time_us + period_us;
            return;
        }
        if (data->time_us > event_time_us) {
            event_time_us += period_us;

            tracker = output->tracker;
            ASSERT_TRUE(tracker);
            for (auto &st : stages) {
                rc_Stage rc_stage;
                EXPECT_TRUE(rc_getStage(tracker, st.name, &rc_stage));
                EXPECT_EQ(0, std::strcmp(st.name, rc_stage.name));
            }

            std::string name = "stage" + std::to_string(stages.size());
            rc_Pose pose_m = rc_getPose(tracker, nullptr, nullptr, rc_DATA_PATH_SLOW).pose_m;
            if (rc_setStage(tracker, name.c_str(), pose_m))
                stages.emplace_back(name, pose_m);
        }
    });

    rp->start();
    ASSERT_TRUE(rp->save_map(map_filename));

    ASSERT_TRUE(tracker);
    size_t n = 0;
    for (rc_Stage stage = {}; rc_getStage(tracker, nullptr, &stage); ++n);

    EXPECT_TRUE(n == stages.size() || n + 1 == stages.size());

    rp->end();

    if (n < stages.size())
        stages.pop_back();  // see note about asynchronous behavior above

    std::swap(persistent_stages_, stages);
}

TEST(Stages, LoadAndCheckStages)
{
    auto rp = create_replay(RELOCALIZE | LOAD_MAP);
    if (!rp) return;

    rp->set_data_callback([](const replay_output *output, const rc_Data *) {
        rc_Tracker *tracker = output->tracker;
        sensor_fusion *sf = (sensor_fusion*)output->tracker;
        ASSERT_TRUE(tracker);
        ASSERT_TRUE(sf->sfm.map);
        EXPECT_EQ(persistent_stages_.size(), sf->sfm.map->stages->size());
    });

    rp->start();
    rp->end();
}

TEST(Stages, RelocalizeSaveMap)
{
    auto rp_save = create_replay(SAVE_MAP, "data/other/WW50/rectangles/strafe/strafe_9.stereo.rc");
    if (!rp_save) return;

    bool stage_set = false;
    rc_Tracker* tracker = nullptr;
    rp_save->set_data_callback([&stage_set, &tracker](const replay_output *output, const rc_Data *data) {
        if (data->path != rc_DATA_PATH_SLOW) return;
        if(stage_set) return;
        tracker = output->tracker;
        rc_Pose pose_m = rc_getPose(tracker, nullptr, nullptr, rc_DATA_PATH_SLOW).pose_m;
        if(rc_setStage(tracker, "stage", pose_m))
            stage_set = true;
    });
    rp_save->start();
    ASSERT_TRUE(rp_save->save_map(map_filename));
    ASSERT_TRUE(tracker);
    rc_Stage stage = {}; ASSERT_TRUE(rc_getStage(tracker, "stage", &stage));
    rp_save->end();
}

TEST(Stages, RelocalizeLoadMap)
{
    auto rp_load = create_replay(LOAD_MAP | RELOCALIZE, "data/other/WW50/rectangles/strafe/strafe_8.stereo.rc");
    if (!rp_load) return;

    bool stage_found = false;
    rc_Tracker* tracker = nullptr;
    rp_load->set_data_callback([&stage_found, &tracker](const replay_output *output, const rc_Data *) {
        if(stage_found)
            return;
        tracker = output->tracker;
        rc_Stage stage = {};
        if(rc_getStage(tracker, "stage", &stage)) {
            stage_found = true;
            rc_stopTracker(tracker);
        }
    });
    rp_load->start();
    rp_load->end();
    ASSERT_TRUE(stage_found);
}
