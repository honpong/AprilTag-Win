//
//  TMLocation+TMLocationSync.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLocation+TMLocationSync.h"

@implementation TMLocation (TMLocationSync)

static const NSString *ID_FIELD = @"id";
static const NSString *NAME_FIELD = @"name";
static const NSString *LAT_FIELD = @"latitude";
static const NSString *LONG_FIELD = @"longitude";
static const NSString *ACCURACY_FIELD = @"accuracy_in_meters";
static const NSString *ADDRESS_FIELD = @"address";
static const NSString *DATE_FIELD = @"created_at";
static const NSString *DELETED_FIELD = @"is_deleted";


- (NSMutableDictionary*)getParamsForPost
{
    return [NSMutableDictionary
            dictionaryWithObjectsAndKeys:
            self.locationName ? self.locationName : [NSNull null], NAME_FIELD,
            [NSNumber numberWithDouble:self.latititude], LAT_FIELD,
            [NSNumber numberWithDouble:self.longitude], LONG_FIELD,
            [NSNumber numberWithDouble:self.accuracyInMeters], ACCURACY_FIELD,
            self.address ? self.address : [NSNull null], ADDRESS_FIELD,
            [[RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss"] stringFromDate:[NSDate dateWithTimeIntervalSince1970:self.timestamp]], DATE_FIELD,
            [NSNumber numberWithBool:self.deleted], DELETED_FIELD,
            nil];
}

- (NSMutableDictionary*)getParamsForPut
{
    NSMutableDictionary *params = [self getParamsForPost];
    return params;
}

- (void)fillFromJson:(NSDictionary*)json
{
    //    NSLog(@"%@", json);
    
    if (self.dbid <= 0 && [[json objectForKey:ID_FIELD] isKindOfClass:[NSNumber class]])
        self.dbid = [(NSNumber*)[json objectForKey:ID_FIELD] intValue];
    
    if (![[json objectForKey:NAME_FIELD] isKindOfClass:[NSNull class]] && [[json objectForKey:NAME_FIELD] isKindOfClass:[NSString class]])
        self.locationName = [json objectForKey:NAME_FIELD];
    
    if ([[json objectForKey:LAT_FIELD] isKindOfClass:[NSNumber class]])
        self.latititude = [(NSNumber*)[json objectForKey:LAT_FIELD] doubleValue];
    
    if ([[json objectForKey:LONG_FIELD] isKindOfClass:[NSNumber class]])
        self.longitude = [(NSNumber*)[json objectForKey:LONG_FIELD] doubleValue];
    
    if ([[json objectForKey:ACCURACY_FIELD] isKindOfClass:[NSNumber class]])
        self.accuracyInMeters = [(NSNumber*)[json objectForKey:ACCURACY_FIELD] doubleValue];
    
    if (![[json objectForKey:ADDRESS_FIELD] isKindOfClass:[NSNull class]] && [[json objectForKey:ADDRESS_FIELD] isKindOfClass:[NSString class]])
        self.address = [json objectForKey:ADDRESS_FIELD];
    
    if (self.timestamp <= 0 && [[json objectForKey:DATE_FIELD] isKindOfClass:[NSString class]])
    {
        NSDate *date = [[RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss'+00:00'"] dateFromString:(NSString *)[json objectForKey:DATE_FIELD]];
        self.timestamp = [date timeIntervalSince1970];
    }
    
    if ([[json objectForKey:DELETED_FIELD] isKindOfClass:[NSValue class]])
        self.deleted = [[json objectForKey:DELETED_FIELD] boolValue];
}

@end
