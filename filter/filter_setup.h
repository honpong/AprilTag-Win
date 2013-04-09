#ifndef __FILTER_SETUP_H
#define __FILTER_SETUP_H

extern "C" {
#include "../cor/cor.h"
#include "../calibration/calibration.h"
}

#include "filter.h"

struct corvis_device_parameters
{
    float Fx, Fy;
    float Cx, Cy;
    float px, py;
    float K[3];
    float a_bias[3];
    float a_bias_var[3];
    float w_bias[3];
    float w_bias_var[3];
    float w_meas_var;
    float a_meas_var;
    float Tc[3];
    float Tc_var[3];
    float Wc[3];
    float Wc_var[3];
    int image_width, image_height;
    int shutter_delay, shutter_period;
};

class filter_setup
{
public:
    camera_calibration cal;
    filter sfm;
    mapbuffer calibdata;
    mapbuffer trackdata;
    mapbuffer solution;
    mapbuffer track_control;
    struct corvis_device_parameters device;
    filter_setup(dispatch_t *input, const char *outfn, struct corvis_device_parameters * device_parameters);
    ~filter_setup();
    struct corvis_device_parameters get_device_parameters();
protected:
    void calibration_config();
    void tracker_config();
    void filter_config();
};

#endif
