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
@end

@implementation RCAVSessionManagerImpl

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
//        [[NSNotificationCenter defaultCenter] addObserver:self
//                                                 selector:@selector(handleResume)
//                                                     name:UIApplicationDidBecomeActiveNotification
//                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleTerminate)
                                                     name:UIApplicationWillTerminateNotification
                                                   object:nil];
    }
    
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)setupAVSession
{
    if (!session)
    {
        NSLog(@"TMAVSessionManager.createAndConfigAVSession");
        
        session = [[AVCaptureSession alloc] init];
        
        [session beginConfiguration];
        [session setSessionPreset:AVCaptureSessionPreset640x480];
        
        [self addInputToSession];
        
        [session commitConfiguration];
        
        videoPreviewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
    }
}

- (void)addInputToSession
{
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

- (BOOL)startSession
{
    NSLog(@"TMAVSessionManager.startSession");
    
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
    NSLog(@"TMAVSessionManager.endSession");
    
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

@implementation RCAVSessionManagerFactory

static id<RCAVSessionManager> instance;

+ (id<RCAVSessionManager>)getAVSessionManagerInstance
{
    if (instance == nil)
    {
        instance = [[RCAVSessionManagerImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setAVSessionManagerInstance:(id<RCAVSessionManager>)mockObject
{
    instance = mockObject;
}

@end
