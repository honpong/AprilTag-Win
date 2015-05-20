#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "CalibrationManager.h"
#include "Debug.h"
#include <string>
#include "libpxcimu_internal.h"

using namespace RealityCap;
using namespace std;

CalibrationManager::CalibrationManager(PXCSenseManager* senseMan) : SensorManager(senseMan), _isCalibrating(false), _delegate(NULL)
{
}

bool CalibrationManager::StartCalibration()
{
    if (isCalibrating()) return true;
    if (!StartSensors()) return false;
    _isCalibrating = true;
    if (_delegate) _delegate->OnStatusUpdated(1);
    return isCalibrating();
}

void CalibrationManager::StopCalibration()
{
    if (!isCalibrating()) return;
    StopSensors();
    _isCalibrating = false;
}

bool CalibrationManager::isCalibrating()
{
    return _isCalibrating;
}

void CalibrationManager::SetDelegate(CalibrationManagerDelegate* del)
{
    _delegate = del;
}

void CalibrationManager::OnColorFrame(PXCImage* colorSample)
{
    if (!isCalibrating()) return;
    //SensorManager::OnColorFrame(colorSample); // causes debug logging

    /*auto data = camera_data(colorSample);
    cp.receive_camera(std::move(data));*/
}

void CalibrationManager::OnAmeterSample(imu_sample_t* sample)
{
    if (!isCalibrating()) return;
    //SensorManager::OnAmeterSample(sample); // causes debug logging

    //accelerometer_data data;
    ////windows gives acceleration in g-units, so multiply by standard gravity in m/s^2
    //data.accel_m__s2[0] = -sample->data[0] * 9.80665;
    //data.accel_m__s2[1] = sample->data[1] * 9.80665;
    //data.accel_m__s2[2] = -sample->data[2] * 9.80665;
    //data.timestamp = sensor_clock::time_point(sensor_clock::duration(sample->coordinatedUniversalTime100ns));
    //cp.receive_accelerometer(std::move(data));
}

void CalibrationManager::OnGyroSample(imu_sample_t* sample)
{
    if (!isCalibrating()) return;
    //SensorManager::OnGyroSample(sample); // causes debug logging

    //gyro_data data;
    ////windows gives angular velocity in degrees per second
    //data.angvel_rad__s[0] = sample->data[0] * M_PI / 180.;
    //data.angvel_rad__s[1] = sample->data[1] * M_PI / 180.;
    //data.angvel_rad__s[2] = sample->data[2] * M_PI / 180.;
    //data.timestamp = sensor_clock::time_point(sensor_clock::duration(sample->coordinatedUniversalTime100ns));
    //cp.receive_gyro(std::move(data));
}


