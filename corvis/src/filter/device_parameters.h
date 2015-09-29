#ifndef DEVICE_PARAMETERS_H
#define DEVICE_PARAMETERS_H

#include <stdbool.h>
#include "../cor/platform/sensor_clock.h"
#include <unordered_map>

using namespace std;

#define CALIBRATION_VERSION 7

//For device_parameters, but this should really be factored into a shared header if used directly there.
#include "rc_intel_interface.h"

typedef rc_DeviceParameters device_parameters;

typedef enum
{
    DEVICE_TYPE_UNKNOWN,
    DEVICE_TYPE_IPHONE4S,
    DEVICE_TYPE_IPHONE5,
    DEVICE_TYPE_IPHONE5C,
    DEVICE_TYPE_IPHONE5S,
    DEVICE_TYPE_IPHONE6,
    DEVICE_TYPE_IPHONE6PLUS,

    DEVICE_TYPE_IPOD5,
    
    DEVICE_TYPE_IPAD2,
    DEVICE_TYPE_IPAD3,
    DEVICE_TYPE_IPAD4,
    DEVICE_TYPE_IPADAIR,
    DEVICE_TYPE_IPADAIR2,

    DEVICE_TYPE_IPADMINI,
    DEVICE_TYPE_IPADMINIRETINA,
    DEVICE_TYPE_IPADMINIRETINA2,

    DEVICE_TYPE_GIGABYTES11
} corvis_device_type;

corvis_device_type get_device_by_name(const char *name);
bool get_parameters_for_device(corvis_device_type type, device_parameters *dc);

bool is_calibration_valid(const device_parameters &calibration, const device_parameters &deviceDefaults);

void device_set_resolution(device_parameters *dc, int image_width, int image_height);

void get_device_type_string_map(unordered_map<corvis_device_type, string> &map);
string get_device_type_string(corvis_device_type type);

#endif
