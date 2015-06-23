#pragma once
#include <string>
#include "device_parameters.h"

bool calibration_serialize(const corvis_device_parameters &cal, std::string &jsonString);
bool calibration_deserialize(const std::string &jsonString, corvis_device_parameters &cal);
bool calibration_load_defaults(const corvis_device_type deviceType, corvis_device_parameters &cal);
