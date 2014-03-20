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
            @(self.latititude), LAT_FIELD,
            @(self.longitude), LONG_FIELD,
            @(self.accuracyInMeters), ACCURACY_FIELD,
            self.address ? self.address : [NSNull null], ADDRESS_FIELD,
            [[RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss"] stringFromDate:[NSDate dateWithTimeIntervalSince1970:self.timestamp]], DATE_FIELD,
            @(self.deleted), DELETED_FIELD,
            nil];
}

- (NSMutableDictionary*)getParamsForPut
{
    NSMutableDictionary *params = [self getParamsForPost];
    return params;
}

- (void)fillFromJson:(NSDictionary*)json
{
    //    DLog(@"%@", json);
    
    if (self.dbid <= 0 && [json[ID_FIELD] isKindOfClass:[NSNumber class]])
        self.dbid = [(NSNumber*)json[ID_FIELD] intValue];
    
    if (![json[NAME_FIELD] isKindOfClass:[NSNull class]] && [json[NAME_FIELD] isKindOfClass:[NSString class]])
        self.locationName = json[NAME_FIELD];
    
    if ([json[LAT_FIELD] isKindOfClass:[NSNumber class]])
        self.latititude = [(NSNumber*)json[LAT_FIELD] doubleValue];
    
    if ([json[LONG_FIELD] isKindOfClass:[NSNumber class]])
        self.longitude = [(NSNumber*)json[LONG_FIELD] doubleValue];
    
    if ([json[ACCURACY_FIELD] isKindOfClass:[NSNumber class]])
        self.accuracyInMeters = [(NSNumber*)json[ACCURACY_FIELD] doubleValue];
    
    if (![json[ADDRESS_FIELD] isKindOfClass:[NSNull class]] && [json[ADDRESS_FIELD] isKindOfClass:[NSString class]])
        self.address = json[ADDRESS_FIELD];
    
    if (self.timestamp <= 0 && [json[DATE_FIELD] isKindOfClass:[NSString class]])
    {
        NSDate *date = [[RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss'+00:00'"] dateFromString:(NSString *)json[DATE_FIELD]];
        self.timestamp = [date timeIntervalSince1970];
    }
    
    if ([json[DELETED_FIELD] isKindOfClass:[NSValue class]])
        self.deleted = [json[DELETED_FIELD] boolValue];
}

@end
