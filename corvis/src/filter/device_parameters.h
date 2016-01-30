#ifndef DEVICE_PARAMETERS_H
#define DEVICE_PARAMETERS_H

#include <stdbool.h>

#define CALIBRATION_VERSION 8

#include "SP_Calibration.h"

typedef SP_Calibration device_parameters;

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

bool get_parameters_for_device(corvis_device_type type, device_parameters *dc);
void device_set_resolution(device_parameters *dc, int image_width, int image_height);

#endif
