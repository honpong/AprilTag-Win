//
//  Constants.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#ifndef TapeMeasure_Constants_h
#define TapeMeasure_Constants_h
#endif

#define DATA_MANAGER [TMDataManagerFactory getInstance]
#define SESSION_MANAGER [RCAVSessionManager sharedInstance]
#define SENSOR_FUSION [RCSensorFusion sharedInstance]
#define VIDEO_MANAGER [RCVideoManager sharedInstance]
#define MOTION_MANAGER [RCMotionManager sharedInstance]
#define LOCATION_MANAGER [RCLocationManager sharedInstance]
#define USER_MANAGER [RCUserManager sharedInstance]
#define HTTP_CLIENT [RCHTTPClient sharedInstance]
#define SERVER_OPS [TMServerOpsFactory getInstance]
#define OPENGL_MANAGER [RCOpenGLManagerFactory getInstance]

#define DOCUMENTS_DIRECTORY [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]

#define PREF_UNITS @"units_preference"
#define PREF_ADD_LOCATION @"addlocation_preference"
#define PREF_LAST_TRANS_ID @"last_trans_id"
#define PREF_SHOW_LOCATION_EXPLANATION @"show_location_explanation"
#define PREF_IS_CALIBRATED @"is_calibrated"
#define PREF_TUTORIAL_ANSWER @"tutorial_answer"
#define PREF_SHOW_INSTRUCTIONS @"show_instructions"
#define PREF_SHOW_ACCURACY_QUESTION @"show_accuracy_question"
#define PREF_IS_FIRST_START @"is_first_start"

#define API_VERSION 1
#ifdef ARCHIVE
#define API_BASE_URL @"https://app.realitycap.com/"
#else
#define API_BASE_URL @"https://internal.realitycap.com/"
#endif
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.0"
#define API_DATUM_LOGGED @"api/v1/datum_logged/"

#define INCHES_PER_METER 39.3700787

#ifdef DEBUG
#define LOGME NSLog(@"%s", __PRETTY_FUNCTION__);
#define DLog(fmt, ...) NSLog((@"%s " fmt), __PRETTY_FUNCTION__, ##__VA_ARGS__);
#else
#define LOGME // do nothing
#define DLog(fmt, ...) // do nothing
#endif

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

