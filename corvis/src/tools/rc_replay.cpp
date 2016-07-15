//  Copyright (c) 2015 RealityCap. All rights reserved.
#include "rc_replay.h"
#include <iostream>
#include <cmath>
#include <stdio.h>
#include <string.h>

int main(int c, char **v)
{
    if (0) { usage:
        std::cerr << "Usage: " << v[0] << " [--drop-depth] [--qvga] [--output-poses] [--output-status] [--output-summary] [--pause] [--version] <logfile>..\n";
        return 1;
    }

    rc::replay rp;

    int filenames = 0; bool summary = false; bool pause = false;
    for (int i=1; i<c; i++)
        if      (v[i][0] != '-') v[1+filenames++] = v[i];
        else if (strcmp(v[i], "--drop-depth") == 0) rp.disable_depth();
        else if (strcmp(v[i], "--qvga") == 0) rp.enable_qvga();
        else if (strcmp(v[i], "--output-poses") == 0) rp.enable_pose_output();
        else if (strcmp(v[i], "--output-status") == 0) rp.enable_status_output();
        else if (strcmp(v[i], "--output-summary") == 0) summary = true;
        else if (strcmp(v[i], "--pause") == 0) pause = true;
        else if (strcmp(v[i], "--version") == 0) {
            std::cerr << rp.get_version() << "\n\n"; goto usage;
        }
        else goto usage;

    if (!filenames)
        goto usage;

    for (int i=0; i<filenames; i++) {
        char *filename = v[1+i];
        if (i) rp.reset();

        if (pause) {
            printf("Paused, press enter to start\n");
            getchar();
            printf("Running\n");
        }

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
