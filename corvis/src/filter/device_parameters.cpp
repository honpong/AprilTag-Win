//
//  device_parameters.c
//  RC3DK
//
//  Created by Eagle Jones on 4/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "device_parameters.h"
#include <math.h>
#include <string>
#include <iostream>

corvis_device_type get_device_by_name(const char *name)
{
    if(!name) return DEVICE_TYPE_UNKNOWN;

    std::string device_name(name);
    
    /*
     ******** IMPORTANT! *************
     Longer variants (iphone5s, iphone5c) have to come before the shorter variants (iphone5), since we only match the start of the name
     */
    if(device_name.find("ipodtouch") == 0) return DEVICE_TYPE_IPOD5;
    if(device_name.find("ipod5") == 0) return DEVICE_TYPE_IPOD5;

    if(device_name.find("iphone4s") == 0) return DEVICE_TYPE_IPHONE4S;
    if(device_name.find("iphone5c") == 0) return DEVICE_TYPE_IPHONE5C;
    if(device_name.find("iphone5s") == 0) return DEVICE_TYPE_IPHONE5S;
    if(device_name.find("iphone5") == 0) return DEVICE_TYPE_IPHONE5;
    if(device_name.find("iphone6plus") == 0) return DEVICE_TYPE_IPHONE6PLUS;
    if(device_name.find("iphone6") == 0) return DEVICE_TYPE_IPHONE6;

    if(device_name.find("ipad2") == 0) return DEVICE_TYPE_IPAD2;
    if(device_name.find("ipad3") == 0) return DEVICE_TYPE_IPAD3;
    if(device_name.find("ipad4") == 0) return DEVICE_TYPE_IPAD4;

    if(device_name.find("ipadair2") == 0) return DEVICE_TYPE_IPADAIR2;
    if(device_name.find("ipadair") == 0) return DEVICE_TYPE_IPADAIR;
    if(device_name.find("ipadminiretina2") == 0) return DEVICE_TYPE_IPADMINIRETINA2;
    if(device_name.find("ipadminiretina") == 0) return DEVICE_TYPE_IPADMINIRETINA;
    if(device_name.find("ipadmini") == 0) return DEVICE_TYPE_IPADMINI;

    return DEVICE_TYPE_UNKNOWN;
}

void device_set_resolution(struct corvis_device_parameters *dc, int image_width, int image_height)
{
    int max_old_dim = dc->image_width > dc->image_height ? dc->image_width : dc->image_height;
    int max_new_dim = image_width > image_height ? image_width : image_height;

    dc->image_width = image_width;
    dc->image_height = image_height;
    dc->Cx = (dc->image_width - 1)/2.f;
    dc->Cy = (dc->image_height - 1)/2.f;
    // Scale the focal length depending on the resolution
    dc->Fx = dc->Fx * max_new_dim / max_old_dim;
    dc->Fy = dc->Fx;
}

void device_set_framerate(struct corvis_device_parameters *dc, float framerate_hz)
{
    dc->shutter_delay = sensor_clock::duration(0);
    dc->shutter_period = std::chrono::duration_cast<sensor_clock::duration>(std::chrono::duration<float>(1.f/framerate_hz));
}

bool get_parameters_for_device(corvis_device_type type, struct corvis_device_parameters *dc)
{
    dc->px = 0.;
    dc->py = 0.;
    dc->K[2] = 0.;
    dc->Wc[0] = (float) (sqrt(2.)/2. * M_PI);
    dc->Wc[1] = (float) (-sqrt(2.)/2. * M_PI);
    dc->Wc[2] = 0.;
    double a_bias_stdev = .02 * 9.8 / 2.; //20 mg "typical", assuming that means two-sigma
    double w_bias_stdev = 10. / 180. * M_PI / 2.; //10 dps typical according to specs, but not in practice - factory or apple calibration? Found some devices with much larger range
    float w_stdev = (float) (.03 * sqrt(50.) / 180. * M_PI); //.03 dps / sqrt(hz) at 50 hz
    float a_stdev = (float) (.000218 * sqrt(50.) * 9.8); //218 ug / sqrt(hz) at 50 hz
    dc->w_meas_var = w_stdev * w_stdev;
    dc->a_meas_var = a_stdev * a_stdev;
    dc->image_width = 640;
    dc->image_height = 480;
    dc->Cx = (dc->image_width - 1)/2.f;
    dc->Cy = (dc->image_height - 1)/2.f;
    int max_dim = dc->image_width > dc->image_height ? dc->image_width : dc->image_height;
    device_set_framerate(dc, 30);


    for(int i = 0; i < 3; ++i) {
        dc->a_bias[i] = 0.;
        dc->w_bias[i] = 0.;
        dc->a_bias_var[i] = (float) (a_bias_stdev * a_bias_stdev);
        dc->w_bias_var[i] = (float) (w_bias_stdev * w_bias_stdev);
        dc->Tc_var[i] = 1.e-6f;
        dc->Wc_var[i] = 1.e-6f;
    }
    dc->Tc_var[2] = 1.e-10f; //Letting this float gives ambiguity with focal length

    switch(type)
    {
        case DEVICE_TYPE_IPHONE4S: //Tc from new sequence appears reasonably consistent with teardown
            dc->Fx = 606.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .21f;
            dc->K[1] = -.45f;
            dc->Tc[0] = 0.010f;
            dc->Tc[1] = 0.006f;
            dc->Tc[2] = 0.000f;
            return true;
        case DEVICE_TYPE_IPHONE5: //Tc from new sequence is consistent with teardown
            dc->Fx = 596.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .12f;
            dc->K[1] = -.20f;
            dc->Tc[0] = 0.010f;
            dc->Tc[1] = 0.008f;
            dc->Tc[2] = 0.000f;
            return true;
        case DEVICE_TYPE_IPHONE5C: //Guess from teardown - Tc is different from iphone 5
            dc->Fx = 596.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .12f;
            dc->K[1] = -.20f;
            dc->Tc[0] = 0.005f;
            dc->Tc[1] = 0.025f;
            dc->Tc[2] = 0.000f;
            return true;
        case DEVICE_TYPE_IPHONE5S: //Tc from sequence appears consistent with teardown; static calibration for my 5s is not right for older sequences
            //iphone5s_sam is a bit different, but within range
            dc->Fx = 547.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .09f;
            dc->K[1] = -.15f;
            dc->Tc[0] = 0.015f;
            dc->Tc[1] = 0.050f;
            dc->Tc[2] = 0.000f;
            return true;
        case DEVICE_TYPE_IPHONE6: //Calibrated on Eagle's iPhone 6; consistent with teardwon
            dc->Fx = 548.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .10f;
            dc->K[1] = -.15f;
            dc->Tc[0] = 0.015f;
            dc->Tc[1] = 0.065f;
            dc->Tc[2] = 0.000f;
            return true;
        case DEVICE_TYPE_IPHONE6PLUS: //Based on 6, but calibrated with real iPhone 6 plus
            dc->Fx = 548.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .10f;
            dc->K[1] = -.15f;
            dc->Tc[0] = 0.008f;
            dc->Tc[1] = 0.075f;
            dc->Tc[2] = 0.000f;
            return true;
        case DEVICE_TYPE_IPOD5: //Tc is reasonably consistent with teardown
            dc->Fx = 591.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .18f;
            dc->K[1] = -.37f;
            dc->Tc[0] = 0.043f;
            dc->Tc[1] = 0.020f;
            dc->Tc[2] = 0.000f;
            return true;
        case DEVICE_TYPE_IPAD2: //Tc from sequence appears consistent with teardown, except x offset seems a bit large
            dc->Fx = 782.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .03f;
            dc->K[1] = -.21f;
            dc->Tc[0] = -0.030f;
            dc->Tc[1] = 0.118f;
            dc->Tc[2] = 0.f;
            return true;
        case DEVICE_TYPE_IPAD3: //Tc from sequence seems stable, but can't see on teardown. y offset seems hard to estimate, not sure if it's right
            dc->Fx = 627.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .17f;
            dc->K[1] = -.38f;
            dc->Tc[0] = 0.064f;
            dc->Tc[1] = -0.017f;
            dc->Tc[2] = 0.f;
            return true;
        case DEVICE_TYPE_IPAD4: //Tc from sequence seems stable, but can't see on teardown.
            dc->Fx = 594.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .17f;
            dc->K[1] = -.47f;
            dc->Tc[0] = .010f;
            dc->Tc[1] = .050f;
            dc->Tc[2] = 0.f;
            return true;
        case DEVICE_TYPE_IPADAIR: //Tc from sequence appears consistent with teardown
            dc->Fx = 582.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .12f;
            dc->K[1] = -.25f;
            dc->Tc[0] = -.012f;
            dc->Tc[1] = .065f;
            dc->Tc[2] = .000f;
            return true;
        case DEVICE_TYPE_IPADAIR2: //Calibrated from device; very similar to old
            dc->Fx = 573.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .13f;
            dc->K[1] = -.26f;
            dc->Tc[0] = -.003f;
            dc->Tc[1] = .068f;
            dc->Tc[2] = .000f;
            return true;
        case DEVICE_TYPE_IPADMINI: //Tc from sequence is consistent with teardown
            dc->Fx = 583.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .13f;
            dc->K[1] = -.21f;
            dc->Tc[0] = -0.014f;
            dc->Tc[1] = 0.074f;
            dc->Tc[2] = 0.f;
            return true;
        case DEVICE_TYPE_IPADMINIRETINA: //Tc from sequence is consistent with teardown
            dc->Fx = 580.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .14f;
            dc->K[1] = -.33f;
            dc->Tc[0] = -0.003f;
            dc->Tc[1] = 0.070f;
            dc->Tc[2] = 0.000f;
            return true;
        case DEVICE_TYPE_IPADMINIRETINA2: //Just copied from old
            dc->Fx = 580.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .14f;
            dc->K[1] = -.33f;
            dc->Tc[0] = -0.003f;
            dc->Tc[1] = 0.070f;
            dc->Tc[2] = 0.000f;
            return true;
        case DEVICE_TYPE_UNKNOWN:
        default:
            dc->Fx = 600.f/640.f * max_dim;
            dc->Fy = dc->Fx;
            dc->K[0] = .20f;
            dc->K[1] = -.20f;
            dc->Tc[0] = 0.01f;
            dc->Tc[1] = 0.03f;
            dc->Tc[2] = 0.0f;
            return false;
    }
}

void set_initialized(struct corvis_device_parameters *dc)
{
    for(int i=0; i < 3; i++) {
        dc->a_bias_var[i] = 6.4e-3f;
        dc->w_bias_var[i] = 1.e-4f; //5.e-5;
    }
}

bool get_parameters_for_device_name(const char * config_name, struct corvis_device_parameters *dc)
{
    corvis_device_type device_type = get_device_by_name(config_name);
    if(!get_parameters_for_device(device_type, dc))
        return false;

    std::string device_name(config_name);

    if(device_name == "iphone4s_camelia") {
        dc->a_bias[0] = -0.1241252f;
        dc->a_bias[1] = 0.01688302f;
        dc->a_bias[2] = 0.3423786f;
        dc->w_bias[0] = -0.03693274f;
        dc->w_bias[1] = 0.004188713f;
        dc->w_bias[2] = -0.02701478f;
        set_initialized(dc);
    }

    else if(device_name == "iphone4s_eagle") {
        dc->a_bias[0] = -0.169f;
        dc->a_bias[1] = -0.072f;
        dc->a_bias[2] = 0.064f;
        dc->w_bias[0] = -0.018f;
        dc->w_bias[1] = 0.009f;
        dc->w_bias[2] = -0.018f;
        set_initialized(dc);
    }

    else if(device_name == "iphone5_jordan") {
        dc->a_bias[0] = -0.00145f;
        dc->a_bias[1] = -0.143f;
        dc->a_bias[2] = 0.027f;
        dc->w_bias[0] = 0.0036f;
        dc->w_bias[1] = 0.0207f;
        dc->w_bias[2] = -0.0437f;
        set_initialized(dc);
    }

    else if(device_name == "iphone5_sam") {
        dc->a_bias[0] = -0.127f;
        dc->a_bias[1] = -0.139f;
        dc->a_bias[2] = -0.003f;
        dc->w_bias[0] = 0.005f;
        dc->w_bias[1] = 0.007f;
        dc->w_bias[2] = -0.003f;
        //set_initialized(dc);
    }

    else if(device_name == "iphone5s_eagle") {
        dc->a_bias[0] = 0.036f;
        dc->a_bias[1] = -0.012f;
        dc->a_bias[2] = -0.096f;
        dc->w_bias[0] = 0.017f;
        dc->w_bias[1] = 0.004f;
        dc->w_bias[2] = -0.010f;
        set_initialized(dc);
    }

    else if(device_name == "iphone5s_sam") {
        dc->a_bias[0] = 0.047f;
        dc->a_bias[1] = 0.028f;
        dc->a_bias[2] = -0.027f;
        dc->w_bias[0] = 0.058f;
        dc->w_bias[1] = -0.029f;
        dc->w_bias[2] = -0.002f;
        set_initialized(dc);
    }

    else if(device_name == "iphone5s_brian") {
        dc->a_bias[0] = -0.036f;
        dc->a_bias[1] = 0.015f;
        dc->a_bias[2] = -0.07f;
        dc->w_bias[0] = 0.03f;
        dc->w_bias[1] = 0.008f;
        dc->w_bias[2] = -0.017f;
        set_initialized(dc);
    }

    else if(device_name == "ipad2_ben") {
        return true;
        dc->a_bias[0] = -0.114f;
        dc->a_bias[1] = -0.171f;
        dc->a_bias[2] = 0.226f;
        dc->w_bias[0] = 0.007f;
        dc->w_bias[1] = 0.011f;
        dc->w_bias[2] = 0.010f;
        set_initialized(dc);
    }

    else if(device_name == "ipad2_brian") {
        dc->a_bias[0] = 0.1417f;
        dc->a_bias[1] = -0.0874f;
        dc->a_bias[2] = 0.2256f;
        dc->w_bias[0] = -0.0014f;
        dc->w_bias[1] = 0.0055f;
        dc->w_bias[2] = -0.0076f;
        set_initialized(dc);
    }

    else if(device_name == "ipad3_eagle") {
        dc->a_bias[0] = 0.001f;
        dc->a_bias[1] = -0.225f;
        dc->a_bias[2] = -0.306f;
        dc->w_bias[0] = 0.016f;
        dc->w_bias[1] = -0.015f;
        dc->w_bias[2] = 0.011f;
        set_initialized(dc);
    }

    else if(device_name == "ipad4_jordan") {
        dc->a_bias[0] = 0.089f;
        dc->a_bias[1] = -0.176f;
        dc->a_bias[2] = -0.129f;
        dc->w_bias[0] = -0.023f;
        dc->w_bias[1] = -0.003f;
        dc->w_bias[2] = 0.015f;
        set_initialized(dc);
    }

    else if(device_name == "ipadair_eagle") {
        dc->a_bias[0] = 0.002f;
        dc->a_bias[1] = -0.0013f;
        dc->a_bias[2] = 0.07f;
        dc->w_bias[0] = 0.012f;
        dc->w_bias[1] = 0.017f;
        dc->w_bias[2] = 0.004f;
        set_initialized(dc);
    }

    else if(device_name == "ipadmini_ben") {
        return true;
        dc->a_bias[0] = 0.075f;
        dc->a_bias[1] = 0.07f;
        dc->a_bias[2] = 0.015f;
        dc->w_bias[0] = 0.004f;
        dc->w_bias[1] = -0.044f;
        dc->w_bias[2] = .0088f;
        set_initialized(dc);
    }

    else if(device_name == "ipadminiretina_ben") {
        dc->a_bias[0] = 0.108f;
        dc->a_bias[1] = -.082f;
        dc->a_bias[2] = 0.006f;
        dc->w_bias[0] = 0.013f;
        dc->w_bias[1] = -0.015f;
        dc->w_bias[2] = -0.026f;
        set_initialized(dc);
    }

    else if(device_name == "ipodtouch_ben") {
        dc->a_bias[0] = -0.092f;
        dc->a_bias[1] = 0.053f;
        dc->a_bias[2] = 0.104f;
        dc->w_bias[0] = 0.021f;
        dc->w_bias[1] = 0.021f;
        dc->w_bias[2] = -0.013f;
        set_initialized(dc);
    }

    /* These never get used because we don't get an initial
     * device_type
    else if(device_name == "ipad3-front") {
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

    else if(device_name == "nexus7-front") {
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

    else if(device_name == "simulator") {
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

    else if(device_name == "intel") {
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

    else if(device_name == "intel-blank") {
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


    else if(device_name == "intel-old") {
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
