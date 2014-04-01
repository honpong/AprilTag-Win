//
//  RCConstants.h
//  RCUtility
//
//  Created by Ben Hirashima on 4/1/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#ifndef RCUtility_RCConstants_h
#define RCUtility_RCConstants_h

#define SESSION_MANAGER [RCAVSessionManager sharedInstance]
#define SENSOR_FUSION [RCSensorFusion sharedInstance]
#define VIDEO_MANAGER [RCVideoManager sharedInstance]
#define MOTION_MANAGER [RCMotionManager sharedInstance]
#define LOCATION_MANAGER [RCLocationManager sharedInstance]

#define LOGME NSLog(@"%s", __PRETTY_FUNCTION__);
#define DLog(fmt, ...) NSLog((@"%s " fmt), __PRETTY_FUNCTION__, ##__VA_ARGS__);

#endif
