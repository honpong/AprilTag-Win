//
//  TMMeasurement.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class TMLocation;

@interface TMMeasurement : NSManagedObject

@property (nonatomic) BOOL fractional;
@property (nonatomic) float horzDist;
@property (nonatomic, retain) NSString * name;
@property (nonatomic) float pointToPoint;
@property (nonatomic) NSTimeInterval timestamp;
@property (nonatomic) float totalPath;
@property (nonatomic) int32_t type;
@property (nonatomic) int16_t units;
@property (nonatomic) int16_t unitsScaleImperial;
@property (nonatomic) int16_t unitsScaleMetric;
@property (nonatomic) float vertDist;
@property (nonatomic, retain) TMLocation *location;

@end
