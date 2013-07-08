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
    NSOperationQueue* queueMotion;
    BOOL isCapturing;
}
@synthesize cmMotionManager;

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
        cmMotionManager = [CMMotionManager new];
        isCapturing = NO;
	}
	return self;
}

/** @returns True if successfully started motion capture. False if setupMotionCapture has not been called, or plugins not started. */
- (BOOL) startMotionCapture
{
    LOGME
    return [self startMotionCapWithQueue:[NSOperationQueue new]];
}

- (BOOL)startMotionCapWithQueue:(NSOperationQueue*)queue
{
    queueMotion = queue;
    
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    
    if(sensorFusion == nil || ![sensorFusion isSensorFusionRunning])
    {
        NSLog(@"Failed to start motion capture. Plugins not started.");
        return NO;
    }
    
    if (!isCapturing)
    {
        if(cmMotionManager == nil)
        {
            NSLog(@"Failed to start motion capture. Motion Manager is nil");
            return NO;
        }
        
        [cmMotionManager setAccelerometerUpdateInterval:.01];
        [cmMotionManager setGyroUpdateInterval:.01];
        
        if(queueMotion == nil)
        {
            NSLog(@"Failed to start motion capture. Operation queue is nil");
            return NO;
        }
        
        [queueMotion setMaxConcurrentOperationCount:1]; //makes this into a serial queue, instead of concurrent
        
        [cmMotionManager startAccelerometerUpdatesToQueue:queueMotion withHandler:
         ^(CMAccelerometerData *accelerometerData, NSError *error){
             if (error) {
                 NSLog(@"Error starting accelerometer updates");
                 [cmMotionManager stopAccelerometerUpdates];
             } else {
                 //             NSLog(@"%f,accel,%f,%f,%f\n",
                 //                    accelerometerData.timestamp,
                 //                    accelerometerData.acceleration.x,
                 //                    accelerometerData.acceleration.y,
                 //                    accelerometerData.acceleration.z);
                 
                 [sensorFusion receiveAccelerometerData:accelerometerData];
             }
         }];
        
        [cmMotionManager startGyroUpdatesToQueue:queueMotion withHandler:
         ^(CMGyroData *gyroData, NSError *error){
             if (error) {
                 NSLog(@"Error starting gyro updates");
                 [cmMotionManager stopGyroUpdates];
             } else {
                 //             NSLog(@"%f,gyro,%f,%f,%f\n",
                 //                   gyroData.timestamp,
                 //                   gyroData.rotationRate.x,
                 //                   gyroData.rotationRate.y,
                 //                   gyroData.rotationRate.z);
                 
                 [sensorFusion receiveGyroData:gyroData];
             }
         }];
        
        isCapturing = YES;
    }
	    
    return isCapturing;
}

- (void) stopMotionCapture
{
	LOGME
    
    if(queueMotion) [queueMotion cancelAllOperations];
    
    if(cmMotionManager)
    {
        if(cmMotionManager.isAccelerometerActive) [cmMotionManager stopAccelerometerUpdates];
        if(cmMotionManager.isGyroActive) [cmMotionManager stopGyroUpdates];
    }
    
    isCapturing = NO;
    
    queueMotion = nil;   
}

- (BOOL)isCapturing
{
    return isCapturing;
}

@end
