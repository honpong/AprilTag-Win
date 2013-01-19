//
//  TMMotionCapManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMotionCapManagerFactory.h"

@interface TMMotionCapManagerImpl : NSObject <TMMotionCapManager>
{
    CMMotionManager *_motionMan;
    NSOperationQueue *_queueMotion;
}
@end

@implementation TMMotionCapManagerImpl

- (id)init
{
	if(self = [super init])
	{
        NSLog(@"Init motion capture");
	}
	return self;
}

- (void)startMotionCapture
{
	NSLog(@"Starting motion capture");
	
    _motionMan = [[CMMotionManager alloc] init];
    
	if(!_motionMan)
	{
		NSLog(@"Failed to start motion capture. Motion Manager is nil");
		return;
	}
	
	_motionMan.accelerometerUpdateInterval = .01;
	_motionMan.gyroUpdateInterval = .01;
    
    _queueMotion = [[NSOperationQueue alloc] init];
    [_queueMotion setMaxConcurrentOperationCount:1]; //makes this into a serial queue, instead of concurrent
    
	[_motionMan startAccelerometerUpdatesToQueue:_queueMotion withHandler:
	 ^(CMAccelerometerData *accelerometerData, NSError *error){
		 if (error) {
			 NSLog(@"Error starting accelerometer updates");
			 [_motionMan stopAccelerometerUpdates];
		 } else {
//             NSLog(@"%f,accel,%f,%f,%f\n",
//                    accelerometerData.timestamp,
//                    accelerometerData.acceleration.x,
//                    accelerometerData.acceleration.y,
//                    accelerometerData.acceleration.z);
             
             [CORVIS_MANAGER receiveAccelerometerData:accelerometerData.timestamp
                                            withX:accelerometerData.acceleration.x
                                            withY:accelerometerData.acceleration.y
                                            withZ:accelerometerData.acceleration.z];
             
         }
	 }];
	
	[_motionMan startGyroUpdatesToQueue:_queueMotion withHandler:
	 ^(CMGyroData *gyroData, NSError *error){
		 if (error) {
			 NSLog(@"Error starting gyro updates");
			 [_motionMan stopGyroUpdates];
		 } else {
//             NSLog(@"%f,gyro,%f,%f,%f\n",
//                   gyroData.timestamp,
//                   gyroData.rotationRate.x,
//                   gyroData.rotationRate.y,
//                   gyroData.rotationRate.z);
             
			 //pass packet here
             [CORVIS_MANAGER receiveGyroData:gyroData.timestamp
                                   withX:gyroData.rotationRate.x
                                   withY:gyroData.rotationRate.y
                                   withZ:gyroData.rotationRate.z];
         }
	 }];
}

- (void)stopMotionCapture
{
	NSLog(@"Stopping motion capture");
    
    [_queueMotion cancelAllOperations];
    
    if(!_motionMan)
	{
		NSLog(@"Failed to stop motion capture. Motion Manager is nil");
		return;
	}
	
	if(_motionMan.isAccelerometerActive) [_motionMan stopAccelerometerUpdates];
	if(_motionMan.isGyroActive) [_motionMan stopGyroUpdates];
    _motionMan = nil;
}

@end

@implementation TMMotionCapManagerFactory

static id<TMMotionCapManager> instance;

+ (id<TMMotionCapManager>)getMotionCapManagerInstance
{
    if (instance == nil)
    {
        instance = [[TMMotionCapManagerImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setMotionCapManagerInstance:(id<TMMotionCapManager>)mockObject
{
    instance = mockObject;
}

@end
