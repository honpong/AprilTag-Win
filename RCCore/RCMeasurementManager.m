//
//  RCMeasurementManager.m
//  RCCore
//
//  Created by Ben Hirashima on 6/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMeasurementManager.h"

// TODO: something better
#define DATA_MANAGER [TMDataManagerFactory getDataManagerInstance]
#define SESSION_MANAGER [RCAVSessionManagerFactory getAVSessionManagerInstance]
#define CORVIS_MANAGER [RCCorvisManagerFactory getCorvisManagerInstance]
#define VIDEOCAP_MANAGER [RCVideoCapManagerFactory getVideoCapManagerInstance]
#define MOTIONCAP_MANAGER [RCMotionCapManagerFactory getMotionCapManagerInstance]
#define LOCATION_MANAGER [RCLocationManagerFactory getLocationManagerInstance]
//#define USER_MANAGER [RCUserManagerFactory getInstance]
//#define HTTP_CLIENT [RCHttpClientFactory getInstance]
//#define SERVER_OPS [TMServerOpsFactory getInstance]

@implementation RCMeasurementManager

- (id) initWithDelegate:(id<RCCorvisManagerDelegate>)delegate
{
    self = [super init];
    
    if (self)
    {
        CORVIS_MANAGER.delegate = delegate;
    }
    
    return self;
}

- (void) startDataCapture:(CLLocation*)location
{
    LOGME
    
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

- (void) shutdownDataCapture
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

- (void) setDelegate:(id<RCCorvisManagerDelegate>)delegate
{
    CORVIS_MANAGER.delegate = delegate;
}

@end
