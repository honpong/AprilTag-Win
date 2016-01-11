//  Copyright (c) 2015 RealityCap. All rights reserved.
#include "rc_replay.h"
#include <iostream>
#include <cmath>
#include <stdio.h>

int main(int c, char **v)
{
    if (0) { usage:
        std::cerr << "Usage: " << v[0] << " [--drop-depth] [--qvga] [--output-poses] [--output-status] [--output-log] [--output-summary] [--output-trace] <logfile>..\n";
        return 1;
    }

    rc::replay rp;

    int filenames = 0; bool summary = false;
    for (int i=1; i<c; i++)
        if      (v[i][0] != '-') v[1+filenames++] = v[i];
        else if (strcmp(v[i], "--drop-depth") == 0) rp.disable_depth();
        else if (strcmp(v[i], "--qvga") == 0) rp.enable_qvga();
        else if (strcmp(v[i], "--output-trace") == 0) rp.enable_trace_output();
        else if (strcmp(v[i], "--output-poses") == 0) rp.enable_pose_output();
        else if (strcmp(v[i], "--output-status") == 0) rp.enable_status_output();
        else if (strcmp(v[i], "--output-log") == 0) rp.enable_log_output(0, 333333);
        else if (strcmp(v[i], "--output-summary") == 0) summary = true;
        else goto usage;

    if (!filenames)
        goto usage;

    for (int i=0; i<filenames; i++) {
        char *filename = v[1+i];
        if (i) rp.reset();

        if (!rp.open(filename) || !rp.run()) {
            std::cerr << "error replaying " << filename << "\n";
            return 1;
        }

        if (summary) {
            printf("Reference Straight-line length is %.2f cm, total path length %.2f cm\n", 100*rp.get_reference_length(), NAN);
            printf("Computed  Straight-line length is %.2f cm, total path length %.2f cm\n", 100*rp.get_length(), NAN);
        }
    }

    return 0;
}
