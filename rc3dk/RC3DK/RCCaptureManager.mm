//
//  CaptureController.m
//  RCCapture
//
//  Created by Brian on 10/3/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <CoreMotion/CoreMotion.h>

#import "RCAVSessionManager.h"
#import "RCCaptureManager.h"

#include "capture.h"

#define POLL

@interface RCCaptureManager ()
{
    capture cp;
    
    AVCaptureSession * session;
    AVCaptureVideoDataOutput * output;
    AVCaptureDevice * device;
    AVCaptureVideoOrientation videoOrientation;

    CMMotionManager * cmMotionManager;
    NSTimer* timerMotion;
    NSTimeInterval lastGyro, lastAccelerometer;
    bool isCapturing;
    bool isFocusing;
    bool hasFocused;
}
@end

@implementation RCCaptureManager

@synthesize delegate;

- (id)init
{
	if(self = [super init])
	{
        cmMotionManager = [CMMotionManager new];
        isCapturing = false;
        timerMotion = nil;
	}
	return self;
}

- (void) startVideoCapture
{
    isCapturing = true;
    hasFocused = true;
    if([delegate respondsToSelector:@selector(captureDidStart)])
        dispatch_async(dispatch_get_main_queue(), ^{
            [delegate captureDidStart];
        });
}

- (void) observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([keyPath isEqualToString:@"adjustingFocus"]) {
        bool wasFocusing = isFocusing;
        isFocusing = [change[NSKeyValueChangeNewKey] isEqualToNumber:@1];
        if(wasFocusing && !isFocusing & !hasFocused) {
            // Capture doesn't start until focus has locked and finished focusing
            if ([device lockForConfiguration:nil]) {
                if([device isFocusModeSupported:AVCaptureFocusModeLocked]) {
                    [device setFocusMode:AVCaptureFocusModeLocked];
                }
                [device unlockForConfiguration];
            }
            [self startVideoCapture];
        }
    }
}

- (void) startVideoCaptureAutofocus
{
    session = [RCAVSessionManager sharedInstance].session;
    device = [RCAVSessionManager sharedInstance].videoDevice;

    output = [[AVCaptureVideoDataOutput alloc] init];
    [output setAlwaysDiscardsLateVideoFrames:YES];
    [output setVideoSettings:@{(id)kCVPixelBufferPixelFormatTypeKey: [NSNumber numberWithInt:'420f']}];

    [device addObserver:self forKeyPath:@"adjustingFocus" options:NSKeyValueObservingOptionNew context:nil];
    if(![device isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
        [self startVideoCapture];
    }
    if([device lockForConfiguration:nil]) {
        if([device isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
            [device setFocusMode:AVCaptureFocusModeAutoFocus];
        }
        [device unlockForConfiguration];
    }

    //causes lag
    [session addOutput:output];

    AVCaptureConnection *videoConnection = [output connectionWithMediaType:AVMediaTypeVideo];
    videoOrientation = [videoConnection videoOrientation];

    dispatch_queue_t queue = dispatch_queue_create("MyQueue", DISPATCH_QUEUE_SERIAL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
    [output setSampleBufferDelegate:self queue:queue];
}

- (void) stopVideoCapture
{
    [device removeObserver:self forKeyPath:@"adjustingFocus"];
    if([device lockForConfiguration:nil]) {
        if([device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus]) {
            [device setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
        }
        [device unlockForConfiguration];
    }
    [session removeOutput:output];
}

-(void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
	if(!isCapturing)
        return;

    cp.receive_camera(camera_data(sampleBuffer));
}

- (void)timerCallback:(id)userInfo
{
    if(!isCapturing)
        return;

    CMGyroData *gyroData = cmMotionManager.gyroData;
    if(gyroData && gyroData.timestamp != lastGyro)
    {
        cp.receive_gyro(gyro_data((__bridge void *)gyroData));
        lastGyro = gyroData.timestamp;
    }

    CMAccelerometerData *accelerometerData = cmMotionManager.accelerometerData;
    if(accelerometerData && accelerometerData.timestamp != lastAccelerometer)
    {
        cp.receive_accelerometer(accelerometer_data((__bridge void *)accelerometerData));
        lastAccelerometer = accelerometerData.timestamp;
    }
}

- (BOOL)startMotionCaptureWithTimer:(NSTimer *)timer
{
    timerMotion = timer;
    if(timerMotion == nil)
    {
        NSLog(@"Failed to start motion capture. Timer is nil");
        return NO;
    }

    lastAccelerometer = lastGyro = 0.;
    [cmMotionManager startGyroUpdates];
    [cmMotionManager startAccelerometerUpdates];
    return true;
}

/** @returns True if successfully started motion capture. False if setupMotionCapture has not been called, or plugins not started. */
- (BOOL) startMotionCapture
{
    if(cmMotionManager == nil)
    {
        NSLog(@"Failed to start motion capture. Motion Manager is nil");
        return NO;
    }

    if (!isCapturing)
    {
        //Timer interval of .011 determined experimentally using DEBUG_TIMER - setting it to .01 makes the gyro rate drop dramatically
        [cmMotionManager setAccelerometerUpdateInterval:.011];
        [cmMotionManager setGyroUpdateInterval:.011];
        [self startMotionCaptureWithTimer:[NSTimer scheduledTimerWithTimeInterval:.011 target:self selector:@selector(timerCallback:) userInfo:nil repeats:true]];
    }
    return true;
}

- (void) stopMotionCapture
{
    if(timerMotion) [timerMotion invalidate];

    if(cmMotionManager)
    {
        if(cmMotionManager.isAccelerometerActive) [cmMotionManager stopAccelerometerUpdates];
        if(cmMotionManager.isGyroActive) [cmMotionManager stopGyroUpdates];
    }

    timerMotion = nil;
}

- (void)startCaptureWithPath:(NSString *)path withDelegate:(id<RCCaptureManagerDelegate>)captureDelegate;
{
    if (isCapturing) return;

    hasFocused = false;
    cp.start([path UTF8String]);

    self.delegate = captureDelegate;
    [self startMotionCapture];
    // isCapturing is set after focus finishes
    [self startVideoCaptureAutofocus];
}

- (void) stopCapture
{
    if (isCapturing)
    {
        [self stopVideoCapture];
        [self stopMotionCapture];

        isCapturing = NO;
        cp.stop();
        if([delegate respondsToSelector:@selector(captureDidStop)])
            dispatch_async(dispatch_get_main_queue(), ^{
                [delegate captureDidStop];
            });
    }
}

@end
