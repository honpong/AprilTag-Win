//
//  TMMeasurement+TMMeasurementExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement+TMMeasurementExt.h"

@implementation TMMeasurement (TMMeasurementExt)

static BOOL isSyncInProgress;
static int lastTransId;

+ (void)syncMeasurements:(void (^)(int transCount))successBlock onFailure:(void (^)(int))failureBlock
{
    [TMMeasurement syncMeasurementsWithPage:1 onSuccess:successBlock onFailure:failureBlock];
}

+ (void)syncMeasurementsWithPage:(int)pageNum
                       onSuccess:(void (^)(int transCount))successBlock
                       onFailure:(void (^)(int))failureBlock
{
    if (isSyncInProgress) {
        NSLog(@"Sync already in progress");
        return;
    }
    
    isSyncInProgress = YES;
    
    [[NSUserDefaults standardUserDefaults] synchronize] ? NSLog(@"Synced prefs") : NSLog(@"Failed to sync prefs");
    lastTransId = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_LAST_TRANS_ID];
    NSLog(@"lastTransId = %i", lastTransId);
    
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:lastTransId], @"sinceTransId", nil];
    
    [HTTP_CLIENT getPath:@"api/measurements/"
              parameters:params
                 success:^(AFHTTPRequestOperation *operation, id JSON)
                         {
                             NSLog(@"Sync measurements");
                             
                             id payload = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];
                             
                             [self saveMeasurements:payload];
                             
                             int nextPageNum = [TMMeasurement getNextPageNumber:payload];
                             
                             if (nextPageNum) {
                                 isSyncInProgress = NO; //needed to allow recursive call to execute
                                 [TMMeasurement syncMeasurementsWithPage:nextPageNum onSuccess:successBlock onFailure:failureBlock];
                             }
                             else
                             {
                                 if (successBlock) successBlock(count);
                             }
                             
                             isSyncInProgress = NO;
                         }
                 failure:^(AFHTTPRequestOperation *operation, NSError *error)
                         {
                             NSLog(@"Failed to sync measurements: %i", operation.response.statusCode);
                             
                             if (failureBlock) failureBlock(operation.response.statusCode);
                             
                             isSyncInProgress = NO;
                         }
     ];
}

+ (BOOL)isSyncInProgress
{
    return isSyncInProgress;
}

+ (void)saveMeasurements:(id)jsonArray
{
    NSLog(@"saveMeasurements");
    
    int count = 0;
    int countNew = 0;
    
    if ([jsonArray isKindOfClass:[NSDictionary class]] && [[jsonArray objectForKey:@"content"] isKindOfClass:[NSArray class]])
    {
        NSArray *content = [jsonArray objectForKey:@"content"];
    
        for (NSDictionary *json in content)
        {
            NSLog(@"\n%@", json);
            
            if ([json isKindOfClass:[NSDictionary class]] && [[json objectForKey:@"id"] isKindOfClass:[NSNumber class]])
            {
                int dbid = [[json objectForKey:@"id"] intValue];
                
                TMMeasurement *m = [DATA_MANAGER getMeasurementById:dbid];
                
                if (m == nil)
                {
                    //existing measurement not found, so create new one
                    m = [DATA_MANAGER getNewMeasurement];
                    [m fillFromJson:json];
                    [DATA_MANAGER insertMeasurement:m];
                    
                    countNew++;
                }
                else
                {
                    //found measurement. just update it.
                    [m fillFromJson:json];
                }
                
                count++;
            }
        }
    }
    
    [DATA_MANAGER saveContext];
    
    NSLog(@"%i measurements, %i new", count, countNew);
}

+ (int)getNextPageNumber:(id)json
{
    int nextPageNum = 0;
    
    if ([json isKindOfClass:[NSDictionary class]] &&
        [[json objectForKey:@"pages_total"] isKindOfClass:[NSNumber class]] &&
        [[json objectForKey:@"page_number"] isKindOfClass:[NSNumber class]])
    {
        int pagesTotal = [[json objectForKey:@"pages_total"] intValue];
        int pageNum = [[json objectForKey:@"page_number"] intValue];
        
        nextPageNum = pagesTotal > pageNum ? pageNum++ : 0; //returning zero indicates no pages left
    }
    else
    {
        NSLog(@"Failed to find next page number"); //TODO: handle error
    }
    
    return nextPageNum;
}

- (void)postMeasurement:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [self getParamsForPost];
    
    NSLog(@"POST \n%@", params);
    
    [HTTP_CLIENT postPath:@"api/measurements/"
               parameters:params
                  success:^(AFHTTPRequestOperation *operation, id JSON)
                         {
                             NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];
                             NSLog(@"POST Response\n%@", response);
                             
                             lastTransId = [self saveLastTransId:[response objectForKey:@"transaction_log_id"]];
                                                          
                             [self fillFromJson:response];
                             [DATA_MANAGER saveContext];
                             
                             if (successBlock) successBlock(lastTransId);
                         }
                  failure:^(AFHTTPRequestOperation *operation, NSError *error)
                         {
                             NSLog(@"Failed to POST measurement: %i", operation.response.statusCode);
                             
                             if (failureBlock) failureBlock(operation.response.statusCode);
                         }
     ];
}

- (void)putMeasurement:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [self getParamsForPut];
    
    NSLog(@"PUT \n%@", params);
    
    [HTTP_CLIENT putPath:[NSString stringWithFormat:@"api/measurement/%i/", self.dbid]
              parameters:params
                 success:^(AFHTTPRequestOperation *operation, id JSON)
                         {
                             NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];
                             NSLog(@"PUT response\n%@", response);
                             
                             lastTransId = [self saveLastTransId:[response objectForKey:@"transaction_log_id"]];
                             
                             [[NSUserDefaults standardUserDefaults] setInteger:lastTransId forKey:PREF_LAST_TRANS_ID];
                             [[NSUserDefaults standardUserDefaults] synchronize];
                             
                             if (successBlock) successBlock(lastTransId);
                         }
                 failure:^(AFHTTPRequestOperation *operation, NSError *error)
                         {
                             NSLog(@"Failed to PUT measurement: %i", operation.response.statusCode);
                             
                             if (failureBlock) failureBlock(operation.response.statusCode);
                         }
     ];
}

//- (void)deleteMeasurementOnServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
//{
//    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:self.dbid], @"id", nil];
//    
//    NSLog(@"DELETE\n%@", params);
//    
//    [HTTP_CLIENT deletePath:@"api/measurements/"
//                 parameters:params
//                    success:^(AFHTTPRequestOperation *operation, id JSON)
//                             {
//                                 NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];
//                                 NSLog(@"DELETE response\n%@", response);
//                                 
//                                 int transId = (int)[response objectForKey:@"transaction_log_id"];
//                                 
//                                 if (successBlock) successBlock(transId);
//                             }
//                    failure:^(AFHTTPRequestOperation *operation, NSError *error)
//                             {
//                                 NSLog(@"Failed to DELETE measurement: %i", operation.response.statusCode);
//                                 
//                                 if (failureBlock) failureBlock(operation.response.statusCode);
//                             }
//     ];
//}

- (int)saveLastTransId:(id)transId
{
    int result = 0;
    
    if ([transId isKindOfClass:[NSNumber class]])
    {
        result = [transId intValue];
        NSLog(@"lastTransId = %i", lastTransId);
        [[NSUserDefaults standardUserDefaults] setInteger:lastTransId forKey:PREF_LAST_TRANS_ID];
        [[NSUserDefaults standardUserDefaults] synchronize] ? NSLog(@"Saved lastTransId") : NSLog(@"Failed to save lastTransId");
    }
    else
    {
        NSLog(@"Failed to get transaction_log_id");
    }
    
    return result;
}

- (NSMutableDictionary*)getParamsForPost
{
    NSArray *keys = [NSArray arrayWithObjects:
                    @"name",
                     @"p_to_p_dist",
                     @"path_dist",
                     @"horizontal_dist",
                     @"vertical_dist",
                     @"measured_at",
                     @"location_id",
                     @"display_fractional",
                     @"display_type",
                     @"display_units",
                     @"display_scale_metric",
                     @"display_scale_imperial",
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
                       nil];
    
    return [NSMutableDictionary dictionaryWithObjects: values forKeys:keys];
}

- (NSMutableDictionary*)getParamsForPut
{
//    NSMutableDictionary *params = [self getParamsForPost];
//    [params setObject:[NSNumber numberWithInt:self.dbid] forKey:@"id"];
//    [params setObject:[NSNumber numberWithBool:self.isDeleted] forKey:@"is_deleted"];
//    [params removeObjectForKey:@"location_id"];
    NSMutableDictionary *params = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                   self.name, @"name",
                                   [[RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss"] stringFromDate:[NSDate dateWithTimeIntervalSince1970:self.timestamp]], @"measured_at",
                                   nil];
    return params;
}

- (void)fillFromJson:(NSDictionary*)json
{
//    NSLog(@"%@", json);
    
    if (self.dbid <= 0 && [[json objectForKey:@"id"] isKindOfClass:[NSNumber class]])
        self.dbid = [(NSNumber*)[json objectForKey:@"id"] intValue];
    
    if (![[json objectForKey:@"name"] isKindOfClass:[NSNull class]] && [[json objectForKey:@"name"] isKindOfClass:[NSString class]])
        self.name = [json objectForKey:@"name"];
    
    if ([[json objectForKey:@"p_to_p_dist"] isKindOfClass:[NSNumber class]])
        self.pointToPoint = [(NSNumber*)[json objectForKey:@"p_to_p_dist"] floatValue];
    
    if ([[json objectForKey:@"path_dist"] isKindOfClass:[NSNumber class]])
        self.totalPath = [(NSNumber*)[json objectForKey:@"path_dist"] floatValue];
    
    if ([[json objectForKey:@"horizontal_dist"] isKindOfClass:[NSNumber class]])
        self.horzDist = [(NSNumber*)[json objectForKey:@"horizontal_dist"] floatValue];
    
    if ([[json objectForKey:@"vertical_dist"] isKindOfClass:[NSNumber class]])
        self.vertDist = [(NSNumber*)[json objectForKey:@"vertical_dist"] floatValue];
    
    if (self.timestamp <= 0 && [[json objectForKey:@"measured_at"] isKindOfClass:[NSString class]])
    {
        NSDate *date = [[RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss'+00:00'"] dateFromString:(NSString *)[json objectForKey:@"measured_at"]];
        self.timestamp = [date timeIntervalSince1970];
    }
    
    if ([[json objectForKey:@"display_fractional"] isKindOfClass:[NSNumber class]])
        self.fractional = [(NSNumber*)[json objectForKey:@"display_fractional"] boolValue];
    
    if ([[json objectForKey:@"display_units"] isKindOfClass:[NSNumber class]])
        self.units = [(NSNumber*)[json objectForKey:@"display_units"] intValue];
    
    if ([[json objectForKey:@"display_scale_metric"] isKindOfClass:[NSNumber class]])
        self.unitsScaleMetric = [(NSNumber*)[json objectForKey:@"display_scale_metric"] intValue];
    
    if ([[json objectForKey:@"display_scale_imperial"] isKindOfClass:[NSNumber class]])
        self.unitsScaleImperial = [(NSNumber*)[json objectForKey:@"display_scale_imperial"] intValue];
    
    if ([[json objectForKey:@"display_type"] isKindOfClass:[NSNumber class]])
        self.type = [(NSNumber*)[json objectForKey:@"display_type"] intValue];
    
    if ([[json objectForKey:@"is_deleted"] isKindOfClass:[NSNumber class]])
        self.deleted = [(NSNumber*)[json objectForKey:@"is_deleted"] boolValue];
    
//    if ([[json objectForKey:@"location_id"] isKindOfClass:[NSString class]])
//        self.locationDbid = [(NSString*)[json objectForKey:@"location_id"] intValue];
    
    //TODO:fill in the rest of the fields
}

/** Gets formatted distance string according to this instance's properties, such as units and units scale */
- (NSString*)getFormattedDistance:(float)meters
{
    if((Units)self.units == UnitsImperial)
    {
        return [RCDistanceFormatter getFormattedDistance:meters
                                               withUnits:self.units
                                               withScale:self.unitsScaleImperial
                                          withFractional:self.fractional];
    }
    else
    {
        return [RCDistanceFormatter getFormattedDistance:meters
                                               withUnits:self.units
                                               withScale:self.unitsScaleMetric
                                          withFractional:self.fractional];
    }
}

@end
