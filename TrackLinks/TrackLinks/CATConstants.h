//
//  CATConstants.h
//  TrackLinks
//
//  Created by Ben Hirashima on 11/29/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "RCDebugLog.h"

#ifndef TapeMeasure_Constants_h
#define TapeMeasure_Constants_h
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

#define PREF_UNITS @"units_preference"
#define PREF_ADD_LOCATION @"addlocation_preference"
#define PREF_LAST_TRANS_ID @"last_trans_id"
#define PREF_SHOW_LOCATION_NAG @"show_location_explanation"
#define PREF_IS_CALIBRATED @"is_calibrated"
#define PREF_TUTORIAL_ANSWER @"tutorial_answer"
#define PREF_SHOW_INSTRUCTIONS @"show_instructions"
#define PREF_SHOW_ACCURACY_QUESTION @"show_accuracy_question"
#define PREF_IS_FIRST_START @"is_first_start"
#define PREF_SHOW_RATE_NAG @"show_rate_nag"
#define PREF_RATE_NAG_TIMESTAMP @"rate_nag_timestamp"
#define PREF_LOCATION_NAG_TIMESTAMP @"location_nag_timestamp"
#define PREF_USE_LOCATION @"use_location"
#define PREF_PROMPTED_LOCATION_SERVICES @"prompted_location_services"

#define INCHES_PER_METER 39.3700787

#define KEY_DATE_STARTED @"dateStarted"

#define SYSTEM_VERSION_EQUAL_TO(v)                  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedSame)
#define SYSTEM_VERSION_GREATER_THAN(v)              ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedDescending)
#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN(v)                 ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN_OR_EQUAL_TO(v)     ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedDescending)

#define ERROR_DOMAIN @"com.cat.ErrorDomain"

static NSString * const CATUIOrientationDidChangeNotification = @"com.cat.UIOrientationDidChangeNotification";
static NSString * const CATCapturePhotoDidAppearNotification = @"com.cat.CapturePhotoDidAppearNotification";

#define IS_DISK_SPACE_LOW [RCDeviceInfo getFreeDiskSpaceInBytes] < 5000000

typedef enum {
    UnitsMetric = 0, UnitsImperial = 1
} Units;
