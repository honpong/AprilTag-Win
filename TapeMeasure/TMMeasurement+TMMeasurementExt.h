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

@interface TMMeasurement (TMMeasurementExt)

+ (void)syncMeasurements:(void (^)(int transCount))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)postMeasurement:(void (^)(int dbid, int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)putMeasurement:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)delMeasurementOnServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;

- (NSString*)getFormattedDistance:(float)meters;

@end
