#ifndef __FILTER_SETUP_H
#define __FILTER_SETUP_H

extern "C" {
#include "../cor/cor.h"
#include "../calibration/calibration.h"
}

#include "filter.h"

class filter_setup
{
public:
    camera_calibration cal;
    filter sfm;
    mapbuffer calibdata;
    mapbuffer trackdata;
    mapbuffer solution;
    mapbuffer track_control;
    filter_setup(dispatch_t *input, const char *outfn);
    ~filter_setup();
protected:
    void calibration_config();
    void tracker_config();
    void filter_config();
};

#endif
