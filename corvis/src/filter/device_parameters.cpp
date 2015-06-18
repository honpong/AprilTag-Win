//
//  device_parameters.c
//  RC3DK
//
//  Created by Eagle Jones on 4/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "device_parameters.h"
#include <string>
#include <iostream>
#include "calibration_json_store.h"
#include <memory>

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void get_device_type_string_map(unordered_map<corvis_device_type, string> &map)
{
    if (map.size()) return;
    map[DEVICE_TYPE_IPOD5] = "ipodtouch";
    map[DEVICE_TYPE_IPHONE4S] = "iphone4s";
    map[DEVICE_TYPE_IPHONE5C] = "iphone5c";
    map[DEVICE_TYPE_IPHONE5S] = "iphone5s";
    map[DEVICE_TYPE_IPHONE5] = "iphone5";
    map[DEVICE_TYPE_IPHONE6PLUS] = "iphone6plus";
    map[DEVICE_TYPE_IPHONE6] = "iphone6";
    map[DEVICE_TYPE_IPAD2] = "ipad2";
    map[DEVICE_TYPE_IPAD3] = "ipad3";
    map[DEVICE_TYPE_IPAD4] = "ipad4";
    map[DEVICE_TYPE_IPADAIR2] = "ipadair2";
    map[DEVICE_TYPE_IPADAIR] = "ipadair";
    map[DEVICE_TYPE_IPADMINIRETINA2] = "ipadminiretina2";
    map[DEVICE_TYPE_IPADMINIRETINA] = "ipadminiretina";
    map[DEVICE_TYPE_IPADMINI] = "ipadmini";
    map[DEVICE_TYPE_GIGABYTES11] = "gigabytes11";
    map[DEVICE_TYPE_UNKNOWN] = "unknown";
}

string get_device_type_string(corvis_device_type type)
{
    unordered_map<corvis_device_type, string> map;
    get_device_type_string_map(map);

    if (map.find(type) != map.end())
        return map[type];
    else
        return NULL;
}

corvis_device_type get_device_by_name(const char *name)
{
    if(!name) return DEVICE_TYPE_UNKNOWN;

    std::string full_name(name), device_name = full_name.substr(0, full_name.find_first_of('_'));
    corvis_device_type type = DEVICE_TYPE_UNKNOWN;

    unordered_map<corvis_device_type, string> map;
    get_device_type_string_map(map);

    // search map values. slow, but this function will be called infrequently.
    for (unordered_map<corvis_device_type, string>::iterator i = map.begin(); i != map.end(); i++)
    {
        if (i->second.compare(device_name) == 0)
        {
            type = i->first;
            break;
        }
    }

    return type;
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

// TODO: should this go away?
bool get_parameters_for_device(corvis_device_type type, struct corvis_device_parameters *dc)
{
    return calibration_load_defaults(type, *dc);
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
    if (device_type == DEVICE_TYPE_UNKNOWN)
        return false;

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

    else if(device_name == "iphone6_jim") {
        dc->a_bias[0] = -0.01877559f;
        dc->a_bias[1] = -0.01222747f;
        dc->a_bias[2] = -0.007556485f;
        dc->w_bias[0] = -0.02167661f;
        dc->w_bias[1] = -0.03419807f;
        dc->w_bias[2] = -0.01573849f;
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

    else if(device_name == "gigabytes11_ben") {
        dc->Fx = 635.9521484375f;
        dc->Fy = 635.9521484375f;
        dc->Cx = 351.518310546875f;
        dc->Cy = 232.09027099609375f;
        dc->a_bias[0] = 0.63581299781799316f;
        dc->a_bias[1] = 0.36253389716148376f;
        dc->a_bias[2] = 1.1042510271072388f;
        dc->w_bias[0] = -0.018003974109888077f;
        dc->w_bias[1] = -0.0072470856830477715f;
        dc->w_bias[2] = 0.0027284857351332903f;
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

bool is_calibration_valid(const corvis_device_parameters &cal, const corvis_device_parameters &deviceDefaults)
{
    if (cal.version != CALIBRATION_VERSION) return false;

    //check if biases are within 5 sigma
    const float sigma = 5.;
    float a = cal.a_bias[0];
    if (a * a > sigma * sigma * deviceDefaults.a_bias_var[0]) return false;
    a = cal.a_bias[1];
    if (a * a > sigma * sigma * deviceDefaults.a_bias_var[1]) return false;
    a = cal.a_bias[2];
    if (a * a > sigma * sigma * deviceDefaults.a_bias_var[2]) return false;
    a = cal.w_bias[0];
    if (a * a > sigma * sigma * deviceDefaults.w_bias_var[0]) return false;
    a = cal.w_bias[1];
    if (a * a > sigma * sigma * deviceDefaults.w_bias_var[1]) return false;
    a = cal.w_bias[2];
    if (a * a > sigma * sigma * deviceDefaults.w_bias_var[2]) return false;

    return true;
}
