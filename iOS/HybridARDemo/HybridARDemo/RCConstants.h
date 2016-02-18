//
//  RCConstants.h
//  HybridARDemo
//
//  Created by Ben Hirashima on 5/19/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef HybridARDemo_RCConstants_h
#define HybridARDemo_RCConstants_h

#define SESSION_MANAGER [RCAVSessionManager sharedInstance]
#define SENSOR_FUSION [RCSensorFusion sharedInstance]
#define VIDEO_MANAGER [RCVideoManager sharedInstance]
#define MOTION_MANAGER [RCMotionManager sharedInstance]
#define LOCATION_MANAGER [RCLocationManager sharedInstance]
#define STEREO [RCStereo sharedInstance]

#define PREF_UNITS @"units_preference"
#define PREF_IS_CALIBRATED @"is_calibrated"
#define PREF_USE_LOCATION @"use_location"

typedef enum {
    UnitsMetric = 0, UnitsImperial = 1
} Units;

#endif
