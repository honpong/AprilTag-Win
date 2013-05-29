//
//  TMMeasurement+TMMeasurementExt.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "RCCore/RCDistanceFormatter.h"
#import "TMDataManagerFactory.h"
#import "TMSyncable+TMSyncableExt.h"
#import "RCCore/RCDistanceImperialFractional.h"

@interface TMMeasurement (TMMeasurementExt)

+ (TMMeasurement*) getNewMeasurement;
+ (TMMeasurement*) getMeasurementById:(int)dbid;

- (NSString*) getFormattedDistance:(float)meters;
- (void) autoSelectUnitsScale;
- (float) getPrimaryMeasurementDist;
- (id<RCDistance>) getDistance:(float)meters;
- (id<RCDistance>) getPrimaryDistance;

@end
