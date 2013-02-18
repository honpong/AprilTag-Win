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

@interface TMMeasurement (TMMeasurementExt)

+ (NSArray*)getAllMeasurementsExceptDeleted;
+ (TMMeasurement*)getNewMeasurement;
+ (TMMeasurement*)getMeasurementById:(int)dbid;
- (void)insertMeasurement;
- (void)deleteMeasurement;
+ (void)cleanOutDeleted;

- (NSString*)getFormattedDistance:(float)meters;

@end
