//
//  TMNewMeasurementVCTests.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVCAsyncTests.h"
#import <OCMock.h>
#import "RCAVSessionManager.h"
#import "RCVideoManager.h"
#import "RCMotionManager.h"
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
    [RCAVSessionManager setInstance:sessionMan];
    
    id videoMan = [OCMockObject mockForProtocol:@protocol(RCVideoCapManager)];
    [RCVideoManager setInstance:videoMan];
    
    id motionMan = [OCMockObject mockForProtocol:@protocol(RCMotionCapManager)];
    [RCMotionManager setMotionCapManagerInstance:motionMan];
    
    id corvisMan = [OCMockObject niceMockForProtocol:@protocol(RCPimManager)];
    [RCSensorFusion setInstance:corvisMan];
    
    UIStoryboard *storyboard = [UIStoryboard storyboardWithName:@"MainStoryboard_iPhone" bundle:nil];
    nav = [storyboard instantiateViewControllerWithIdentifier:@"NavController"];
    vc = [storyboard instantiateViewControllerWithIdentifier:@"NewMeasurement"];
    [nav pushViewController:vc animated:NO];
    vc.type = TypePointToPoint;
}

- (void) tearDown
{
    [RCAVSessionManager setInstance:nil];
    [RCVideoManager setInstance:nil];
    [RCMotionManager setMotionCapManagerInstance:nil];
    [RCSensorFusion setInstance:nil];
    nav = nil;
    vc = nil;
    [super tearDown];
}

- (void) measurementStart
{
    id videoMan = [RCVideoManager sharedInstance];
    id motionMan = [RCMotionManager getMotionCapManagerInstance];
    id corvisMan = [RCSensorFusion sharedInstance];

    [(id <RCVideoCapManager>) [videoMan expect] startVideoCapture];
    [(id <RCMotionCapManager>) [motionMan expect] startMotionCapture];
    
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
    id corvisMan = [RCSensorFusion sharedInstance];
    
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
    
    id videoMan = [RCVideoManager sharedInstance];
    id motionMan = [RCMotionManager getMotionCapManagerInstance];
    id corvisMan = [RCSensorFusion sharedInstance];

    [(id <RCVideoCapManager>) [videoMan expect] stopVideoCapture];
    [(id <RCMotionCapManager>) [motionMan expect] stopMotionCapture];
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
    
    DLog(@"started poll");
    int pollCount = 0;
    
    while (vc.isProcessingData == NO && pollCount < MAX_POLL_COUNT) {
        DLog(@"polling... %i", pollCount);
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
    
    id videoMan = [RCVideoManager sharedInstance];
    id motionMan = [RCMotionManager getMotionCapManagerInstance];
    id corvisMan = [RCSensorFusion sharedInstance];

    [(id <RCVideoCapManager>) [videoMan expect] stopVideoCapture];
    [(id <RCMotionCapManager>) [motionMan expect] stopMotionCapture];
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
    
    id videoMan = [RCVideoManager sharedInstance];
    id motionMan = [RCMotionManager getMotionCapManagerInstance];
    id corvisMan = [RCSensorFusion sharedInstance];

    [(id <RCVideoCapManager>) [videoMan expect] stopVideoCapture];
    [(id <RCMotionCapManager>) [motionMan expect] stopMotionCapture];
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
    
    DLog(@"started poll");
    int pollCount = 0;
    
    while (vc.isProcessingData == NO && pollCount < MAX_POLL_COUNT) {
        DLog(@"polling... %i", pollCount);
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
