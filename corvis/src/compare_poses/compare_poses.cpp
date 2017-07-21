#include "benchmark.h"
#include "tpose.h"
#include <iostream>

using namespace std;
int main(int argc, char ** argv)
{
    if(argc != 3) {
        cerr << "Usage: " << argv[0] << " <ground_truth.tum> <pose.tum>\n";
        return 1;
    }
    tpose_sequence gt_sequence;
    gt_sequence.load_from_file(argv[1]);

    tpose_sequence pose_sequence;
    pose_sequence.load_from_file(argv[2]);

    benchmark_result result;
    uint64_t pose_count = 0;
    for(auto & pose : pose_sequence.tposes) {
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
    cout << "ATE:" << result.errors.ate << "\n";

    return 0;
}

