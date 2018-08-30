#include "benchmark.h"
#include "tpose.h"
#include <iostream>

using namespace std;
int main(int argc, char ** argv)
{
    if(argc < 3) {
        cerr << "Usage: " << argv[0] << " <ground_truth.tum> <pose.tum> [<pose2.tum> ...]\n";
        return 1;
    }
    tpose_sequence gt_sequence;
    gt_sequence.load_from_file(argv[1]);

    for(int i = 2; i < argc; i++) {
        cout << "Comparing " << argv[1] << " and " << argv[i] << "\n";
        tpose_sequence pose_sequence;
        pose_sequence.load_from_file(argv[i]);

        benchmark_result result;
        for(auto &pose : pose_sequence.tposes) {
            tpose gt_interp(pose.t);
            if(gt_sequence.get_pose(pose.t, gt_interp)) {
                result.errors.add_pose(pose, gt_interp);
            }
        }

        if(!result.errors.calculate_ate()) {
            cerr << "There was a problem calculating the ATE, exiting\n";
            return 1;
        }

        //save the ground truth after ate registration
        tpose_sequence gt_with_current_ate_sequence;
        gt_with_current_ate_sequence.tposes = gt_sequence.tposes;

        tpose ate_registration{ gt_sequence.tposes.at(0).t, transformation(result.errors.ate_s.R, result.errors.ate_s.T)*gt_sequence.tposes.at(0).G };


        gt_with_current_ate_sequence.set_relative_pose(gt_sequence.tposes.at(0).t, ate_registration);

        {
            ofstream pose_st(std::string(argv[i]) + "_gt_ate.tum");
            for (auto &pose : gt_with_current_ate_sequence.tposes) {
                pose_st << tpose_tum(sensor_clock::tp_to_micros(pose.t) / 1e6, pose.G, 0, 3);
            }
        }

        if(!result.errors.calculate_rpe_600ms()) {
            cerr << "There was a problem calculating the RPE, exiting\n";
            return 1;
        }

        cout << "Compared " << result.errors.nposes << " poses\n";
        cout << "ATE (m):" << result.errors.ate << "\n";
        cout << "RPE-T (m):" << result.errors.rpe_T << "\n";
        cout << "RPE-R (deg):" << result.errors.rpe_R * (180.f / M_PI) << "\n\n";

        if(result.errors.calculate_ate_60s()) {
            cout << "ATE (60s chunks) (m):" << result.errors.ate_60s << "m\n";
        }
        if(result.errors.calculate_ate_600ms()) {
            cout << "ATE (600ms chunks) (m):" << result.errors.ate_600ms << "m\n";
        }
        cout << "SpaceSeparated" << " " << result.errors.nposes << " " << pose_sequence.tposes.size() << " " << result.errors.ate.rmse << " " << result.errors.ate.max;
        cout << " " << result.errors.rpe_R.rmse * (180.f / M_PI) << " " << result.errors.rpe_R.max * (180.f / M_PI) << "\n";
    }

    return 0;
}

