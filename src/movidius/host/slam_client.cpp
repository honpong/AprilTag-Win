/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#include <getopt.h>

#include <tpose.h>
#include "usb_definitions.h"
#include "tm2_host_stream.h"
#include "replay.h"
#include "rc_compat.h"

#define USE_FAST_PATH rc_RUN_FAST_PATH

static char        calibration_filename_default[] = "calibration.json";
static char *      calibration_filename = calibration_filename_default, *replay_filename = nullptr, 
                   *map_save_filename = nullptr, *map_load_filename = nullptr, *pose_filename = nullptr;
static uint16_t    replay_flags      = REPLAY_SYNC;
static uint16_t    sixdof_flags      = SIXDOF_FLAG_MULTI_THREADED;

using namespace std;

/// check if file input option is valid.
bool check_file_input_option(const char *file_name, int mode, const char *file_type)
{
    if (!file_name) return false;
    bool is_error = false;
    if (!strcmp(file_name, "-")) {
        cerr << "Missing " << file_type << " file name" << endl;
        is_error = true;
    }
    else {
        fstream file_str(file_name, (ios_base::openmode) mode);
        if (!file_str.is_open()) {
            cerr << "Unable to open " << file_type << " file: " << file_name << endl;
            is_error = true;
        }
        else cout << file_type << " file: " << file_name << endl;
    }
    return is_error;
}

/// function wrapper to calculate transfer bandwidth, requiring input function to
/// provide the number of packets and number of bytes been transferred.
template<typename Func, class ... Types>
bool measure_bw(const char *type_name, Func func, Types ... args) {
    uint64_t        num_packets = 0, total_bytes = 0;
    auto            start_ts = std::chrono::steady_clock::now();

    bool val = func(total_bytes, num_packets, args...); //execute the function

    auto end_ts = std::chrono::steady_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end_ts - start_ts);
    float rate_mb__sec = total_bytes * 1.f / micros.count();  // bytes / microsecond = megabytes / sec
    printf("%s: sent %" PRIu64 " packets with %" PRIu64 " bytes in %.2f MB/sec\n",
        type_name, num_packets, total_bytes, rate_mb__sec);
    return val;
}

void print_usage(const char *program_name) {
    cerr << "Usage: " << program_name
        << " [-p <replay_filename>] [-6 <6dof_filename>] [-r] [-s] -c <calibration_filename>"
        << " [-m <map_save_filename>] [-l <map_load_filename>]\n";
    cerr << "switches:\n";
    cerr << "\t -p <replay_filename>: file to replay\n";
    cerr << "\t -6 <6dof_filename>: print tum format poses to 6dof_file "
        "(default stdout)\n";
    //cerr << "\t -r: enable real-time playback\n";
    //cerr << "\t -s: enable single threaded mode (disables realtime playback)\n";
    cerr << "\t -c <calibration_filename>: calibration file, required if replay is"
        << " enabled and no calibration.json file exists\n";
    cerr << "\t -m <map_save_filename>: map saving file\n";
    cerr << "\t -l <map_load_filename>: map load file.\n";
    exit(1);
}

int main(int argc, char ** argv)
{
    const char *  optstring = "rsp:6:c:l:m:";
    extern int    optind, optopt;
    extern char * optarg;
    bool          option_error = false;
    int           c;
    bool          start_paused = false;
    while((c = getopt(argc, argv, optstring)) != -1) {
        switch(c) {
            case 'r':
                //replay_flags = REPLAY_REALTIME;
                break;
            case 's':
                //sixdof_flags = SIXDOF_FLAG_SINGLE_THREADED;
                break;
            case 'p': {
                option_error = check_file_input_option(optarg, ios_base::binary | ios_base::in, "replay");
                replay_filename = optarg;
                break;
            }
            case 'c':
                calibration_filename = optarg;
                break;
            case '6':
                option_error = check_file_input_option(optarg, ios_base::out, "pose");
                pose_filename = optarg;
                break;
            case 'l':
                option_error = check_file_input_option(optarg, ios_base::binary | ios_base::in, "load map");
                map_load_filename = optarg;
                break;
            case 'm':
                option_error = check_file_input_option(optarg, ios_base::binary | ios_base::out, "save map");
                map_save_filename = optarg;
                break;
            case ':':
                cerr << "Option -" << (char)optopt << " requires an operand\n";
                option_error = true;
                break;
            case '?':
                cerr << "Unrecognized option -" << (char)optopt << "\n";
                option_error = true;
                break;
        }
        if (option_error) break;
    }
    option_error = option_error || check_file_input_option(calibration_filename, ios_base::in, "calibration");

    if (option_error || optind != argc) print_usage(argv[0]);

    if(replay_filename && !(replay_flags == REPLAY_REALTIME))
        sixdof_flags = SIXDOF_FLAG_SINGLE_THREADED;

    auto rp = std::make_unique<replay>(new tm2_host_stream(replay_filename), start_paused);
    if (!rp->init()) {
        cerr << "Failed to start!\n";
        exit(1);
    }
    if (replay_flags == REPLAY_REALTIME) rp->enable_realtime();
    if (map_save_filename || map_load_filename) rp->start_mapping(true, map_save_filename != NULL); // needs to happen before enable pose thread
    rp->enable_fast_path();

    std::ofstream pose_fs;
    if (pose_filename) {
        pose_fs.open(pose_filename);
        rp->set_data_callback([&pose_fs](const replay_output *rp_output, const rc_Data *data) {
            // only output fast path poses when using the fast path
            if ((rc_DATA_PATH_FAST == rp_output->data_path && rc_RUN_FAST_PATH != USE_FAST_PATH) ||
                (rc_DATA_PATH_SLOW == rp_output->data_path && rc_RUN_NO_FAST_PATH != USE_FAST_PATH))
                return;
            if (pose_fs.is_open() && pose_fs.good()) {
                auto &pose = rp_output->rc_getPose(rp_output->data_path);
                pose_fs << tpose_tum(pose.time_us / 1.e6, to_transformation(pose.pose_m));
            }
        });
    }

    if(replay_filename) {
        rp->set_progress_callback([](float perc) {
            static float thres_hold = 0.099f;
            if (perc >= thres_hold) {
                printf("%.0f%% sensor data processed\n", std::ceil(10 * perc) * 10);
                thres_hold += 0.1f;
            }
        });
        if (calibration_filename) rp->load_calibration(calibration_filename);
        if (map_load_filename) rp->load_map(map_load_filename);
        printf("Replay started\n");
        if (!measure_bw("Sensor Data",
                [](uint64_t &total_bytes, uint64_t &number_packets, replay *rp_instance)->bool {
                rp_instance->start();
                total_bytes = rp_instance->get_bytes_dispatched();
                number_packets = rp_instance->get_packets_dispatched();
                return true;
            }, rp.get()))
        {
            cerr << "Error: reading replay file, exiting.\n";
            exit(1);
        }
    }
    if (map_save_filename) rp->save_map(map_save_filename);
    printf("Timing:\n%s\n", rp->get_track_stat().c_str());
    rp->end();
    return 0;
}
