//
//  RCMeasurementManager.m
//  RCCore
//
//  Created by Ben Hirashima on 6/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMeasurementManager.h"

// TODO: something better
#define SESSION_MANAGER [RCAVSessionManagerFactory getInstance]
#define CORVIS_MANAGER [RCCorvisManagerFactory getInstance]
#define VIDEOCAP_MANAGER [RCVideoCapManagerFactory getInstance]
#define MOTIONCAP_MANAGER [RCMotionCapManagerFactory getInstance]
#define LOCATION_MANAGER [RCLocationManagerFactory getInstance]

@implementation RCMeasurementManager

- (void) startSensorFusion:(AVCaptureSession*)session withLocation:(CLLocation*)location
{
    LOGME
    
//    [RCVideoCapManagerFactory setupVideoCapWithSession:[SESSION_MANAGER session]];
    
    [CORVIS_MANAGER
     setupPluginsWithFilter:true
     withCapture:false
     withReplay:false
     withLocationValid:location ? true : false
     withLatitude:location ? location.coordinate.latitude : 0
     withLongitude:location ? location.coordinate.longitude : 0
     withAltitude:location ? location.altitude : 0
     ];
    
    [CORVIS_MANAGER startPlugins];
    [MOTIONCAP_MANAGER startMotionCap];
    [VIDEOCAP_MANAGER startVideoCap];
}

- (void) stopSensorFusion
{
    LOGME
    
    [VIDEOCAP_MANAGER stopVideoCap];
    [MOTIONCAP_MANAGER stopMotionCap];
    
    [NSThread sleepForTimeInterval:0.2]; //hack to prevent CorvisManager from receiving a video frame after plugins have stopped.
    
    [CORVIS_MANAGER stopPlugins];
    [CORVIS_MANAGER teardownPlugins];
}

- (void) startMeasuring
{
    LOGME
    [CORVIS_MANAGER startMeasurement];
}

- (void) stopMeasuring
{
    LOGME
    [CORVIS_MANAGER stopMeasurement];
    [CORVIS_MANAGER saveDeviceParameters];
}

- (id<RCMeasurementManagerDelegate>) delegate
{
    return CORVIS_MANAGER.delegate;
}

- (void) setDelegate:(id<RCMeasurementManagerDelegate>)delegate
{
    CORVIS_MANAGER.delegate = delegate;
}

@end
