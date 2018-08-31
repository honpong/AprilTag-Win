#include "benchmark.h"
#include "tpose.h"
#include <iostream>

using namespace std;
int main(int argc, char ** argv)
{
    std::vector<std::string> sequence_filenames;
    bool align_output = false;

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-a") == 0) align_output = true;
        else                           sequence_filenames.push_back(argv[i]);
    }

    if(sequence_filenames.size() < 2) {
        cerr << "Usage: " << argv[0] << "[-a] <ground_truth.tum> <pose.tum> [<pose2.tum> ...]\n";
        return 1;
    }

    tpose_sequence gt_sequence;
    gt_sequence.load_from_file(sequence_filenames[0]);

    for(size_t i = 1; i < sequence_filenames.size(); i++) {
        cout << "Comparing " << sequence_filenames[0] << " and " << sequence_filenames[i] << "\n";
        tpose_sequence pose_sequence;
        pose_sequence.load_from_file(sequence_filenames[i]);

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

        //save the ground truth after ate registration
        if(align_output) {
            tpose_sequence gt_sequence_aligned;
            gt_sequence_aligned.tposes = gt_sequence.tposes;

            tpose ate_registration{ gt_sequence.tposes.at(0).t, transformation(result.errors.ate_s.R, result.errors.ate_s.T)*gt_sequence.tposes.at(0).G };
            gt_sequence_aligned.set_relative_pose(gt_sequence.tposes.at(0).t, ate_registration);
            ofstream pose_st(std::string(sequence_filenames[i]) + "_gt_ate.tum");
            pose_st << gt_sequence_aligned;
        }
    }

    return 0;
}

