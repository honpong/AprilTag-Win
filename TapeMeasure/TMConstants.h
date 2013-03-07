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
#define USER_MANAGER [RCUserManagerFactory getInstance]
#define HTTP_CLIENT [RCHttpClientFactory getInstance]

#define DOCUMENTS_DIRECTORY [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]

#define PREF_UNITS @"units_preference"
#define PREF_ADD_LOCATION @"addlocation_preference"
#define PREF_LAST_TRANS_ID @"last_trans_id"

#define API_BASE_URL @"https://internal.realitycap.com/"
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.2"
#define API_MEASUREMENT_GET @"api/measurements/"
#define API_MEASUREMENT_PUT @"api/measurement/%i/"
#define API_LOCATION_GET @"api/locations/"
#define API_LOCATION_PUT @"api/location/%i/"
//post url not needed because it's the same as get in restful APIs

#define FLURRY_KEY_DEV @"D3NDKGP5MZCKVBZCD5BF"

typedef enum {
    TypePointToPoint = 0, TypeTotalPath = 1, TypeHorizontal = 2, TypeVertical = 3
} MeasurementType;
