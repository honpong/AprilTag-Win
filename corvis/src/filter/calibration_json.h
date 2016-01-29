#pragma once
#include <string>
#include "rc_intel_interface.h"
#include "SP_Calibration.h"

bool calibration_serialize(const rcCalibration &cal, std::string &jsonString);
bool calibration_deserialize(const std::string &jsonString, rcCalibration &cal, const rcCalibration *defaults);
