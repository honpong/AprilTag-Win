//
//  RCVideoCapManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 1/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoCapManagerFactoryTests.h"
#import "RCVideoManager.h"
#import <OCMock.h>

@implementation RCVideoCapManagerFactoryTests

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void)tearDown
{
    [RCVideoManager setInstance:nil];
    
    [super tearDown];
}

- (void)testGetInstanceWithoutSetupFails
{
    id<RCVideoCapManager> videoMan = [RCVideoManager sharedInstance];
    
    STAssertNil(videoMan, @"Video cap instance requested without setup first");
}

- (void)testReturnsSameInstance
{
    id mockSession = [OCMockObject niceMockForClass:[AVCaptureSession class]];
    
    [RCVideoManager setupVideoCapWithSession:mockSession];
    
    id<RCVideoCapManager> videoMan1 = [RCVideoManager sharedInstance];
    id<RCVideoCapManager> videoMan2 = [RCVideoManager sharedInstance];
    
    STAssertEqualObjects(videoMan1, videoMan2, @"Get instance failed to return the same instance");
}

- (void)testSetInstance
{
    id videoMan1 = [OCMockObject mockForProtocol:@protocol(RCVideoCapManager)];

    [RCVideoManager setInstance:videoMan1];
    
    id videoMan2 = [RCVideoManager sharedInstance];
    
    STAssertEqualObjects(videoMan1, videoMan2, @"Get instance failed to return the same instance after set instance was called");
}

- (void)testStartCap
{
    id mockSession = [OCMockObject mockForClass:[AVCaptureSession class]];
    [(AVCaptureSession*)[mockSession expect]  addOutput:[OCMArg any]];
    [[[mockSession stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
        
    id mockOutput = [OCMockObject mockForClass:[AVCaptureVideoDataOutput class]];
    [(AVCaptureVideoDataOutput*)[mockOutput expect]  setAlwaysDiscardsLateVideoFrames:NO];
    [(AVCaptureVideoDataOutput*)[mockOutput expect]  setVideoSettings:[OCMArg any]];
    [(AVCaptureVideoDataOutput*)[mockOutput expect]  setSampleBufferDelegate:[OCMArg any] queue:(__bridge dispatch_queue_t)([OCMArg isNotNil])];
    
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCPimManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isSensorFusionRunning];

    [RCVideoManager setupVideoCapWithSession:mockSession withOutput:mockOutput withSensorFusion:mockCorvisMan];
    
    id<RCVideoCapManager> videoMan = [RCVideoManager sharedInstance];
    
    STAssertTrue([videoMan startVideoCapture], @"Failed to start video cap");
    
    STAssertTrue([videoMan isCapturing], @"isCapturing returned false after started");
    
    [mockSession verify];
    [mockOutput verify];
}

- (void)testStartCapFailsIfPluginsNotStarted
{
    id mockSession = [OCMockObject niceMockForClass:[AVCaptureSession class]];
    [[[mockSession stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
    
    id mockOutput = [OCMockObject niceMockForClass:[AVCaptureVideoDataOutput class]];
       
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCPimManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL) {NO})] isSensorFusionRunning];

    [RCVideoManager setupVideoCapWithSession:mockSession withOutput:mockOutput withSensorFusion:mockCorvisMan];
    
    id<RCVideoCapManager> videoMan = [RCVideoManager sharedInstance];
    
    STAssertFalse([videoMan startVideoCapture], @"Video cap started while corvis plugins not started");
}

- (void)testStartCapFailsIfSessionNotStarted
{
    id mockSession = [OCMockObject niceMockForClass:[AVCaptureSession class]];
    [[[mockSession stub] andReturnValue:OCMOCK_VALUE((BOOL){NO})] isRunning];
    
    id mockOutput = [OCMockObject niceMockForClass:[AVCaptureVideoDataOutput class]];
    
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCPimManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isSensorFusionRunning];

    [RCVideoManager setupVideoCapWithSession:mockSession withOutput:mockOutput withSensorFusion:mockCorvisMan];
    
    id<RCVideoCapManager> videoMan = [RCVideoManager sharedInstance];
    
    STAssertFalse([videoMan startVideoCapture], @"Video cap started while session not started");
}

- (void)testStopCap
{
    id mockSession = [OCMockObject niceMockForClass:[AVCaptureSession class]];
    [[[mockSession stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
    
    id mockOutput = [OCMockObject niceMockForClass:[AVCaptureVideoDataOutput class]];
    
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCPimManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isSensorFusionRunning];

    [RCVideoManager setupVideoCapWithSession:mockSession withOutput:mockOutput withSensorFusion:mockCorvisMan];
    
    id<RCVideoCapManager> videoMan = [RCVideoManager sharedInstance];
    
    STAssertTrue([videoMan startVideoCapture], @"Failed to start video cap");
    
    [(AVCaptureVideoDataOutput*)[mockOutput expect]  setSampleBufferDelegate:[OCMArg isNil] queue:(__bridge dispatch_queue_t)([OCMArg isNil])];

    [videoMan stopVideoCapture];
        
    STAssertFalse([videoMan isCapturing], @"isCapturing returned true after stopped");
    
    [mockOutput verify];
}

@end
