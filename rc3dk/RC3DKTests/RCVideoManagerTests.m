//
//  RCVideoCapManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 1/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoManagerTests.h"
#import "RCVideoManager.h"
#import "OCMock.h"
#import "RCSensorFusion+RCSensorFusionMock.h"

@implementation RCVideoManagerTests

- (void)setUp
{
    [super setUp];
    // Set-up code here.
}

- (void)tearDown
{
    [super tearDown];
    [RCSensorFusion tearDownMock];
}

- (void)testReturnsSameInstance
{
    RCVideoManager* videoMan1 = [RCVideoManager sharedInstance];
    RCVideoManager* videoMan2 = [RCVideoManager sharedInstance];
    
    STAssertEqualObjects(videoMan1, videoMan2, @"Get instance failed to return the same instance");
}

- (void)testStartCap
{
    id mockSession = [OCMockObject mockForClass:[AVCaptureSession class]];
    [(AVCaptureSession*)[mockSession expect]  addOutput:[OCMArg any]];
    [[[mockSession stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
        
    id mockOutput = [OCMockObject niceMockForClass:[AVCaptureVideoDataOutput class]];
    [(AVCaptureVideoDataOutput*)[mockOutput expect]  setSampleBufferDelegate:[OCMArg any] queue:(dispatch_queue_t)([OCMArg isNotNil])];
    
    [RCSensorFusion setupNiceMock];
    id mockSensorFusion = [RCSensorFusion sharedInstance];
    [[[mockSensorFusion stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isSensorFusionRunning];

    RCVideoManager* videoMan = [RCVideoManager sharedInstance];
    [videoMan setupWithSession:mockSession withOutput:mockOutput];
    
    [mockOutput verify];
    
    STAssertNotNil(videoMan, @"RCVideoManager shared instance was nil");
    STAssertTrue([videoMan startVideoCapture], @"Failed to start video cap");
    STAssertTrue([videoMan isCapturing], @"isCapturing returned false after started");
}

- (void)testStartCapFailsIfPluginsNotStarted
{
    id mockSession = [OCMockObject niceMockForClass:[AVCaptureSession class]];
    [[[mockSession stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
    
    [RCSensorFusion setupNiceMock];
    id mockSensorFusion = [RCSensorFusion sharedInstance];
    [[[mockSensorFusion stub] andReturnValue:OCMOCK_VALUE((BOOL) {NO})] isSensorFusionRunning];

    [[RCVideoManager sharedInstance] setupWithSession:mockSession withOutput:[OCMockObject niceMockForClass:[AVCaptureVideoDataOutput class]]];
    RCVideoManager* videoMan = [RCVideoManager sharedInstance];
    
    STAssertFalse([videoMan startVideoCapture], @"Video cap started while filter plugins not started");
}

- (void)testStartCapFailsIfSessionNotStarted
{
    [RCSensorFusion setupNiceMock];
    id mockSensorFusion = [RCSensorFusion sharedInstance];
    [[[mockSensorFusion stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isSensorFusionRunning];

    [[RCVideoManager sharedInstance] setupWithSession:[AVCaptureSession new] withOutput:[OCMockObject niceMockForClass:[AVCaptureVideoDataOutput class]]];
    RCVideoManager* videoMan = [RCVideoManager sharedInstance];
    
    STAssertFalse([videoMan startVideoCapture], @"Video cap started while session not started");
}

- (void)testStopCap
{
    id mockSession = [OCMockObject niceMockForClass:[AVCaptureSession class]];
    [[[mockSession stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
    
    [RCSensorFusion setupNiceMock];
    id mockSensorFusion = [RCSensorFusion sharedInstance];
    [[[mockSensorFusion stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isSensorFusionRunning];
    
    [[RCVideoManager sharedInstance] setupWithSession:mockSession withOutput:[OCMockObject niceMockForClass:[AVCaptureVideoDataOutput class]]];
    RCVideoManager* videoMan = [RCVideoManager sharedInstance];
    
    STAssertTrue([videoMan startVideoCapture], @"Failed to start video cap");

    [videoMan stopVideoCapture];
        
    STAssertFalse([videoMan isCapturing], @"isCapturing returned true after stopped");
}

@end
