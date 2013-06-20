//
//  TMMeasurement+TMMeasurementSync.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "RCCore/RCHTTPClient.h"
#import "TMDataManagerFactory.h"
#import "RCCore/RCDateFormatter.h"
#import "TMSyncable+TMSyncableSync.h"
#import "TMLocation+TMLocationExt.h"

@interface TMMeasurement (TMMeasurementSync)

+ (void)associateWithLocations;

@end
