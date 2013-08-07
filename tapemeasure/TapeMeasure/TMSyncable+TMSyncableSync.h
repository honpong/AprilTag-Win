//
//  TMSyncable+TMSyncableSync.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMSyncable.h"
#import "RCCore/RCHTTPClient.h"
#import "TMDataManagerFactory.h"
#import "TMMeasurement+TMMeasurementSync.h"

@interface TMSyncable (TMSyncableSync)

+ (NSString*) httpGetPath;
- (NSString*) httpPostPath;
- (NSString*) httpPutPath;

+ (void)syncWithServer:(int)sinceTransId onSuccess:(void (^)(int lastTransId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
+ (BOOL)isSyncInProgress;

+ (void)downloadChanges:(int)sinceTransId withPage:(int)pageNum onSuccess:(void (^)(int lastTransId))successBlock onFailure:(void (^)(int))failureBlock;;
+ (void)uploadChanges:(void (^)())successBlock onFailure:(void (^)(int))failureBlock;

- (void)postToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)postToServer:(NSDictionary*)params onSuccess:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;

- (void)putToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void)putToServer:(NSDictionary*)params onSuccess:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock;

+ (int)saveJson:(id)jsonArray;
+ (int)getStoredTransactionId;
+ (void)saveLastTransIdIfHigher:(int)transId;
+ (int)getNextPageNumber:(id)json;

@end
