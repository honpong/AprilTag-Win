#include "rc_replay.h"
#include <iostream>
#include <cmath>
#include <stdio.h>

int main(int c, char **v)
{
    if (0) { usage:
        std::cerr << "Usage: " << v[0] << " [--drop-depth] [--output-poses] [--output-status] [--output-log] <logfile>\n";
        return 1;
    }

    rc::replay rp;

    char *filename = nullptr;
    for (int i=1; i<c; i++)
        if      (v[i][0] != '-' && !filename) filename = v[i];
        else if (strcmp(v[i], "--drop-depth") == 0) rp.disable_depth();
        else if (strcmp(v[i], "--output-poses") == 0) rp.enable_pose_output();
        else if (strcmp(v[i], "--output-status") == 0) rp.enable_status_output();
        else if (strcmp(v[i], "--output-log") == 0) rp.enable_log_output(0, 333333);
        else goto usage;

    if (!filename)
        goto usage;

    if (!rp.open(filename) || !rp.run()) {
        std::cerr << "error replaying " << filename << "\n";
        return 1;
    }

    printf("Reference Straight-line length is %.2f cm, total path length %.2f cm\n", 100*rp.get_reference_length(), NAN);
    printf("Computed  Straight-line length is %.2f cm, total path length %.2f cm\n", 100*rp.get_length(), NAN);
    return 0;
}
