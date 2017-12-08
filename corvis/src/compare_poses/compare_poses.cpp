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

        cout << "Compared " << result.errors.nposes << " poses\n";
        cout << "ATE (m):" << result.errors.ate << "\n";
        cout << "RPE-T (m):" << result.errors.rpe_T << "\n";
        cout << "RPE-R (deg):" << result.errors.rpe_R * (180.f / M_PI) << "\n\n";
    }

    return 0;
}

