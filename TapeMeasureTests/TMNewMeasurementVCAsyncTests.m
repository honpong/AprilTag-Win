//
//  TMNewMeasurementVCTests.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVCAsyncTests.h"
#import <OCMock.h>
#import "RCCore/RCAVSessionManagerFactory.h"
#import "RCCore/RCVideoCapManagerFactory.h"
#import "RCCore/RCMotionCapManagerFactory.h"
#import "RCPimManagerFactory.h"
#import "TMConstants.h"

#define POLL_INTERVAL 0.05 //50ms
#define N_SEC_TO_POLL 2.0 //in seconds
#define MAX_POLL_COUNT N_SEC_TO_POLL / POLL_INTERVAL

@implementation TMNewMeasurementVCAsyncTests

- (void) setUp
{
    [super setUp];
    
    id sessionMan = [OCMockObject niceMockForProtocol:@protocol(RCAVSessionManager)];
    [[[sessionMan stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
    [RCAVSessionManagerFactory setInstance:sessionMan];
    
    id videoMan = [OCMockObject mockForProtocol:@protocol(RCVideoCapManager)];
    [RCVideoCapManagerFactory setInstance:videoMan];
    
    id motionMan = [OCMockObject mockForProtocol:@protocol(RCMotionCapManager)];
    [RCMotionCapManagerFactory setMotionCapManagerInstance:motionMan];
    
    id corvisMan = [OCMockObject niceMockForProtocol:@protocol(RCPimManager)];
    [RCPimManagerFactory setInstance:corvisMan];
    
    UIStoryboard *storyboard = [UIStoryboard storyboardWithName:@"MainStoryboard_iPhone" bundle:nil];
    nav = [storyboard instantiateViewControllerWithIdentifier:@"NavController"];
    vc = [storyboard instantiateViewControllerWithIdentifier:@"NewMeasurement"];
    [nav pushViewController:vc animated:NO];
    vc.type = TypePointToPoint;
}

- (void) tearDown
{
    [RCAVSessionManagerFactory setInstance:nil];
    [RCVideoCapManagerFactory setInstance:nil];
    [RCMotionCapManagerFactory setMotionCapManagerInstance:nil];
    [RCPimManagerFactory setInstance:nil];
    nav = nil;
    vc = nil;
    [super tearDown];
}

- (void) measurementStart
{
    id videoMan = [RCVideoCapManagerFactory getInstance];
    id motionMan = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    id corvisMan = [RCPimManagerFactory getInstance];
    
    [(id<RCVideoCapManager>)[videoMan expect]  startVideoCap];
    [(id<RCMotionCapManager>)[motionMan expect]  startMotionCap];
    
    CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
    [(id<RCPimManager>)[corvisMan expect]
     setupPluginsWithFilter:false
     withCapture:true
     withReplay:false
     withLocationValid:loc ? true : false
     withLatitude:loc ? loc.coordinate.latitude : 0
     withLongitude:loc ? loc.coordinate.longitude : 0
     withAltitude:loc ? loc.altitude : 0
     withUpdateProgress:NULL
     withUpdateMeasurement:NULL
     withCallbackObject:NULL];
    
    [(id<RCPimManager>)[corvisMan expect] startPlugins];
    
    [vc loadView];
    [vc handleResume];
    [vc startMeasuring];
    
    [videoMan verify];
    [motionMan verify];
    [corvisMan verify];
    
    STAssertTrue(vc.isCapturingData, nil);
    STAssertFalse(vc.isProcessingData, nil);
    STAssertFalse(vc.isMeasurementComplete, nil);
    STAssertFalse(vc.isMeasurementCanceled, nil);
}

- (void) resumeAfterPausedAndCanceledMeasurement
{
    id corvisMan = [RCPimManagerFactory getInstance];
    
    CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
    [(id<RCPimManager>)[corvisMan expect]
     setupPluginsWithFilter:false
     withCapture:true
     withReplay:false
     withLocationValid:loc ? true : false
     withLatitude:loc ? loc.coordinate.latitude : 0
     withLongitude:loc ? loc.coordinate.longitude : 0
     withAltitude:loc ? loc.altitude : 0
     withUpdateProgress:NULL
     withUpdateMeasurement:NULL
     withCallbackObject:NULL];
    
    [vc handleResume];
    [corvisMan verify];
    
    STAssertFalse(vc.isMeasurementCanceled, nil);
    STAssertFalse(vc.isCapturingData, nil);
    STAssertFalse(vc.isProcessingData, nil);
    STAssertFalse(vc.isMeasurementComplete, nil);
}

- (void) testNormalMeasurement
{
    [self measurementStart];
    
    id videoMan = [RCVideoCapManagerFactory getInstance];
    id motionMan = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    id corvisMan = [RCPimManagerFactory getInstance];
            
    [(id<RCVideoCapManager>)[videoMan expect]  stopVideoCap];
    [(id<RCMotionCapManager>)[motionMan expect]  stopMotionCap];
    [(id<RCPimManager>)[corvisMan expect] stopPlugins];
    [(id<RCPimManager>)[corvisMan expect] teardownPlugins];
//    [(id<RCPimManager>)[corvisMan expect]
//     setupPluginsWithFilter:true
//     withCapture:false
//     withReplay:true
//     withLocationValid:loc ? true : false
//     withLatitude:loc ? loc.coordinate.latitude : 0
//     withLongitude:loc ? loc.coordinate.longitude : 0
//     withAltitude:loc ? loc.altitude : 0
//     withUpdateProgress:TMNewMeasurementVCUpdateProgress
//     withUpdateMeasurement:TMNewMeasurementVCUpdateMeasurement
//     withCallbackObject:(__bridge void *)vc];
    [(id<RCPimManager>)[corvisMan expect] startPlugins];
    
    [vc stopMeasuring];
    
    NSLog(@"started poll");
    int pollCount = 0;
    
    while (vc.isProcessingData == NO && pollCount < MAX_POLL_COUNT) {
        NSLog(@"polling... %i", pollCount);
        NSDate* untilDate = [NSDate dateWithTimeIntervalSinceNow:POLL_INTERVAL];
        [[NSRunLoop currentRunLoop] runUntilDate:untilDate];
        pollCount++;
    }
    if (pollCount == MAX_POLL_COUNT) STFail(@"polling timed out");
    
    [videoMan verify];
    [motionMan verify];
    [corvisMan verify];
    
    STAssertFalse(vc.isCapturingData, nil);
    STAssertTrue(vc.isProcessingData, nil);
    STAssertFalse(vc.isMeasurementComplete, nil);
    STAssertFalse(vc.isMeasurementCanceled, nil);
    
    [(id<RCPimManager>)[corvisMan expect] teardownPlugins];
    
//    [vc processingFinished];
    
    [corvisMan verify];
        
    STAssertFalse(vc.isCapturingData, nil);
    STAssertFalse(vc.isProcessingData, nil);
    STAssertTrue(vc.isMeasurementComplete, nil);
    STAssertFalse(vc.isMeasurementCanceled, nil);
    
    //test that we keep the finished measurement after pause/resume
    [vc handlePause];
    [vc handleResume];
    
    STAssertFalse(vc.isCapturingData, nil);
    STAssertFalse(vc.isProcessingData, nil);
    STAssertTrue(vc.isMeasurementComplete, nil);
    STAssertFalse(vc.isMeasurementCanceled, nil);
}

- (void) testPauseWhileCapturingThenResume
{
    [self measurementStart];
    
    id videoMan = [RCVideoCapManagerFactory getInstance];
    id motionMan = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    id corvisMan = [RCPimManagerFactory getInstance];
            
    [(id<RCVideoCapManager>)[videoMan expect]  stopVideoCap];
    [(id<RCMotionCapManager>)[motionMan expect]  stopMotionCap];
    [(id<RCPimManager>)[corvisMan expect] stopPlugins];
    [(id<RCPimManager>)[corvisMan expect] teardownPlugins];
    
    [vc handlePause];
    
    STAssertFalse(vc.isCapturingData, nil);
    
    [videoMan verify];
    [motionMan verify];
    [corvisMan verify];
    
    STAssertTrue(vc.isMeasurementCanceled, nil);
    STAssertFalse(vc.isCapturingData, nil);
    
    [self resumeAfterPausedAndCanceledMeasurement];
}

- (void) testPauseWhileProcessingThenResume
{
    [self measurementStart];
    
    id videoMan = [RCVideoCapManagerFactory getInstance];
    id motionMan = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    id corvisMan = [RCPimManagerFactory getInstance];
    
    [(id<RCVideoCapManager>)[videoMan expect]  stopVideoCap];
    [(id<RCMotionCapManager>)[motionMan expect]  stopMotionCap];
    [(id<RCPimManager>)[corvisMan expect] stopPlugins];
    [(id<RCPimManager>)[corvisMan expect] teardownPlugins];
    //    [(id<RCPimManager>)[corvisMan expect]
    //     setupPluginsWithFilter:true
    //     withCapture:false
    //     withReplay:true
    //     withLocationValid:loc ? true : false
    //     withLatitude:loc ? loc.coordinate.latitude : 0
    //     withLongitude:loc ? loc.coordinate.longitude : 0
    //     withAltitude:loc ? loc.altitude : 0
    //     withUpdateProgress:TMNewMeasurementVCUpdateProgress
    //     withUpdateMeasurement:TMNewMeasurementVCUpdateMeasurement
    //     withCallbackObject:(__bridge void *)vc];
    [(id<RCPimManager>)[corvisMan expect] startPlugins];
    
    [vc stopMeasuring];
    
    NSLog(@"started poll");
    int pollCount = 0;
    
    while (vc.isProcessingData == NO && pollCount < MAX_POLL_COUNT) {
        NSLog(@"polling... %i", pollCount);
        NSDate* untilDate = [NSDate dateWithTimeIntervalSinceNow:POLL_INTERVAL];
        [[NSRunLoop currentRunLoop] runUntilDate:untilDate];
        pollCount++;
    }
    if (pollCount == MAX_POLL_COUNT) STFail(@"polling timed out");
    
    [videoMan verify];
    [motionMan verify];
    [corvisMan verify];
    
    STAssertFalse(vc.isCapturingData, nil);
    STAssertTrue(vc.isProcessingData, nil);
    STAssertFalse(vc.isMeasurementComplete, nil);
    STAssertFalse(vc.isMeasurementCanceled, nil);
    
    [(id<RCPimManager>)[corvisMan expect] stopPlugins];
    [(id<RCPimManager>)[corvisMan expect] teardownPlugins];
    
    [vc handlePause];
    
    [corvisMan verify];
    
    STAssertFalse(vc.isCapturingData, nil);
    STAssertFalse(vc.isProcessingData, nil);
    STAssertFalse(vc.isMeasurementComplete, nil);
    STAssertTrue(vc.isMeasurementCanceled, nil);
    
    [self resumeAfterPausedAndCanceledMeasurement];
}

@end
