//
//  device_parameters.c
//  RC3DK
//
//  Created by Eagle Jones on 4/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "device_parameters.h"
#include <math.h>
#include <string.h>

corvis_device_type get_device_by_name(const char *name)
{
    if(!name) return DEVICE_TYPE_UNKNOWN;
    
    /*
     ******** IMPORTANT! *************
     Longer variants (iphone5s, iphone5c) have to come before the shorter variants (iphone5), since we only match the start of the name
     */
    if(!strncmp(name, "ipodtouch", 9)) return DEVICE_TYPE_IPOD5;
    if(!strncmp(name, "ipod5", 5)) return DEVICE_TYPE_IPOD5;

    if(!strncmp(name, "iphone4s", 8)) return DEVICE_TYPE_IPHONE4S;
    if(!strncmp(name, "iphone5c", 8)) return DEVICE_TYPE_IPHONE5C;
    if(!strncmp(name, "iphone5s", 8)) return DEVICE_TYPE_IPHONE5S;
    if(!strncmp(name, "iphone5", 7)) return DEVICE_TYPE_IPHONE5;
    if(!strncmp(name, "iphone6plus", 11)) return DEVICE_TYPE_IPHONE6PLUS;
    if(!strncmp(name, "iphone6", 7)) return DEVICE_TYPE_IPHONE6;
    
    if(!strncmp(name, "ipad2", 5)) return DEVICE_TYPE_IPAD2;
    if(!strncmp(name, "ipad3", 5)) return DEVICE_TYPE_IPAD3;
    if(!strncmp(name, "ipad4", 5)) return DEVICE_TYPE_IPAD4;
    
    if(!strncmp(name, "ipadair2", 8)) return DEVICE_TYPE_IPADAIR2;
    if(!strncmp(name, "ipadair", 7)) return DEVICE_TYPE_IPADAIR;
    if(!strncmp(name, "ipadminiretina2", 15)) return DEVICE_TYPE_IPADMINIRETINA2;
    if(!strncmp(name, "ipadminiretina", 14)) return DEVICE_TYPE_IPADMINIRETINA;
    if(!strncmp(name, "ipadmini", 8)) return DEVICE_TYPE_IPADMINI;

    return DEVICE_TYPE_UNKNOWN;
}

void device_set_resolution(struct corvis_device_parameters *dc, int image_width, int image_height)
{
    int max_old_dim = dc->image_width > dc->image_height ? dc->image_width : dc->image_height;
    int max_new_dim = image_width > image_height ? image_width : image_height;

    dc->image_width = image_width;
    dc->image_height = image_height;
    dc->Cx = (dc->image_width - 1)/2.;
    dc->Cy = (dc->image_height - 1)/2.;
    // Scale the focal length depending on the resolution
    dc->Fx = dc->Fx * max_new_dim / max_old_dim;
    dc->Fy = dc->Fx;
}

void device_set_framerate(struct corvis_device_parameters *dc, float framerate_hz)
{
    dc->shutter_delay = 0;
    dc->shutter_period = 1e6 * 1./framerate_hz;
}

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
    dc->Cx = (dc->image_width - 1)/2.;
    dc->Cy = (dc->image_height - 1)/2.;
    int max_dim = dc->image_width > dc->image_height ? dc->image_width : dc->image_height;
    device_set_framerate(dc, 30);


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

void set_initialized(struct corvis_device_parameters *dc)
{
    for(int i=0; i < 3; i++) {
        dc->a_bias_var[i] = 6.4e-3;
        dc->w_bias_var[i] = 1.e-4; //5.e-5;
    }
}

bool get_parameters_for_device_name(const char * config_name, struct corvis_device_parameters *dc)
{
    corvis_device_type device_type = get_device_by_name(config_name);
    if(!get_parameters_for_device(device_type, dc))
        return false;

    if(!strcmp(config_name, "iphone4s_camelia")) {
        dc->a_bias[0] = -0.1241252;
        dc->a_bias[1] = 0.01688302;
        dc->a_bias[2] = 0.3423786;
        dc->w_bias[0] = -0.03693274;
        dc->w_bias[1] = 0.004188713;
        dc->w_bias[2] = -0.02701478;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "iphone4s_eagle")) {
        dc->a_bias[0] = -0.169;
        dc->a_bias[1] = -0.072;
        dc->a_bias[2] = 0.064;
        dc->w_bias[0] = -0.018;
        dc->w_bias[1] = 0.009;
        dc->w_bias[2] = -0.018;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "iphone5_jordan")) {
        dc->a_bias[0] = -0.00145;
        dc->a_bias[1] = -0.143;
        dc->a_bias[2] = 0.027;
        dc->w_bias[0] = 0.0036;
        dc->w_bias[1] = 0.0207;
        dc->w_bias[2] = -0.0437;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "iphone5_sam")) {
        dc->a_bias[0] = -0.127;
        dc->a_bias[1] = -0.139;
        dc->a_bias[2] = -0.003;
        dc->w_bias[0] = 0.005;
        dc->w_bias[1] = 0.007;
        dc->w_bias[2] = -0.003;
        //set_initialized(dc);
    }

    else if(!strcmp(config_name, "iphone5s_eagle")) {
        dc->a_bias[0] = 0.036;
        dc->a_bias[1] = -0.012;
        dc->a_bias[2] = -0.096;
        dc->w_bias[0] = 0.017;
        dc->w_bias[1] = 0.004;
        dc->w_bias[2] = -0.010;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "iphone5s_sam")) {
        dc->a_bias[0] = 0.047;
        dc->a_bias[1] = 0.028;
        dc->a_bias[2] = -0.027;
        dc->w_bias[0] = 0.058;
        dc->w_bias[1] = -0.029;
        dc->w_bias[2] = -0.002;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "iphone5s_brian")) {
        dc->a_bias[0] = -0.036;
        dc->a_bias[1] = 0.015;
        dc->a_bias[2] = -0.07;
        dc->w_bias[0] = 0.03;
        dc->w_bias[1] = 0.008;
        dc->w_bias[2] = -0.017;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "ipad2_ben")) {
        return true;
        dc->a_bias[0] = -0.114;
        dc->a_bias[1] = -0.171;
        dc->a_bias[2] = 0.226;
        dc->w_bias[0] = 0.007;
        dc->w_bias[1] = 0.011;
        dc->w_bias[2] = 0.010;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "ipad2_brian")) {
        dc->a_bias[0] = 0.1417;
        dc->a_bias[1] = -0.0874;
        dc->a_bias[2] = 0.2256;
        dc->w_bias[0] = -0.0014;
        dc->w_bias[1] = 0.0055;
        dc->w_bias[2] = -0.0076;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "ipad3_eagle")) {
        dc->a_bias[0] = 0.001;
        dc->a_bias[1] = -0.225;
        dc->a_bias[2] = -0.306;
        dc->w_bias[0] = 0.016;
        dc->w_bias[1] = -0.015;
        dc->w_bias[2] = 0.011;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "ipad4_jordan")) {
        dc->a_bias[0] = 0.089;
        dc->a_bias[1] = -0.176;
        dc->a_bias[2] = -0.129;
        dc->w_bias[0] = -0.023;
        dc->w_bias[1] = -0.003;
        dc->w_bias[2] = 0.015;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "ipadair_eagle")) {
        dc->a_bias[0] = 0.002;
        dc->a_bias[1] = -0.0013;
        dc->a_bias[2] = 0.07;
        dc->w_bias[0] = 0.012;
        dc->w_bias[1] = 0.017;
        dc->w_bias[2] = 0.004;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "ipadmini_ben")) {
        return true;
        dc->a_bias[0] = 0.075;
        dc->a_bias[1] = 0.07;
        dc->a_bias[2] = 0.015;
        dc->w_bias[0] = 0.004;
        dc->w_bias[1] = -0.044;
        dc->w_bias[2] = .0088;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "ipadminiretina_ben")) {
        dc->a_bias[0] = 0.108;
        dc->a_bias[1] = -.082;
        dc->a_bias[2] = 0.006;
        dc->w_bias[0] = 0.013;
        dc->w_bias[1] = -0.015;
        dc->w_bias[2] = -0.026;
        set_initialized(dc);
    }

    else if(!strcmp(config_name, "ipodtouch_ben")) {
        dc->a_bias[0] = -0.092;
        dc->a_bias[1] = 0.053;
        dc->a_bias[2] = 0.104;
        dc->w_bias[0] = 0.021;
        dc->w_bias[1] = 0.021;
        dc->w_bias[2] = -0.013;
        set_initialized(dc);
    }

    /* These never get used because we don't get an initial
     * device_type
    else if(!strcmp(config_name, "ipad3-front")) {
        #parameters for generic ipad 3 - front cam
        dc->Fx = 604.;
        dc->Fy = 604.;
        dc->Cx = 319.5;
        dc->Cy = 239.5;
        dc->px = 0.;
        dc->py = 0.;
        dc->K[0] = 0.; #.2774956;
        dc->K[1] = 0.; #-1.0795446;
        dc->K[2] = 0.; #1.14524733;
        dc->a_bias[0] = 0.;
        dc->a_bias[1] = 0.;
        dc->a_bias[2] = 0.;
        dc->w_bias[0] = 0.;
        dc->w_bias[1] = 0.;
        dc->w_bias[2] = 0.;
        dc->Tc[0] = 0.#-.0940;
        dc->Tc[1] = 0.#.0396;
        dc->Tc[2] = 0.#.0115;
        dc->Wc[0] = 0.;
        dc->Wc[1] = 0.;
        dc->Wc[2] = -pi/2.;
        #dc->a_bias_var[0] = 1.e-4;
        #dc->a_bias_var[1] = 1.e-4;
        #dc->a_bias_var[2] = 1.e-4;
        #dc->w_bias_var[0] = 1.e-7;
        #dc->w_bias_var[1] = 1.e-7;
        #dc->w_bias_var[2] = 1.e-7;
        a_bias_stdev = .02 * 9.8; #20 mg
        dc->a_bias_var = a_bias_stdev * a_bias_stdev;
        w_bias_stdev = 10. / 180. * pi; #10 dps
        dc->w_bias_var = w_bias_stdev * w_bias_stdev;
        dc->Tc_var[0] = 1.e-6;
        dc->Tc_var[1] = 1.e-6;
        dc->Tc_var[2] = 1.e-6;
        dc->Wc_var[0] = 1.e-7;
        dc->Wc_var[1] = 1.e-7;
        dc->Wc_var[2] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc->w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; #218 ug / sqrt(hz) at 50 hz
        dc->a_meas_var = a_stdev * a_stdev;
        dc->image_width = 640;
        dc->image_height = 480;
        dc->shutter_delay = 0;
        dc->shutter_period = 31000;

    else if(!strcmp(config_name, "nexus7-front")) {
        #parameters for sam's nexus 7 - front cam
        dc->Fx = 604.;
        dc->Fy = 604.;
        dc->Cx = 319.5;
        dc->Cy = 239.5;
        dc->px = 0.;
        dc->py = 0.;
        dc->K[0] = 0.; #.2774956;
        dc->K[1] = 0.; #-1.0795446;
        dc->K[2] = 0.; #1.14524733;
        dc->a_bias[0] = 0.126;
        dc->a_bias[1] = 0.024;
        dc->a_bias[2] = 0.110;
        dc->w_bias[0] = 1.e-5;
        dc->w_bias[1] = -3.e-6;
        dc->w_bias[2] = -8.e-6;
        dc->Tc[0] = 0.#-.0940;
        dc->Tc[1] = 0.#.0396;
        dc->Tc[2] = 0.#.0115;
        dc->Wc[0] = 0.;
        dc->Wc[1] = 0.;
        dc->Wc[2] = -pi/2.;
        #dc->a_bias_var[0] = 1.e-4;
        #dc->a_bias_var[1] = 1.e-4;
        #dc->a_bias_var[2] = 1.e-4;
        #dc->w_bias_var[0] = 1.e-7;
        #dc->w_bias_var[1] = 1.e-7;
        #dc->w_bias_var[2] = 1.e-7;
        a_bias_stdev = .02 * 9.8; #20 mg
        dc->a_bias_var = a_bias_stdev * a_bias_stdev;
        w_bias_stdev = 10. / 180. * pi; #10 dps
        dc->w_bias_var = w_bias_stdev * w_bias_stdev;
        dc->Tc_var[0] = 1.e-6;
        dc->Tc_var[1] = 1.e-6;
        dc->Tc_var[2] = 1.e-6;
        dc->Wc_var[0] = 1.e-7;
        dc->Wc_var[1] = 1.e-7;
        dc->Wc_var[2] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc->w_meas_var = 1.e-3 * 1.e-3 #w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; #218 ug / sqrt(hz) at 50 hz
        dc->a_meas_var = 2.e-2 * 2.e-2 #a_stdev * a_stdev;
        dc->image_width = 640;
        dc->image_height = 480;
        dc->shutter_delay = 0;
        dc->shutter_period = 31000;

    else if(!strcmp(config_name, "simulator")) {
        dc->Fx = 585.;
        dc->Fy = 585.;
        dc->Cx = 319.5;
        dc->Cy = 239.5;
        dc->px = 0.;
        dc->py = 0.;
        dc->K[0] = .10;
        dc->K[1] = -.10;
        dc->K[2] = 0.;
        dc->Tc[0] = 0.000;
        dc->Tc[1] = 0.000;
        dc->Tc[2] = -0.008;
        dc->Wc[0] = sqrt(2.)/2. * pi;
        dc->Wc[1] = -sqrt(2.)/2. * pi;
        dc->Wc[2] = 0.;
        a_bias_stdev = .02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; #10 dps
        for i in range(3):
            dc->a_bias[i] = 0.;
            dc->w_bias[i] = 0.;
            dc->a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc->w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc->Tc_var[i] = 1.e-7;
            dc->Wc_var[i] = 1.e-7;
        w_stdev = .03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc->w_meas_var = w_stdev * w_stdev;
        a_stdev = .000218 * sqrt(50.) * 9.8; # 218 ug / sqrt(hz) at 50 hz
        dc->a_meas_var = a_stdev * a_stdev;
        dc->image_width = 640;
        dc->image_height = 480;
        dc->shutter_delay = 0;
        dc->shutter_period = 31000;

    else if(!strcmp(config_name, "intel")) {
        dc->Fx = 577.;
        dc->Fy = 577.;
        dc->Cx = 319.5;
        dc->Cy = 239.5;
        dc->px = 0.;
        dc->py = 0.;
        dc->K[0] = .14;
        dc->K[1] = -.27;
        dc->K[2] = 0.;
        dc->Tc[0] = 0.008;
        dc->Tc[1] = 0.002;
        dc->Tc[2] = 0.;
        dc->Wc[0] = sqrt(2.)/2. * pi;
        dc->Wc[1] = -sqrt(2.)/2. * pi;
        dc->Wc[2] = 0.;
        dc->a_bias[0] = 0.0; #0.10
        dc->a_bias[1] = .20; #-0.09
        dc->a_bias[2] = -.30;  #-.6
        dc->w_bias[0] = .021; #.034 #-.029
        dc->w_bias[1] = -.005; #-.001 #.012
        dc->w_bias[2] = .030; #.004 #.014
        a_bias_stdev = 0.02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc->a_bias_var[i] = 1.e-4 #a_bias_stdev * a_bias_stdev;
            dc->w_bias_var[i] = 1.e-6 #w_bias_stdev * w_bias_stdev;
            dc->Tc_var[i] = 1.e-6;
            dc->Wc_var[i] = 1.e-7;
        w_stdev = 3.e-2; #.03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc->w_meas_var = w_stdev * w_stdev;
        a_stdev = 3.e-2; #.000220 * sqrt(50.) * 9.8; # 220 ug / sqrt(hz) at 50 hz
        dc->a_meas_var = a_stdev * a_stdev;
        dc->image_width = 640;
        dc->image_height = 480;
        dc->shutter_delay = 0;
        dc->shutter_period = 31000;

    else if(!strcmp(config_name, "intel-blank")) {
        dc->Fx = 580.;
        dc->Fy = 580.;
        dc->Cx = 319.5;
        dc->Cy = 239.5;
        dc->px = 0.;
        dc->py = 0.;
        dc->K[0] = .16;
        dc->K[1] = -.29;
        dc->K[2] = 0.;
        dc->Tc[0] = 0.008;
        dc->Tc[1] = 0.002;
        dc->Tc[2] = 0.;
        dc->Wc[0] = sqrt(2.)/2. * pi;
        dc->Wc[1] = -sqrt(2.)/2. * pi;
        dc->Wc[2] = 0.;
        dc->a_bias[0] = 0;
        dc->a_bias[1] = 0;
        dc->a_bias[2] = 0;
        dc->w_bias[0] = 0;
        dc->w_bias[1] = 0;
        dc->w_bias[2] = 0;
        a_bias_stdev = 0.02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc->a_bias_var[i] = a_bias_stdev * a_bias_stdev;
            dc->w_bias_var[i] = w_bias_stdev * w_bias_stdev;
            dc->Tc_var[i] = 1.e-6;
            dc->Wc_var[i] = 1.e-7;
        w_stdev = 3.e-2; #.03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc->w_meas_var = w_stdev * w_stdev;
        a_stdev = 3.e-2; #.000220 * sqrt(50.) * 9.8; # 220 ug / sqrt(hz) at 50 hz
        dc->a_meas_var = a_stdev * a_stdev;
        dc->image_width = 640;
        dc->image_height = 480;
        dc->shutter_delay = 0;
        dc->shutter_period = 31000;


    else if(!strcmp(config_name, "intel-old")) {
        dc->Fx = 610.;
        dc->Fy = 610.;
        dc->Cx = 319.5;
        dc->Cy = 239.5;
        dc->px = 0.;
        dc->py = 0.;
        dc->K[0] = .20;
        dc->K[1] = -.55;
        dc->K[2] = 0.;
        dc->Tc[0] = 0.008;
        dc->Tc[1] = 0.002;
        dc->Tc[2] = 0.;
        dc->Wc[0] = sqrt(2.)/2. * pi;
        dc->Wc[1] = -sqrt(2.)/2. * pi;
        dc->Wc[2] = 0.;
        dc->a_bias[0] = 0.10;
        dc->a_bias[1] = -0.09;
        dc->a_bias[2] = -.6;
        dc->w_bias[0] = .034; #-.029
        dc->w_bias[1] = -.001; #.012
        dc->w_bias[2] = .004; #.014
        a_bias_stdev = 0.02 * 9.8; # 20 mg
        w_bias_stdev = 10. / 180. * pi; # 10 dps
        for i in range(3):
            dc->a_bias_var[i] = 1.e-4; #a_bias_stdev * a_bias_stdev;
            dc->w_bias_var[i] = 1.e-4; #w_bias_stdev * w_bias_stdev;
            dc->Tc_var[i] = 1.e-6;
            dc->Wc_var[i] = 1.e-7;
        w_stdev = 3.e-2; #.03 * sqrt(50.) / 180. * pi; #.03 dps / sqrt(hz) at 50 hz
        dc->w_meas_var = w_stdev * w_stdev;
        a_stdev = 3.e-2; #.000220 * sqrt(50.) * 9.8; # 220 ug / sqrt(hz) at 50 hz
        dc->a_meas_var = a_stdev * a_stdev;
        dc->image_width = 640;
        dc->image_height = 480;
        dc->shutter_delay = 0;
        dc->shutter_period = 31000;
        */

    return true;
}
