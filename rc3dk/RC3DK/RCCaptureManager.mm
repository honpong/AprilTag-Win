//
//  CaptureController.m
//  RCCapture
//
//  Created by Brian on 10/3/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "RCCaptureManager.h"

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
        cp.receive_camera(camera_data(sampleBuffer));
}

- (void) receiveAccelerometerData:(CMAccelerometerData *)accelerationData
{
    if(isCapturing)
        cp.receive_accelerometer(accelerometer_data((__bridge void *)accelerationData));
}

- (void) receiveGyroData:(CMGyroData *)gyroData
{
    if(isCapturing)
        cp.receive_gyro(gyro_data((__bridge void *)gyroData));
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
