//
//  RCConstants.h
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#ifndef RCCore_RCConstants_h
#define RCCore_RCConstants_h

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
#define HTTP_CLIENT [RCHTTPClient sharedInstance]

#define DOCS_DIRECTORY [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]

#define API_VERSION 1
#define API_BASE_URL @"https://internal.realitycap.com/"
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.0"
#define API_LICENSING_POST @"api/v1/licensing/"

#endif
