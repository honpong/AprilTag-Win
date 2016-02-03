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
            LOC_ID_FIELD : self.location.dbid ? @(self.location.dbid) : [NSNull null],
            FRACT_FIELD : [NSNumber numberWithBool:self.fractional],
            TYPE_FIELD : @(self.type),
            UNITS_FIELD : [NSNumber numberWithInt:self.units],
            METRIC_SCALE_FIELD : [NSNumber numberWithInt:self.unitsScaleMetric],
            IMP_SCALE_FIELD : [NSNumber numberWithInt:self.unitsScaleImperial],
            DELETED_FIELD : @(self.deleted),
            NOTE_FIELD : self.note ? self.note : [NSNull null],
            P2P_FIELD : @(self.pointToPoint),
            PATH_FIELD : @(self.totalPath),
            HORZ_FIELD : @(self.horzDist),
            P2P_STDEV_FIELD : self.pointToPoint_stdev ? @(self.pointToPoint_stdev) : [NSNull null],
            PATH_STDEV_FIELD : self.totalPath_stdev ? @(self.totalPath_stdev) : [NSNull null],
            HORZ_STDEV_FIELD : self.horzDist_stdev ? @(self.horzDist_stdev) : [NSNull null],
            X_DISP_FIELD : @(self.xDisp),
            Y_DISP_FIELD : @(self.yDisp),
            Z_DISP_FIELD : @(self.zDisp),
            X_STDEV_FIELD : self.xDisp_stdev ? @(self.xDisp_stdev) : [NSNull null],
            Y_STDEV_FIELD : self.yDisp_stdev ? @(self.yDisp_stdev) : [NSNull null],
            Z_STDEV_FIELD : self.zDisp_stdev ? @(self.zDisp_stdev) : [NSNull null],
            ROT_X_FIELD : self.rotationX ? @(self.rotationX) : [NSNull null],
            ROT_Y_FIELD : self.rotationY ? @(self.rotationY) : [NSNull null],
            ROT_Z_FIELD : self.rotationZ ? @(self.rotationZ) : [NSNull null],
            ROT_X_STDEV_FIELD : self.rotationX_stdev ? @(self.rotationX_stdev) : [NSNull null],
            ROT_Y_STDEV_FIELD : self.rotationY_stdev ? @(self.rotationY_stdev) : [NSNull null],
            ROT_Z_STDEV_FIELD : self.rotationZ_stdev ? @(self.rotationZ_stdev) : [NSNull null]
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
    
    if (self.dbid <= 0 && [json[ID_FIELD] isKindOfClass:[NSNumber class]])
        self.dbid = [(NSNumber*)json[ID_FIELD] intValue];
    
    if (![json[NAME_FIELD] isKindOfClass:[NSNull class]] && [json[NAME_FIELD] isKindOfClass:[NSString class]])
        self.name = json[NAME_FIELD];
    
    if (self.timestamp <= 0 && [json[DATE_FIELD] isKindOfClass:[NSString class]])
    {
        NSDate *date = [DATE_FORMATTER_INBOUND dateFromString:(NSString *)json[DATE_FIELD]];
        self.timestamp = [date timeIntervalSince1970];
    }
    
    if ([json[FRACT_FIELD] isKindOfClass:[NSValue class]])
        self.fractional = [json[FRACT_FIELD] boolValue];
    
    if ([json[UNITS_FIELD] isKindOfClass:[NSNumber class]])
        self.units = [(NSNumber*)json[UNITS_FIELD] intValue];
    
    if ([json[METRIC_SCALE_FIELD] isKindOfClass:[NSNumber class]])
        self.unitsScaleMetric = [(NSNumber*)json[METRIC_SCALE_FIELD] intValue];
    
    if ([json[IMP_SCALE_FIELD] isKindOfClass:[NSNumber class]])
        self.unitsScaleImperial = [(NSNumber*)json[IMP_SCALE_FIELD] intValue];
    
    if ([json[TYPE_FIELD] isKindOfClass:[NSNumber class]])
        self.type = [(NSNumber*)json[TYPE_FIELD] intValue];
    
    if ([json[DELETED_FIELD] isKindOfClass:[NSValue class]])
        self.deleted = [json[DELETED_FIELD] boolValue];
    
    if (![json[NOTE_FIELD] isKindOfClass:[NSNull class]] && [json[NOTE_FIELD] isKindOfClass:[NSString class]])
        self.note = json[NOTE_FIELD];
    
    if ([json[LOC_ID_FIELD] isKindOfClass:[NSNumber class]])
        self.locationDbid = [(NSNumber*)json[LOC_ID_FIELD] intValue];
    
    
    if ([json[P2P_FIELD] isKindOfClass:[NSNumber class]])
        self.pointToPoint = [(NSNumber*)json[P2P_FIELD] floatValue];
    
    if ([json[PATH_FIELD] isKindOfClass:[NSNumber class]])
        self.totalPath = [(NSNumber*)json[PATH_FIELD] floatValue];
    
    if ([json[HORZ_FIELD] isKindOfClass:[NSNumber class]])
        self.horzDist = [(NSNumber*)json[HORZ_FIELD] floatValue];
    
    if ([json[P2P_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.pointToPoint_stdev = [(NSNumber*)json[P2P_STDEV_FIELD] floatValue];
    
    if ([json[PATH_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.totalPath_stdev = [(NSNumber*)json[PATH_STDEV_FIELD] floatValue];
    
    if ([json[HORZ_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.horzDist_stdev = [(NSNumber*)json[HORZ_STDEV_FIELD] floatValue];
    
    
    if ([json[X_DISP_FIELD] isKindOfClass:[NSNumber class]])
        self.xDisp = [(NSNumber*)json[X_DISP_FIELD] floatValue];
    
    if ([json[Y_DISP_FIELD] isKindOfClass:[NSNumber class]])
        self.yDisp = [(NSNumber*)json[Y_DISP_FIELD] floatValue];
    
    if ([json[Z_DISP_FIELD] isKindOfClass:[NSNumber class]])
        self.zDisp = [(NSNumber*)json[Z_DISP_FIELD] floatValue];
    
    if ([json[X_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.xDisp_stdev = [(NSNumber*)json[X_STDEV_FIELD] floatValue];
    
    if ([json[Y_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.yDisp_stdev = [(NSNumber*)json[Y_STDEV_FIELD] floatValue];
    
    if ([json[Z_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.zDisp_stdev = [(NSNumber*)json[Z_STDEV_FIELD] floatValue];
    
    
    if ([json[ROT_X_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationX = [(NSNumber*)json[ROT_X_FIELD] floatValue];
    
    if ([json[ROT_Y_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationY = [(NSNumber*)json[ROT_Y_FIELD] floatValue];
    
    if ([json[ROT_Z_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationZ = [(NSNumber*)json[ROT_Z_FIELD] floatValue];
    
    if ([json[ROT_X_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationX_stdev = [(NSNumber*)json[ROT_X_STDEV_FIELD] floatValue];
    
    if ([json[ROT_Y_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationY_stdev = [(NSNumber*)json[ROT_Y_STDEV_FIELD] floatValue];
    
    if ([json[ROT_Z_STDEV_FIELD] isKindOfClass:[NSNumber class]])
        self.rotationZ_stdev = [(NSNumber*)json[ROT_Z_STDEV_FIELD] floatValue];
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


