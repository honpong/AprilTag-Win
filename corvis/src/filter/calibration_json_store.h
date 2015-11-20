#pragma once
#include <string>
#include "device_parameters.h"

bool calibration_serialize(const device_parameters &cal, std::string &jsonString);
bool calibration_deserialize(const std::string &jsonString, device_parameters &cal);
bool calibration_load_defaults(const corvis_device_type deviceType, device_parameters &cal);
