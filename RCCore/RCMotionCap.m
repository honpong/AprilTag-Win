//
//  RCMotionCap.m
//  RCCore
//
//  Created by Ben Hirashima on 10/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCMotionCap.h"
#include "cor.h"

@implementation RCMotionCap

- (id)initWithMotionManager:(CMMotionManager *)motionMan withOutput:(struct mapbuffer *) output
{
	if(self = [super init])
	{
		_motionMan = motionMan;
	}
    _output = output;
	
	return self;
}

- (void)startMotionCapture
{
	NSLog(@"Starting motion capture");
	
	if(!_motionMan)
	{
		NSLog(@"Failed to start motion capture. Motion Manager is nil");
		return;
	}
	
	_motionMan.accelerometerUpdateInterval = 1.0/60;
	_motionMan.gyroUpdateInterval = 1.0/60;
	
	NSOperationQueue *queueAccel = [[NSOperationQueue alloc] init];
	[queueAccel setMaxConcurrentOperationCount:1]; //makes this into a serial queue, instead of concurrent
	
	[_motionMan startAccelerometerUpdatesToQueue:queueAccel withHandler:
	 ^(CMAccelerometerData *accelerometerData, NSError *error){
		 if (error) {
			 NSLog(@"Error starting accelerometer updates");
			 [_motionMan stopAccelerometerUpdates];
		 } else {
             NSString *logLine = [NSString stringWithFormat:@"%f,accel,%f,%f,%f\n", accelerometerData.timestamp, accelerometerData.acceleration.x, accelerometerData.acceleration.y, accelerometerData.acceleration.z];
			 NSLog(logLine);
			 
			 //pass packet here
         }
	 }];
    
    NSOperationQueue *queueGyro = [[NSOperationQueue alloc] init];
	[queueGyro setMaxConcurrentOperationCount:1]; //makes this into a serial queue, instead of concurrent
	
	[_motionMan startGyroUpdatesToQueue:queueGyro withHandler:
	 ^(CMGyroData *gyroData, NSError *error){
		 if (error) {
			 NSLog(@"Error starting gyro updates");
			 [_motionMan stopGyroUpdates];
		 } else {
             NSString *logLine = [NSString stringWithFormat:@"%f,gyro,%f,%f,%f\n", gyroData.timestamp, gyroData.rotationRate.x, gyroData.rotationRate.y, gyroData.rotationRate.z];
			 NSLog(logLine);
			 //pass packet here
             packet_t *p = mapbuffer_alloc(_output, packet_imu, 6*4);
             ((float*)p->data)[0] = gyroData.rotationRate.x;
             ((float*)p->data)[1] = gyroData.rotationRate.y;
             ((float*)p->data)[2] = gyroData.rotationRate.z;
             mapbuffer_enqueue(_output, p, gyroData.timestamp * 100000);
         }
	 }];
}

- (void)stopMotionCapture
{
	NSLog(@"Stopping motion capture");
	
	if(!_motionMan)
	{
		NSLog(@"Failed to stop motion capture. Motion Manager is nil");
		return;
	}
	
	if(_motionMan.isAccelerometerActive) [_motionMan stopAccelerometerUpdates];
	if(_motionMan.isGyroActive) [_motionMan stopGyroUpdates];
}

@end
