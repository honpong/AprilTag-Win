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

    bool first_pose = false;
    tpose first_tm2_pose(sensor_clock::s_to_tp(0));
    tpose first_gt_pose(sensor_clock::s_to_tp(0));

    uint64_t poses = 0;
    uint64_t total_time_us = 0;
    float max_dist = 0;
    float max_dist_gt = 0;
    std::vector<uint64_t> unsafe_intervals;
    std::vector<uint64_t> gt_outside_times;
    std::vector<uint64_t> tm2_outside_times;
    tpose_sequence shifted_tm2;
    tpose_sequence shifted_gt;
    m3 dR, dR2;
    v3 tm2_center_offset;

    v3 gt_center(0,0,0);
    uint64_t gt_count = 0;
    uint64_t middle_pose = pose_sequence.tposes.size()/2;
    for(auto &pose : pose_sequence.tposes) {
        tpose gt_interp(pose.t);
        if(gt_sequence.get_pose(pose.t, gt_interp)) {
            gt_count++;
            gt_center += gt_interp.G.T;
        }
        if(gt_count == middle_pose) {
            dR = (pose.G.Q*gt_interp.G.Q.inverse()).toRotationMatrix();
        }
    }
    gt_center /= gt_count;


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
    uint64_t found_gt_crossings = 0;
    uint64_t false_tm2_crossings = 0;
    for(auto &pose : pose_sequence.tposes) {
        tpose gt_interp(pose.t);
        if(gt_sequence.get_pose(pose.t, gt_interp)) {
            if(!first_pose) {
                first_pose = true;
                first_tm2_pose = pose;
                first_gt_pose = gt_interp;
                dR2 = (first_tm2_pose.G.Q*first_gt_pose.G.Q.inverse()).toRotationMatrix();
                tm2_center_offset = -dR2*(gt_center - first_gt_pose.G.T);
            }

            v3 dt_now = pose.G.T - first_tm2_pose.G.T + tm2_center_offset;
            v3 dt_now_gt = dR2*(gt_interp.G.T - first_gt_pose.G.T) + tm2_center_offset;
            tpose now(pose.t);
            tpose now_gt(pose.t);
            now.G = transformation(quaternion(1,0,0,0), dt_now);
            now_gt.G = transformation(quaternion(1,0,0,0), dt_now_gt);
            shifted_tm2.tposes.push_back(now);
            shifted_gt.tposes.push_back(now_gt);
            poses++;

            float tm2_dist = sqrt(dt_now[0]*dt_now[0] + dt_now[1]*dt_now[1]);
            float gt_dist = sqrt(dt_now_gt[0]*dt_now_gt[0] + dt_now_gt[1]*dt_now_gt[1]);

            if(gt_dist > max_dist_gt) max_dist_gt = gt_dist;
            if(tm2_dist > max_dist) max_dist = tm2_dist;

            if(!gt_outside && gt_dist > radius) {
                gt_outside = true;
                gt_crossed_time = pose.t;
                gt_crossings.push_back(gt_crossed_time);
                if(tm2_dist > radius - margin)
                    gt_stat.good++;
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
    printf("TM2: %d total %d good\n", tm2_stat.total, tm2_stat.good);

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

