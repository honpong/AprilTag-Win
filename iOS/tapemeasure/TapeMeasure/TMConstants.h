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

#define ENTITY_STRING_MEASUREMENT @"TMMeasurement"
#define ENTITY_MEASUREMENT [TMMeasurement getEntity]
#define ENTITY_STRING_LOCATION @"TMLocation"
#define ENTITY_LOCATION [TMLocation getEntity]
#define DATA_MODEL_URL @"TMMeasurementDM"

#define CAPTURE_DATA YES

#define DATA_MANAGER [TMDataManagerFactory getInstance]
#define SESSION_MANAGER [RCAVSessionManager sharedInstance]
#define SENSOR_FUSION [RCSensorFusion sharedInstance]
#define VIDEO_MANAGER [RCVideoManager sharedInstance]
#define MOTION_MANAGER [RCMotionManager sharedInstance]
#define LOCATION_MANAGER [RCLocationManager sharedInstance]
#define USER_MANAGER [RCUserManager sharedInstance]
#define HTTP_CLIENT [RCHTTPClient sharedInstance]
#define SERVER_OPS [TMServerOpsFactory getInstance]
#define OPENGL_MANAGER [RCGLManagerFactory getInstance]
#define SENSOR_DELEGATE [SensorDelegate sharedInstance]

#define DOCUMENTS_DIRECTORY [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]

#define PREF_UNITS @"units_preference"
#define PREF_ADD_LOCATION @"addlocation_preference"
#define PREF_LAST_TRANS_ID @"last_trans_id"
#define PREF_SHOW_LOCATION_NAG @"show_location_explanation"
#define PREF_LOCATION_NAG_TIMESTAMP @"location_nag_timestamp"
#define PREF_IS_CALIBRATED @"is_calibrated"
#define PREF_IS_FIRST_LAUNCH @"is_first_launch"
#define PREF_SHOW_RATE_NAG @"show_rate_nag"
#define PREF_RATE_NAG_TIMESTAMP @"rate_nag_timestamp"
#define PREF_LAST_TIP_INDEX @"last_tip_index"
#define PREF_USE_LOCATION @"use_location"
#define PREF_PROMPTED_LOCATION_SERVICES @"prompted_location_services"

#define API_VERSION 1
#define API_BASE_URL @"https://internal.realitycap.com/"
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.0"
#define API_MEASUREMENT_GET @"api/v1/measurements/"
#define API_MEASUREMENT_PUT @"api/v1/measurement/%i/"
#define API_LOCATION_GET @"api/v1/locations/"
#define API_LOCATION_PUT @"api/v1/location/%i/"
#define API_DATUM_LOGGED @"api/v1/datum_logged/"

#define URL_WEBSITE @"http://realitycap.com"
#define URL_SHARING @"http://app.realitycap.com/endless"
#define URL_APPSTORE @"itms-apps://itunes.com/apps/EndlessTapeMeasure"

#define INCHES_PER_METER 39.3700787

#ifdef ARCHIVE
#define FLURRY_KEY @"NZ3QP9KQNBVZKW53SPM2" //prod
#else
#define FLURRY_KEY @"D3NDKGP5MZCKVBZCD5BF" //dev
#endif

typedef enum {
    TypePointToPoint = 0, TypeTotalPath = 1, TypeHorizontal = 2, TypeVertical = 3
} MeasurementType;

#define SYSTEM_VERSION_EQUAL_TO(v)                  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedSame)
#define SYSTEM_VERSION_GREATER_THAN(v)              ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedDescending)
#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN(v)                 ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN_OR_EQUAL_TO(v)     ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedDescending)

static NSString* RCApplicationCredential_Facebook_Key = @"671645849539796";
static NSString* RCApplicationCredential_Facebook_Secret = @"827cfb4e92aff3551ffeb3618be46e08";

#define COLOR_DULL_YELLOW [UIColor colorWithRed:219./255. green:166./255. blue:46./255. alpha:1.]
