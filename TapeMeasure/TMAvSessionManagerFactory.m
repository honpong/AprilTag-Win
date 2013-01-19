//
//  TMAvSessionManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMAvSessionManagerFactory.h"

@interface TMAVSessionManagerImpl : NSObject <TMAVSessionManager>
@property AVCaptureSession *session;
@property AVCaptureVideoPreviewLayer *videoPreviewLayer;
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

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

/** Called automatically when instance is created. This is available as a method just so that you can force the session to be instantiated
 and configured (which is expensive and causes lag) before you actually call startSession. Another way to get the same effect would be to 
 simply call getAVSessionManagerInstance.
 */
- (void)createAndConfigAVSession
{
    NSLog(@"TMAVSessionManager.createAndConfigAVSession");
    
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
    if (![session isRunning]) [session startRunning];
}

- (void)endSession
{
    NSLog(@"TMAVSessionManager.endSession");
    if ([session isRunning]) [session stopRunning];
}

- (bool)isRunning
{
    return session.isRunning;
}

- (void)addOutput:(AVCaptureVideoDataOutput*)output
{
    [session addOutput:output];
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

static id<TMAVSessionManager> instance;

+ (id<TMAVSessionManager>)getAVSessionManagerInstance
{
    if (instance == nil)
    {
        instance = [[TMAVSessionManagerImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setAVSessionManagerInstance:(id<TMAVSessionManager>)mockObject
{
    instance = mockObject;
}

@end
