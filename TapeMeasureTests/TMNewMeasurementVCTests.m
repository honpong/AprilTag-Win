//
//  TMNewMeasurementVCTests.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVCTests.h"
#import <OCMock.h>
#import "RCCore/RCAVSessionManagerFactory.h"
#import "RCCore/RCVideoCapManagerFactory.h"
#import "RCCore/RCMotionCapManagerFactory.h"
#import "RCCore/RCCorvisManagerFactory.h"
#import "TMConstants.h"

#define POLL_INTERVAL 0.05 //50ms
#define N_SEC_TO_POLL 2.0 //in seconds
#define MAX_POLL_COUNT N_SEC_TO_POLL / POLL_INTERVAL

@implementation TMNewMeasurementVCTests

- (void) setUp
{
    [super setUp];
    
    done = YES;
    
    id sessionMan = [OCMockObject niceMockForProtocol:@protocol(RCAVSessionManager)];
    [[[sessionMan stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isRunning];
    [RCAVSessionManagerFactory setAVSessionManagerInstance:sessionMan];
    
    id videoMan = [OCMockObject mockForProtocol:@protocol(RCVideoCapManager)];
    [RCVideoCapManagerFactory setVideoCapManagerInstance:videoMan];
    
    id motionMan = [OCMockObject mockForProtocol:@protocol(RCMotionCapManager)];
    [RCMotionCapManagerFactory setMotionCapManagerInstance:motionMan];
    
    id corvisMan = [OCMockObject niceMockForProtocol:@protocol(RCCorvisManager)];
    [RCCorvisManagerFactory setCorvisManagerInstance:corvisMan];
    
    UIStoryboard *storyboard = [UIStoryboard storyboardWithName:@"MainStoryboard_iPhone" bundle:nil];
    nav = [storyboard instantiateViewControllerWithIdentifier:@"NavController"];
    vc = [storyboard instantiateViewControllerWithIdentifier:@"NewMeasurement"];
    [nav pushViewController:vc animated:NO];
    vc.type = TypePointToPoint;
}

- (void) tearDown
{
    [RCAVSessionManagerFactory setAVSessionManagerInstance:nil];
    [RCVideoCapManagerFactory setVideoCapManagerInstance:nil];
    [RCMotionCapManagerFactory setMotionCapManagerInstance:nil];
    [RCCorvisManagerFactory setCorvisManagerInstance:nil];
    nav = nil;
    vc = nil;
    [super tearDown];
}

- (BOOL)waitForCompletion:(NSTimeInterval)timeoutSecs {
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSecs];
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if([timeoutDate timeIntervalSinceNow] < 0.0)
            break;
    } while (!done);
    
    return done;
}

- (void) testNormalMeasurement
{
    id videoMan = [RCVideoCapManagerFactory getVideoCapManagerInstance];
    id motionMan = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    id corvisMan = [RCCorvisManagerFactory getCorvisManagerInstance];
    
    [(id<RCVideoCapManager>)[videoMan expect]  startVideoCap];
    [(id<RCMotionCapManager>)[motionMan expect]  startMotionCap];
    
    CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
    [(id<RCCorvisManager>)[corvisMan expect]
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
    
    [(id<RCCorvisManager>)[corvisMan expect] startPlugins];
    
    [vc loadView];
    [vc handleResume];
    [vc startMeasuring];
    
    [videoMan verify];
    [motionMan verify];
    [corvisMan verify];
    
    [(id<RCVideoCapManager>)[videoMan expect]  stopVideoCap];
    [(id<RCMotionCapManager>)[motionMan expect]  stopMotionCap];
    [(id<RCCorvisManager>)[corvisMan expect] stopPlugins];
    [(id<RCCorvisManager>)[corvisMan expect] teardownPlugins];
//    [(id<RCCorvisManager>)[corvisMan expect]
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
    [(id<RCCorvisManager>)[corvisMan expect] startPlugins];
    
    [vc stopMeasuring];
    
    NSLog(@"started poll");
    int pollCount = 0;
    
    while (vc.isProcessingData == NO && pollCount < MAX_POLL_COUNT) {
        NSLog(@"polling... %i", pollCount);
        NSDate* untilDate = [NSDate dateWithTimeIntervalSinceNow:POLL_INTERVAL];
        [[NSRunLoop currentRunLoop] runUntilDate:untilDate];
        pollCount++;
    }
    if (pollCount == MAX_POLL_COUNT) {
        STFail(@"polling timed out");
    }
    
    [videoMan verify];
    [motionMan verify];
    [corvisMan verify];
    
    [(id<RCCorvisManager>)[corvisMan expect] teardownPlugins];
    
    [vc processingFinished];
    
    [corvisMan verify];
}

@end
