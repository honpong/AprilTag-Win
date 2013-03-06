//
//  TMMeasurement.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import "TMSyncable.h"

@class TMLocation;

@interface TMMeasurement : TMSyncable

@property (nonatomic) int32_t fractional;
@property (nonatomic) float horzDist;
@property (nonatomic, retain) NSString * name;
@property (nonatomic, retain) NSString * note;
@property (nonatomic) float pointToPoint;
@property (nonatomic) NSTimeInterval timestamp;
@property (nonatomic) float totalPath;
@property (nonatomic) int32_t type;
@property (nonatomic) int16_t units;
@property (nonatomic) int16_t unitsScaleImperial;
@property (nonatomic) int16_t unitsScaleMetric;
@property (nonatomic) float vertDist;
@property (nonatomic) int32_t locationDbid;
@property (nonatomic, retain) TMLocation *location;

@end
