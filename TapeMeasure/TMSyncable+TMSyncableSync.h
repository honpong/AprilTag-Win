//
//  TMSyncable+TMSyncableSync.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMSyncable.h"
#import "RCCore/RCHttpClientFactory.h"
#import "TMDataManagerFactory.h"
#import "TMMeasurement+TMMeasurementSync.h"

@interface TMSyncable (TMSyncableSync)

+ (NSString*) httpGetPath;
- (NSString*) httpPostPath;
- (NSString*) httpPutPath;

+ (void)syncWithServer:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock;
+ (BOOL)isSyncInProgress;

- (void)postToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)putToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;

@end
