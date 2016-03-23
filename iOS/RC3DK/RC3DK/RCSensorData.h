//
//  RCSensorData.h
//  RC3DK
//
//  Created by Eagle Jones on 5/27/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef RC3DK_RCSensorData_h_h
#define RC3DK_RCSensorData_h_h

#include "sensor_data.h"
#import <CoreMedia/CoreMedia.h>
#import <CoreMotion/CoreMotion.h>

image_gray8 camera_data_from_CMSampleBufferRef(CMSampleBufferRef sampleBuffer);
accelerometer_data accelerometer_data_from_CMAccelerometerData(CMAccelerometerData *accelerationData);
gyro_data gyro_data_from_CMGyroData(CMGyroData *gyroData);

#endif
