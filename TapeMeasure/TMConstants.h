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

#define DATA_MANAGER [TMDataManagerFactory getDataManagerInstance]
#define SESSION_MANAGER [RCAVSessionManagerFactory getAVSessionManagerInstance]
#define CORVIS_MANAGER [RCCorvisManagerFactory getCorvisManagerInstance]
#define VIDEOCAP_MANAGER [RCVideoCapManagerFactory getVideoCapManagerInstance]
#define MOTIONCAP_MANAGER [RCMotionCapManagerFactory getMotionCapManagerInstance]
#define LOCATION_MANAGER [RCLocationManagerFactory getLocationManagerInstance]
#define OPENGL_MANAGER [TMOpenGLManagerFactory getInstance]
#define USER_MANAGER [RCUserManagerFactory getInstance]
#define HTTP_CLIENT [RCHttpClientFactory getInstance]
#define SERVER_OPS [TMServerOpsFactory getInstance]

#define DOCUMENTS_DIRECTORY [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]

#define PREF_UNITS @"units_preference"
#define PREF_ADD_LOCATION @"addlocation_preference"
#define PREF_LAST_TRANS_ID @"last_trans_id"
#define PREF_SHOW_LOCATION_EXPLANATION @"show_location_explanation"

#define API_VERSION 1
#define API_BASE_URL @"https://internal.realitycap.com/"
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.0"
#define API_MEASUREMENT_GET @"api/v1/measurements/"
#define API_MEASUREMENT_PUT @"api/v1/measurement/%i/"
#define API_LOCATION_GET @"api/v1/locations/"
#define API_LOCATION_PUT @"api/v1/location/%i/"
#define API_DATUM_LOGGED @"api/v1/datum_logged/"

#define INCHES_PER_METER 39.3700787

#ifdef DEBUG
#define FLURRY_KEY @"D3NDKGP5MZCKVBZCD5BF" //dev
#else
#define FLURRY_KEY @"F88HYCQ8TFYKVT5CQZWV" //beta TODO: change to prod
#endif

#define LOGME NSLog(@"%s", __PRETTY_FUNCTION__);

typedef enum {
    TypePointToPoint = 0, TypeTotalPath = 1, TypeHorizontal = 2, TypeVertical = 3
} MeasurementType;
