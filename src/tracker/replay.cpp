/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2016-2018 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#include "replay.h"
#include "file_stream.h"
#include <string.h>
#include <regex>
using namespace std;

replay::replay(host_stream *host_stream_, bool start_paused_): stream(host_stream_), start_paused(start_paused_) {}

void replay::set_relative_pose(const sensor_clock::time_point & timestamp, const tpose & pose)
{
    if(reference_seq) reference_seq->set_relative_pose(timestamp, pose);
}

bool replay::get_reference_pose(const sensor_clock::time_point & timestamp, tpose & pose_out)
{
    return reference_seq && reference_seq->get_pose(timestamp, pose_out);
}

bool replay::set_reference_from_filename(const char *filename) {
    if (!filename) return false;
    string name(filename);
    return load_reference_from_pose_file(name + ".tum") ||
           load_reference_from_pose_file(name + ".pose") ||
           load_reference_from_pose_file(name + ".vicon") ||
           find_reference_in_filename(name);
}

bool replay::load_reference_from_pose_file(const string &filename)
{
    unique_ptr<tpose_sequence> seq = make_unique<tpose_sequence>();
    if (seq->load_from_file(filename)) {
        reference_path_length = seq->get_path_length();
        reference_length = seq->get_length();
        reference_seq = move(seq);
        return true;
    }
    return false;
}

bool replay::set_loaded_map_reference_from_file(const char *filename) {
    if (!filename) return false;
    string name(filename);
    auto p = name.find_last_of('.');
    if (p != string::npos)
        name = name.substr(0, p);  // remove map-json extension

    auto load_from_file = [](const std::string& file) {
        unique_ptr<tpose_sequence> seq = make_unique<tpose_sequence>();
        return (seq->load_from_file(file) ? std::move(seq) : nullptr);
    };

    loaded_map_reference_seq = load_from_file(name + ".tum");
    if (!loaded_map_reference_seq)
        loaded_map_reference_seq = load_from_file(name + ".pose");
    if (!loaded_map_reference_seq)
        loaded_map_reference_seq = load_from_file(name + ".vicon");
    return static_cast<bool>(loaded_map_reference_seq);
}

static bool find_prefixed_number(const std::string in, const std::string &prefix, double &n)
{
    smatch m; if (!regex_search(in, m, regex(prefix + "(\\d+(?:\\.\\d*)?)"))) return false;
    n = std::stod(m[1]);
    return true;
}

bool replay::find_reference_in_filename(const string &filename)
{
    bool PL = find_prefixed_number(filename, "_PL", reference_path_length); reference_path_length /= 100;
    bool L = find_prefixed_number(filename, "_L", reference_length); reference_length /= 100;
    return PL | L;
}

void replay::start_async(){
    if (!is_started) {
        request(packet_command_start);
        is_started = true;
    }
    request(packet_camera_extrinsics);
    if (start_paused) toggle_pause();
    if (!replay_thread.joinable()) replay_thread = std::thread([&]() {stream->start_stream(); }); //thread is needed for supporting with file_stream
}

void replay::start() {
    start_async();
    stream->wait_device_packet({ packet_command_stop });
}

bool replay::load_map(const char *filename) {
    if (!filename) return false;
    ifstream file_handle(filename, std::ios_base::binary);
    if (file_handle.fail())
        return false;
    request(packet_load_map, filename, strlen(filename) + 1);
    return true;
}

bool replay::save_map(const char *filename) {
    if (!filename) return false;
    ofstream file_handle(filename, std::ios_base::binary);
    if (file_handle.fail())
        return false;
    request(packet_save_map, filename, strlen(filename) + 1);
    stream->wait_device_packet({ packet_save_end });
    return true;
}

bool replay::load_internal_calibration(const string &filename)
{
    ifstream rc(filename);
    packet_header_t header = {};
    rc.read((char *)&header, sizeof(header));
    if (!rc || header.bytes <= sizeof(header) || (header.type != packet_calibration_json && header.type != packet_calibration_bin))
        return false;
    std::string json;
    calibration_file = filename + ".json"; // avoid the default save file being the rc file itself
    json.resize(header.bytes - sizeof(header));
    rc.read(&json[0], header.bytes - sizeof(header));
    if (header.type == packet_calibration_json)
        request(packet_load_calibration, json.c_str(), json.size() + 1);
    else if (header.type == packet_calibration_bin)
        request(packet_load_calibration_bin, &json[0], json.size());
    return true;
}

bool replay::load_calibration(const char *filename) {
    if (!filename) return false;
    ifstream file_handle(filename);
    if (file_handle.fail())
        return false;
    string calib_str((istreambuf_iterator<char>(file_handle)), istreambuf_iterator<char>());
    if (calib_str.empty()) return false;
    calibration_file = filename;
    request(packet_load_calibration, calib_str.c_str(), calib_str.size() + 1);
    return true;
}

bool replay::append_calibration(const char *filename) {
    if (!filename) return false;
    ifstream file_handle(filename);
    if (file_handle.fail())
        return false;
    string calib_str((istreambuf_iterator<char>(file_handle)), istreambuf_iterator<char>());
    if (calib_str.empty()) return false;
    request(packet_calibration_json_extra, calib_str.c_str(), calib_str.size() + 1);
    return true;
}

bool replay::set_calibration_from_filename(const char *filename) {
    if (!filename) return false;
    string fn(filename), json;
    if (!load_calibration((json = fn + ".json").c_str())) {
        auto found = fn.find_last_of("/\\");
        string path = fn.substr(0, found + 1);
        if (!load_internal_calibration(filename) && !load_calibration((json = path + "calibration.json").c_str()))
            return false;
    }
    return true;
}

bool replay::save_calibration(const char *filename) {
    if (!filename) return false;
    ofstream file_handle(filename);
    if (file_handle.fail())
        return false;
    request(packet_save_calibration, filename, strlen(filename) + 1);
    stream->wait_device_packet({ packet_save_end });
    return true;
}

std::string replay::get_track_stat() {
    request(packet_timing_stat);
    stream->wait_device_packet({ packet_timing_stat });
    return stream->tracking_stat;
}

rc_StorageStats replay::get_storage_stat() {
    request(packet_storage_stat);
    stream->wait_device_packet({ packet_storage_stat });
    return stream->storage_stat;
}

rc_Extrinsics replay::get_camera_extrinsics(uint8_t camera_id) {
    if (camera_id >= 2) return rc_Extrinsics();
    return stream->camera_extrinsics[camera_id];
}

replay::~replay() {
    end();
}
