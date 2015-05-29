/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2014 Intel Corporation. All Rights Reserved.
*******************************************************************************/
/// @file libpxcimu_internal.h
#ifndef LIBPXCIMU_INTERNAL
#define LIBPXCIMU_INTERNAL
#include "libpxcimu.h"
#include <list>

typedef enum
{
    LINEAR_ACCELERATION,
    ANGULAR_VELOCITY,
    TILT,
    GRAVITY,
    NUM_SENSOR_TYPES
}
sensor_t;

enum IMUProperties
{
    PROPERTY_IMU                         = 0x03001000,
    PROPERTY_SENSORS_LINEAR_ACCELERATION = PROPERTY_IMU, // e.g. SENSOR_DATA_TYPE_ACCELERATION_X_G
    PROPERTY_SENSORS_ANGULAR_VELOCITY,                   // e.g. SENSOR_DATA_TYPE_ANGULAR_VELOCITY_X_DEGREES_PER_SECOND
    PROPERTY_SENSORS_TILT,                               // e.g. SENSOR_DATA_TYPE_TILT_X_DEGREES
    PROPERTY_SENSORS_GRAVITY,                            // SENSOR_DATA_TYPE_ROTATION_MATRIX (6-9)
};

// Enables the IMU system and create a sensor data capture thread.
// Returns true on success, or false if all of the required sensors (and sensor fields) are not available.
// Only the first call to this function does any work. Subsequent calls will return the original return code.
// This call is thread safe.
bool __stdcall IMUEnable(const std::list<std::pair<pxcI32, pxcF32>>& imuRequests);

// Fill a caller allocated ring buffer, with an ordered copy of the IMU sensor samples.
// The number of elements in the ring buffer must be IMU_RING_BUFFER_SAMPLES.
// This call is thread safe. 
void IMUGetOrderedSamples(sensor_t type, imu_sample_t* outRingBuffer);

#endif
