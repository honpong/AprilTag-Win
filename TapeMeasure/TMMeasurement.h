//
//  TMMeasurement.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/8/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import "TMSyncable.h"

@class TMLocation;

@interface TMMeasurement : TMSyncable

@property (nonatomic) int32_t fractional;
@property (nonatomic) float horzDist;
@property (nonatomic) int32_t locationDbid;
@property (nonatomic, retain) NSString * name;
@property (nonatomic, retain) NSString * note;
@property (nonatomic) float pointToPoint;
@property (nonatomic) NSTimeInterval timestamp;
@property (nonatomic) float totalPath;
@property (nonatomic) int32_t type;
@property (nonatomic) int16_t units;
@property (nonatomic) int16_t unitsScaleImperial;
@property (nonatomic) int16_t unitsScaleMetric;
@property (nonatomic) float zDisp;
@property (nonatomic) float pointToPoint_stdev;
@property (nonatomic) float totalPath_stdev;
@property (nonatomic) float horzDist_stdev;
@property (nonatomic) float xDisp;
@property (nonatomic) float xDisp_stdev;
@property (nonatomic) float yDisp_stdev;
@property (nonatomic) float zDisp_stdev;
@property (nonatomic) float rotationX;
@property (nonatomic) float yDisp;
@property (nonatomic) float rotationY;
@property (nonatomic) float rotationZ;
@property (nonatomic) float rotationX_stdev;
@property (nonatomic) float rotationY_stdev;
@property (nonatomic) float rotationZ_stdev;
@property (nonatomic, retain) TMLocation *location;

@end
