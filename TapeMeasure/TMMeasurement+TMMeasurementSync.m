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
static const NSString *DATE_FIELD = @"measured_at";
static const NSString *DELETED_FIELD = @"is_deleted";
static const NSString *LOC_ID_FIELD = @"location_id";

static const NSString *FRACT_FIELD = @"display_fractional";
static const NSString *TYPE_FIELD = @"display_type";
static const NSString *UNITS_FIELD = @"display_units";
static const NSString *METRIC_SCALE_FIELD = @"display_scale_metric";
static const NSString *IMP_SCALE_FIELD = @"display_scale_imperial";
static const NSString *NOTE_FIELD = @"note";

static const NSString *P2P_FIELD = @"p_to_p_dist";
static const NSString *PATH_FIELD = @"path_dist";
static const NSString *HORZ_FIELD = @"horizontal_dist";
static const NSString *P2P_STDEV_FIELD = @"p_to_p_stdev";
static const NSString *PATH_STDEV_FIELD = @"path_stdev";
static const NSString *HORZ_STDEV_FIELD = @"horizontal_stdev";

static const NSString *X_DISP_FIELD = @"x_disp";
static const NSString *Y_DISP_FIELD = @"y_disp";
static const NSString *Z_DISP_FIELD = @"z_disp";
static const NSString *X_STDEV_FIELD = @"x_stdev";
static const NSString *Y_STDEV_FIELD = @"y_stdev";
static const NSString *Z_STDEV_FIELD = @"z_stdev";

static const NSString *ROT_X_FIELD = @"rotation_vec_x";
static const NSString *ROT_Y_FIELD = @"rotation_vec_y";
static const NSString *ROT_Z_FIELD = @"rotation_vec_z";
static const NSString *ROT_X_STDEV_FIELD = @"rotation_x_stdev";
static const NSString *ROT_Y_STDEV_FIELD = @"rotation_y_stdev";
static const NSString *ROT_Z_STDEV_FIELD = @"rotation_z_stdev";

#define DATE_FORMATTER_INBOUND [RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss'Z'"]
#define DATE_FORMATTER_OUTBOUND [RCDateFormatter getInstanceForFormat:@"yyyy-MM-dd'T'HH:mm:ss"]

- (NSDictionary*)getParamsForPost
{
    return @{
            NAME_FIELD : self.name ? self.name : [NSNull null],
            DATE_FIELD : [DATE_FORMATTER_OUTBOUND stringFromDate:[NSDate dateWithTimeIntervalSince1970:self.timestamp]],
            LOC_ID_FIELD : self.location.dbid ? [NSNumber numberWithInt:self.location.dbid] : [NSNull null],
            FRACT_FIELD : [NSNumber numberWithBool:self.fractional],
            TYPE_FIELD : [NSNumber numberWithInt:self.type],
            UNITS_FIELD : [NSNumber numberWithInt:self.units],
            METRIC_SCALE_FIELD : [NSNumber numberWithInt:self.unitsScaleMetric],
            IMP_SCALE_FIELD : [NSNumber numberWithInt:self.unitsScaleImperial],
            DELETED_FIELD : [NSNumber numberWithBool:self.deleted],
            NOTE_FIELD : self.note ? self.note : [NSNull null],
            P2P_FIELD : [NSNumber numberWithFloat:self.pointToPoint],
            PATH_FIELD : [NSNumber numberWithFloat:self.totalPath],
            HORZ_FIELD : [NSNumber numberWithFloat:self.horzDist],
            P2P_STDEV_FIELD : self.pointToPoint_stdev ? [NSNumber numberWithFloat:self.pointToPoint_stdev] : [NSNull null],
            PATH_STDEV_FIELD : self.totalPath_stdev ? [NSNumber numberWithFloat:self.totalPath_stdev] : [NSNull null],
            HORZ_STDEV_FIELD : self.horzDist_stdev ? [NSNumber numberWithFloat:self.horzDist_stdev] : [NSNull null],
            X_DISP_FIELD : [NSNumber numberWithFloat:self.xDisp],
            Y_DISP_FIELD : [NSNumber numberWithFloat:self.yDisp],
            Z_DISP_FIELD : [NSNumber numberWithFloat:self.zDisp],
            X_STDEV_FIELD : self.xDisp_stdev ? [NSNumber numberWithFloat:self.xDisp_stdev] : [NSNull null],
            Y_STDEV_FIELD : self.yDisp_stdev ? [NSNumber numberWithFloat:self.yDisp_stdev] : [NSNull null],
            Z_STDEV_FIELD : self.zDisp_stdev ? [NSNumber numberWithFloat:self.zDisp_stdev] : [NSNull null],
            ROT_X_FIELD : self.rotationX ? [NSNumber numberWithFloat:self.rotationX] : [NSNull null],
            ROT_Y_FIELD : self.rotationY ? [NSNumber numberWithFloat:self.rotationY] : [NSNull null],
            ROT_Z_FIELD : self.rotationZ ? [NSNumber numberWithFloat:self.rotationZ] : [NSNull null],
            ROT_X_STDEV_FIELD : self.rotationX_stdev ? [NSNumber numberWithFloat:self.rotationX_stdev] : [NSNull null],
            ROT_Y_STDEV_FIELD : self.rotationY_stdev ? [NSNumber numberWithFloat:self.rotationY_stdev] : [NSNull null],
            ROT_Z_STDEV_FIELD : self.rotationZ_stdev ? [NSNumber numberWithFloat:self.rotationZ_stdev] : [NSNull null]
            };
}

- (NSDictionary*)getParamsForPut
{
    NSDictionary *params = [self getParamsForPost];
    return params;
}

- (void)fillFromJson:(NSDictionary*)json
{
    //    DLog(@"%@", json);
    
    if (self.dbid <= 0 && [[json objectForKey:ID_FIELD] isKindOfClass:[NSNumber class]])
        self.dbid = [(NSNumber*)[json objectForKey:ID_FIELD] intValue];
    
    if (![[json objectForKey:NAME_FIELD] isKindOfClass:[NSNull class]] && [[json objectForKey:NAME_FIELD] isKindOfClass:[NSString class]])
        self.name = [json objectForKey:NAME_FIELD];
    
    if (self.timestamp <= 0 && [[json objectForKey:DATE_FIELD] isKindOfClass:[NSString class]])
    {
        NSDate *date = [DATE_FORMATTER_INBOUND dateFromString:(NSString *)[json objectForKey:DATE_FIELD]];
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
    
    
    if ([[json objectForKey:P2P_FIELD] isKindOfClass:[NSNumber class]])
        self.pointToPoint = [(NSNumber*)[json objectForKey:P2P_FIELD] floatValue];
    
    if ([[json objectForKey:PATH_FIELD] isKindOfClass:[NSNumber class]])
        self.totalPath = [(NSNumber*)[json objectForKey:PATH_FIELD] floatValue];
    
    if ([[json objectForKey:HORZ_FIELD] isKindOfClass:[NSNumber class]])
        self.horzDist = [(NSNumber*)[json objectForKey:HORZ_FIELD] floatValue];
    
    if ([[json objectForKey:P2P_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.pointToPoint_stdev = [(NSNumber*)[json objectForKey:P2P_STDEV_FIELD] floatValue];
    
    if ([[json objectForKey:PATH_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.totalPath_stdev = [(NSNumber*)[json objectForKey:PATH_STDEV_FIELD] floatValue];
    
    if ([[json objectForKey:HORZ_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.horzDist_stdev = [(NSNumber*)[json objectForKey:HORZ_STDEV_FIELD] floatValue];
    
    
    if ([[json objectForKey:X_DISP_FIELD] isKindOfClass:[NSNumber class]])
        self.xDisp = [(NSNumber*)[json objectForKey:X_DISP_FIELD] floatValue];
    
    if ([[json objectForKey:Y_DISP_FIELD] isKindOfClass:[NSNumber class]])
        self.yDisp = [(NSNumber*)[json objectForKey:Y_DISP_FIELD] floatValue];
    
    if ([[json objectForKey:Z_DISP_FIELD] isKindOfClass:[NSNumber class]])
        self.zDisp = [(NSNumber*)[json objectForKey:Z_DISP_FIELD] floatValue];
    
    if ([[json objectForKey:X_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.xDisp_stdev = [(NSNumber*)[json objectForKey:X_STDEV_FIELD] floatValue];
    
    if ([[json objectForKey:Y_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.yDisp_stdev = [(NSNumber*)[json objectForKey:Y_STDEV_FIELD] floatValue];
    
    if ([[json objectForKey:Z_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.zDisp_stdev = [(NSNumber*)[json objectForKey:Z_STDEV_FIELD] floatValue];
    
    
    if ([[json objectForKey:ROT_X_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationX = [(NSNumber*)[json objectForKey:ROT_X_FIELD] floatValue];
    
    if ([[json objectForKey:ROT_Y_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationY = [(NSNumber*)[json objectForKey:ROT_Y_FIELD] floatValue];
    
    if ([[json objectForKey:ROT_Z_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationZ = [(NSNumber*)[json objectForKey:ROT_Z_FIELD] floatValue];
    
    if ([[json objectForKey:ROT_X_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationX_stdev = [(NSNumber*)[json objectForKey:ROT_X_STDEV_FIELD] floatValue];
    
    if ([[json objectForKey:ROT_Y_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationY_stdev = [(NSNumber*)[json objectForKey:ROT_Y_STDEV_FIELD] floatValue];
    
    if ([[json objectForKey:ROT_Z_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationZ_stdev = [(NSNumber*)[json objectForKey:ROT_Z_STDEV_FIELD] floatValue];
}

+ (void)associateWithLocations
{
    LOGME
    
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
                DLog(@"Failed to find location with dbid %i", measurement.dbid);
            }
        }
    }
    
    [DATA_MANAGER saveContext];
}

@end


