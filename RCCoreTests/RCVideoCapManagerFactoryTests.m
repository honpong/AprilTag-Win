//
//  RCVideoCapManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 1/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoCapManagerFactoryTests.h"
#import "RCVideoCapManagerFactory.h"
#import <OCMock.h>

@implementation RCVideoCapManagerFactoryTests

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void)tearDown
{
    [RCVideoCapManagerFactory setVideoCapManagerInstance:nil];
    
    [super tearDown];
}

- (void)testStartCap
{
    id<RCVideoCapManager> videoMan = [RCVideoCapManagerFactory getVideoCapManagerInstance];
    
    id mockSession = [OCMockObject mockForClass:[AVCaptureSession class]];
    [(AVCaptureSession*)[mockSession expect]  addOutput:[OCMArg any]];
    [[[mockSession stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
        
    id mockOutput = [OCMockObject mockForClass:[AVCaptureVideoDataOutput class]];
    [(AVCaptureVideoDataOutput*)[mockOutput expect]  setAlwaysDiscardsLateVideoFrames:NO];
    [(AVCaptureVideoDataOutput*)[mockOutput expect]  setVideoSettings:[OCMArg any]];
    [(AVCaptureVideoDataOutput*)[mockOutput expect]  setSampleBufferDelegate:[OCMArg any] queue:(__bridge dispatch_queue_t)([OCMArg isNotNil])];
    
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCCorvisManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isPluginsStarted];
    
    [videoMan setupVideoCapWithSession:mockSession withOutput:mockOutput withCorvisManager:mockCorvisMan];
        
    STAssertTrue([videoMan startVideoCap], @"Failed to start video cap");
    
    [mockSession verify];
    [mockOutput verify];
}

- (void)testStopCap
{
    id<RCVideoCapManager> videoMan = [RCVideoCapManagerFactory getVideoCapManagerInstance];
    
    id mockSession = [OCMockObject niceMockForClass:[AVCaptureSession class]];
    [[[mockSession stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
    
    id mockOutput = [OCMockObject niceMockForClass:[AVCaptureVideoDataOutput class]];
    
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCCorvisManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isPluginsStarted];
    
    [videoMan setupVideoCapWithSession:mockSession withOutput:mockOutput withCorvisManager:mockCorvisMan];
    
    STAssertTrue([videoMan startVideoCap], @"Failed to start video cap");
    
    [(AVCaptureVideoDataOutput*)[mockOutput expect]  setSampleBufferDelegate:[OCMArg isNil] queue:(__bridge dispatch_queue_t)([OCMArg isNil])];
    
    [videoMan stopVideoCap];
    
    [mockOutput verify];
}

- (void)testStartWithoutSetupReturnsFalse
{
    id<RCVideoCapManager> videoMan = [RCVideoCapManagerFactory getVideoCapManagerInstance];
    
    STAssertFalse([videoMan startVideoCap], @"Video cap started without setup first");
}

@end
