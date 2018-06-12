#include "bstream.h"
#include "packet.h"

using namespace std;

bool bstream_writer::save_sorted = false;

size_t mem_load_callback(void * handle, void *buffer, size_t length) {
    auto *stream_pos = (replay_output::bstream_buffer *)handle;
    return stream_pos->copy_to(buffer, length);
}

static void mem_save_callback(void * handle, const void *buffer, size_t length) {
    auto * stream_pos = (replay_output::bstream_buffer *)handle;
    stream_pos->copy_from(buffer, length);
}

template<typename A>
static array<char, sizeof(A)> & binary_array(A & val) {
    return (array<char, sizeof(A)>&) val;
}

const rc_PoseTime &pose_feature_output::rc_getPose(rc_DataPath path, rc_PoseVelocity *v,
    rc_PoseAcceleration *a) const {
    const auto &pose = (path == rc_DATA_PATH_SLOW) ? poses.slow : poses.fast;
    if (v) *v = pose.veloc;
    if (a) *a = pose.accel;
    return pose.pose_time;
}

void pose_feature_output::rc_setPose(rc_DataPath path, const rc_PoseTime &&pt, const rc_PoseVelocity *v,
    const rc_PoseAcceleration *a) {
    auto &pose = (path == rc_DATA_PATH_SLOW) ? poses.slow : poses.fast;
    if (v) pose.veloc = *v;
    if (a) pose.accel = *a;
    pose.pose_time = pt;
}

void pose_feature_output::rc_setFeatures(rc_Feature *feat, uint32_t num_feat) {
    rc_resetFeatures();
    reuse_features_ptr = feat;
    num_features = num_feat;
}

void pose_feature_output::rc_resetFeatures() {
    num_features = 0;
    features = nullptr;
    reuse_features_ptr = nullptr;
}

uint32_t pose_feature_output::rc_getFeatures(rc_Feature **feat) const {
    if (feat) *feat = (reuse_features_ptr) ? reuse_features_ptr : features.get();
    return num_features;
}

void pose_feature_output::rc_setRelocPoses(rc_Pose *reloc_pose, uint32_t num_poses) {
    rc_resetRelocPoses();
    reuse_reloc_poses_ptr = reloc_pose;
    num_reloc_poses = num_poses;
}

void pose_feature_output::rc_resetRelocPoses() {
    num_reloc_poses = 0;
    reloc_poses = nullptr;
    reuse_reloc_poses_ptr = nullptr;
}

uint32_t pose_feature_output::rc_getRelocPoses(rc_Pose **reloc_pose) const {
    if (reloc_pose) *reloc_pose = (reuse_reloc_poses_ptr) ? reuse_reloc_poses_ptr : reloc_poses.get();
    return num_reloc_poses;
}

bool pose_feature_output::set_data(bstream_reader &cur_stream) {
    rc_resetFeatures();
    cur_stream >> binary_array(poses) >> (uint8_t &)output_type;
    cur_stream >> num_features;
    if (num_features > 0 &&
        (output_type == output_mode::POSE_FEATURE || output_type == output_mode::POSE_FEATURE_MAP)) {
        features = unique_ptr<rc_Feature[]>(new rc_Feature[num_features]);
        cur_stream.read((char *)features.get(), num_features * sizeof(rc_Feature));
    }
    cur_stream >> num_reloc_poses;
    if (num_reloc_poses > 0 &&
        (output_type == output_mode::POSE_MAP || output_type == output_mode::POSE_FEATURE_MAP)) {
        reloc_poses = unique_ptr<rc_Pose[]>(new rc_Pose[num_reloc_poses]);
        cur_stream.read((char *)reloc_poses.get(), num_reloc_poses * sizeof(rc_Pose));
    }
    return cur_stream.good();
}

size_t pose_feature_output::get_data(bstream_writer &cur_stream) const {
    cur_stream << binary_array(poses) << (uint8_t)output_type;
    uint32_t num_saved_feature = (num_features < max_features) ? num_features : max_features;
    cur_stream << num_saved_feature;
    if (output_type == output_mode::POSE_FEATURE || output_type == output_mode::POSE_FEATURE_MAP) {
        rc_Feature *cur_feats = (reuse_features_ptr) ? reuse_features_ptr : features.get();
        if (cur_feats) cur_stream.write((char *)cur_feats, num_saved_feature * sizeof(rc_Feature));
    }
    uint32_t num_saved_poses = (num_reloc_poses < max_reloc_pose) ? num_reloc_poses : max_reloc_pose;
    cur_stream << num_saved_poses;
    if (output_type == output_mode::POSE_MAP || output_type == output_mode::POSE_FEATURE_MAP) {
        rc_Pose *cur_poses = (reuse_reloc_poses_ptr) ? reuse_reloc_poses_ptr : reloc_poses.get();
        if (cur_poses) cur_stream.write((char *)cur_poses, num_saved_poses * sizeof(rc_Pose));
    }
    return cur_stream.total_io_bytes;
}

size_t pose_feature_output::get_feature_size() const {
    size_t cur_size = 0;
    if (num_features == 0 && //if not set features yet
        (output_type == output_mode::POSE_FEATURE || output_type == output_mode::POSE_FEATURE_MAP))
        cur_size += max_features * sizeof(rc_Feature);
    return cur_size;
}

size_t pose_feature_output::get_reloc_poses_size() const {
    size_t cur_size = 0;
    if (num_reloc_poses == 0 && //if not set poses yet
        (output_type == output_mode::POSE_MAP || output_type == output_mode::POSE_FEATURE_MAP))
        cur_size += max_reloc_pose * sizeof(rc_Pose);
    return cur_size;
}

replay_output::replay_output() : packet_header((char *)malloc(sizeof(packet_header_t)), free) {
    buffer_size = get_buffer_size();
    write_buffer = { malloc(buffer_size), free };
}

size_t replay_output::get_data(bstream_buffer &&pos) const {
    bstream_writer cur_stream(mem_save_callback, &pos);
    cur_stream.write(packet_header.get(), sizeof(packet_header_t));
    cur_stream << (uint8_t)data_path << path_length << sensor_time_us;
    cur_stream << (uint8_t)sensor_type << sensor_id << (uint8_t)confidence;
    pose_feature_output::get_data(cur_stream);
    cur_stream.end_stream(); //call to complete delayed writing.
    return cur_stream.total_io_bytes;
}

bool replay_output::set_data(bstream_buffer &&pos) {
    bstream_reader cur_stream(mem_load_callback, &pos);
    cur_stream.read(packet_header.get(), sizeof(packet_header_t));
    cur_stream >> (uint8_t &)data_path >> path_length >> sensor_time_us;
    cur_stream >> (uint8_t &)sensor_type >> sensor_id >> (uint8_t &)confidence;
    return pose_feature_output::set_data(cur_stream);
}

void replay_output::set_output_type(output_mode type) {
    pose_feature_output::output_type = type;
    size_t new_size = get_buffer_size();
    if (buffer_size != new_size) {
        buffer_size = new_size;
        write_buffer = { realloc(write_buffer.release(), buffer_size), free };
    }
}

size_t replay_output::get_buffer_size() const {
    unique_ptr<void, decltype(&free)> buf(malloc(500), free); // some initial value
    return get_data(bstream_buffer(0, 500, buf)) + pose_feature_output::get_feature_size() + pose_feature_output::get_reloc_poses_size();
}

bool replay_output::get(void **data, size_t *size) const {
    if (!data) return false;
    bstream_buffer pos{ 0, buffer_size, write_buffer };
    get_data(pos);
    buffer_size = pos.get_length();
    if (size) *size = pos.get_length();
    *data = write_buffer.get();
    return true;
}

bool replay_output::set(void *data, size_t size) {
    if (!data) return false;
    return set_data(bstream_buffer(0, size, data));
}

void replay_output::set_pose_output(const replay_output &other) {
    path_length = other.path_length;
    poses = other.poses;
}

#include <inttypes.h>
void replay_output::print_pose (uint8_t data_path) const
{
    const auto &pose = (data_path == rc_DATA_PATH_SLOW) ? poses.slow.pose_time.pose_m : poses.fast.pose_time.pose_m;
    printf("\nPose(%" PRId64 ", Tx:%.9f, Ty:%.9f, Tz:%.9f, Qx:%.9f, Qy:%.9f, Qz:%.9f, Qw:%.9f",
        sensor_time_us, pose.T.x, pose.T.y, pose.T.z, pose.Q.x, pose.Q.y, pose.Q.z, pose.Q.w);
}
