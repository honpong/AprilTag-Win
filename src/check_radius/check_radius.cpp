#include "benchmark.h"
#include "tpose.h"
#include <iostream>
#include <inttypes.h>

using namespace std;
int main(int argc, char ** argv)
{
    if(argc != 5 && argc != 8) {
        cerr << "Usage: " << argv[0] << " <ground_truth.tum> <pose.tum> <safe_radius_m> <margin_m> [<aligned_gt_output.tum> <aligned_tm2_output.tum> <intervals.txt>]\n";
        return 1;
    }
    tpose_sequence gt_sequence;
    gt_sequence.load_from_file(argv[1]);

    tpose_sequence pose_sequence;
    pose_sequence.load_from_file(argv[2]);

    float radius = stof(argv[3]);
    float margin = stof(argv[4]);
    cout << "Comparing " << argv[1] << " and " << argv[2] << " with radius " << radius << "m and margin " << margin << "m\n";

    tpose first_tm2_pose(sensor_clock::s_to_tp(0));

    uint64_t poses = 0;
    uint64_t total_time_us = 0;
    float max_dist = 0;
    float max_dist_gt = 0;
    tpose_sequence shifted_tm2;
    tpose_sequence shifted_gt;

    transformation G;
    for(auto &pose : pose_sequence.tposes) {
        tpose gt_interp(pose.t);
        if(gt_sequence.get_pose(pose.t, gt_interp) && pose.confidence == rc_E_CONFIDENCE_HIGH) {
            first_tm2_pose = pose;
            G = pose.G*invert(gt_interp.G);
            break;
        }
    }

    v3 gt_center(0,0,0);
    uint64_t gt_count = 0;
    for(auto &pose : pose_sequence.tposes) {
        tpose gt_interp(pose.t);
        if(gt_sequence.get_pose(pose.t, gt_interp) && pose.confidence == rc_E_CONFIDENCE_HIGH) {
            gt_count++;
            gt_center += gt_interp.G.T;
        }
    }
    gt_center /= gt_count;
    transformation G_tm2_center(quaternion::Identity(), -(G*gt_center));

    enum stat_type {GT_TYPE = 0, TM2_TYPE};
    typedef struct stat_interval {
        enum stat_type type;
        uint64_t start, stop;
        bool good;
        stat_interval(enum stat_type _type, uint64_t _start, bool _good) : type(_type), start(_start), stop(_start), good(_good) {}
    } stat_interval;
    std::vector<stat_interval> intervals;

    stat_interval gt_interval(GT_TYPE, 0, false);
    bool gt_started = false;

    stat_interval tm2_interval(TM2_TYPE, 0, false);
    bool tm2_started = false;

    for(auto &pose : pose_sequence.tposes) {
        tpose gt_interp(pose.t);
        if(gt_sequence.get_pose(pose.t, gt_interp)) {
            tpose now(pose.t);
            tpose now_gt(pose.t);
            now.G = G_tm2_center*pose.G;
            now_gt.G = G_tm2_center*G*gt_interp.G;

            shifted_tm2.tposes.push_back(now);
            shifted_gt.tposes.push_back(now_gt);
            poses++;

            float tm2_dist = now.G.T.head(2).norm();
            float gt_dist = now_gt.G.T.head(2).norm();

            if(gt_dist > max_dist_gt) max_dist_gt = gt_dist;
            if(tm2_dist > max_dist) max_dist = tm2_dist;

            if(!gt_started && gt_dist > radius) {
                gt_started = true;
                if(tm2_dist > radius - margin)
                    gt_interval = stat_interval(GT_TYPE, sensor_clock::tp_to_micros(pose.t), true);
                else
                    gt_interval = stat_interval(GT_TYPE, sensor_clock::tp_to_micros(pose.t), false);
            }
            if(gt_started && gt_dist < radius) {
                gt_interval.stop = sensor_clock::tp_to_micros(pose.t);
                intervals.push_back(gt_interval);
                gt_started = false;
            }

            if(!tm2_started && tm2_dist > radius - margin) {
                tm2_started = true;
                if(gt_dist > radius - 2*margin)
                    tm2_interval = stat_interval(TM2_TYPE, sensor_clock::tp_to_micros(pose.t), true);
                else
                    tm2_interval = stat_interval(TM2_TYPE, sensor_clock::tp_to_micros(pose.t), false);
            }
            if(tm2_started && tm2_dist < radius - margin) {
                tm2_interval.stop = sensor_clock::tp_to_micros(pose.t);
                intervals.push_back(tm2_interval);
                tm2_started = false;
            }
        }
        total_time_us = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(pose.t - first_tm2_pose.t).count());
    }

    struct stat {
        int total, good;
    };
    struct stat gt_stat{0};
    struct stat tm2_stat{0};

    for(const auto & i : intervals) {
        if(i.type == TM2_TYPE) {
            tm2_stat.total++;
            if(i.good) tm2_stat.good++;
        }
        else {
            gt_stat.total++;
            if(i.good) gt_stat.good++;
        }
    }

    printf("GT: %d total %d good\n", gt_stat.total, gt_stat.good);
    printf("TM2: %d total %d good\n", tm2_stat.total, tm2_stat.good);

    uint64_t outside_us = 0;
    for(const auto & i : intervals)
        if(i.type == GT_TYPE)
            outside_us += (i.stop - i.start);

    printf("Total time outside playarea: %.2fs\n", outside_us/1.e6);

    printf("Max distance %.2fm (GT) %.2fm (TM2)\n", max_dist, max_dist_gt);
    printf("Total poses: %" PRIu64 "\n", poses);
    printf("Total capture time: %.2fs\n", total_time_us/1.e6);
    printf("CSVHeader,total_time,gt_outside_time,gt_crossings,found_gt_crossings,missed_gt_crossings,tm2_crossings,correct_tm2_crossings,false_tm2_crossings\n");
    printf("CSVContent,%.2f,%.2f,%d,%d,%d,%d,%d,%d\n", total_time_us/1.e6, outside_us/1.e6,
            gt_stat.total, gt_stat.good, gt_stat.total - gt_stat.good,
            tm2_stat.total, tm2_stat.good, tm2_stat.total - tm2_stat.good);

    if(argc == 8) {
        ofstream f_gt (argv[5]);
        f_gt << shifted_gt;
        ofstream f_tm2 (argv[6]);
        f_tm2 << shifted_tm2;
        ofstream f_intervals (argv[7]);
        for(const auto & interval : intervals)
            f_intervals << interval.start << " " << interval.stop << " " << interval.type << " " << interval.good << "\n";
    }


    return 0;
}

