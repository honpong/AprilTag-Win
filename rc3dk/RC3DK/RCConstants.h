//
//  RCConstants.h
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#ifndef RCCore_RCConstants_h
#define RCCore_RCConstants_h

// we have to define this here because there's no way to detect the library version at runtime.
// there's a build script that checks this and fails the build if it doesn't match the framework version.
#define RC3DK_VERSION @"0.3.1"

#ifdef DEBUG
#define LOGME NSLog(@"%s", __PRETTY_FUNCTION__);
#define DLog(fmt, ...) NSLog((@"%s " fmt), __PRETTY_FUNCTION__, ##__VA_ARGS__);
#else
#define LOGME // do nothing
#define DLog(fmt, ...) // do nothing
#endif

#define PREF_DBID @"dbid"
#define PREF_USERNAME @"username"
#define PREF_PASSWORD @"password"
#define PREF_FIRST_NAME @"firstName"
#define PREF_LAST_NAME @"lastName"
#define PREF_DEVICE_PARAMS @"DeviceCalibration"

#define KEYCHAIN_ITEM_IDENTIFIER @"LoginPassword"

#define SESSION_MANAGER [RCAVSessionManager sharedInstance]
#define SENSOR_FUSION [RCSensorFusion sharedInstance]
#define VIDEO_MANAGER [RCVideoManager sharedInstance]
#define MOTION_MANAGER [RCMotionManager sharedInstance]
#define LOCATION_MANAGER [RCLocationManager sharedInstance]
#define USER_MANAGER [RCUserManager sharedInstance]
#define HTTP_CLIENT [RCPrivateHTTPClient sharedInstance]
#define OPENGL_MANAGER [RCOpenGLManagerFactory getInstance]

#define DOCS_DIRECTORY [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]

#define API_VERSION 1
#ifdef ARCHIVE
#define API_BASE_URL @"https://app.realitycap.com/"
#else
#define API_BASE_URL @"https://internal.realitycap.com/"
#endif
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.0"
#define API_LICENSING_POST @"api/v1/licensing/"
#define API_DATUM_LOGGED @"api/v1/datum_logged/"

#define ERROR_DOMAIN @"com.realitycap.TrueMeasure.ErrorDomain"

#endif
