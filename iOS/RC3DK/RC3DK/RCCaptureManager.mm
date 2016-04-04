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
    sensor accelerometer, gyro, camera;
}
@end

@implementation RCCaptureManager

- (id)init
{
	if(self = [super init])
	{
        isCapturing = false;
        
        camera.id = 0;
        accelerometer.id = 1;
        gyro.id = 2;
	}
	return self;
}

- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;
{
    if(isCapturing)
        cp.write_camera(camera_data_from_CMSampleBufferRef(camera, sampleBuffer));
}

- (void) receiveAccelerometerData:(CMAccelerometerData *)accelerationData
{
    if(isCapturing)
        cp.write_accelerometer(accelerometer_data_from_CMAccelerometerData(accelerometer, accelerationData));
}

- (void) receiveGyroData:(CMGyroData *)gyroData
{
    if(isCapturing)
        cp.write_gyro(gyro_data_from_CMGyroData(gyro, gyroData));
}

- (void)startCaptureWithPath:(NSString *)path
{
    if (isCapturing)
        return;

    cp.start([path UTF8String], false);
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
