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
    dc->px = 0.;
    dc->py = 0.;
    dc->K[2] = 0.;
    dc->Wc[0] = sqrt(2.)/2. * M_PI;
    dc->Wc[1] = -sqrt(2.)/2. * M_PI;
    dc->Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    double w_bias_stdev = 10. / 180. * M_PI / 2.; //10 dps typical according to specs, but not in practice - factory or apple calibration? Found some devices with much larger range
    float w_stdev = .03 * sqrt(50.) / 180. * M_PI; //.03 dps / sqrt(hz) at 50 hz
    float a_stdev = .000218 * sqrt(50.) * 9.8; //218 ug / sqrt(hz) at 50 hz
    dc->w_meas_var = w_stdev * w_stdev;
    dc->a_meas_var = a_stdev * a_stdev;
    dc->image_width = 640;
    dc->image_height = 480;
    int max_dim = dc->image_width > dc->image_height ? dc->image_width : dc->image_height;

    dc->Cx = (dc->image_width - 1)/2.;
    dc->Cy = (dc->image_height - 1)/2.;
    dc->shutter_delay = 0;
    dc->shutter_period = 31000;

    for(int i = 0; i < 3; ++i) {
        dc->a_bias[i] = 0.;
        dc->w_bias[i] = 0.;
        dc->a_bias_var[i] = a_bias_stdev * a_bias_stdev;
        dc->w_bias_var[i] = w_bias_stdev * w_bias_stdev;
        dc->Tc_var[i] = 1.e-6;
        dc->Wc_var[i] = 1.e-6;
    }
    dc->Tc_var[2] = 1.e-10; //Letting this float gives ambiguity with focal length

    switch(type)
    {
        case DEVICE_TYPE_IPHONE4S: //Tc from new sequence appears reasonably consistent with teardown
            dc->Fx = 606./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .21;
            dc->K[1] = -.45;
            dc->Tc[0] = 0.010;
            dc->Tc[1] = 0.006;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_IPHONE5: //Tc from new sequence is consistent with teardown
            dc->Fx = 596./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .12;
            dc->K[1] = -.20;
            dc->Tc[0] = 0.010;
            dc->Tc[1] = 0.008;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_IPHONE5C: //Guess from teardown - Tc is different from iphone 5
            dc->Fx = 596./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .12;
            dc->K[1] = -.20;
            dc->Tc[0] = 0.005;
            dc->Tc[1] = 0.025;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_IPHONE5S: //Tc from sequence appears consistent with teardown; static calibration for my 5s is not right for older sequences
            //iphone5s_sam is a bit different, but within range
            dc->Fx = 547./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .09;
            dc->K[1] = -.15;
            dc->Tc[0] = 0.015;
            dc->Tc[1] = 0.050;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_IPHONE6: //Calibrated on Eagle's iPhone 6; consistent with teardwon
            dc->Fx = 548./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .10;
            dc->K[1] = -.15;
            dc->Tc[0] = 0.015;
            dc->Tc[1] = 0.065;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_IPHONE6PLUS: //Based on 6, but calibrated with real iPhone 6 plus
            dc->Fx = 548./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .10;
            dc->K[1] = -.15;
            dc->Tc[0] = 0.008;
            dc->Tc[1] = 0.075;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_IPOD5: //Tc is reasonably consistent with teardown
            dc->Fx = 591./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .18;
            dc->K[1] = -.37;
            dc->Tc[0] = 0.043;
            dc->Tc[1] = 0.020;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_IPAD2: //Tc from sequence appears consistent with teardown, except x offset seems a bit large
            dc->Fx = 782./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .03;
            dc->K[1] = -.21;
            dc->Tc[0] = -0.030;
            dc->Tc[1] = 0.118;
            dc->Tc[2] = 0.;
            return true;
        case DEVICE_TYPE_IPAD3: //Tc from sequence seems stable, but can't see on teardown. y offset seems hard to estimate, not sure if it's right
            dc->Fx = 627./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .17;
            dc->K[1] = -.38;
            dc->Tc[0] = 0.064;
            dc->Tc[1] = -0.017;
            dc->Tc[2] = 0.;
            return true;
        case DEVICE_TYPE_IPAD4: //Tc from sequence seems stable, but can't see on teardown.
            dc->Fx = 594./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .17;
            dc->K[1] = -.47;
            dc->Tc[0] = .010;
            dc->Tc[1] = .050;
            dc->Tc[2] = 0.;
            return true;
        case DEVICE_TYPE_IPADAIR: //Tc from sequence appears consistent with teardown
            dc->Fx = 582./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .12;
            dc->K[1] = -.25;
            dc->Tc[0] = -.012;
            dc->Tc[1] = .065;
            dc->Tc[2] = .000;
            return true;
        case DEVICE_TYPE_IPADAIR2: //Calibrated from device; very similar to old
            dc->Fx = 573./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .13;
            dc->K[1] = -.26;
            dc->Tc[0] = -.003;
            dc->Tc[1] = .068;
            dc->Tc[2] = .000;
            return true;
        case DEVICE_TYPE_IPADMINI: //Tc from sequence is consistent with teardown
            dc->Fx = 583./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .13;
            dc->K[1] = -.21;
            dc->Tc[0] = -0.014;
            dc->Tc[1] = 0.074;
            dc->Tc[2] = 0.;
            return true;
        case DEVICE_TYPE_IPADMINIRETINA: //Tc from sequence is consistent with teardown
            dc->Fx = 580./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .14;
            dc->K[1] = -.33;
            dc->Tc[0] = -0.003;
            dc->Tc[1] = 0.070;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_IPADMINIRETINA2: //Just copied from old
            dc->Fx = 580./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .14;
            dc->K[1] = -.33;
            dc->Tc[0] = -0.003;
            dc->Tc[1] = 0.070;
            dc->Tc[2] = 0.000;
            return true;
        case DEVICE_TYPE_UNKNOWN:
        default:
            dc->Fx = 600./640 * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .20;
            dc->K[1] = -.20;
            dc->Tc[0] = 0.01;
            dc->Tc[1] = 0.03;
            dc->Tc[2] = 0.0;
            return false;
    }
}


