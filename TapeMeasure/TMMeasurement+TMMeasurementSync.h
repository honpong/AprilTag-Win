//
//  TMMeasurement+TMMeasurementSync.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "RCCore/RCHttpClientFactory.h"
#import "TMDataManagerFactory.h"
#import "RCCore/RCDateFormatter.h"

@interface TMMeasurement (TMMeasurementSync)

+ (void)syncMeasurements:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock;
+ (BOOL)isSyncInProgress;

- (void)postMeasurement:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)putMeasurement:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;

@end
