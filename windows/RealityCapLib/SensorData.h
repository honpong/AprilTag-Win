//
//  RCSensorData.h
//  RC3DK
//
//  Created by Eagle Jones on 5/28/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RCSensorData_h_
#define __RCSensorData_h_

#include "..\..\corvis\src\cor\sensor_data.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include "libpxcimu_internal.h"

camera_data camera_data_from_PXCImage(PXCImage *image);
accelerometer_data accelerometer_data_from_imu_sample_t(const imu_sample_t *sample);
gyro_data gyro_data_from_imu_sample_t(const imu_sample_t *sample);

#endif