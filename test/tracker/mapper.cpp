/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2017-2018 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#include "gtest/gtest.h"
#include "mapper.h"
#include "state_vision.h"
#include "map_loader.h"

using namespace std;

typedef fast_tracker::fast_feature<DESCRIPTOR> track_feature;

template<typename feature_type, typename track_descriptor_t, typename reloc_descriptor_t, typename bow_t>
class test_mapper;
using t_mapper = test_mapper<track_feature, patch_descriptor, orb_descriptor, DBoW2::BowVector>;

#define NUM_WORLD_POINTS 200
#define MAX_NUM_CAMERA_FRAMES 300

// get hard-coded fisheye camera intrinsics
static state_vision_intrinsics *get_cam_intrinsics() {
    static state_vision_intrinsics intrinsics(false);
    if (intrinsics.type != rc_CALIBRATION_TYPE_FISHEYE) {
        const float f_x_px = 266.2363f, f_y_px = 265.3752f, height_px = 800, width_px = 848,
            c_x_px = 423.6028f, c_y_px = 398.468f, w = 0.9286f;
        //as set in filter_initialize
        intrinsics.focal_length.v = (f_x_px + f_y_px) / 2.f / height_px;
        intrinsics.center.v.x() = (c_x_px - width_px / 2.f + .5f) / height_px;
        intrinsics.center.v.y() = (c_y_px - height_px / 2.f + .5f) / height_px;
        intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
        intrinsics.k.v[0] = w;
        intrinsics.focal_length.set_process_noise(2.3e-3f / height_px / height_px);
        intrinsics.center.set_process_noise(      2.3e-3f / height_px / height_px);
        intrinsics.k.set_process_noise(2.3e-7f);
        intrinsics.focal_length.set_initial_variance(10.f / height_px / height_px);
        intrinsics.center.set_initial_variance(2.f / height_px / height_px);
        intrinsics.k.set_initial_variance(.01f*.01f);
        intrinsics.image_width = (int)width_px;
        intrinsics.image_height = (int)height_px;
    }
    return &intrinsics;
}

static state_extrinsics *get_cam_extrinsics() {
    static state_extrinsics extrinsics("Qc", "Tc", false, false);
    extrinsics.reset();
    return &extrinsics;
}

static const uint32_t orb_block_w = orb_descriptor::orb_half_patch_size * 2 + 1;
static const uint32_t patch_block_w = patch_descriptor::full_patch_size;

/// get a randomly generated descriptor, including patch and orb descriptor.
static DESCRIPTOR get_desc(const tracker::image &img){
    DESCRIPTOR new_desc_point(orb_descriptor(orb_block_w / 2, orb_block_w / 2, img));
    static const uint32_t orb_patch_offset = (orb_block_w - patch_block_w) / 2;
    for (uint32_t r = 0; r < patch_block_w; r++)
        for (uint32_t c = 0; c < patch_block_w; c++)
            new_desc_point.patch.descriptor.data()[r * patch_block_w + c] =
            img.image[(r + orb_patch_offset)  * orb_block_w + c + orb_patch_offset];
    new_desc_point.patch = decltype(new_desc_point.patch)(new_desc_point.patch.descriptor); //calculate mean/variance
    new_desc_point.orb_computed = true;
    return new_desc_point;
}

/// structure includes reference descriptor, 3D points and image block for later permutation.
typedef tuple <v3, DESCRIPTOR, array<uint8_t, orb_block_w * orb_block_w>> ref_desc_point;

//get a list of random 3D points and their default RANDOM orb and patch descriptors
static const auto & get_3dw_points() {
    static vector<ref_desc_point> g_3dw_points;
    if (g_3dw_points.empty()) { //generate once for all test
        default_random_engine gen(300);
        uniform_real_distribution<float> xy_range(-3.0f, 3.0f), z_range(0.f, 4.0f);
        uniform_int_distribution<int> pixel_range(0, 255);
        array<uint8_t, orb_block_w * orb_block_w> orb_block = {{ 0 }};
        tracker::image img = { &orb_block[0], orb_block_w, orb_block_w, orb_block_w };
        for (int p = 0; p < NUM_WORLD_POINTS; p++) {
            for (auto &val : orb_block) val = (uint8_t)(pixel_range(gen));
            g_3dw_points.emplace_back(make_tuple(v3(xy_range(gen), xy_range(gen), z_range(gen)), get_desc(img), orb_block));
        }
    }
    return g_3dw_points;
}

static DESCRIPTOR permute(const ref_desc_point &org_pt, int num_byte_changes) {
    static default_random_engine gen(300);
    uniform_int_distribution<int> orb_pos_range(0, orb_block_w * orb_block_w - 1),
        num_changes(0, num_byte_changes), pixel_range(0, 255);
    array<uint8_t, orb_block_w * orb_block_w> orb_block = get<2>(org_pt);
    for (int i = 0; i < num_changes(gen); i++)
        orb_block[orb_pos_range(gen)] = (uint8_t)pixel_range(gen);
    tracker::image img = { &orb_block[0], orb_block_w, orb_block_w, orb_block_w };
    return get_desc(img);
}

static auto observable(const feature_t &pix, const state_vision_intrinsics &intrinsics) {
    return (pix.x() >= 0 && pix.x() < intrinsics.image_width && pix.y() >= 0 && pix.y() < intrinsics.image_height);
}

class mapper_check {
public:
    template <typename T, typename enable_if<is_floating_point<T>::value, int>::type = 0>
    static void expect_eq(const T   &obj1, const T &obj2) {
        EXPECT_FLOAT_EQ(             obj1,         obj2);
    }

    template <typename T, typename enable_if<is_fundamental<T>::value && !is_floating_point<T>::value, int>::type = 0>
    static void expect_eq(const T   &obj1, const T &obj2) {
        EXPECT_EQ(                   obj1,          obj2);
    }

    template<typename Key, typename T>
    static void expect_eq(const pair<Key, T>    &entry1, const pair<Key, T> &entry2) {
        EXPECT_EQ(                               entry1.first,               entry2.first);
        expect_eq(                               entry1.second,              entry2.second);
    }

    static void expect_eq(const v2 &entry1, const v2    &entry2) {
        EXPECT_FLOAT_EQ(            entry1.x(),          entry2.x());
        EXPECT_FLOAT_EQ(            entry1.y(),          entry2.y());
    }

    static void expect_eq(const log_depth   &entry1, const log_depth    &entry2) {
        EXPECT_FLOAT_EQ(                     entry1.v,                   entry2.v);
        expect_eq(                           entry1.initial,             entry2.initial);
    }

    static void expect_eq(const patch_descriptor    &entry1, const patch_descriptor &entry2) {
        EXPECT_EQ(                                   entry1.descriptor,              entry2.descriptor);
        EXPECT_FLOAT_EQ(                             entry1.mean,                    entry2.mean);
        EXPECT_FLOAT_EQ(                             entry1.variance,                entry2.variance);
    }

    static void expect_eq(const orb_descriptor &entry1, const orb_descriptor    &entry2) {
        EXPECT_EQ(                              entry1.sin_,                     entry2.sin_);
        EXPECT_EQ(                              entry1.cos_,                     entry2.cos_);
        EXPECT_EQ(                              entry1.descriptor,               entry2.descriptor);
    }

    static void expect_eq(const patch_orb_descriptor    &entry1, const patch_orb_descriptor &entry2) {
        EXPECT_EQ(                                       entry1.orb_computed,                entry2.orb_computed);
        if (compare_orb)
            expect_eq(                                   entry1.orb,                         entry2.orb);
        expect_eq(                                       entry1.patch,                       entry2.patch);
    }

    static void expect_eq(const shared_ptr<track_feature>   &entry1, const shared_ptr<track_feature>    &entry2) {
        EXPECT_EQ(                                       entry1 != nullptr,                      entry2 != nullptr);
        if (                                             entry1 != nullptr &&                    entry2 != nullptr) {
            EXPECT_EQ(                                   entry1->id,                             entry2->id);
            expect_eq(                                   entry1->descriptor,                     entry2->descriptor);
        }
    }

    static void expect_eq(const map_edge    &entry1, const map_edge     &entry2) {
        EXPECT_EQ(                           entry1.type,                entry2.type);
        EXPECT_TRUE(                         entry1.G ==                 entry2.G);
    }

    template<typename T, typename Func>
    static void expect_eq(const T   &obj1, const T         &obj2, Func test_member) {
        EXPECT_EQ(                   obj1.size(),           obj2.size());
        auto entry1 =                obj1.begin(), entry2 = obj2.begin();
        for (; entry1 != obj1.end(); entry1++, entry2++)
                      test_member(*entry1,               *entry2);
    }

    template <template <class, class> class TContainer, class T, class Alloc>
    static void expect_eq(const TContainer<T, Alloc> &obj1, const TContainer<T, Alloc>  &obj2) {
        expect_eq(                                    obj1,                              obj2,
                                        [](auto        &p1, auto                          &p2) {
                                        expect_eq(      p1,                                p2);
        });
    }

    template <class T, class Alloc>
    static void expect_eq(const vector<T, Alloc>    &obj1, const vector<T, Alloc>   &obj2) {
        return expect_eq<vector, T, Alloc>(          obj1,                           obj2);
    }

    template <template <class...> class TContainer, class E, class...TArgs, typename std::enable_if<!is_std_pair<E>::value, int>::type = 0>
    static auto find(const TContainer<TArgs...> &obj, const E    &entry) {
        return obj.find(entry);
    }

    template <template <class...> class TContainer, class E, class...TArgs, typename std::enable_if<is_std_pair<E>::value, int>::type = 0>
    static auto find(const TContainer<TArgs...> &obj, const E    &entry) {
        return obj.find(entry.first);
    }

    template <template <class, class, class, class...> class TContainer, class Key, class T, class... TArgs>
    static void expect_eq(const TContainer<Key, T, TArgs...>   &obj1, const TContainer<Key, T, TArgs...>  &obj2) {
        expect_eq(                                              obj1,                                obj2,
                                               [&](auto          &p1, auto                            &p2) {
                                                   auto           p1_itr2 = find(obj2, p1);
                                                   EXPECT_NE(     p1_itr2,                           obj2.end());
                                                   if (           p1_itr2 !=                         obj2.end())
                                                       expect_eq( p1,                            *p1_itr2);
        });
    }

    static void expect_eq(const map_feature &entry1, const map_feature &entry2) {
        EXPECT_EQ(                           entry1.v == nullptr,       entry2.v == nullptr);
        if (entry1.v != nullptr && entry2.v != nullptr)
            expect_eq(                      *entry1.v,                 *entry2.v);
        //EXPECT_EQ(                          entry1.type,                entry2.type); //all loaded of type tracked
        compare_orb = false;
        expect_eq(                          entry1.feature,             entry2.feature);
    }

    static void expect_eq(const frame_t &e1, const frame_t      &e2) {
        compare_orb = true;
        //EXPECT_EQ(e1.timestamp,         e2.timestamp); //checking is not valid when loading in a future.
        expect_eq(                      e1.keypoints,            e2.keypoints);
        expect_eq(                      e1.keypoints_xy,         e2.keypoints_xy);
        expect_eq(                      e1.dbow_histogram,       e2.dbow_histogram);
    }

    static void expect_eq(const map_node &e1, const map_node        &e2) {
        EXPECT_EQ(                        e1.id,                      e2.id);
        expect_eq(                        e1.edges,                   e2.edges);
        expect_eq(                        e1.covisibility_edges,      e2.covisibility_edges);
//        EXPECT_TRUE(                      e1.global_transformation == e2.global_transformation);
        EXPECT_EQ(                        e1.camera_id,               e2.camera_id);
        //EXPECT_EQ(                        e1.status,                 e2.status); //loaded status is finished
        EXPECT_EQ(                        e1.frame != nullptr,        e2.frame != nullptr);
        if                               (e1.frame != nullptr &&      e2.frame != nullptr)
            expect_eq(                   *e1.frame,                  *e2.frame);
        expect_eq(                        e1.features,                e2.features);
    }
private:
    static bool compare_orb; //whether to compare orb descriptor.
};

bool mapper_check::compare_orb = true;

template<typename track_feat_t, typename track_descriptor_t, typename reloc_descriptor_t, typename bow_t>
class test_mapper : public mapper {
public:
     //do not compare compute-intermediate members or members populated by map loading such as orb_voc,
     //node_id_offset, feature_id_offset and unlinked.
    auto operator==(const test_mapper &rhs) const {
        mapper_check::expect_eq(*features_dbow, *rhs.features_dbow);
        mapper_check::expect_eq(*nodes, *rhs.nodes);
        return true;
    }

    void get_max_ids(uint32_t &max_node_id, uint32_t &max_feature_id) const {
        for (auto &node_pair : *nodes) {
            if (max_node_id < node_pair.second.id) max_node_id = node_pair.second.id;
            for (auto &feat_pair : node_pair.second.features) //generally for map/unordered_map
                if (max_feature_id < feat_pair.first)  max_feature_id = feat_pair.first;
        }
    }

    bool is_unlinked() const { return unlinked; }

    auto create_filled_frame(frame_t &frame, map<uint64_t, v3> * map_feats, map<uint64_t, shared_ptr<log_depth>> *map_depth,
            const transformation & node_global_tran, const vector<ref_desc_point> &points_3dw, int desc_change_byte = 50) const {
        static uint64_t g_kp_id = 0; //incremental id of keypoints
        static sensor_clock::time_point g_ts_ms(chrono::microseconds(1));//incremental timestamps
        frame.timestamp = g_ts_ms;
        g_ts_ms += chrono::microseconds(33333);

        frame.keypoints.clear();
        frame.keypoints_xy.clear();
        // fill key points
        const auto G_CW = invert(node_global_tran);
        auto &intrinsics = *mapper::camera_intrinsics[0];
        for (uint64_t point_idx = 0; point_idx < points_3dw.size(); point_idx++) {
            auto point_3d_c = G_CW * get<0>(points_3dw[point_idx]);
            if (point_3d_c.z() > 0) {
                auto p_projected = intrinsics.project_feature(point_3d_c);
                if (observable(p_projected, intrinsics)) {
                    frame.keypoints.emplace_back(make_shared<track_feat_t>(g_kp_id,
                        permute(points_3dw[point_idx], desc_change_byte))); //minor random change to descriptor
                    frame.keypoints_xy.emplace_back(v2(p_projected.x(), p_projected.y()));

                    if (map_feats) (*map_feats)[g_kp_id] = point_3d_c; //map keypoint id to map_feature, do not have tracking of feature
                    if (map_depth) {
                        auto feat_ld = make_shared<log_depth>();
                        feat_ld->v = std::log(point_3d_c.z());
                        feat_ld->initial = v2(p_projected.x(), p_projected.y());
                        map_depth->emplace(g_kp_id, move(feat_ld));
                    }
                    g_kp_id++;
                }
            }
        }
        frame.calculate_dbow(mapper::orb_voc.get());
    }

    auto create_filled_node(uint64_t nid, const transformation & global_tran) {
        // create map_frame for node
        map<uint64_t, v3> map_feats; //map features' ids to their x, y z in camera coordinates.
        map<uint64_t, shared_ptr<log_depth>> map_depth; // map features' ids to their log depth.
        auto new_frame = make_shared<frame_t>();
        create_filled_frame(*new_frame, &map_feats, &map_depth, global_tran, get_3dw_points(), 50); // 50 number of pixel changes

        if (map_feats.size() <= 20) return false; //minimum number of features observed per frame.

        add_node(nid, 0);
        set_node_transformation(nid, global_tran);
        nodes->at(nid).frame = new_frame;

        for (auto &kp : nodes->at(nid).frame->keypoints) {
            add_feature(nid, kp, map_depth[kp->id], 1e-3);
            (*features_dbow)[kp->id] = nid;
        }
        finish_node(nid);
        return true;
    }

    void fill_map(list<transformation> &g_trans) {
        camera_intrinsics.push_back(get_cam_intrinsics());
        camera_extrinsics.push_back(get_cam_extrinsics());
        list<list<transformation>::iterator> removed;
        nodeid nid = 0;
        transformation edge_tran;
        for (auto tran = g_trans.begin(); tran != g_trans.end(); tran++) {
            if (create_filled_node(nid, *tran)) {
                if (nid > 0) { // fill data for testing saving/loading
                    edge_tran = nodes->at(nid - 1).global_transformation * invert(nodes->at(nid).global_transformation);
                    mapper::add_edge(nid - 1, nid, edge_tran, edge_type::filter);
                    mapper::add_covisibility_edge(nid - 1, nid);
                }
                nid++;
            }
            else removed.push_back(tran);
        }
        for (auto rem : removed)
            g_trans.erase(rem);
    }
};

/// get a new map at every call that lasts until the next call.
/// NOTE: create_map_file w/o parameter also calls get_new_map internally, hence invalidating map.
static auto &get_new_map(list<transformation> *frame_trans = nullptr) {
    static unique_ptr<t_mapper> test_map(nullptr);
    test_map.reset(new t_mapper());
    default_random_engine gen(200);
    uniform_real_distribution<float> r, t(-2.f, 2.f); //t unit in meters
    list<transformation> used_trans;
    for (int i = 0; i < MAX_NUM_CAMERA_FRAMES; i++)
        used_trans.emplace_back(transformation(quaternion(r(gen), r(gen), r(gen), r(gen)).normalized(), v3(t(gen), t(gen), t(gen))));
    test_map->fill_map(used_trans);
    if (frame_trans) *frame_trans = used_trans;
    return *test_map;
}

static void save_map_callback(void *handle, const void *buffer, size_t length)
{
    ostringstream * out = (ostringstream *)handle;
    out->write((const char *)buffer, length);
}

static size_t load_map_callback(void *handle, void *buffer, size_t length)
{
    istringstream * out = (istringstream *)handle;
    out->read((char *)buffer, length);
    return (*out) ? length : out->gcount();
}

/// create a new map string as content of serialization of the map param.
static string serialize_map_content(t_mapper &custom_map) {
    ostringstream write_ss;
    custom_map.serialize(save_map_callback, &write_ss);
    return write_ss.str();
}

/// get, and create once at the first call a new map, serialization content of a the map.
static string get_serialized_content() {
    static string map_content;
    ostringstream write_ss;
    if (map_content.size() == 0) {
        t_mapper &map = get_new_map();
        map.serialize(save_map_callback, &write_ss);
        map_content = write_ss.str();
    }
    return map_content;
}

static bool deserialize_map_content(const string &map_content, t_mapper &map_deserialized) {
    istringstream read_ss(map_content, ios_base::in);
    return mapper::deserialize(load_map_callback, &read_ss, map_deserialized);
}

TEST(Mapper, Repeatable_Serialization)
{
    t_mapper &map = get_new_map();
    string map_content1 = serialize_map_content(map);
    EXPECT_GT(map_content1.size(), 0U);

    string map_content2 = serialize_map_content(map);
    EXPECT_EQ(map_content1, map_content2);
}

TEST(Mapper, Deserialize)
{
    t_mapper &map = get_new_map();
    string map_content;
    { // deserialize empty map
        t_mapper map_deserialized;
        EXPECT_FALSE(deserialize_map_content(map_content, map_deserialized));
    }

    map_content = serialize_map_content(map);
    t_mapper map_deserialized;
    EXPECT_TRUE(deserialize_map_content(map_content, map_deserialized));

    // De-serialized map has correct feature and node offsets
    uint32_t max_node_id = 0, max_feat_id = 0;
    map.get_max_ids(max_node_id, max_feat_id);
    EXPECT_EQ(max_node_id, map_deserialized.get_node_id_offset() - 1);
    EXPECT_EQ(max_feat_id, map_deserialized.get_feature_id_offset() - 1);
    EXPECT_TRUE(map_deserialized.is_unlinked());

    // De-serialized map is equivalent to original map
    EXPECT_TRUE(map == map_deserialized);
}

TEST(Mapper, Repeatable_Deserialization)
{
    string map_content = get_serialized_content();

    t_mapper map_deserialized;
    deserialize_map_content(map_content, map_deserialized);

    t_mapper map_deserialized2;
    deserialize_map_content(map_content, map_deserialized2);

    // de-serialization gives consistent map
    EXPECT_TRUE(map_deserialized == map_deserialized2);

    // de-serialized maps produce same serialization outputs
    string map_content1 = serialize_map_content(map_deserialized);
    string map_content2 = serialize_map_content(map_deserialized2);
    EXPECT_EQ(map_content1, map_content2);
}

TEST(Mapper, Serialize_Deserialize_Empty_Map)
{
    t_mapper empty_map;
    string map_content = serialize_map_content(empty_map);
    EXPECT_GT(map_content.size(), 0U);

    t_mapper empty_map_deserialized;
    deserialize_map_content(map_content, empty_map_deserialized);
    EXPECT_TRUE(empty_map == empty_map_deserialized);
}

TEST(Mapper, Deserialize_Serialize)
{
    t_mapper &map = get_new_map();
    string map_content = serialize_map_content(map);
    t_mapper map_deserialized;
    EXPECT_TRUE(deserialize_map_content(map_content, map_deserialized));

    //// Serialize a de-serialized map gives same content when saving in order
    bstream_writer::save_sorted = true;
    map_content = serialize_map_content(map);
    string map_content2 = serialize_map_content(map_deserialized);
    EXPECT_EQ(map_content, map_content2);
    bstream_writer::save_sorted = false;
}

TEST(Mapper, Deserialize_Wrong_Format)
{
    string wrong_map = get_serialized_content().replace(0, 3, "EAM");
    t_mapper fail_deserialized;
    EXPECT_FALSE(deserialize_map_content(wrong_map, fail_deserialized));
}

/// This test is not completed since a big wrong container size value could fail allocation
TEST(Mapper, Deserialize_Corrupted_Stream)
{
    string map_content = get_serialized_content();
    int m_size = map_content.size();
    t_mapper fail_deserialized;
    try {
        for (int t = 1; t < 4; t++) {
            string corrupted = map_content;
            string wrong_data = map_content.substr(m_size / (t * 4), m_size / (t * 4));
            corrupted.replace(m_size / 3, m_size / (t * 4), wrong_data.c_str());
            EXPECT_FALSE(deserialize_map_content(corrupted, fail_deserialized));
        }
    }
    catch (bad_alloc& ba)
    {
        cerr << "bad_alloc caught during loading corrupted map" << ba.what() << '\n';
    }
}
