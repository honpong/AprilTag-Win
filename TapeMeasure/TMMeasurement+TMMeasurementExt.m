//
//  TMMeasurement+TMMeasurementExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement+TMMeasurementExt.h"

@implementation TMMeasurement (TMMeasurementExt)

+ (void)syncMeasurements:(void (^)(int transCount))successBlock onFailure:(void (^)(int))failureBlock
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

+ (int)saveMeasurements:(NSArray*)jsonArray
{
    int count = 0;
    
    for (NSDictionary *json in jsonArray)
    {
        TMMeasurement *m = [DATA_MANAGER getMeasurementById:(int)[json objectForKey:@"id"]];
        
        if (m == nil)
        {
            //existing measurement not found, so create new one
            m = [DATA_MANAGER getNewMeasurement];
            [m fillFromJson:json];
            [DATA_MANAGER insertMeasurement:m];
        }
        else
        {
            //found measurement. just update it.
            [m fillFromJson:json];
        }
        
        count++;
    }
    
    [DATA_MANAGER saveContext];
    
    return count;
}

- (void)postMeasurement:(void (^)(int dbid, int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [self getParamsForRequest];
    
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

- (void)putMeasurement:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [self getParamsForRequest];
    
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

- (void)delMeasurementOnServer:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:self.dbid], @"id", nil];
    
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

- (NSDictionary*)getParamsForRequest
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
                       self.name ? self.name : @"<null>",
                       [NSNumber numberWithInt:self.pointToPoint],
                       [NSNumber numberWithInt:self.totalPath],
                       [NSNumber numberWithInt:self.horzDist],
                       [NSNumber numberWithInt:self.vertDist],
                       [dateFormatter stringFromDate:[NSDate dateWithTimeIntervalSince1970:self.timestamp]],
                       nil];
    //TODO:fill in the rest of the fields
    
    return [NSDictionary dictionaryWithObjects: values forKeys:keys];
}

- (void)fillFromJson:(NSDictionary*)json
{
    NSLog(@"%@", json);
    
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ssZZZZ"];
    
    if (self.dbid <= 0 && [[json objectForKey:@"id"] isKindOfClass:[NSNumber class]])
        self.dbid = [(NSNumber*)[json objectForKey:@"id"] intValue];
    
    if ([[json objectForKey:@"name"] isKindOfClass:[NSString class]])
        self.name = [json objectForKey:@"name"];
    
    if ([[json objectForKey:@"p_to_p_dist"] isKindOfClass:[NSString class]])
        self.pointToPoint = [(NSString*)[json objectForKey:@"p_to_p_dist"] floatValue];
    
    if ([[json objectForKey:@"path_dist"] isKindOfClass:[NSString class]])
        self.totalPath = [(NSString*)[json objectForKey:@"path_dist"] floatValue];
    
    if ([[json objectForKey:@"horizontal_dist"] isKindOfClass:[NSString class]])
        self.horzDist = [(NSString*)[json objectForKey:@"horizontal_dist"] floatValue];
    
    if ([[json objectForKey:@"vertical_dist"] isKindOfClass:[NSString class]])
        self.vertDist = [(NSString*)[json objectForKey:@"vertical_dist"] floatValue];
    
    if (self.timestamp <= 0 && [[json objectForKey:@"measured_at"] isKindOfClass:[NSString class]])
    {
        NSDate *date = [dateFormatter dateFromString:(NSString *)[json objectForKey:@"measured_at"]];
        self.timestamp = [date timeIntervalSince1970];
    }
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
