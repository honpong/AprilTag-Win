#ifndef DEVICE_PARAMETERS_H
#define DEVICE_PARAMETERS_H

#include <stdbool.h>
#include "../cor/platform/sensor_clock.h"

#define CALIBRATION_VERSION 7

#define KEY_FX "Fx"
#define KEY_FY "Fy"
#define KEY_CX "Cx"
#define KEY_CY "Cy"
#define KEY_PX "px"
#define KEY_PY "py"
#define KEY_K0 "K0"
#define KEY_K1 "K1"
#define KEY_K2 "K2"
#define KEY_ABIAS0 "abias0"
#define KEY_ABIAS1 "abias1"
#define KEY_ABIAS2 "abias2"
#define KEY_WBIAS0 "wbias0"
#define KEY_WBIAS1 "wbias1"
#define KEY_WBIAS2 "wbias2"
#define KEY_TC0 "Tc0"
#define KEY_TC1 "Tc1"
#define KEY_TC2 "Tc2"
#define KEY_WC0 "Wc0"
#define KEY_WC1 "Wc1"
#define KEY_WC2 "Wc2"
#define KEY_ABIASVAR0 "abiasvar0"
#define KEY_ABIASVAR1 "abiasvar1"
#define KEY_ABIASVAR2 "abiasvar2"
#define KEY_WBIASVAR0 "wbiasvar0"
#define KEY_WBIASVAR1 "wbiasvar1"
#define KEY_WBIASVAR2 "wbiasvar2"
#define KEY_TCVAR0 "TcVar0"
#define KEY_TCVAR1 "TcVar1"
#define KEY_TCVAR2 "TcVar2"
#define KEY_WCVAR0 "WcVar0"
#define KEY_WCVAR1 "WcVar1"
#define KEY_WCVAR2 "WcVar2"
#define KEY_WMEASVAR "wMeasVar"
#define KEY_AMEASVAR "aMeasVar"
#define KEY_IMAGE_WIDTH "imageWidth"
#define KEY_IMAGE_HEIGHT "imageHeight"
#define KEY_SHUTTER_DELAY "shutterDelay"
#define KEY_SHUTTER_PERIOD "shutterPeriod"
#define KEY_CALIBRATION_VERSION "calibrationVersion"

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
    sensor_clock::duration shutter_delay, shutter_period;
    unsigned long int version;
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
    DEVICE_TYPE_IPADAIR2,

    DEVICE_TYPE_IPADMINI,
    DEVICE_TYPE_IPADMINIRETINA,
    DEVICE_TYPE_IPADMINIRETINA2,

    DEVICE_TYPE_GIGABYTE_S11
} corvis_device_type;

corvis_device_type get_device_by_name(const char *name);
bool get_parameters_for_device(corvis_device_type type, struct corvis_device_parameters *dc);
bool get_parameters_for_device_name(const char * name, struct corvis_device_parameters *dc);

bool is_calibration_valid(const corvis_device_parameters &calibration, const corvis_device_parameters &deviceDefaults);

void device_set_resolution(struct corvis_device_parameters *dc, int image_width, int image_height);
void device_set_framerate(struct corvis_device_parameters *dc, float framerate_hz);

#endif
