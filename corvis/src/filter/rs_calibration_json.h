#pragma once
#include <string>
#include "rc_intel_interface.h"

bool rs_calibration_serialize(const rcCalibration &cal, std::string &jsonString);
bool rs_calibration_deserialize(const std::string &jsonString, rcCalibration &cal);
