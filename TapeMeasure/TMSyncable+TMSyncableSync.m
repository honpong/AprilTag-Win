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

+ (void)syncWithServer:(int)sinceTransId onSuccess:(void (^)(int lastTransId))successBlock onFailure:(void (^)(int))failureBlock
{
    if (isSyncInProgress) {
        NSLog(@"Sync already in progress for %@", [[self class] description]);
        return;
    }
    
    NSLog(@"Sync started for %@", [[self class] description]);
    
    isSyncInProgress = YES;
    
    //download changes, then upload changes, then clean out deleted
    [self
     downloadChanges:sinceTransId
     withPage:1
     onSuccess:^(int lastTransId){
         [self
          uploadChanges:^(){
              isSyncInProgress = NO;
              [self cleanOutDeleted];
              if (successBlock) successBlock(lastTransId);
          }
          onFailure:^(int statusCode)
          {
              isSyncInProgress = NO;
              if (failureBlock) failureBlock(statusCode);
          }];
     }
     onFailure:^(int statusCode)
     {
         isSyncInProgress = NO;
         if (failureBlock) failureBlock(statusCode);
     }
     ];
}

+ (void)downloadChanges:(int)sinceTransId
               withPage:(int)pageNum
              onSuccess:(void (^)(int lastTransId))successBlock
              onFailure:(void (^)(int))failureBlock
{
    NSLog(@"Fetching page %i for %@", pageNum, [[self class] description]);
    
    NSMutableDictionary *params = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                   [NSNumber numberWithInt:sinceTransId], SINCE_TRANS_PARAM,
                                   [NSNumber numberWithInt:pageNum], PAGE_NUM_PARAM,
                                   nil];
    if (sinceTransId <= 0) [params setObject:@"False" forKey:DELETED_PARAM]; //don't download deleted if this is a first sync
    
    NSString *url  = [self httpGetPath];
    NSLog(@"GET %@\n%@", url, params);
    
    [HTTP_CLIENT
     getPath:url
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         NSLog(@"GET response\n%@", operation.responseString);
         
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
         NSLog(@"Failed to download changes for %@: %i %@", [[self class] description], operation.response.statusCode, operation.responseString);
         [TMAnalytics
          logError:@"HTTP.GET"
          message:[NSString stringWithFormat:@"%i: %@", operation.response.statusCode, operation.request.URL.relativeString]
          error:error
          ];
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

+ (void)uploadChanges:(void (^)())successBlock onFailure:(void (^)(int))failureBlock
{
    NSLog(@"Uploading changes for %@", [[self class] description]);
    
    NSArray *objects = [self getAllPendingSync];
    
    for (TMSyncable *m in objects)
    {
        if (m.dbid > 0)
        {
            [m putToServer:^(int transId) {
                m.syncPending = NO;
                [DATA_MANAGER saveContext];
            } onFailure:^(int statusCode) {
                NSLog(@"uploadChanges for %@ PUT failure block", [[self class] description]);
            }];
        }
        else
        {
            [m postToServer:^(int transId) {
                m.syncPending = NO;
                [DATA_MANAGER saveContext];
            } onFailure:^(int statusCode) {
                NSLog(@"uploadChanges for %@ POST failure block", [[self class] description]);
            }];
        }
    }
    
    //    NSLog(@"%i changes uploaded", measurements.count);
    
    if (successBlock) successBlock(); //TODO:check for upload failures
}

+ (BOOL)isSyncInProgress
{
    return isSyncInProgress;
}

+ (int)saveJson:(id)jsonArray
{
    NSLog(@"saveJson for %@", [[self class] description]);
    
    int count = 0;
    int countUpdated = 0;
    int countDeleted = 0;
    int countNew = 0;
    int lastTransId = 0;
    
    NSEntityDescription *entity = [self getEntity];
    
    if ([jsonArray isKindOfClass:[NSDictionary class]] && [[jsonArray objectForKey:CONTENT_FIELD] isKindOfClass:[NSArray class]])
    {
        NSArray *content = [jsonArray objectForKey:CONTENT_FIELD];
        
        for (NSDictionary *json in content)
        {
            //            NSLog(@"\n%@", json);
            
            if ([json isKindOfClass:[NSDictionary class]] && [[json objectForKey:ID_FIELD] isKindOfClass:[NSNumber class]])
            {
                int dbid = [[json objectForKey:ID_FIELD] intValue];
                
                if ([[json objectForKey:TRANS_LOG_ID_FIELD] isKindOfClass:[NSNumber class]])
                {
                    lastTransId = [[json objectForKey:TRANS_LOG_ID_FIELD] intValue];
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
    
    NSLog(@"%i objects of type %@, %i new", count, entity.name, countNew);
    
    return lastTransId;
}

+ (int)getNextPageNumber:(id)json
{
    int nextPageNum = 0;
    
    if ([json isKindOfClass:[NSDictionary class]] &&
        [[json objectForKey:NUM_PAGES_FIELD] isKindOfClass:[NSNumber class]] &&
        [[json objectForKey:PAGE_NUM_FIELD] isKindOfClass:[NSNumber class]])
    {
        int pagesTotal = [[json objectForKey:NUM_PAGES_FIELD] intValue];
        int pageNum = [[json objectForKey:PAGE_NUM_FIELD] intValue];
        
        nextPageNum = pagesTotal > pageNum ? pageNum + 1 : 0; //returning zero indicates no pages left
    }
    else
    {
        NSLog(@"Failed to find next page number"); //TODO: handle error
    }
    
    return nextPageNum;
}

- (void) postToServer:(void (^)(int))successBlock onFailure:(void (^)(int))failureBlock
{
    [self postToServer:[self getParamsForPost] onSuccess:successBlock onFailure:failureBlock];
}

- (void)postToServer:(NSDictionary*)params onSuccess:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSString *url = [self httpPostPath];
    
    NSLog(@"POST %@\n%@", url, params);
    
    [HTTP_CLIENT
     postPath:url
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         NSLog(@"POST Response\n%@", operation.responseString);
         
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];//TODO:handle error
         
         NSNumber *transId = (NSNumber*)[response objectForKey:TRANS_LOG_ID_FIELD];
         [TMSyncable saveLastTransIdIfHigher:[transId intValue]];
         
         [self fillFromJson:response];
         [DATA_MANAGER saveContext];
         
         if (successBlock) successBlock(lastTransId);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"Failed to POST object: %i %@", operation.response.statusCode, operation.responseString);
         [TMAnalytics
          logError:@"HTTP.POST"
          message:[NSString stringWithFormat:@"%i: %@", operation.response.statusCode, operation.request.URL.relativeString]
          error:error
          ];
         
         NSString *requestBody = [[NSString alloc] initWithData:operation.request.HTTPBody encoding:NSUTF8StringEncoding];
         NSLog(@"%@", requestBody);
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

- (void)putToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    [self putToServer:[self getParamsForPut] onSuccess:successBlock onFailure:failureBlock];
}

- (void)putToServer:(NSDictionary*)params onSuccess:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSString *url = [NSString stringWithFormat:[self httpPutPath], self.dbid];
    
    NSLog(@"PUT %@\n%@", url, params);
    
    [HTTP_CLIENT
     putPath:url
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         NSLog(@"PUT response\n%@", operation.responseString);
         
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil]; //TODO:handle error
         
         NSNumber *transId = (NSNumber*)[response objectForKey:TRANS_LOG_ID_FIELD];
         [TMSyncable saveLastTransIdIfHigher:[transId intValue]];
         
         if (successBlock) successBlock(lastTransId);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"Failed to PUT object: %i %@", operation.response.statusCode, operation.responseString);
         [TMAnalytics
          logError:@"HTTP.PUT"
          message:[NSString stringWithFormat:@"%i: %@", operation.response.statusCode, operation.request.URL.relativeString]
          error:error
          ];
         NSString *requestBody = [[NSString alloc] initWithData:operation.request.HTTPBody encoding:NSUTF8StringEncoding];
         NSLog(@"%@", requestBody);
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

+ (int)getStoredTransactionId
{
    return [[NSUserDefaults standardUserDefaults] integerForKey:PREF_LAST_TRANS_ID];
}

+ (void)saveLastTransIdIfHigher:(int)transId
{
    NSInteger storedTransId = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_LAST_TRANS_ID];
    
    if (transId > storedTransId)
    {
        [[NSUserDefaults standardUserDefaults] setInteger:transId forKey:PREF_LAST_TRANS_ID];
        [[NSUserDefaults standardUserDefaults] synchronize] ? NSLog(@"Saved lastTransId: %i", transId) : NSLog(@"Failed to save lastTransId");
    }
}

@end
