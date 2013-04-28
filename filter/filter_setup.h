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
protected:
    void tracker_config();
    int check_health();
};

#endif
