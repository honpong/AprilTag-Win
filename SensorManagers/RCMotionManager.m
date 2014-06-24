//
//  RCMotionManager.m
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMotionManager.h"
#import "RCDebugLog.h"

/** This implements two different methods of capturing data from CoreMotion - polling or updates to a queue. Experimentally, as of iOS 6, capturing updates to a queue consumes signficantly more CPU than polling.
 
 */
#define POLL
//#define DEBUG_TIMER

@implementation RCMotionManager
{
    NSOperationQueue* queueMotion;
    NSTimer* timerMotion;
    BOOL isCapturing;
    NSTimeInterval lastGyro, lastAccelerometer;
#ifdef DEBUG_TIMER
    NSTimeInterval firstGyro, firstAccelerometer;
    int countGyro, countAccelerometer;
#endif
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
        timerMotion = nil;
        queueMotion = nil;
	}
	return self;
}

/** @returns True if successfully started motion capture. False if setupMotionCapture has not been called, or plugins not started. */
- (BOOL) startMotionCapture
{
    LOGME
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    
    if(sensorFusion == nil)
    {
        DLog(@"Failed to start motion capture. Couldn't get the RCSensorFusion instance.");
        return NO;
    }
    if(cmMotionManager == nil)
    {
        DLog(@"Failed to start motion capture. Motion Manager is nil");
        return NO;
    }

    if (!isCapturing)
    {
        //Timer interval of .011 determined experimentally using DEBUG_TIMER - setting it to .01 makes the gyro rate drop dramatically
        [cmMotionManager setAccelerometerUpdateInterval:.011];
        [cmMotionManager setGyroUpdateInterval:.011];
#ifdef POLL
        isCapturing = [self startMotionCaptureWithTimer:[NSTimer scheduledTimerWithTimeInterval:.011 target:self selector:@selector(timerCallback:) userInfo:nil repeats:true]];
#else
        isCapturing = [self startMotionCapWithQueue:[NSOperationQueue new]];
#endif
#ifdef DEBUG_TIMER
        firstAccelerometer = firstGyro = 0.;
        countAccelerometer = countGyro = 0;
        [NSTimer scheduledTimerWithTimeInterval:1. target:self selector:@selector(rateCallback:) userInfo:nil repeats:true];
#endif
    }
    return isCapturing;
}

#ifdef DEBUG_TIMER
- (void)rateCallback:(id)userInfo
{
    float accelrate = countAccelerometer / (lastAccelerometer - firstAccelerometer);
    float gyrorate = countGyro / (lastGyro - firstGyro);
    fprintf(stderr, "accel rate is %f, gyro rate is %f\n", accelrate, gyrorate);
}
#endif

- (void)timerCallback:(id)userInfo
{
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    CMGyroData *gyroData = cmMotionManager.gyroData;
    if(gyroData && gyroData.timestamp != lastGyro)
    {
        [sensorFusion receiveGyroData:gyroData];
        lastGyro = gyroData.timestamp;
#ifdef DEBUG_TIMER
        if(!countGyro) firstGyro = lastGyro;
        ++countGyro;
#endif
    }
    CMAccelerometerData *accelerometerData = cmMotionManager.accelerometerData;
    if(accelerometerData && accelerometerData.timestamp != lastAccelerometer)
    {
        [sensorFusion receiveAccelerometerData:accelerometerData];
        lastAccelerometer = accelerometerData.timestamp;
#ifdef DEBUG_TIMER
        if(!countAccelerometer) firstAccelerometer = lastAccelerometer;
        ++countAccelerometer;
#endif
    }
}

- (BOOL)startMotionCaptureWithTimer:(NSTimer *)timer
{
    timerMotion = timer;
    if(timerMotion == nil)
    {
        DLog(@"Failed to start motion capture. Timer is nil");
        return NO;
    }

    lastAccelerometer = lastGyro = 0.;
    [cmMotionManager startGyroUpdates];
    [cmMotionManager startAccelerometerUpdates];
    return true;
}

- (BOOL)startMotionCapWithQueue:(NSOperationQueue*)queue
{
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    queueMotion = queue;
    if(queueMotion == nil)
    {
        DLog(@"Failed to start motion capture. Operation queue is nil");
        return NO;
    }
    
    [queueMotion setMaxConcurrentOperationCount:1]; //makes this into a serial queue, instead of concurrent
    
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
             lastGyro = gyroData.timestamp;
#ifdef DEBUG_TIMER
             if(!countGyro) firstGyro = gyroData.timestamp;
             ++countGyro;
#endif
         }
     }];

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
             lastAccelerometer = accelerometerData.timestamp;
#ifdef DEBUG_TIMER
             if(!countAccelerometer) firstAccelerometer = accelerometerData.timestamp;
             ++countAccelerometer;
#endif
         }
     }];
    
    return true;
}

- (void) stopMotionCapture
{
	LOGME
    
    if(queueMotion) [queueMotion cancelAllOperations];
    
    if(timerMotion) [timerMotion invalidate];
    
    if(cmMotionManager)
    {
        if(cmMotionManager.isAccelerometerActive) [cmMotionManager stopAccelerometerUpdates];
        if(cmMotionManager.isGyroActive) [cmMotionManager stopGyroUpdates];
    }
    
    isCapturing = NO;
    
    queueMotion = nil;
    timerMotion = nil;
}

- (BOOL)isCapturing
{
    return isCapturing;
}

@end
