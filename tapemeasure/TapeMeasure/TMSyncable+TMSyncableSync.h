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

+ (void)syncWithServer:(NSInteger)sinceTransId onSuccess:(void (^)(NSInteger lastTransId))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;
+ (BOOL)isSyncInProgress;

+ (void)downloadChanges:(NSInteger)sinceTransId withPage:(int)pageNum onSuccess:(void (^)(NSInteger lastTransId))successBlock onFailure:(void (^)(NSInteger))failureBlock;;
+ (void)uploadChanges:(void (^)())successBlock onFailure:(void (^)(NSInteger))failureBlock;

- (void)postToServer:(void (^)(NSInteger transId))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;
- (void)postToServer:(NSDictionary*)params onSuccess:(void (^)(NSInteger transId))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;

- (void)putToServer:(void (^)(NSInteger transId))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;
- (void)putToServer:(NSDictionary*)params onSuccess:(void (^)(NSInteger transId))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;

+ (int)saveJson:(id)jsonArray;
+ (NSInteger)getStoredTransactionId;
+ (void)saveLastTransIdIfHigher:(NSInteger)transId;
+ (int)getNextPageNumber:(id)json;

@end
