//
//  TMMeasurement+TMMeasurementSync.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/18/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement+TMMeasurementSync.h"

@implementation TMMeasurement (TMMeasurementSync)

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
                       self.location.dbid ? [NSNumber numberWithInt:self.location.dbid] : [NSNull null],
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
    
    if (![[json objectForKey:NOTE_FIELD] isKindOfClass:[NSNull class]] && [[json objectForKey:NOTE_FIELD] isKindOfClass:[NSString class]])
        self.note = [json objectForKey:NOTE_FIELD];
    
    if ([[json objectForKey:LOC_ID_FIELD] isKindOfClass:[NSNumber class]])
        self.locationDbid = [(NSNumber*)[json objectForKey:LOC_ID_FIELD] intValue];
}

+ (void)associateWithLocations
{
    NSLog(@"associateWithLocations");
    
    NSArray *measurements = [TMMeasurement getAllExceptDeleted];
    
    for (TMMeasurement *measurement in measurements) {
        if (measurement.locationDbid > 0) {
            TMLocation *location = (TMLocation*)[TMLocation getLocationById:measurement.locationDbid];
            
            if (location)
            {
                [location addMeasurementObject:measurement];
            }
            else
            {
                NSLog(@"Failed to find location with dbid %i", measurement.dbid);
            }
        }
    }
    
    [DATA_MANAGER saveContext];
}

@end


