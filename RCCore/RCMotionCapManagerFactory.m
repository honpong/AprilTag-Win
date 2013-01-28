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

- (id)init
{
	if(self = [super init])
	{
        NSLog(@"Init motion capture");
        
        isCapturing = NO;
	}
	return self;
}

/** @returns True if successfully started motion capture. False if setupMotionCapture has not been called, or Corvis plugins not started. */
- (BOOL)startMotionCap
{
    return [self startMotionCapWithMotionManager:[[CMMotionManager alloc] init]
                                       withQueue:[[NSOperationQueue alloc] init]
                               withCorvisManager:[RCCorvisManagerFactory getCorvisManagerInstance]];
}

- (BOOL)startMotionCapWithMotionManager:(CMMotionManager*)motionMan
                              withQueue:(NSOperationQueue*)queue
                      withCorvisManager:(id<RCCorvisManager>)corvisManager
{
	NSLog(@"Starting motion capture");
    
    _corvisManager = corvisManager;
    
    if(!_corvisManager || ![_corvisManager isPluginsStarted])
    {
        NSLog(@"Failed to start motion capture. Corvis plugins not started.");
        return NO;
    }
    
    if (!isCapturing)
    {
        _motionMan = motionMan;
        
        if(!_motionMan)
        {
            NSLog(@"Failed to start motion capture. Motion Manager is nil");
            return NO;
        }
        
        _motionMan.accelerometerUpdateInterval = .01;
        _motionMan.gyroUpdateInterval = .01;
        
        if(!_queueMotion)
        {
            NSLog(@"Failed to start motion capture. Operation queue is nil");
            return NO;
        }
        
        _queueMotion = queue;
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
	    
    return YES;
}

- (void)releaseObjects
{
    _queueMotion = nil;
    _motionMan = nil;
    _corvisManager = nil;
}

- (void)stopMotionCap
{
	NSLog(@"Stopping motion capture");
    
    if(!_queueMotion)
	{
		NSLog(@"Failed to cancel queue operations.");
	}
    else
    {
        [_queueMotion cancelAllOperations];
    }
    
    if(!_motionMan)
	{
		NSLog(@"Failed to stop motion updates.");
	}
    else
    {
        if(_motionMan.isAccelerometerActive) [_motionMan stopAccelerometerUpdates];
        if(_motionMan.isGyroActive) [_motionMan stopGyroUpdates];
    }
    
    [self releaseObjects];
}

@end

@implementation RCMotionCapManagerFactory

static id<RCMotionCapManager> instance;

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
