//
//  CATConstants.h
//  TrackLinks
//
//  Created by Ben Hirashima on 11/29/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#include "RCDebugLog.h"

#ifndef CATConstants_h
#define CATConstants_h
#endif

#define SESSION_MANAGER [RCAVSessionManager sharedInstance]
#define SENSOR_FUSION [RCSensorFusion sharedInstance]
#define VIDEO_MANAGER [RCVideoManager sharedInstance]
#define MOTION_MANAGER [RCMotionManager sharedInstance]
#define LOCATION_MANAGER [RCLocationManager sharedInstance]
#define USER_MANAGER [RCUserManager sharedInstance]
#define HTTP_CLIENT [RCHTTPClient sharedInstance]
#define SERVER_OPS [TMServerOpsFactory getInstance]
#define OPENGL_MANAGER [RCGLManagerFactory getInstance]
#define STEREO [RCStereo sharedInstance]

#define DOCUMENTS_DIRECTORY_URL [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]
#define CACHE_DIRECTORY_URL [[[NSFileManager defaultManager] URLsForDirectory:NSCachesDirectory inDomains:NSUserDomainMask] lastObject]
#define WORKING_DIRECTORY_URL [[[NSFileManager defaultManager] URLsForDirectory:NSCachesDirectory inDomains:NSUserDomainMask] lastObject]

#define PREF_UNITS @"units_preference"
#define PREF_IS_CALIBRATED @"is_calibrated"

#define INCHES_PER_METER 39.3700787

#define ERROR_DOMAIN @"com.cat.ErrorDomain"

#define IS_DISK_SPACE_LOW [RCDeviceInfo getFreeDiskSpaceInBytes] < 5000000

typedef enum {
    UnitsMetric = 0, UnitsImperial = 1
} Units;
