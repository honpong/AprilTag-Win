//
//  RCAVSessionManager.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCAVSessionManager.h"

@implementation RCAVSessionManager
@synthesize session, videoDevice;

+ (id) sharedInstance
{
    static RCAVSessionManager *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [RCAVSessionManager new];
    });
    return instance;
}

- (id) init
{
    self = [super init];
    
    if (self)
    {
        LOGME
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handlePause)
                                                     name:UIApplicationWillResignActiveNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleTerminate)
                                                     name:UIApplicationWillTerminateNotification
                                                   object:nil];
        
        session = [[AVCaptureSession alloc] init];
        
        [session beginConfiguration];
        [session setSessionPreset:AVCaptureSessionPreset640x480];
        
        [self addInputToSession];
        
        [session commitConfiguration];
    }
    
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self endSession];
}

- (void) addInputToSession
{
    videoDevice = [self cameraWithPosition:AVCaptureDevicePositionBack];

    if ([videoDevice lockForConfiguration:nil]) {
        if([videoDevice isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus])
            [videoDevice setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
        if([videoDevice isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure])
            [videoDevice setExposureMode:AVCaptureExposureModeContinuousAutoExposure];
        if([videoDevice isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance])
            [videoDevice setWhiteBalanceMode:AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance];
        [videoDevice unlockForConfiguration];
        //DLog(@"Camera modes initialized");
    } else {
        DLog(@"error while configuring camera");
        return;
    }
    
    if (videoDevice)
    {
        NSError *error;
        AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error];
        if (error)
        {
            DLog(@"Error getting AVCaptureDeviceInput object: %@", error);
            return;
        }
        
        [session addInput:input];
    }
    else
    {
        DLog(@"Couldn't get video device");
        return;
    }
}

- (void) lockFocus
{
    LOGME
    if ([videoDevice lockForConfiguration:nil]) {
        if([videoDevice isFocusModeSupported:AVCaptureFocusModeLocked])
            [videoDevice setFocusMode:AVCaptureFocusModeLocked];
        [videoDevice unlockForConfiguration];
    }
}

- (void) focusOnce
{
    LOGME
    if ([videoDevice lockForConfiguration:nil]) {
        if([videoDevice isFocusModeSupported:AVCaptureFocusModeAutoFocus])
            [videoDevice setFocusMode:AVCaptureFocusModeAutoFocus];
        [videoDevice unlockForConfiguration];
    }
    
}

- (void) unlockFocus
{
    LOGME
    if ([videoDevice lockForConfiguration:nil]) {
        if([videoDevice isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus])
            [videoDevice setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
        [videoDevice unlockForConfiguration];
    }

}

- (BOOL) startSession
{
    LOGME
    
    if (!session)
    {
        DLog(@"Session is nil");
        return false;
    }
        
    if (![session isRunning]) [session startRunning];
    
    return true;
}

- (void) endSession
{
    LOGME
    if ([session isRunning]) [session stopRunning];
}

- (BOOL) isRunning
{
    return session ? session.isRunning : false;
}

- (BOOL) addOutput:(AVCaptureVideoDataOutput*)output
{
    if (!session)
    {
        DLog(@"Session is nil");
        return false;
    }
    
    if ([session canAddOutput:output])
    {
        [session addOutput:output];
    }
    else
    {
        DLog(@"Can't add output to session");
        return false;
    }
    
    return true;
}

- (void) setVideoOrientation:(AVCaptureVideoOrientation)orientation
{
    if (session == nil || session.outputs == nil || session.outputs.count == 0) return;
    AVCaptureVideoDataOutput* output = session.outputs[0];
    if (output == nil || output.connections == nil || output.connections.count == 0) return;
    AVCaptureConnection* conn = output.connections[0];
    conn.videoOrientation = orientation;
}

- (bool) isImageClean
{
    if(videoDevice.adjustingFocus) {
        //DLog(@"Adjusting focus");
        return false;
    }
    /*if(videoDevice.adjustingWhiteBalance) {
        DLog(@"Adjusting white balance");
        return false;
    }
    if(videoDevice.adjustingExposure) {
        DLog(@"Adjusting exposure");
        return false;
    }*/
    return true;
}

- (void) handlePause
{
    LOGME
    [self endSession];    
}

- (void) handleTerminate
{
    LOGME
    [self endSession];
}

- (AVCaptureDevice *)cameraWithPosition:(AVCaptureDevicePosition) position
{
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    
    for (AVCaptureDevice *device in devices)
    {
        if ([device position] == position) return device;
    }
    
    return nil;
}

@end
