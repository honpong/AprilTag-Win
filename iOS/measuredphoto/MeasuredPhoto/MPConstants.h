//
//  Constants.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//
#import <Foundation/Foundation.h>

#ifndef TapeMeasure_Constants_h
#define TapeMeasure_Constants_h
#endif

#define CONTEXT [NSManagedObjectContext MR_defaultContext]

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
#define WORKING_DIRECTORY_URL [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]

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

#define API_VERSION 1
#ifdef ARCHIVE
#define API_HOST @"app.realitycap.com"
#else
#define API_HOST @"internal.realitycap.com"
#endif
#define API_BASE_URL [NSString stringWithFormat:@"https://%@/", API_HOST]
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.0"
#define API_DATUM_LOGGED @"api/v1/datum_logged/"

#define INCHES_PER_METER 39.3700787

#define KEY_DATE_STARTED @"dateStarted"

#define SYSTEM_VERSION_EQUAL_TO(v)                  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedSame)
#define SYSTEM_VERSION_GREATER_THAN(v)              ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedDescending)
#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN(v)                 ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN_OR_EQUAL_TO(v)     ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedDescending)

#define JSON_KEY_FLAG @"flag"
#define JSON_KEY_BLOB @"blob"

typedef NS_ENUM(int, JsonBlobFlag) {
    JsonBlobFlagCalibrationData = 1,
    JsonBlobFlagAccuracyQuestion = 2
};

// answer to the question do you want to watch tutorial video
typedef NS_ENUM(int, MPTutorialAnswer) {
    MPTutorialAnswerDontAskAgain = -1,
    MPTutorialAnswerNotNow = 0,
    MPTutorialAnswerYes = 1
};

#define ERROR_DOMAIN @"com.realitycap.TrueMeasure.ErrorDomain"

static NSString * const MPUIOrientationDidChangeNotification = @"com.realitycap.MPUIOrientationDidChangeNotification";
static NSString * const MPCapturePhotoDidAppearNotification = @"com.realitycap.MPCapturePhotoDidAppearNotification";

#define URL_WEBSITE @"http://realitycap.com"
#define URL_SHARING @"http://app.realitycap.com/3Dimension"
#define URL_APPSTORE @"itms-apps://itunes.com/apps/3Dimension"

static NSString* RCApplicationCredential_Facebook_Key = @"319631631561155";
static NSString* RCApplicationCredential_Facebook_Secret = @"b9e0aab41c060755551521b325e693f1"; 

#define IS_DISK_SPACE_LOW [RCDeviceInfo getFreeDiskSpaceInBytes] < 10000000

#ifdef ARCHIVE
#define FLURRY_KEY @"N3827F4P9DWMD5FFFSHV" //prod
#else
#define FLURRY_KEY @"27CGCMKSPPYVS9HX3SXC" //dev
#endif
