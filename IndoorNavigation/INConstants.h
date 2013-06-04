//
//  Constants.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#ifndef IndoorNavigation_Constants_h
#define IndoorNavigation_Constants_h
#endif

#define DATA_MANAGER [TMDataManagerFactory getDataManagerInstance]
#define SESSION_MANAGER [RCAVSessionManagerFactory getAVSessionManagerInstance]
#define CORVIS_MANAGER [RCCorvisManagerFactory getCorvisManagerInstance]
#define VIDEOCAP_MANAGER [RCVideoCapManagerFactory getVideoCapManagerInstance]
#define MOTIONCAP_MANAGER [RCMotionCapManagerFactory getMotionCapManagerInstance]
#define LOCATION_MANAGER [RCLocationManagerFactory getLocationManagerInstance]
#define USER_MANAGER [RCUserManagerFactory getInstance]
#define HTTP_CLIENT [RCHttpClientFactory getInstance]
#define SERVER_OPS [TMServerOpsFactory getInstance]

#define DOCUMENTS_DIRECTORY [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]

#define API_VERSION 1
#define API_BASE_URL @"https://internal.realitycap.com/"
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.0"
#define API_MEASUREMENT_GET @"api/v1/measurements/"
#define API_MEASUREMENT_PUT @"api/v1/measurement/%i/"
#define API_LOCATION_GET @"api/v1/locations/"
#define API_LOCATION_PUT @"api/v1/location/%i/"
#define API_DATUM_LOGGED @"api/v1/datum_logged/"

#define INCHES_PER_METER 39.3700787

#define LOGME NSLog(@"%s", __PRETTY_FUNCTION__);

