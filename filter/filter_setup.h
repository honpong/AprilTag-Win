#ifndef __FILTER_SETUP_H
#define __FILTER_SETUP_H

extern "C" {
#include "../cor/cor.h"
}

#include "device_parameters.h"
#include "filter.h"

class filter_setup
{
public:
    filter sfm;
    mapbuffer calibdata;
    mapbuffer trackdata;
    mapbuffer solution;
    mapbuffer track_control;
    dispatch_t *input;
    struct corvis_device_parameters device;
    filter_setup(dispatch_t *_input, const char *outfn, struct corvis_device_parameters * device_parameters);
    ~filter_setup();
    struct corvis_device_parameters get_device_parameters();
    int get_failure_code();
    bool get_speed_warning();
    bool get_vision_failure();
    bool get_speed_failure();
    bool get_other_failure();
    float get_filter_converged();
    bool get_device_steady();
    bool get_device_aligned();
protected:
};

#endif
