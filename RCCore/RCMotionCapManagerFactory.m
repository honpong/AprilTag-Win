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
    NSOperationQueue *_queueMotion;
    id<RCCorvisManager> _corvisManager;
    BOOL isCapturing;
}

@property CMMotionManager* motionManager;

@end

@implementation RCMotionCapManagerImpl
@synthesize motionManager;

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
    LOGME
    return [self startMotionCapWithQueue:[NSOperationQueue new]];
}

- (BOOL)startMotionCapWithQueue:(NSOperationQueue*)queue
{
    _corvisManager = [RCCorvisManagerFactory getInstance];
    
    if(!_corvisManager || ![_corvisManager isPluginsStarted])
    {
        NSLog(@"Failed to start motion capture. Corvis plugins not started.");
        return NO;
    }
    
    if (!isCapturing)
    {
        if(motionManager == nil)
        {
            NSLog(@"Failed to start motion capture. Motion Manager is nil");
            return NO;
        }
        
        [motionManager setAccelerometerUpdateInterval:.01];
        [motionManager setGyroUpdateInterval:.01];
        
        _queueMotion = queue;
        
        if(_queueMotion == nil)
        {
            NSLog(@"Failed to start motion capture. Operation queue is nil");
            return NO;
        }
        
        [_queueMotion setMaxConcurrentOperationCount:1]; //makes this into a serial queue, instead of concurrent
        
        [motionManager startAccelerometerUpdatesToQueue:_queueMotion withHandler:
         ^(CMAccelerometerData *accelerometerData, NSError *error){
             if (error) {
                 NSLog(@"Error starting accelerometer updates");
                 [motionManager stopAccelerometerUpdates];
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
        
        [motionManager startGyroUpdatesToQueue:_queueMotion withHandler:
         ^(CMGyroData *gyroData, NSError *error){
             if (error) {
                 NSLog(@"Error starting gyro updates");
                 [motionManager stopGyroUpdates];
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
	    
    return isCapturing;
}

- (void)stopMotionCap
{
	LOGME
    
    if(_queueMotion) [_queueMotion cancelAllOperations];
    
    if(motionManager)
    {
        if(motionManager.isAccelerometerActive) [motionManager stopAccelerometerUpdates];
        if(motionManager.isGyroActive) [motionManager stopGyroUpdates];
    }
    
    isCapturing = NO;
    
    _queueMotion = nil;   
}

- (BOOL)isCapturing
{
    return isCapturing;
}

@end

@implementation RCMotionCapManagerFactory

static id<RCMotionCapManager> instance;

+ (void)setupMotionCap
{
    [self setupMotionCap:[CMMotionManager new]];
}

+ (void)setupMotionCap:(CMMotionManager *)motionMan
{
    instance = nil;
    instance = [RCMotionCapManagerImpl new];
    instance.motionManager = motionMan;
}

+ (id<RCMotionCapManager>)getInstance
{
    return instance;
}

#ifdef DEBUG
//for testing. you can set this factory to return a mock object.
+ (void)setInstance:(id<RCMotionCapManager>)mockObject
{
    instance = mockObject;
}
#endif

@end
