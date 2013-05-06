//
//  TMAvSessionManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCAVSessionManagerFactory.h"

@interface RCAVSessionManagerImpl : NSObject <RCAVSessionManager>
@property AVCaptureSession *session;
@property AVCaptureVideoPreviewLayer *videoPreviewLayer;
@property AVCaptureDevice *videoDevice;
@end

@implementation RCAVSessionManagerImpl

@synthesize session, videoPreviewLayer, videoDevice;

- (id)init
{
    self = [super init];
    
    if (self)
    {
        NSLog(@"RCAVSessionManagerImpl init");
        
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
        
        videoPreviewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
    }
    
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)addInputToSession
{
    videoDevice = [self cameraWithPosition:AVCaptureDevicePositionBack];

    if ([videoDevice lockForConfiguration:nil]) {
        if([videoDevice isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus])
            [videoDevice setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
        if([videoDevice isExposureModeSupported:AVCaptureExposureModeAutoExpose])
            [videoDevice setExposureMode:AVCaptureExposureModeAutoExpose];
        if([videoDevice isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeAutoWhiteBalance])
            [videoDevice setWhiteBalanceMode:AVCaptureWhiteBalanceModeAutoWhiteBalance];
        [videoDevice unlockForConfiguration];
        NSLog(@"Camera modes initialized");
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

- (void)lockFocus
{
    NSLog(@"lockFocus");
    if ([videoDevice lockForConfiguration:nil]) {
        if([videoDevice isFocusModeSupported:AVCaptureFocusModeLocked])
            [videoDevice setFocusMode:AVCaptureFocusModeLocked];
        [videoDevice unlockForConfiguration];
    }
}

- (void)unlockFocus
{
    NSLog(@"unlockFocus");
    if ([videoDevice lockForConfiguration:nil]) {
        if([videoDevice isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus])
            [videoDevice setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
        [videoDevice unlockForConfiguration];
    }

}

- (BOOL)startSession
{
    NSLog(@"RCAVSessionManagerImpl startSession");
    
    if (!session)
    {
        NSLog(@"Session is nil");
        return false;
    }
        
    if (![session isRunning]) [session startRunning];
    
    return true;
}

- (void)endSession
{
    NSLog(@"RCAVSessionManagerImpl endSession");
    if ([session isRunning]) [session stopRunning];
}

- (BOOL)isRunning
{
    return session ? session.isRunning : false;
}

- (BOOL)addOutput:(AVCaptureVideoDataOutput*)output
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

- (bool) isImageClean
{
    if(videoDevice.adjustingFocus) {
        NSLog(@"Adjusting focus");
        return false;
    }
    /*if(videoDevice.adjustingWhiteBalance) {
        NSLog(@"Adjusting white balance");
        return false;
    }
    if(videoDevice.adjustingExposure) {
        NSLog(@"Adjusting exposure");
        return false;
    }*/
    return true;
}

- (void)handlePause
{
    NSLog(@"RCAVSessionManagerImpl handlePause");
    [self endSession];    
}

- (void)handleTerminate
{
    NSLog(@"RCAVSessionManagerImpl handleTerminate");
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

@implementation RCAVSessionManagerFactory

static id<RCAVSessionManager> instance;

+ (void)setupAVSession
{
    if (!instance)
    {
        instance = [[RCAVSessionManagerImpl alloc] init];
    }
}

+ (id<RCAVSessionManager>)getAVSessionManagerInstance
{
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setAVSessionManagerInstance:(id<RCAVSessionManager>)mockObject
{
    instance = mockObject;
}

@end
