//
//  device_parameters.c
//  RC3DK
//
//  Created by Eagle Jones on 4/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "device_parameters.h"
#include <math.h>

bool get_parameters_for_device(corvis_device_type type, struct corvis_device_parameters *dc)
{
    dc->Cx = 319.5;
    dc->Cy = 239.5;
    dc->px = 0.;
    dc->py = 0.;
    dc->K[2] = 0.;
    dc->Wc[0] = sqrt(2.)/2. * M_PI;
    dc->Wc[1] = -sqrt(2.)/2. * M_PI;
    dc->Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    double w_bias_stdev = 1. / 180. * M_PI; //10 dps typical according to specs, but not in practice - factory or apple calibration?
    float w_stdev = .03 * sqrt(50.) / 180. * M_PI; //.03 dps / sqrt(hz) at 50 hz
    float a_stdev = .000218 * sqrt(50.) * 9.8; //218 ug / sqrt(hz) at 50 hz
    dc->w_meas_var = w_stdev * w_stdev;
    dc->a_meas_var = a_stdev * a_stdev;
    dc->image_width = 640;
    dc->image_height = 480;
    dc->shutter_delay = 0;
    dc->shutter_period = 31000;

    for(int i = 0; i < 3; ++i) {
        dc->a_bias[i] = 0.;
        dc->w_bias[i] = 0.;
        dc->a_bias_var[i] = a_bias_stdev * a_bias_stdev;
        dc->w_bias_var[i] = w_bias_stdev * w_bias_stdev;
        dc->Tc_var[i] = 1.e-6;
        dc->Wc_var[i] = 1.e-7;
    }

    switch(type)
    {
        case DEVICE_TYPE_IPHONE4S:
            dc->Fx = 620.;
            dc->Fy = 620.;
            dc->K[0] = .23;
            dc->K[1] = -.52;
            dc->Tc[0] = 0.015;
            dc->Tc[1] = 0.022;
            dc->Tc[2] = 0.001;
            return true;
        case DEVICE_TYPE_IPHONE5:
        case DEVICE_TYPE_IPHONE5C:
            dc->Fx = 585.;
            dc->Fy = 585.;
            dc->K[0] = .10;
            dc->K[1] = -.10;
            dc->Tc[0] = 0.;
            dc->Tc[1] = 0.;
            dc->Tc[2] = -0.008;
            return true;
        case DEVICE_TYPE_IPHONE5S:
            dc->Fx = 525.;
            dc->Fy = 525.;
            dc->K[0] = .06;
            dc->K[1] = -.06;
            dc->Tc[0] = -0.005;
            dc->Tc[1] = 0.030;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_IPOD5:
            dc->Fx = 598.;
            dc->Fy = 598.;
            dc->K[0] = .20;
            dc->K[1] = -.56;
            dc->Tc[0] = -0.03;
            dc->Tc[1] = -0.03;
            dc->Tc[2] = 0.00;
            return true;
        case DEVICE_TYPE_IPAD2:
            dc->Fx = 783.;
            dc->Fy = 783.;
            dc->K[0] = -.03;
            dc->K[1] = .19;
            dc->Tc[0] = -0.015;
            dc->Tc[1] = 0.100;
            dc->Tc[2] = 0.;
            return true;
        case DEVICE_TYPE_IPAD3:
            dc->Fx = 620.;
            dc->Fy = 620.;
            dc->K[0] = .17;
            dc->K[1] = -.38;
            dc->Tc[0] = .05;
            dc->Tc[1] = .005;
            dc->Tc[2] = -.010;
            return true;
        case DEVICE_TYPE_IPAD4:
            dc->Fx = 589.;
            dc->Fy = 589.;
            dc->K[0] = .17;
            dc->K[1] = -.41;
            dc->Tc[0] = .046;
            dc->Tc[1] = .008;
            dc->Tc[2] = -.009;
            return true;
        case DEVICE_TYPE_IPADAIR:
            dc->Fx = 584.;
            dc->Fy = 584.;
            dc->K[0] = .13;
            dc->K[1] = -.31;
            dc->Tc[0] = .05;
            dc->Tc[1] = .005;
            dc->Tc[2] = -.010;
            return true;
        case DEVICE_TYPE_IPADMINI:
            dc->Fx = 590.;
            dc->Fy = 590.;
            dc->K[0] = .20;
            dc->K[1] = -.40;
            dc->Tc[0] = -0.012;
            dc->Tc[1] = 0.047;
            dc->Tc[2] = 0.003;
            return true;
        case DEVICE_TYPE_IPADMINIRETINA:
            dc->Fx = 582.;
            dc->Fy = 582.;
            dc->K[0] = .13;
            dc->K[1] = -.32;
            dc->Tc[0] = -0.003;
            dc->Tc[1] = 0.062;
            dc->Tc[2] = 0.003;
            return true;
        case DEVICE_TYPE_UNKNOWN:
        default:
            dc->Fx = 600.;
            dc->Fy = 600.;
            dc->K[0] = .20;
            dc->K[1] = -.20;
            dc->Tc[0] = 0.01;
            dc->Tc[1] = 0.03;
            dc->Tc[2] = 0.0;
            return false;
    }
}


