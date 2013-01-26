//
//  TMMotionCapManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMotionCapManagerFactory.h"

@interface RCMotionCapManagerImpl : NSObject <RCMotionCapManager>
{
    CMMotionManager *_motionMan;
    NSOperationQueue *_queueMotion;
    id<RCCorvisManager> _corvisManager;
    bool isCapturing;
}
@end

@implementation RCMotionCapManagerImpl

- (id)initWithCorvisManager:(id<RCCorvisManager>)corvisManager
{
	if(self = [super init])
	{
        NSLog(@"Init motion capture");
        _corvisManager = corvisManager;
        isCapturing = NO;
	}
	return self;
}

/** @returns True if successfully started motion capture. False if setupMotionCapture has not been called, or Corvis plugins not started. */
- (bool)startMotionCap
{
	NSLog(@"Starting motion capture");
    
    if(!_corvisManager || ![_corvisManager isPluginsStarted])
    {
        NSLog(@"Failed to start motion capture. Corvis plugins not started.");
        return false;
    }
    
    if (!isCapturing)
    {
        _motionMan = [[CMMotionManager alloc] init];
        
        if(!_motionMan)
        {
            NSLog(@"Failed to start motion capture. Motion Manager is nil");
            return false;
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
                 
                 [_corvisManager receiveAccelerometerData:accelerometerData.timestamp
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
                 [_corvisManager receiveGyroData:gyroData.timestamp
                                           withX:gyroData.rotationRate.x
                                           withY:gyroData.rotationRate.y
                                           withZ:gyroData.rotationRate.z];
             }
         }];
        
        isCapturing = YES;
    }
	    
    return true;
}

- (void)stopMotionCap
{
	NSLog(@"Stopping motion capture");
    
    if(!_motionMan || !_queueMotion)
	{
		NSLog(@"Failed to stop motion capture.");
		return;
	}
    
    [_queueMotion cancelAllOperations];
	
	if(_motionMan.isAccelerometerActive) [_motionMan stopAccelerometerUpdates];
	if(_motionMan.isGyroActive) [_motionMan stopGyroUpdates];

    _queueMotion = nil;
    _motionMan = nil;
}

@end

@implementation RCMotionCapManagerFactory

static id<RCMotionCapManager> instance;

+ (void)setupMotionCap:(id<RCCorvisManager>)corvisManager
{
    if (!instance) instance = [[RCMotionCapManagerImpl alloc] initWithCorvisManager:corvisManager];
}

+ (id<RCMotionCapManager>)getMotionCapManagerInstance
{
    if (instance == nil)
    {
        instance = [[RCMotionCapManagerImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setMotionCapManagerInstance:(id<RCMotionCapManager>)mockObject
{
    instance = mockObject;
}

@end
