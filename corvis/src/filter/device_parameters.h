#ifndef DEVICE_PARAMETERS_H
#define DEVICE_PARAMETERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

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

    DEVICE_TYPE_IPADMINI,
    DEVICE_TYPE_IPADMINIRETINA,
} corvis_device_type;

bool get_parameters_for_device(corvis_device_type type, struct corvis_device_parameters *dc);

#ifdef __cplusplus
}
#endif

#endif
