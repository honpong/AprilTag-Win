//
//  TMMotionCapManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMotionManager.h"

@implementation RCMotionManager
{
    NSOperationQueue* _queueMotion;
    RCSensorFusion* _sensorFusion;
    BOOL isCapturing;
}
@synthesize motionManager;

+ (void)setupMotionCap
{
    [self setupMotionCap:[CMMotionManager new]];
}

+ (void)setupMotionCap:(CMMotionManager *)motionMan
{
    [RCMotionManager sharedInstance].motionManager = motionMan;
}

+ (RCMotionManager*) sharedInstance
{
    static RCMotionManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (id)init
{
	if(self = [super init])
	{
        NSLog(@"Init motion capture");
        
        isCapturing = NO;
	}
	return self;
}

/** @returns True if successfully started motion capture. False if setupMotionCapture has not been called, or plugins not started. */
- (BOOL)startMotionCap
{
    LOGME
    return [self startMotionCapWithQueue:[NSOperationQueue new]];
}

- (BOOL)startMotionCapWithQueue:(NSOperationQueue*)queue
{
    _sensorFusion = [RCSensorFusion sharedInstance];
    
    if(!_sensorFusion || ![_sensorFusion isPluginsStarted])
    {
        NSLog(@"Failed to start motion capture. Plugins not started.");
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
                 
                 [_sensorFusion receiveAccelerometerData:accelerometerData.timestamp
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
                 [_sensorFusion receiveGyroData:gyroData.timestamp
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
