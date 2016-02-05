//
//  TMMeasurement+TMMeasurementExt.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "TMDataManagerFactory.h"
#import "TMSyncable+TMSyncableExt.h"
#import "RCCore/RCDistanceImperialFractional.h"

@interface TMMeasurement (TMMeasurementExt)

+ (TMMeasurement*) getNewMeasurement;
+ (TMMeasurement*) getMeasurementById:(int)dbid;

- (void) autoSelectUnitsScale;
- (float) getPrimaryMeasurementDist;
- (id<RCDistance>) getDistanceObject:(float)meters;
- (id<RCDistance>) getPrimaryDistanceObject;
- (NSString*) getLocalizedDateString;
- (NSString*) getTypeString;
- (NSString*) getDistRangeString;

@end
