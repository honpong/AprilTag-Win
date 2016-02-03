#ifndef __CALIBRATION_DEFAULTS_H__
#define __CALIBRATION_DEFAULTS_H__

#include "device_parameters.h"

const char *calibration_default_json_for_device_type(corvis_device_type device);
bool calibration_load_defaults(const corvis_device_type deviceType, device_parameters &cal);

#endif
