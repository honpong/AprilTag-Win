//
//  RCConstants.h
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#ifndef RCCore_RCConstants_h
#define RCCore_RCConstants_h

#define LOGME NSLog(@"%s", __PRETTY_FUNCTION__);

#define PREF_DBID @"dbid"
#define PREF_USERNAME @"username"
#define PREF_PASSWORD @"password"
#define PREF_FIRST_NAME @"firstName"
#define PREF_LAST_NAME @"lastName"
#define PREF_DEVICE_PARAMS @"DeviceCalibration"

#define KEYCHAIN_ITEM_IDENTIFIER @"LoginPassword"

typedef enum {
    UnitsMetric = 0, UnitsImperial = 1
} Units;

//items are numbered by their order in the segmented button list. that's why there's two 0s, etc. not ideal, but convenient and workable.
typedef enum {
    UnitsScaleKM = 0, UnitsScaleM = 1, UnitsScaleCM = 2,
    UnitsScaleMI = 0, UnitsScaleYD = 1, UnitsScaleFT = 2, UnitsScaleIN = 3
} UnitsScale;

#endif
