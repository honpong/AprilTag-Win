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

+ (void)syncWithServer:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock;
+ (BOOL)isSyncInProgress;

- (void)postToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)putToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;

@end
