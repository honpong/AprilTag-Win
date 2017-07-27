#include <math.h>
#include <signal.h>  // capture sigint
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // getopt
#include <cinttypes>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include "packet.h"
#include "usb_host.h"

static std::thread thread_6dof, thread_replay;
static bool        enable_replay = false;
static char *      replay_filename;
static FILE *      replay_file        = nullptr;
static char        calibration_filename_default[] = "calibration.json";
static char *      calibration_filename = calibration_filename_default;
static bool        enable_pose_output = true;
static char *      pose_filename;
static FILE *      pose_file         = stderr;
static uint16_t    replay_flags      = REPLAY_SYNC;
static uint16_t    sixdof_flags      = SIXDOF_FLAG_MULTI_THREADED;
static uint64_t    last_exptected_ts = 0;

void shutdown(int _unused = 0)
{
    usb_shutdown(0);
    if(replay_file) fclose(replay_file);
    if(pose_file) fclose(pose_file);
}

using namespace std;
int main(int argc, char ** argv)
{
    const char *  optstring = "rsp:6:c:";
    extern int    optind, optopt;
    extern char * optarg;
    bool          option_error = false;
    int           c;
    while((c = getopt(argc, argv, optstring)) != -1) {
        switch(c) {
            case 'r':
                replay_flags = REPLAY_REALTIME;
                break;
            case 's':
                sixdof_flags = SIXDOF_FLAG_SINGLE_THREADED;
                break;
            case 'p':
                enable_replay   = true;
                replay_filename = optarg;
                replay_file     = fopen(replay_filename, "rb");
                cout << "Replaying from " << replay_filename << "\n";
                if(!replay_file) {
                    cerr << "Unable to open " << replay_filename << "\n";
                    option_error = true;
                }
                break;
            case 'c':
                calibration_filename = optarg;
                break;
            case '6':
                enable_pose_output = true;
                pose_filename      = optarg;
                if(!strcmp(pose_filename, "-")) {
                    pose_file = stderr;
                }
                else {
                    pose_file = fopen(pose_filename, "w");
                }
                cout << "Writing poses to " << pose_filename << "\n";
                if(!pose_file) {
                    cerr << "Unable to open " << pose_filename << "\n";
                    option_error = true;
                }
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
    }

    if(option_error || optind != argc) {
        cerr << "Usage: " << argv[0]
             << " [-p <replay_filename>] [-6 <6dof_filename>] [-r] [-s]\n";
        cerr << "switches:\n";
        cerr << "\t -p <replay_filename>: file to replay\n";
        cerr << "\t -6 <6dof_filename>: print tum format poses to 6dof_file "
                "(default stdout)\n";
        cerr << "\t -r: enable real-time playback\n";
        cerr << "\t -s: enable single threaded mode (disables realtime "
                "playback)\n";
        exit(1);
    }

    usb_init();
    signal(SIGINT, shutdown);

    if(enable_replay && !(replay_flags == REPLAY_REALTIME))
        sixdof_flags = SIXDOF_FLAG_SINGLE_THREADED;

    if(!usb_send_control(CONTROL_MESSAGE_START, 0, 0, 0)) {
        cerr << "Failed to start!\n";
        exit(1);
    }

    // 6dof capture
    if(enable_pose_output) {
        thread_6dof = std::thread([] {
            printf("6dof Capture started\n");
            int poses_read = 0;
            while(1) {
                packet_rc_pose_t pose_in = usb_read_6dof();
                fprintf(pose_file, "%.9f ",
                        pose_in.header.time / 1.e6);  // seconds
                fprintf(pose_file, "%.9f %.9f %.9f ", pose_in.pose.T.x,
                        pose_in.pose.T.y, pose_in.pose.T.z);
                fprintf(pose_file, "%.9f %.9f %.9f %.9f\n", pose_in.pose.Q.x,
                        pose_in.pose.Q.y, pose_in.pose.Q.z, pose_in.pose.Q.w);
                poses_read++;
                if(last_exptected_ts != 0) {
                    if(pose_in.header.time + 10 > last_exptected_ts) {
                        printf("Got the last expected packet - exit 6dof\n");
                        break;
                    }
                    else {
                        printf("draining out. Recevied %" PRIu64
                               " waiting for %f\n",
                               pose_in.header.time, float(last_exptected_ts));
                    }
                }
            }
        });
    }

    // replay thread
    if(enable_replay) {
        thread_replay = std::thread([] {
            printf("Replay started\n");
            uint64_t        packets     = 0;
            uint64_t        total_bytes = 0;
            packet_header_t header;
            auto            start_ts    = std::chrono::steady_clock::now();
            uint64_t        sent_imu_ts = 0;

            // write calibration
            std::ifstream calibration_file(calibration_filename);

            if(!calibration_file) {
                cerr << "Failed to open " << calibration_filename << "\n";
                exit(1);
            }
            cout << "Loading " << calibration_filename << "\n";
            std::stringstream calibration_buffer;
            calibration_buffer << calibration_file.rdbuf();
            size_t calibration_bytes = calibration_buffer.str().length();
            packet_calibration_json_t * packet = (packet_calibration_json_t *)malloc(calibration_bytes + sizeof(packet_header_t) + 1);
            packet->header.bytes = calibration_bytes + sizeof(packet_header_t) + 1;
            packet->header.type = packet_calibration_json;
            packet->header.time = 0;
            packet->header.sensor_id = 0;
            memcpy(packet->data, calibration_buffer.str().c_str(), calibration_bytes + 1);
            usb_write_packet((packet_t *)packet);
            free(packet);

            while(fread(&header, sizeof(packet_header_t), 1, replay_file)) {
                packet_t * packet = (packet_t *)malloc(header.bytes);
                packet->header    = header;
                size_t count =
                        fread(&packet->data,
                              packet->header.bytes - sizeof(packet_header_t), 1,
                              replay_file);
                if(count != 1) {
                    fprintf(stderr, "Error reading replay file, exiting\n");
                    exit(1);
                }
                if(packet->header.type == packet_gyroscope ||
                   packet->header.type == packet_accelerometer) {
                    sent_imu_ts = packet->header.time;
                }
                if(packets % 1000 == 0) {
                    printf("%" PRIu64 " packets written\n", packets);
                }
                //printf("writing a packet %u bytes with time %lu\n", packet->header.bytes, packet->header.time);
                usb_write_packet(packet);
                total_bytes += packet->header.bytes;
                free(packet);
                packets++;
            }
            last_exptected_ts = sent_imu_ts;
            auto end_ts       = std::chrono::steady_clock::now();
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
                    end_ts - start_ts);
            float rate_mb__sec =
                    total_bytes * 1.f /
                    micros.count();  // bytes / microsecond = megabytes / sec
            printf("sent %" PRIu64 " packets with %" PRIu64 " bytes in %.2f MB/sec\n",
                   packets, total_bytes, rate_mb__sec);
            printf("Last timestamp sent is: %.9f\n", last_exptected_ts / 1.e6);
        });
    }

    if(enable_pose_output) thread_6dof.join();
    if(enable_replay) thread_replay.join();

    shutdown();
    return 0;
}