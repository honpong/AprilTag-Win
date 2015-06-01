/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2014-2015 Intel Corporation. All Rights Reserved.
*******************************************************************************/
/// @file libpxcimu.h
#ifndef LIBPXCIMU
#define LIBPXCIMU

#include "pxcbase.h"
#include <windows.h>
#include "pxcmetadata.h"

// Sensor types
PXC_DEFINE_CONST(METADATA_IMU_LINEAR_ACCELERATION, 0xfbb7ceaf); // Linear acceleration (g)
PXC_DEFINE_CONST(METADATA_IMU_ANGULAR_VELOCITY, 0xaf579a86);    // Angular velocity (deg/sec)
PXC_DEFINE_CONST(METADATA_IMU_TILT, 0x5c433cc3);                // Inclinometer (deg)
PXC_DEFINE_CONST(METADATA_IMU_GRAVITY, 0x7bce5555);             // Last row (6-8) of rotation matrix

// If capture is enabled for a sensor, sample data is capture into a circular (ring) buffer.
// A sorted copy of this ring buffer is attached to the depth image as meta data (e.g. METADATA_IMU_TILT).
const unsigned IMU_RING_BUFFER_SAMPLE_COUNT = 16; // The number of samples attached (for each sensor type)

typedef struct
{
    /// @param[out] coordinatedUniversalTime100ns 
    /// A single tick represents one hundred nanoseconds 
    /// or one ten - millionth of a second. There are 
    /// 10,000 ticks in a millisecond. The value of this 
    /// property represents the number of 100 - nanosecond 
    /// intervals that have elapsed since 12:00 : 00 
    /// midnight, January 1, 0001 (DateTime.MinValue).
    /// It does not include the number of ticks that are 
    /// attributable to leap seconds.
    /// If the value is zero, the sample should be 
    /// considered as invalid.
    __int64    coordinatedUniversalTime100ns;

    /// @param[out] data
    /// Three dimensional sample data in x, y, z 
    /// or yaw, pitch, roll. All orientations are in the
    /// standard Windows defined formats.
    float      data[3];
}
imu_sample_t;


#endif // #ifndef LIBPXCIMU
