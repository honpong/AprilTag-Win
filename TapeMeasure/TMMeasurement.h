//
//  TMMeasurement.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 12/11/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>


@interface TMMeasurement : NSManagedObject

@property (nonatomic, retain) NSNumber * fractional;
@property (nonatomic, retain) NSNumber * horzDist;
@property (nonatomic, retain) NSString * name;
@property (nonatomic, retain) NSNumber * pointToPoint;
@property (nonatomic, retain) NSDate * timestamp;
@property (nonatomic, retain) NSNumber * totalPath;
@property (nonatomic, retain) NSNumber * units;
@property (nonatomic, retain) NSNumber * unitsScaleImperial;
@property (nonatomic, retain) NSNumber * unitsScaleMetric;
@property (nonatomic, retain) NSNumber * vertDist;
@property (nonatomic, retain) NSManagedObject *location;

@end
