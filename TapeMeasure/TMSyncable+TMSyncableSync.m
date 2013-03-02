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
static int lastTransId;

+ (NSString*) httpGetPath
{
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}
- (NSString*) httpPostPath
{
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}
- (NSString*) httpPutPath
{
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
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

+ (void)syncWithServer:(void (^)())successBlock onFailure:(void (^)(int))failureBlock
{
    if (isSyncInProgress) {
        NSLog(@"Sync already in progress");
        return;
    }
    
    NSLog(@"Sync started");
    
    isSyncInProgress = YES;
    
    [[NSUserDefaults standardUserDefaults] synchronize] ? NSLog(@"Synced prefs") : NSLog(@"Failed to sync prefs");
    lastTransId = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_LAST_TRANS_ID];
    //    lastTransId = 0; //for testing
    NSLog(@"lastTransId = %i", lastTransId);
    
    //download changes, then upload changes, then clean out deleted
    [self
     downloadChangesWithPage:1
     onSuccess:^(){
         [self
          uploadChanges:^(){
              isSyncInProgress = NO;
              [self cleanOutDeleted];
              if (successBlock) successBlock();
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

+ (void)downloadChangesWithPage:(int)pageNum
                      onSuccess:(void (^)())successBlock
                      onFailure:(void (^)(int))failureBlock
{
    NSLog(@"Fetching page %i", pageNum);
    
    NSMutableDictionary *params = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                   [NSNumber numberWithInt:lastTransId], SINCE_TRANS_PARAM,
                                   [NSNumber numberWithInt:pageNum], PAGE_NUM_PARAM,
                                   nil];
    if (lastTransId <= 0) [params setObject:@"False" forKey:DELETED_PARAM]; //don't download deleted if this is a first sync
    
    NSLog(@"Request params: %@", params);
    
    [HTTP_CLIENT
     getPath:@"api/measurements/"
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         id payload = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];//TODO:handle error
         NSLog(@"%@", payload);
         
         [self saveJson:payload];
         
         int nextPageNum = [self getNextPageNumber:payload];
         
         if (nextPageNum) {
             [self downloadChangesWithPage:nextPageNum onSuccess:successBlock onFailure:failureBlock];
         }
         else
         {
             if (successBlock) successBlock();
         }
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"Failed to download changes: %i %@", operation.response.statusCode, operation.responseString);
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

+ (void)uploadChanges:(void (^)())successBlock onFailure:(void (^)(int))failureBlock
{
    NSLog(@"Uploading changes");
    
    NSArray *objects = [self getAllPendingSync];
    
    for (TMSyncable *m in objects)
    {
        if (m.dbid > 0)
        {
            [m putToServer:^(int transId) {
                m.syncPending = NO;
                [DATA_MANAGER saveContext];
            } onFailure:^(int statusCode) {
                NSLog(@"uploadChanges PUT failure block");
            }];
        }
        else
        {
            [m postToServer:^(int transId) {
                m.syncPending = NO;
                [DATA_MANAGER saveContext];
            } onFailure:^(int statusCode) {
                NSLog(@"uploadChanges POST failure block");
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

+ (void)saveJson:(id)jsonArray
{
    NSLog(@"saveJson");
    
    int count = 0;
    int countUpdated = 0;
    int countDeleted = 0;
    int countNew = 0;
    
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
                    int transId = [[json objectForKey:TRANS_LOG_ID_FIELD] intValue];
                    if (transId > lastTransId) lastTransId = transId;
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
        
        [self saveLastTransId];
    }
    
    [DATA_MANAGER saveContext];
    
    NSLog(@"%i objects of type %@, %i new", count, entity.name, countNew);
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

- (void)postToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [self getParamsForPost];
    
    NSLog(@"POST \n%@", params);
    
    [HTTP_CLIENT
     postPath:@"api/measurements/"
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];//TODO:handle error
         NSLog(@"POST Response\n%@", response);
         
         [TMSyncable saveLastTransId:[response objectForKey:TRANS_LOG_ID_FIELD]];
         
         [self fillFromJson:response];
         [DATA_MANAGER saveContext];
         
         if (successBlock) successBlock(lastTransId);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"Failed to POST object: %i %@", operation.response.statusCode, operation.responseString);
         
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

- (void)putToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [self getParamsForPut];
    
    NSLog(@"PUT \n%@", params);
    
    [HTTP_CLIENT
     putPath:[NSString stringWithFormat:[self httpPutPath], self.dbid]
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil]; //TODO:handle error
         NSLog(@"PUT response\n%@", response);
         
         [TMSyncable saveLastTransId:[response objectForKey:TRANS_LOG_ID_FIELD]];
         
         [[NSUserDefaults standardUserDefaults] setInteger:lastTransId forKey:PREF_LAST_TRANS_ID];
         [[NSUserDefaults standardUserDefaults] synchronize];
         
         if (successBlock) successBlock(lastTransId);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"Failed to PUT object: %i %@", operation.response.statusCode, operation.responseString);
         
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

+ (void)saveLastTransId:(id)transId
{
    if ([transId isKindOfClass:[NSNumber class]])
    {
        int result = [transId intValue];
        NSLog(@"transId = %i", result);
        
        if (result > lastTransId)
        {
            lastTransId = result;
            
            [self saveLastTransId];
        }
    }
    else
    {
        NSLog(@"Failed to get transaction_log_id");
    }
}

+ (void)saveLastTransId
{
    [[NSUserDefaults standardUserDefaults] setInteger:lastTransId forKey:PREF_LAST_TRANS_ID];
    [[NSUserDefaults standardUserDefaults] synchronize] ? NSLog(@"Saved lastTransId") : NSLog(@"Failed to save lastTransId");
}


@end
