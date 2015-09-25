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
