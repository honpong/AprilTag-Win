#include "benchmark.h"
#include "tpose.h"
#include <iostream>
#include <inttypes.h>

using namespace std;
int main(int argc, char ** argv)
{
    if(argc != 5) {
        cerr << "Usage: " << argv[0] << " <ground_truth.tum> <pose.tum> <safe_radius_m> <margin_m>\n";
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
    std::vector<uint64_t> gt_outside_times;
    std::vector<uint64_t> tm2_outside_times;
    std::vector<uint64_t> gt_bad_timestamps;
    std::vector<uint64_t> tm2_bad_timestamps;
    tpose_sequence shifted_tm2;
    tpose_sequence shifted_gt;

    transformation G;
    for(auto &pose : pose_sequence.tposes) {
        tpose gt_interp(pose.t);
        if(gt_sequence.get_pose(pose.t, gt_interp)) {
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

    struct stat {
        int total, good;
    };
    struct stat gt_stat{0};
    struct stat tm2_stat{0};

    std::vector<sensor_clock::time_point> gt_crossings;
    std::vector<sensor_clock::time_point> tm2_crossings;

    sensor_clock::time_point gt_crossed_time;
    sensor_clock::time_point tm2_crossed_time;
    bool gt_outside = false;
    bool tm2_outside = false;
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

            if(!gt_outside && gt_dist > radius) {
                gt_outside = true;
                gt_crossed_time = pose.t;
                gt_crossings.push_back(gt_crossed_time);
                if(tm2_dist > radius - margin)
                    gt_stat.good++;
                else
                    gt_bad_timestamps.push_back(sensor_clock::tp_to_micros(pose.t));
            }
            if(gt_outside && gt_dist < radius) {
                gt_outside = false;
                uint64_t d = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(pose.t - gt_crossed_time).count());
                gt_outside_times.push_back(d);
            }

            if(!tm2_outside && tm2_dist > radius - margin) {
                tm2_outside = true;
                tm2_crossed_time = pose.t;
                tm2_stat.total++;
                if(gt_dist > radius - 2*margin)
                    tm2_stat.good++;
                else
                    tm2_bad_timestamps.push_back(sensor_clock::tp_to_micros(pose.t));
            }
            if(tm2_outside && tm2_dist < radius - margin) {
                tm2_outside = false;
                uint64_t d = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(pose.t - tm2_crossed_time).count());
                tm2_outside_times.push_back(d);
            }
        }
        total_time_us = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(pose.t - first_tm2_pose.t).count());
    }

    gt_stat.total  = gt_crossings.size();

    printf("GT: %d total %d good\n", gt_stat.total, gt_stat.good);
    printf("GT bad timestamps: ");
    for(auto t : gt_bad_timestamps) printf("%" PRIu64 " ", t);
    printf("\n");
    printf("TM2: %d total %d good\n", tm2_stat.total, tm2_stat.good);
    printf("TM2 bad timestamps: ");
    for(auto t : tm2_bad_timestamps) printf("%" PRIu64 " ", t);
    printf("\n");

    uint64_t outside_us = 0;
    for(const auto & dt : gt_outside_times)
        outside_us += dt;

    printf("Total time outside playarea: %.2fs\n", outside_us/1.e6);

    printf("Max distance %.2fm (GT) %.2fm (TM2)\n", max_dist, max_dist_gt);
    printf("Total poses: %" PRIu64 "\n", poses);
    printf("Total capture time: %.2fs\n", total_time_us/1.e6);
    printf("CSVHeader,total_time,gt_outside_time,gt_crossings,found_gt_crossings,missed_gt_crossings,tm2_crossings,correct_tm2_crossings,false_tm2_crossings\n");
    printf("CSVContent,%.2f,%.2f,%d,%d,%d,%d,%d,%d\n", total_time_us/1.e6, outside_us/1.e6,
            gt_stat.total, gt_stat.good, gt_stat.total - gt_stat.good,
            tm2_stat.total, tm2_stat.good, tm2_stat.total - tm2_stat.good);


    return 0;
}

