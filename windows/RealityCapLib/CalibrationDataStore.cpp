#include "stdafx.h"
#include "CalibrationDataStore.h"

using namespace RealityCap;

CalibrationDataStore::CalibrationDataStore()
{
}


CalibrationDataStore::~CalibrationDataStore()
{
}

corvis_device_parameters RealityCap::CalibrationDataStore::GetSavedCalibration()
{
    return corvis_device_parameters();
}

void CalibrationDataStore::SaveCalibration(corvis_device_parameters calibration)
{

}

void RealityCap::CalibrationDataStore::ClearCalibration()
{
}

bool RealityCap::CalibrationDataStore::HasCalibration()
{
    return false;
}
