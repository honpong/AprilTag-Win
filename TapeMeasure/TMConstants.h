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

#define ENTITY_MEASUREMENT @"TMMeasurement"
#define ENTITY_LOCATION @"TMLocation"
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
#define MEASUREMENT_SYNCR [TMMeasurementSyncrFactory getInstance]

#define DOCUMENTS_DIRECTORY [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]

#define PREF_UNITS @"units_preference"
#define PREF_ADD_LOCATION @"addlocation_preference"
#define PREF_LAST_TRANS_ID @"last_trans_id"

#define API_BASE_URL @"http://internal.realitycap.com/"
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.0"

typedef enum {
    TypePointToPoint = 0, TypeTotalPath = 1, TypeHorizontal = 2, TypeVertical = 3
} MeasurementType;
