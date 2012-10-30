//
//  RCMotionCap.m
//  RCCore
//
//  Created by Ben Hirashima on 10/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCMotionCap.h"

@implementation RCMotionCap

- (id)initWithMotionManager:(CMMotionManager *)motionMan
{
	if(self = [super init])
	{
		_motionMan = motionMan;
	}
	
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
