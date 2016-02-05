//
//  TMSyncable+TMSyncableSync.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMSyncable+TMSyncableSync.h"

@implementation TMSyncable (TMSyncableSync)

static const NSString *ID_FIELD = @"id";
static const NSString *NUM_PAGES_FIELD = @"number of pages";
static const NSString *PAGE_NUM_FIELD = @"page number";
static const NSString *CONTENT_FIELD = @"content";
static const NSString *TRANS_LOG_ID_FIELD = @"transaction_log_id";

static const NSString *SINCE_TRANS_PARAM = @"sinceTransId";
static const NSString *PAGE_NUM_PARAM = @"page";
static const NSString *DELETED_PARAM = @"is_deleted";

static BOOL isSyncInProgress;
static int lastTransId; //TODO: not thread safe. concurrent operations will have conflicts.

//unfortunately, class methods cannot be overridden by a subclass' category, so we have to detect the class like this
+ (NSString*) httpGetPath
{
    if ([self class] == [TMMeasurement class]) return API_MEASUREMENT_GET;
    if ([self class] == [TMLocation class]) return API_LOCATION_GET;
    return @"";
}

- (NSString*) httpPostPath
{
    if ([self class] == [TMMeasurement class]) return API_MEASUREMENT_GET;
    if ([self class] == [TMLocation class]) return API_LOCATION_GET;
    return @"";
}

- (NSString*) httpPutPath
{
    if ([self class] == [TMMeasurement class]) return API_MEASUREMENT_PUT;
    if ([self class] == [TMLocation class]) return API_LOCATION_PUT;
    return @"";
}

- (NSMutableDictionary*)getParamsForPost
{
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

- (NSMutableDictionary*) getParamsForPut
{
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

- (void) fillFromJson:(id)json
{
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

+ (void)syncWithServer:(NSInteger)sinceTransId onSuccess:(void (^)(NSInteger lastTransId))successBlock onFailure:(void (^)(NSInteger))failureBlock
{
    if (isSyncInProgress) {
        DLog(@"Sync already in progress for %@", [[self class] description]);
        return;
    }
    
    DLog(@"Sync started for %@", [[self class] description]);
    
    isSyncInProgress = YES;
    
    //download changes, then upload changes, then clean out deleted
    [self
     downloadChanges:sinceTransId
     withPage:1
     onSuccess:^(NSInteger lastTransId){
         [self
          uploadChanges:^(){
              isSyncInProgress = NO;
              [self cleanOutDeleted];
              if (successBlock) successBlock(lastTransId);
          }
          onFailure:^(NSInteger statusCode)
          {
              isSyncInProgress = NO;
              if (failureBlock) failureBlock(statusCode);
          }];
     }
     onFailure:^(NSInteger statusCode)
     {
         isSyncInProgress = NO;
         if (failureBlock) failureBlock(statusCode);
     }
     ];
}

+ (void)downloadChanges:(NSInteger)sinceTransId
               withPage:(int)pageNum
              onSuccess:(void (^)(NSInteger lastTransId))successBlock
              onFailure:(void (^)(NSInteger))failureBlock
{
    DLog(@"Fetching page %i for %@", pageNum, [[self class] description]);
    
    NSMutableDictionary *params = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                   @(sinceTransId), SINCE_TRANS_PARAM,
                                   @(pageNum), PAGE_NUM_PARAM,
                                   nil];
    if (sinceTransId <= 0) params[DELETED_PARAM] = @"False"; //don't download deleted if this is a first sync
    
    NSString *url  = [self httpGetPath];
    DLog(@"GET %@\n%@", url, params);
    
    [HTTP_CLIENT
     getPath:url
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"GET response\n%@", operation.responseString);
         
         id payload = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];//TODO:handle error
         
         int transId = [self saveJson:payload];
         if (transId > lastTransId) lastTransId = transId;
         
         int nextPageNum = [self getNextPageNumber:payload];
         
         if (nextPageNum)
         {
             [self downloadChanges:sinceTransId withPage:nextPageNum onSuccess:successBlock onFailure:failureBlock];
         }
         else
         {
             if (successBlock) successBlock(lastTransId);
         }
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"Failed to download changes for %@: %li %@", [[self class] description], (long)operation.response.statusCode, operation.responseString);
         [TMAnalytics
          logError:@"HTTP.GET"
          message:[NSString stringWithFormat:@"%li: %@", (long)operation.response.statusCode, operation.request.URL.relativeString]
          error:error
          ];
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

+ (void)uploadChanges:(void (^)())successBlock onFailure:(void (^)(NSInteger))failureBlock
{
    DLog(@"Uploading changes for %@", [[self class] description]);
    
    NSArray *objects = [self getAllPendingSync];
    
    for (TMSyncable *m in objects)
    {
        if (m.dbid > 0)
        {
            [m putToServer:^(NSInteger transId) {
                m.syncPending = NO;
                [DATA_MANAGER saveContext];
            } onFailure:^(NSInteger statusCode) {
                DLog(@"uploadChanges for %@ PUT failure block", [[self class] description]);
            }];
        }
        else
        {
            [m postToServer:^(NSInteger transId) {
                m.syncPending = NO;
                [DATA_MANAGER saveContext];
            } onFailure:^(NSInteger statusCode) {
                DLog(@"uploadChanges for %@ POST failure block", [[self class] description]);
            }];
        }
    }
    
    //    DLog(@"%i changes uploaded", measurements.count);
    
    if (successBlock) successBlock(); //TODO:check for upload failures
}

+ (BOOL)isSyncInProgress
{
    return isSyncInProgress;
}

+ (int)saveJson:(id)jsonArray
{
    DLog(@"saveJson for %@", [[self class] description]);
    
    int count = 0;
    int countUpdated = 0;
    int countDeleted = 0;
    int countNew = 0;
    int lastTransId = 0;
    
    NSEntityDescription *entity = [self getEntity];
    
    if ([jsonArray isKindOfClass:[NSDictionary class]] && [jsonArray[CONTENT_FIELD] isKindOfClass:[NSArray class]])
    {
        NSArray *content = jsonArray[CONTENT_FIELD];
        
        for (NSDictionary *json in content)
        {
            //            DLog(@"\n%@", json);
            
            if ([json isKindOfClass:[NSDictionary class]] && [json[ID_FIELD] isKindOfClass:[NSNumber class]])
            {
                int dbid = [json[ID_FIELD] intValue];
                
                if ([json[TRANS_LOG_ID_FIELD] isKindOfClass:[NSNumber class]])
                {
                    lastTransId = [json[TRANS_LOG_ID_FIELD] intValue];
                }
                
                TMSyncable *obj = (TMSyncable*)[DATA_MANAGER getObjectOfType:entity byDbid:dbid];
                
                if (obj == nil)
                {
                    //existing measurement not found, so create new one
                    obj = (TMSyncable*)[DATA_MANAGER getNewObjectOfType:entity];
                    [obj fillFromJson:json];
                    
                    //don't recreate a deleted measurement
                    if (!obj.deleted) {
                        [obj insertIntoDb];
                        countNew++;
                    }
                }
                else
                {
                    [obj fillFromJson:json];
                    
                    if (obj.deleted)
                    {
                        countDeleted++;
                    }
                    else
                    {
                        countUpdated++;
                    }
                }
                
                count++;
            }
        }
    }
    
    [DATA_MANAGER saveContext];
    
    DLog(@"%i objects of type %@, %i new", count, entity.name, countNew);
    
    return lastTransId;
}

+ (int)getNextPageNumber:(id)json
{
    int nextPageNum = 0;
    
    if ([json isKindOfClass:[NSDictionary class]] &&
        [json[NUM_PAGES_FIELD] isKindOfClass:[NSNumber class]] &&
        [json[PAGE_NUM_FIELD] isKindOfClass:[NSNumber class]])
    {
        int pagesTotal = [json[NUM_PAGES_FIELD] intValue];
        int pageNum = [json[PAGE_NUM_FIELD] intValue];
        
        nextPageNum = pagesTotal > pageNum ? pageNum + 1 : 0; //returning zero indicates no pages left
    }
    else
    {
        DLog(@"Failed to find next page number"); //TODO: handle error
    }
    
    return nextPageNum;
}

- (void) postToServer:(void (^)(NSInteger))successBlock onFailure:(void (^)(NSInteger))failureBlock
{
    [self postToServer:[self getParamsForPost] onSuccess:successBlock onFailure:failureBlock];
}

- (void)postToServer:(NSDictionary*)params onSuccess:(void (^)(NSInteger transId))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock
{
    NSString *url = [self httpPostPath];
    
    DLog(@"POST %@\n%@", url, params);
    
    [HTTP_CLIENT
     postPath:url
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"POST Response\n%@", operation.responseString);
         
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];//TODO:handle error
         
         NSNumber *transId = (NSNumber*)response[TRANS_LOG_ID_FIELD];
         [TMSyncable saveLastTransIdIfHigher:[transId intValue]];
         
         [self fillFromJson:response];
         [DATA_MANAGER saveContext];
         
         if (successBlock) successBlock(lastTransId);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"Failed to POST object: %li %@", (long)operation.response.statusCode, operation.responseString);
         [TMAnalytics
          logError:@"HTTP.POST"
          message:[NSString stringWithFormat:@"%li: %@", (long)operation.response.statusCode, operation.request.URL.relativeString]
          error:error
          ];
         
         NSString *requestBody = [[NSString alloc] initWithData:operation.request.HTTPBody encoding:NSUTF8StringEncoding];
         DLog(@"%@", requestBody);
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

- (void)putToServer:(void (^)(NSInteger transId))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock
{
    [self putToServer:[self getParamsForPut] onSuccess:successBlock onFailure:failureBlock];
}

- (void)putToServer:(NSDictionary*)params onSuccess:(void (^)(NSInteger transId))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock
{
    NSString *url = [NSString stringWithFormat:[self httpPutPath], self.dbid];
    
    DLog(@"PUT %@\n%@", url, params);
    
    [HTTP_CLIENT
     putPath:url
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"PUT response\n%@", operation.responseString);
         
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil]; //TODO:handle error
         
         NSNumber *transId = (NSNumber*)response[TRANS_LOG_ID_FIELD];
         [TMSyncable saveLastTransIdIfHigher:[transId intValue]];
         
         if (successBlock) successBlock(lastTransId);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"Failed to PUT object: %li %@", (long)operation.response.statusCode, operation.responseString);
         [TMAnalytics
          logError:@"HTTP.PUT"
          message:[NSString stringWithFormat:@"%li: %@", (long)operation.response.statusCode, operation.request.URL.relativeString]
          error:error
          ];
         NSString *requestBody = [[NSString alloc] initWithData:operation.request.HTTPBody encoding:NSUTF8StringEncoding];
         DLog(@"%@", requestBody);
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

+ (NSInteger)getStoredTransactionId
{
    return [[NSUserDefaults standardUserDefaults] integerForKey:PREF_LAST_TRANS_ID];
}

+ (void)saveLastTransIdIfHigher:(NSInteger)transId
{
    NSInteger storedTransId = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_LAST_TRANS_ID];
    
    if (transId > storedTransId)
    {
        [[NSUserDefaults standardUserDefaults] setInteger:transId forKey:PREF_LAST_TRANS_ID];
        if ([[NSUserDefaults standardUserDefaults] synchronize])
        {
            DLog(@"Saved lastTransId: %li", (long)transId);
        }
        else
        {
            DLog(@"Failed to save lastTransId");
        }
    }
}

@end
