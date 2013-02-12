//
//  TMMeasurementSyncrFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurementSyncrFactory.h"

@interface TMMeasurementSyncrImpl : NSObject <TMMeasurementSyncr>

@end

@implementation TMMeasurementSyncrImpl

- (void)syncMeasurements:(void (^)(int transCount))successBlock onFailure:(void (^)(int))failureBlock
{
//    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:0, @"sinceTransId", nil];
    
    [HTTP_CLIENT getPath:@"api/measurements/"
              parameters:nil
                 success:^(AFHTTPRequestOperation *operation, id JSON)
                         {
                             id payload = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];
                             
                             NSLog(@"Sync measurements");
                             
                             if (successBlock) successBlock([self saveMeasurements:payload]);
                         }
                 failure:^(AFHTTPRequestOperation *operation, NSError *error)
                         {
                             NSLog(@"Failed to sync measurements: %i", operation.response.statusCode);
                             
                             if (failureBlock) failureBlock(operation.response.statusCode);
                         }
     ];
}

- (int)saveMeasurements:(NSArray*)jsonArray
{
    int count = 0;
    
    for (NSDictionary *json in jsonArray)
    {
        TMMeasurement *m = [DATA_MANAGER getMeasurementById:(int)[json objectForKey:@"id"]];
        
        if (m == nil)
        {
            //existing measurement not found, so create new one
            m = [DATA_MANAGER getNewMeasurement];
            [self fillMeasurement:m fromJsonDict:json];
            [DATA_MANAGER insertMeasurement:m];
        }
        else
        {
            //found measurement. just update it.
            [self fillMeasurement:m fromJsonDict:json];
        }
        
        count++;
    }
    
    [DATA_MANAGER saveContext];
    
    return count;
}

- (TMMeasurement*)fillMeasurement:(TMMeasurement*)m fromJsonDict:(NSDictionary*)json
{
    NSLog(@"%@", json);
    
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ssZZZZ"];
    
    if (m.dbid <= 0 && [[json objectForKey:@"id"] isKindOfClass:[NSNumber class]])
        m.dbid = [(NSNumber*)[json objectForKey:@"id"] intValue];
    
    if ([[json objectForKey:@"name"] isKindOfClass:[NSString class]])
        m.name = [json objectForKey:@"name"];
    
    if ([[json objectForKey:@"p_to_p_dist"] isKindOfClass:[NSString class]])
        m.pointToPoint = [(NSString*)[json objectForKey:@"p_to_p_dist"] floatValue];
    
    if ([[json objectForKey:@"path_dist"] isKindOfClass:[NSString class]])
        m.totalPath = [(NSString*)[json objectForKey:@"path_dist"] floatValue];
    
    if ([[json objectForKey:@"horizontal_dist"] isKindOfClass:[NSString class]])
        m.horzDist = [(NSString*)[json objectForKey:@"horizontal_dist"] floatValue];
    
    if ([[json objectForKey:@"vertical_dist"] isKindOfClass:[NSString class]])
        m.vertDist = [(NSString*)[json objectForKey:@"vertical_dist"] floatValue];
    
    if (m.timestamp <= 0 && [[json objectForKey:@"measured_at"] isKindOfClass:[NSString class]])
    {
        NSDate *date = [dateFormatter dateFromString:(NSString *)[json objectForKey:@"measured_at"]];
        m.timestamp = [date timeIntervalSince1970];
    }
    //TODO:fill in the rest of the fields
    
    return m;
}

- (void)postMeasurement:(TMMeasurement*)m onSuccess:(void (^)(int dbid, int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [self getDictFromMeasurement:m];
    
    NSLog(@"%@", params);
    
    [HTTP_CLIENT postPath:@"api/measurements/"
          parameters:params
             success:^(AFHTTPRequestOperation *operation, id JSON)
                     {
                         NSLog(@"POST measurement");
                         
                         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];
                        
                         NSLog(@"%@", response);
                         
                         int dbid = (int)[response objectForKey:@"id"];
                         int transId = (int)[response objectForKey:@"transaction_log_id"];
                         
                         if (successBlock) successBlock(dbid, transId);
                     }
             failure:^(AFHTTPRequestOperation *operation, NSError *error)
                     {
                         NSLog(@"Failed to POST measurement: %i", operation.response.statusCode);
                         
                         if (failureBlock) failureBlock(operation.response.statusCode);
                     }
     ];
}

- (void)putMeasurement:(TMMeasurement*)m onSuccess:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [self getDictFromMeasurement:m];
    
    NSLog(@"%@", params);
    
    [HTTP_CLIENT putPath:@"api/measurements/"
              parameters:params
                 success:^(AFHTTPRequestOperation *operation, id JSON)
                         {
                             NSLog(@"PUT measurement");
                             
                             NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];
                             NSLog(@"%@", response);
                             
                             int transId = (int)[response objectForKey:@"transaction_log_id"];
                             
                             if (successBlock) successBlock(transId);
                         }
                 failure:^(AFHTTPRequestOperation *operation, NSError *error)
                         {
                             NSLog(@"Failed to PUT measurement: %i", operation.response.statusCode);
                             
                             if (failureBlock) failureBlock(operation.response.statusCode);
                         }
     ];
}

- (void)deleteMeasurement:(TMMeasurement*)m onSuccess:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:m.dbid], @"id", nil];
    
    NSLog(@"%@", params);
    
    [HTTP_CLIENT deletePath:@"api/measurements/"
                 parameters:params
                    success:^(AFHTTPRequestOperation *operation, id JSON)
                            {
                                NSLog(@"PUT measurement");

                                NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];
                                NSLog(@"%@", response);

                                int transId = (int)[response objectForKey:@"transaction_log_id"];

                                if (successBlock) successBlock(transId);
                            }
                    failure:^(AFHTTPRequestOperation *operation, NSError *error)
                            {
                                NSLog(@"Failed to PUT measurement: %i", operation.response.statusCode);

                                if (failureBlock) failureBlock(operation.response.statusCode);
                            }
     ];
}

- (NSDictionary*)getDictFromMeasurement:(TMMeasurement*)m
{
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ssZZZZ"];
    
    NSArray *keys = [NSArray arrayWithObjects:
                     @"user_id",
                     @"name",
                     @"p_to_p_dist",
                     @"path_dist",
                     @"horizontal_dist",
                     @"vertical_dist",
                     @"measured_at",
                     nil];
    NSArray *values = [NSArray arrayWithObjects:
                       @"2",
                       m.name ? m.name : @"<null>",
                       [NSNumber numberWithInt:m.pointToPoint],
                       [NSNumber numberWithInt:m.totalPath],
                       [NSNumber numberWithInt:m.horzDist],
                       [NSNumber numberWithInt:m.vertDist],
                       [dateFormatter stringFromDate:[NSDate dateWithTimeIntervalSince1970:m.timestamp]],
                       nil];
    //TODO:fill in the rest of the fields
    
    return [NSDictionary dictionaryWithObjects: values forKeys:keys];
}

@end

@implementation TMMeasurementSyncrFactory

static id<TMMeasurementSyncr> instance;

+ (id<TMMeasurementSyncr>)getInstance
{
    if (instance == nil)
    {
        instance = [[TMMeasurementSyncrImpl alloc] init];
    }
    
    return instance;
}

+ (void)setInstance:(id<TMMeasurementSyncr>)mockObject
{
    instance = mockObject;
}

@end

