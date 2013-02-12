//
//  TMMeasurementSyncrFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCore/RCHttpClientFactory.h"
#import "TMMeasurement.h"
#import "TMDataManagerFactory.h"

@protocol TMMeasurementSyncr <NSObject>

- (void)syncMeasurements:(void (^)(int transCount))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)postMeasurement:(TMMeasurement*)m onSuccess:(void (^)(int dbid, int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)putMeasurement:(TMMeasurement*)m onSuccess:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)deleteMeasurement:(TMMeasurement*)m onSuccess:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;

@end

@interface TMMeasurementSyncrFactory : NSObject

+ (id<TMMeasurementSyncr>)getInstance;
+ (void)setInstance:(id<TMMeasurementSyncr>)mockObject;

@end