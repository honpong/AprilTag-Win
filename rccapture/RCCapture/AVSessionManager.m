//
//  AVSessionManager.m
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AVSessionManager.h"

@implementation AVSessionManager
@synthesize session, videoDevice;

+ (id) sharedInstance
{
    static AVSessionManager *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [AVSessionManager new];
    });
    return instance;
}

- (id) init
{
    if (self = [super init])
    {
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
        [session setSessionPreset:AVCaptureSessionPreset640x480]; // 640x480 required
        
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
    } else {
        NSLog(@"error while configuring camera");
    }
    
    if (videoDevice)
    {
        NSError *error;
        AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error]; //TODO: handle error
        if (error) NSLog(@"Error getting AVCaptureDeviceInput object: %@", error.localizedFailureReason);
        
        [session addInput:input];
    }
    else
    {
        NSLog(@"Couldn't get video device");
    }
}

- (BOOL) startSession
{

    if (!session)
    {
        NSLog(@"Session is nil");
        return false;
    }
        
    if (![session isRunning]) [session startRunning];
    
    return true;
}

- (void) endSession
{
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
        NSLog(@"Session is nil");
        return false;
    }
    
    if ([session canAddOutput:output])
    {
        [session addOutput:output];
    }
    else
    {
        NSLog(@"Can't add output to session");
        return false;
    }
    
    return true;
}

- (void) handlePause
{
    [self endSession];
}

- (void) handleTerminate
{
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
