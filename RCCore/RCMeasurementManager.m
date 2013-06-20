//
//  RCMeasurementManager.m
//  RCCore
//
//  Created by Ben Hirashima on 6/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMeasurementManager.h"

// TODO: something better
#define SESSION_MANAGER [RCAVSessionManager sharedInstance]
#define SENSOR_FUSION [RCSensorFusion sharedInstance]
#define VIDEOCAP_MANAGER [RCVideoCapManagerFactory getInstance]
#define MOTIONCAP_MANAGER [RCMotionCapManagerFactory getInstance]
#define LOCATION_MANAGER [RCLocationManagerFactory getInstance]

@implementation RCMeasurementManager

- (void) startSensorFusion:(AVCaptureSession*)session withLocation:(CLLocation*)location
{
    LOGME
    
//    [RCVideoCapManagerFactory setupVideoCapWithSession:[SESSION_MANAGER session]];
    
    [SENSOR_FUSION
     setupPluginsWithFilter:true
     withCapture:false
     withReplay:false
     withLocationValid:location ? true : false
     withLatitude:location ? location.coordinate.latitude : 0
     withLongitude:location ? location.coordinate.longitude : 0
     withAltitude:location ? location.altitude : 0
     ];
    
    [SENSOR_FUSION startPlugins];
    [MOTIONCAP_MANAGER startMotionCap];
    [VIDEOCAP_MANAGER startVideoCap];
}

- (void) stopSensorFusion
{
    LOGME
    
    [VIDEOCAP_MANAGER stopVideoCap];
    [MOTIONCAP_MANAGER stopMotionCap];
    
    [NSThread sleepForTimeInterval:0.2]; //hack to prevent CorvisManager from receiving a video frame after plugins have stopped.
    
    [SENSOR_FUSION stopPlugins];
    [SENSOR_FUSION teardownPlugins];
}

- (void) startMeasuring
{
    LOGME
    [SENSOR_FUSION startMeasurement];
}

- (void) stopMeasuring
{
    LOGME
    [SENSOR_FUSION stopMeasurement];
    [SENSOR_FUSION saveDeviceParameters];
}

- (id<RCMeasurementManagerDelegate>) delegate
{
    return SENSOR_FUSION.delegate;
}

- (void) setDelegate:(id<RCMeasurementManagerDelegate>)delegate
{
    SENSOR_FUSION.delegate = delegate;
}

@end
