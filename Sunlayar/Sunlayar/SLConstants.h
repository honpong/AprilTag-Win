//
//  SLConstants.h
//  Sunlayar
//
//  Created by Ben Hirashima on 12/29/14.
//  Copyright (c) 2014 Sunlayar. All rights reserved.
//

#ifndef Sunlayar_SLConstants_h
#define Sunlayar_SLConstants_h

#define SESSION_MANAGER [RCAVSessionManager sharedInstance]
#define SENSOR_FUSION [RCSensorFusion sharedInstance]
#define VIDEO_MANAGER [RCVideoManager sharedInstance]
#define MOTION_MANAGER [RCMotionManager sharedInstance]
#define LOCATION_MANAGER [RCLocationManager sharedInstance]
#define STEREO [RCStereo sharedInstance]

#define DOCUMENTS_DIRECTORY_URL [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]
#define CACHE_DIRECTORY_URL [[[NSFileManager defaultManager] URLsForDirectory:NSCachesDirectory inDomains:NSUserDomainMask] lastObject]
#define WORKING_DIRECTORY_URL [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]

#define PREF_UNITS @"units_preference"
#define PREF_IS_CALIBRATED @"is_calibrated"
#define PREF_USE_LOCATION @"use_location"

#define ERROR_DOMAIN @"com.sunlayar.ErrorDomain"

#define IS_DISK_SPACE_LOW [RCDeviceInfo getFreeDiskSpaceInBytes] < 5000000

typedef enum {
    UnitsMetric = 0, UnitsImperial = 1
} Units;

#endif
