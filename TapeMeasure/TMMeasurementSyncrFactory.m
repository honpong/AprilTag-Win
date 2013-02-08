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

//- (id)init
//{
//    self = [super init];
//    
//    if (self)
//    {
//        
//    }
//    
//    return self;
//}

- (void)fetchMeasurementChanges:(void (^)(NSArray*))successBlock onFailure:(void (^)(int))failureBlock
{
//    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:0, @"sinceTransId", nil];
    
    [HTTP_CLIENT getPath:@"api/measurements/"
              parameters:nil
                 success:^(AFHTTPRequestOperation *operation, id JSON)
                 {
                     id payload = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];
                     
                     NSLog(@"Fetched measurement");
                     
                     [self saveMeasurements:payload];
                                          
                     if (successBlock) successBlock(nil);
                 }
                 failure:^(AFHTTPRequestOperation *operation, NSError *error)
                 {
                     NSLog(@"Failed to fetch measurements: %i", operation.response.statusCode);
                     
                     if (failureBlock) failureBlock(operation.response.statusCode);
                 }
     ];
}

- (void)saveMeasurements:(NSArray*)jsonArray
{
    for (NSDictionary *json in jsonArray)
    {
        TMMeasurement *m = [self measurementFromJsonDict:json];
        
        
        
        [DATA_MANAGER insertMeasurement:m];
    }
    
    [DATA_MANAGER saveContext];
}

- (TMMeasurement*)measurementFromJsonDict:(NSDictionary*)json
{
    NSLog(@"%@", json);
    
    TMMeasurement *m = [DATA_MANAGER getNewMeasurement];
    
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ssZZZZ"];
    
    if ([[json objectForKey:@"id"] isKindOfClass:[NSNumber class]])
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
    
    if ([[json objectForKey:@"measured_at"] isKindOfClass:[NSString class]])
    {
        NSDate *date = [dateFormatter dateFromString:(NSString *)[json objectForKey:@"measured_at"]];
        //            NSLog(@"%@", date);
        m.timestamp = [date timeIntervalSince1970];
    }
    
    return m;
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

