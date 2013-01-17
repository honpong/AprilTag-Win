//
//  TMAvSessionManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMAvSessionManagerFactory.h"
#import <AVFoundation/AVFoundation.h>

@interface TMAVSessionManagerImpl : NSObject <TMAVSessionManager>

@property AVCaptureSession *session;
@property AVCaptureVideoPreviewLayer *videoPreviewLayer;

- (void)startSession;
- (void)endSession;
- (BOOL)isRunning;

@end

@implementation TMAVSessionManagerImpl

@synthesize session, videoPreviewLayer;

- (id)init
{
    self = [super init];
    
    if (self)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handlePause)
                                                     name:UIApplicationWillResignActiveNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleResume)
                                                     name:UIApplicationDidBecomeActiveNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleTerminate)
                                                     name:UIApplicationWillTerminateNotification
                                                   object:nil];
        
        //expensive
        [self createAndConfigAVSession];
        videoPreviewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
    }
    
    return self;
}

- (void)createAndConfigAVSession
{
    session = [[AVCaptureSession alloc] init];
    
    [session beginConfiguration];
    [session setSessionPreset:AVCaptureSessionPreset640x480];
    
    AVCaptureDevice * videoDevice = [self cameraWithPosition:AVCaptureDevicePositionFront];
    if (videoDevice == nil) videoDevice = [self cameraWithPosition:AVCaptureDevicePositionBack]; //TODO: remove later. for testing on 3Gs.
    
    /*[AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
     
     // SETUP FOCUS MODE
     if ([videoDevice lockForConfiguration:nil]) {
     [videoDevice setFocusMode:AVCaptureFocusModeLocked];
     NSLog(@"Focus mode locked");
     }
     else{
     NSLog(@"error while configuring focusMode");
     }*/
    
    NSError *error;
    AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error]; //TODO: handle error
    if (error) NSLog(@"Error getting AVCaptureDeviceInput object: %@", error.localizedFailureReason);
    
    [session addInput:input];
    [session commitConfiguration];
}

- (void)startSession
{
    NSLog(@"TMAVSessionManager.startSession");
    [session startRunning];
}

- (void)endSession
{
    NSLog(@"TMAVSessionManager.endSession");
    [session stopRunning];
}

- (BOOL)isRunning
{
    return session.isRunning;
}

- (void)handlePause
{
    NSLog(@"TMAVSessionManager.handlePause");
    [self endSession];    
}

- (void)handleResume
{
    NSLog(@"TMAVSessionManager.handleResume");
    [self startSession];
}

- (void)handleTerminate
{
    NSLog(@"TMAVSessionManager.handleTerminate");
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

@implementation TMAvSessionManagerFactory

static TMAVSessionManagerImpl *sman;

+ (id)getAVSessionManagerInstance
{
    if (sman == nil)
    {
        sman = [[TMAVSessionManagerImpl alloc] init];
    }
    
    return sman;
}

@end
