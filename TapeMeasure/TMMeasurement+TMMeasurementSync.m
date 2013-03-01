//
//  TMMeasurement+TMMeasurementSync.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement+TMMeasurementSync.h"

//@interface MyBoolean : NSObject
//+ (MyBoolean*)initWithBool:(BOOL)value;
//@property (nonatomic, assign) BOOL boolVal;
//@end
//
//@implementation MyBoolean
//@synthesize boolVal;
//+ (MyBoolean*)initWithBool:(BOOL)value
//{
//    MyBoolean *instance = [[MyBoolean alloc] init];
//    instance.boolVal = value;
//    return instance;
//}
//@end

@implementation TMMeasurement (TMMeasurementSync)

static BOOL isSyncInProgress;
static int lastTransId;

static const NSString *ID_FIELD = @"id";
static const NSString *NAME_FIELD = @"name";
static const NSString *P2P_FIELD = @"p_to_p_dist";
static const NSString *PATH_FIELD = @"path_dist";
static const NSString *HORZ_FIELD = @"horizontal_dist";
static const NSString *VERT_FIELD = @"vertical_dist";
static const NSString *DATE_FIELD = @"measured_at";
static const NSString *LOC_ID_FIELD = @"location_id";
static const NSString *FRACT_FIELD = @"display_fractional";
static const NSString *TYPE_FIELD = @"display_type";
static const NSString *UNITS_FIELD = @"display_units";
static const NSString *METRIC_SCALE_FIELD = @"display_scale_metric";
static const NSString *IMP_SCALE_FIELD = @"display_scale_imperial";
static const NSString *DELETED_FIELD = @"is_deleted";
static const NSString *NOTE_FIELD = @"note";

static const NSString *NUM_PAGES_FIELD = @"number of pages";
static const NSString *PAGE_NUM_FIELD = @"page number";
static const NSString *CONTENT_FIELD = @"content";

static const NSString *SINCE_TRANS_PARAM = @"sinceTransId";
static const NSString *PAGE_NUM_PARAM = @"page";
static const NSString *DELETED_PARAM = @"is_deleted";

+ (void)syncWithServer:(void (^)())successBlock onFailure:(void (^)(int))failureBlock
{
    NSLog(@"Sync measurements");
    
    if (isSyncInProgress) {
        NSLog(@"Sync already in progress");
        return;
    }
    
    isSyncInProgress = YES;
    
    [[NSUserDefaults standardUserDefaults] synchronize] ? NSLog(@"Synced prefs") : NSLog(@"Failed to sync prefs");
    lastTransId = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_LAST_TRANS_ID];
    //    lastTransId = 0; //for testing
    NSLog(@"lastTransId = %i", lastTransId);
    
    //download changes, then upload changes, then clean out deleted
    [TMMeasurement
     downloadChangesWithPage:1
     onSuccess:^(){
         [TMMeasurement
          uploadChanges:^(){
              isSyncInProgress = NO; 
              [TMMeasurement cleanOutDeleted];
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
    
    [HTTP_CLIENT getPath:@"api/measurements/"
              parameters:params
                 success:^(AFHTTPRequestOperation *operation, id JSON)
                         {
                             id payload = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];//TODO:handle error
                             NSLog(@"%@", payload);
                             
                             [self saveJson:payload];
                             
                             int nextPageNum = [TMMeasurement getNextPageNumber:payload];
                             
                             if (nextPageNum) {
                                 [TMMeasurement downloadChangesWithPage:nextPageNum onSuccess:successBlock onFailure:failureBlock];
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
    
    NSArray *measurements = [TMMeasurement getAllPendingSync];
    
    for (TMMeasurement *m in measurements)
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
    NSLog(@"saveMeasurements");
    
    int count = 0;
    int countUpdated = 0;
    int countDeleted = 0;
    int countNew = 0;
    
    if ([jsonArray isKindOfClass:[NSDictionary class]] && [[jsonArray objectForKey:CONTENT_FIELD] isKindOfClass:[NSArray class]])
    {
        NSArray *content = [jsonArray objectForKey:CONTENT_FIELD];
        
        for (NSDictionary *json in content)
        {
            //            NSLog(@"\n%@", json);
            
            if ([json isKindOfClass:[NSDictionary class]] && [[json objectForKey:ID_FIELD] isKindOfClass:[NSNumber class]])
            {
                int dbid = [[json objectForKey:ID_FIELD] intValue];
                
                if ([[json objectForKey:@"transaction_log_id"] isKindOfClass:[NSNumber class]])
                {
                    int transId = [[json objectForKey:@"transaction_log_id"] intValue];
                    if (transId > lastTransId) lastTransId = transId;
                }
                
                TMMeasurement *m = [TMMeasurement getMeasurementById:dbid];
                
                if (m == nil)
                {
                    //existing measurement not found, so create new one
                    m = [TMMeasurement getNewMeasurement];
                    [m fillFromJson:json];
                    
                    //don't recreate a deleted measurement
                    if (!m.deleted) {
                        [m insertIntoDb];
                        countNew++;
                    }
                }
                else
                {
                    [m fillFromJson:json];
                    
                    if (m.deleted)
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
        
        [TMMeasurement saveLastTransId];
    }
    
    [DATA_MANAGER saveContext];
    
    NSLog(@"%i measurements, %i new", count, countNew);
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
    
    [HTTP_CLIENT postPath:@"api/measurements/"
               parameters:params
                  success:^(AFHTTPRequestOperation *operation, id JSON)
                         {
                             NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];//TODO:handle error
                             NSLog(@"POST Response\n%@", response);
                             
                             [TMMeasurement saveLastTransId:[response objectForKey:@"transaction_log_id"]];
                             
                             [self fillFromJson:response];
                             [DATA_MANAGER saveContext];
                             
                             if (successBlock) successBlock(lastTransId);
                         }
                  failure:^(AFHTTPRequestOperation *operation, NSError *error)
                         {
                             NSLog(@"Failed to POST measurement: %i %@", operation.response.statusCode, operation.responseString);
                             
                             if (failureBlock) failureBlock(operation.response.statusCode);
                         }
     ];
}

- (void)putToServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [self getParamsForPut];
    
    NSLog(@"PUT \n%@", params);
    
    [HTTP_CLIENT putPath:[NSString stringWithFormat:@"api/measurement/%i/", self.dbid]
              parameters:params
                 success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil]; //TODO:handle error
         NSLog(@"PUT response\n%@", response);
         
         [TMMeasurement saveLastTransId:[response objectForKey:@"transaction_log_id"]];
         
         [[NSUserDefaults standardUserDefaults] setInteger:lastTransId forKey:PREF_LAST_TRANS_ID];
         [[NSUserDefaults standardUserDefaults] synchronize];
         
         if (successBlock) successBlock(lastTransId);
     }
                 failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"Failed to PUT measurement: %i %@", operation.response.statusCode, operation.responseString);
         
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

- (NSMutableDictionary*)getParamsForPost
{
    NSArray *keys = [NSArray arrayWithObjects:
                     NAME_FIELD,
                     P2P_FIELD,
                     PATH_FIELD,
                     HORZ_FIELD,
                     VERT_FIELD,
                     DATE_FIELD,
                     LOC_ID_FIELD,
                     FRACT_FIELD,
                     TYPE_FIELD,
                     UNITS_FIELD,
                     METRIC_SCALE_FIELD,
                     IMP_SCALE_FIELD,
                     DELETED_FIELD,
                     NOTE_FIELD,
                     nil];
    NSArray *values = [NSArray arrayWithObjects:
                       self.name ? self.name : [NSNull null],
                       [NSNumber numberWithFloat:self.pointToPoint],
                       [NSNumber numberWithFloat:self.totalPath],
                       [NSNumber numberWithFloat:self.horzDist],
                       [NSNumber numberWithFloat:self.vertDist],
                       [[RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss"] stringFromDate:[NSDate dateWithTimeIntervalSince1970:self.timestamp]],
                       [NSNull null],
                       [NSNumber numberWithBool:self.fractional],
                       [NSNumber numberWithInt:self.type],
                       [NSNumber numberWithInt:self.units],
                       [NSNumber numberWithInt:self.unitsScaleMetric],
                       [NSNumber numberWithInt:self.unitsScaleImperial],
                       [NSNumber numberWithBool:self.deleted],
                       self.note ? self.note : [NSNull null],
                       nil];
    
    return [NSMutableDictionary dictionaryWithObjects: values forKeys:keys];
}

- (NSMutableDictionary*)getParamsForPut
{
    NSMutableDictionary *params = [self getParamsForPost];
//    [params setObject:self.deleted ? @"true" : @"false" forKey:DELETED_FIELD];
    return params;
}

- (void)fillFromJson:(NSDictionary*)json
{
    //    NSLog(@"%@", json);
    
    if (self.dbid <= 0 && [[json objectForKey:ID_FIELD] isKindOfClass:[NSNumber class]])
        self.dbid = [(NSNumber*)[json objectForKey:ID_FIELD] intValue];
    
    if (![[json objectForKey:NAME_FIELD] isKindOfClass:[NSNull class]] && [[json objectForKey:NAME_FIELD] isKindOfClass:[NSString class]])
        self.name = [json objectForKey:NAME_FIELD];
    
    if ([[json objectForKey:P2P_FIELD] isKindOfClass:[NSNumber class]])
        self.pointToPoint = [(NSNumber*)[json objectForKey:P2P_FIELD] floatValue];
    
    if ([[json objectForKey:PATH_FIELD] isKindOfClass:[NSNumber class]])
        self.totalPath = [(NSNumber*)[json objectForKey:PATH_FIELD] floatValue];
    
    if ([[json objectForKey:HORZ_FIELD] isKindOfClass:[NSNumber class]])
        self.horzDist = [(NSNumber*)[json objectForKey:HORZ_FIELD] floatValue];
    
    if ([[json objectForKey:VERT_FIELD] isKindOfClass:[NSNumber class]])
        self.vertDist = [(NSNumber*)[json objectForKey:VERT_FIELD] floatValue];
    
    if (self.timestamp <= 0 && [[json objectForKey:DATE_FIELD] isKindOfClass:[NSString class]])
    {
        NSDate *date = [[RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss'+00:00'"] dateFromString:(NSString *)[json objectForKey:DATE_FIELD]];
        self.timestamp = [date timeIntervalSince1970];
    }
    
    if ([[json objectForKey:FRACT_FIELD] isKindOfClass:[NSValue class]])
        self.fractional = [[json objectForKey:FRACT_FIELD] boolValue];
    
    if ([[json objectForKey:UNITS_FIELD] isKindOfClass:[NSNumber class]])
        self.units = [(NSNumber*)[json objectForKey:UNITS_FIELD] intValue];
    
    if ([[json objectForKey:METRIC_SCALE_FIELD] isKindOfClass:[NSNumber class]])
        self.unitsScaleMetric = [(NSNumber*)[json objectForKey:METRIC_SCALE_FIELD] intValue];
    
    if ([[json objectForKey:IMP_SCALE_FIELD] isKindOfClass:[NSNumber class]])
        self.unitsScaleImperial = [(NSNumber*)[json objectForKey:IMP_SCALE_FIELD] intValue];
    
    if ([[json objectForKey:TYPE_FIELD] isKindOfClass:[NSNumber class]])
        self.type = [(NSNumber*)[json objectForKey:TYPE_FIELD] intValue];
    
    if ([[json objectForKey:DELETED_FIELD] isKindOfClass:[NSValue class]])
        self.deleted = [[json objectForKey:DELETED_FIELD] boolValue];
    
    //    if ([[json objectForKey:LOC_ID_FIELD] isKindOfClass:[NSString class]])
    //        self.locationDbid = [(NSString*)[json objectForKey:LOC_ID_FIELD] intValue];
    
    if (![[json objectForKey:NOTE_FIELD] isKindOfClass:[NSNull class]] && [[json objectForKey:NOTE_FIELD] isKindOfClass:[NSString class]])
        self.note = [json objectForKey:NOTE_FIELD];
    
    //TODO:fill in the rest of the fields
}

@end


