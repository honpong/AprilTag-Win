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

#pragma mark - Database operations

+ (NSArray*)getAllMeasurementsExceptDeleted
{
    NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] init];
    NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_MEASUREMENT inManagedObjectContext:[DATA_MANAGER getManagedObjectContext]];
    
    NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc]
                                        initWithKey:@"timestamp"
                                        ascending:NO];
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(deleted = false)"];
    
    NSArray *descriptors = [[NSArray alloc] initWithObjects:sortDescriptor, nil];
    
    [fetchRequest setSortDescriptors:descriptors];
    [fetchRequest setPredicate:predicate];
    [fetchRequest setEntity:entity];
    
    NSError *error;
    NSArray *measurementsData = [[DATA_MANAGER getManagedObjectContext] executeFetchRequest:fetchRequest error:&error]; //TODO: Handle fetch error
    
    if(error)
    {
        NSLog(@"Error loading table data: %@", [error localizedDescription]);
    }
    
    return measurementsData;
}

+ (TMMeasurement*)getNewMeasurement
{
    //here, we create the new instance of our model object, but do not yet insert it into the persistent store
    NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_MEASUREMENT inManagedObjectContext:[DATA_MANAGER getManagedObjectContext]];
    TMMeasurement *m = (TMMeasurement*)[[NSManagedObject alloc] initWithEntity:entity insertIntoManagedObjectContext:nil];
    m.units = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_UNITS];
    return m;
}

+ (TMMeasurement*)getMeasurementById:(int)dbid
{
    NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] init];
    NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_MEASUREMENT inManagedObjectContext:[DATA_MANAGER getManagedObjectContext]];
    
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(dbid = %i)", dbid];
    
    [fetchRequest setPredicate:predicate];
    [fetchRequest setEntity:entity];
    
    NSError *error;
    NSArray *measurementsData = [[DATA_MANAGER getManagedObjectContext] executeFetchRequest:fetchRequest error:&error]; //TODO: Handle fetch error
    
    if(error)
    {
        NSLog(@"Error fetching measurement with id %i: %@", dbid, [error localizedDescription]);
    }
    
    return measurementsData.count > 0 ? measurementsData[0] : nil; //TODO:error handling
}

- (void)insertMeasurement
{
    [[DATA_MANAGER getManagedObjectContext] insertObject:self];
}

- (void)deleteMeasurement
{
    [[DATA_MANAGER getManagedObjectContext] deleteObject:self];
    NSLog(@"Measurement deleted");
}

+ (void)cleanOutDeleted
{
    int count = 0;
    
    for (TMMeasurement *m in [TMMeasurement getMarkedForDeletion])
    {
        if (!m.syncPending) [m deleteMeasurement];
        count++;
    }
    
    [DATA_MANAGER saveContext];
    
    if (count) NSLog(@"Cleaned out %i measurements marked for deletion", count);
}

+ (NSArray*)getMarkedForDeletion
{
    NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] init];
    NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_MEASUREMENT inManagedObjectContext:[DATA_MANAGER getManagedObjectContext]];
    
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"(deleted = true)"];
    
    [fetchRequest setPredicate:predicate];
    [fetchRequest setEntity:entity];
    
    NSError *error;
    NSArray *measurementsData = [[DATA_MANAGER getManagedObjectContext] executeFetchRequest:fetchRequest error:&error]; //TODO: Handle fetch error
    
    if(error)
    {
        NSLog(@"Error fetching measurements marked for deletion: %@", [error localizedDescription]);
    }
    
    return measurementsData.count > 0 ? measurementsData : nil; //TODO:error handling
}

#pragma mark - Server operations

+ (void)syncMeasurements:(void (^)(int transCount))successBlock onFailure:(void (^)(int))failureBlock
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
                             
                             id payload = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];//TODO:handle error
                             
                             int count = [self saveMeasurements:payload];
                             [TMMeasurement cleanOutDeleted];
                             
                             if (successBlock) successBlock(count);
                             
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

+ (int)saveMeasurements:(id)jsonArray
{
    NSLog(@"saveMeasurements");
    
    int count = 0;
    int countUpdated = 0;
    int countDeleted = 0;
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
                    [m insertMeasurement];
                    
                    countNew++;
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
    
    NSLog(@"%i found, %i updated, %i deleted, %i new", count, countUpdated, countDeleted, countNew);
    
    return countUpdated;
}

- (void)postMeasurement:(void (^)(int transId))successBlock onFailure:(void (^)(int statusCode))failureBlock
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
                             NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil]; //TODO:handle error
                             NSLog(@"PUT response\n%@", response);
                             
                             [TMMeasurement saveLastTransId:[response objectForKey:@"transaction_log_id"]];
                             
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
    NSMutableDictionary *params = [self getParamsForPost];
    [params setObject:self.deleted ? @"true" : @"false" forKey:@"is_deleted"];
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
    
    if ([[json objectForKey:@"display_fractional"] isKindOfClass:[NSValue class]])
        self.fractional = [[json objectForKey:@"display_fractional"] boolValue];
    
    if ([[json objectForKey:@"display_units"] isKindOfClass:[NSNumber class]])
        self.units = [(NSNumber*)[json objectForKey:@"display_units"] intValue];
    
    if ([[json objectForKey:@"display_scale_metric"] isKindOfClass:[NSNumber class]])
        self.unitsScaleMetric = [(NSNumber*)[json objectForKey:@"display_scale_metric"] intValue];
    
    if ([[json objectForKey:@"display_scale_imperial"] isKindOfClass:[NSNumber class]])
        self.unitsScaleImperial = [(NSNumber*)[json objectForKey:@"display_scale_imperial"] intValue];
    
    if ([[json objectForKey:@"display_type"] isKindOfClass:[NSNumber class]])
        self.type = [(NSNumber*)[json objectForKey:@"display_type"] intValue];
    
    if ([[json objectForKey:@"is_deleted"] isKindOfClass:[NSValue class]])
        self.deleted = [[json objectForKey:@"is_deleted"] boolValue];
    
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
