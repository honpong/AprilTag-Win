//
//  CaptureController.m
//  RCCapture
//
//  Created by Brian on 10/3/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "RCCaptureManager.h"
#import "RCSensorData.h"
#include "capture.h"

@interface RCCaptureManager ()
{
    capture cp;
    
    bool isCapturing;
}
@end

@implementation RCCaptureManager

- (id)init
{
	if(self = [super init])
	{
        isCapturing = false;
	}
	return self;
}

- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;
{
    if(isCapturing)
        cp.write_camera(camera_data_from_CMSampleBufferRef(sampleBuffer));
}

- (void) receiveAccelerometerData:(CMAccelerometerData *)accelerationData
{
    if(isCapturing)
        cp.write_accelerometer(accelerometer_data_from_CMAccelerometerData(accelerationData));
}

- (void) receiveGyroData:(CMGyroData *)gyroData
{
    if(isCapturing)
        cp.write_gyro(gyro_data_from_CMGyroData(gyroData));
}

- (void)startCaptureWithPath:(NSString *)path
{
    if (isCapturing)
        return;

    cp.start([path UTF8String]);
    isCapturing = true;
}

- (void) stopCapture
{
    if (!isCapturing)
        return;

    isCapturing = false;
    cp.stop();
}

@end
