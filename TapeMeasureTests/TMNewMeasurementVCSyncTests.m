//
//  TMNewMeasurementVCSyncTests.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVCSyncTests.h"
#import <OCMock.h>
#import "RCAVSessionManager.h"
#import "RCVideoManager.h"
#import "RCMotionManager.h"
#import "RCPimManagerFactory.h"
#import "TMConstants.h"

#define POLL_INTERVAL 0.05 //50ms
#define N_SEC_TO_POLL 2.0 //in seconds
#define MAX_POLL_COUNT N_SEC_TO_POLL / POLL_INTERVAL

@implementation TMNewMeasurementVCSyncTests

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
    
//    CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
//    [(id<RCPimManager>)[corvisMan expect]
//     setupPluginsWithFilter:false
//     withCapture:true
//     withReplay:false
//     withLocationValid:loc ? true : false
//     withLatitude:loc ? loc.coordinate.latitude : 0
//     withLongitude:loc ? loc.coordinate.longitude : 0
//     withAltitude:loc ? loc.altitude : 0
//     withUpdateProgress:NULL
//     withUpdateMeasurement:NULL
//     withCallbackObject:NULL];
//    [corvisMan verify];
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
    
    [(id<RCPimManager>)[corvisMan expect] startPlugins];
    [(id <RCVideoCapManager>) [videoMan expect] startVideoCapture];
    [(id <RCMotionCapManager>) [motionMan expect] startMotionCapture];
    
    [vc loadView];
    [vc handleStateEvent:EV_RESUME];
    
    [corvisMan verify];
    [videoMan verify];
    [motionMan verify];

    [(id <RCPimManager>) [corvisMan expect] markStart];
    [vc handleStateEvent:EV_TAP];
    
    [corvisMan verify];
}

//- (void) resumeAfterPausedAndCanceledMeasurement
//{
////    id corvisMan = [RCSensorFusion sharedInstance];
//    
////    CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
////    [(id<RCPimManager>)[corvisMan expect]
////     setupPluginsWithFilter:false
////     withCapture:true
////     withReplay:false
////     withLocationValid:loc ? true : false
////     withLatitude:loc ? loc.coordinate.latitude : 0
////     withLongitude:loc ? loc.coordinate.longitude : 0
////     withAltitude:loc ? loc.altitude : 0
////     withUpdateProgress:NULL
////     withUpdateMeasurement:NULL
////     withCallbackObject:NULL];
//    
//    [vc handleResume];
////    [corvisMan verify];
//}

- (void) testNormalMeasurement
{
    [self measurementStart];
    
    id videoMan = [RCVideoManager sharedInstance];
    id motionMan = [RCMotionManager getMotionCapManagerInstance];
    id corvisMan = [RCSensorFusion sharedInstance];

    [(id <RCPimManager>) [corvisMan expect] markStop];

    [(id <RCVideoCapManager>) [videoMan expect] stopVideoCapture];
    [(id <RCMotionCapManager>) [motionMan expect] stopMotionCapture];
    [(id<RCPimManager>)[corvisMan expect] stopPlugins];
    [(id<RCPimManager>)[corvisMan expect] teardownPlugins];
    
    [vc handleStateEvent:EV_TAP];
        
    //wait a bit
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:2];
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if([timeoutDate timeIntervalSinceNow] < 0.0)
            break;
    } while (true);
    
    [videoMan verify];
    [motionMan verify];
    [corvisMan verify];
}

//- (void) testPauseWhileCapturingThenResume
//{
//    [self measurementStart];
//    
//    id videoMan = [RCVideoManager sharedInstance];
//    id motionMan = [RCMotionManager getMotionCapManagerInstance];
//    id corvisMan = [RCSensorFusion sharedInstance];
//    
//    [(id<RCVideoCapManager>)[videoMan expect]  stopVideoCapture];
//    [(id<RCMotionCapManager>)[motionMan expect]  stopMotionCapture];
//    [(id<RCPimManager>)[corvisMan expect] stopPlugins];
//    [(id<RCPimManager>)[corvisMan expect] teardownPlugins];
//    
//    [vc handlePause];
//    
//    NSLog(@"started poll");
//    int pollCount = 0;
//    
//    while (vc.isCapturingData == YES && pollCount < MAX_POLL_COUNT) {
//        NSLog(@"polling... %i", pollCount);
//        NSDate* untilDate = [NSDate dateWithTimeIntervalSinceNow:POLL_INTERVAL];
//        [[NSRunLoop currentRunLoop] runUntilDate:untilDate];
//        pollCount++;
//    }
//    if (pollCount == MAX_POLL_COUNT) STFail(@"polling timed out");
//    
//    STAssertFalse(vc.isCapturingData, nil);
//    
//    [videoMan verify];
//    [motionMan verify];
//    [corvisMan verify];
//    
//    STAssertTrue(vc.isMeasurementCanceled, nil);
//    STAssertFalse(vc.isCapturingData, nil);
//    
//    [self resumeAfterPausedAndCanceledMeasurement];
//}

- (void) testPauseCallsTeardownPlugins
{
    id corvisMan = [RCSensorFusion sharedInstance];
    [(id<RCPimManager>)[corvisMan expect] teardownPlugins];
    
    [vc handlePause];
    
    [corvisMan verify];
}

@end
