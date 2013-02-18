//
//  TMMeasurement+TMMeasurementExt.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "RCCore/RCDistanceFormatter.h"
#import "RCCore/RCHttpClientFactory.h"
#import "TMDataManagerFactory.h"
#import "RCCore/RCDateFormatter.h"

@interface TMMeasurement (TMMeasurementExt)

+ (NSArray*)getAllMeasurementsExceptDeleted;
+ (TMMeasurement*)getNewMeasurement;
+ (TMMeasurement*)getMeasurementById:(int)dbid;
- (void)insertMeasurement;
- (void)deleteMeasurement;
+ (void)cleanOutDeleted;

+ (void)syncMeasurements:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock;
+ (BOOL)isSyncInProgress;

- (void)postMeasurement:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)putMeasurement:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;

- (NSString*)getFormattedDistance:(float)meters;

@end
