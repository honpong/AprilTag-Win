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
        LOGME
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
    
    if(sensorFusion == nil)
    {
        DLog(@"Failed to start motion capture. Couldn't get the RCSensorFusion instance.");
        return NO;
    }
    
    if (!isCapturing)
    {
        if(cmMotionManager == nil)
        {
            DLog(@"Failed to start motion capture. Motion Manager is nil");
            return NO;
        }
        
        [cmMotionManager setAccelerometerUpdateInterval:.01];
        [cmMotionManager setGyroUpdateInterval:.01];
        /*[cmMotionManager setDeviceMotionUpdateInterval:.1];*/
        
        if(queueMotion == nil)
        {
            DLog(@"Failed to start motion capture. Operation queue is nil");
            return NO;
        }
        
        [queueMotion setMaxConcurrentOperationCount:1]; //makes this into a serial queue, instead of concurrent
        
        [cmMotionManager startAccelerometerUpdatesToQueue:queueMotion withHandler:
         ^(CMAccelerometerData *accelerometerData, NSError *error){
             if (error) {
                 DLog(@"Error starting accelerometer updates");
                 [cmMotionManager stopAccelerometerUpdates];
             } else {
                 //             DLog(@"%f,accel,%f,%f,%f\n",
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
                 DLog(@"Error starting gyro updates");
                 [cmMotionManager stopGyroUpdates];
             } else {
                 //             DLog(@"%f,gyro,%f,%f,%f\n",
                 //                   gyroData.timestamp,
                 //                   gyroData.rotationRate.x,
                 //                   gyroData.rotationRate.y,
                 //                   gyroData.rotationRate.z);
                 
                 [sensorFusion receiveGyroData:gyroData];
             }
         }];
        /*
        [cmMotionManager startDeviceMotionUpdatesToQueue:queueMotion withHandler:
         ^(CMDeviceMotion *motionData, NSError *error) {
            if (error) {
                DLog(@"Error starting device motion updates");
                [cmMotionManager stopDeviceMotionUpdates];
            } else {
                //             DLog(@"%f,motion,%f,%f,%f\n",
                //                   motionData.timestamp,
                //                   motionData.rotationRate.x,
                //                   motionData.rotationRate.y,
                //                   motionData.rotationRate.z);
                
                [sensorFusion receiveMotionData:motionData];
            }
         }];
        */
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
        //if(cmMotionManager.isDeviceMotionActive) [cmMotionManager stopDeviceMotionUpdates];
    }
    
    isCapturing = NO;
    
    queueMotion = nil;   
}

- (BOOL)isCapturing
{
    return isCapturing;
}

@end
