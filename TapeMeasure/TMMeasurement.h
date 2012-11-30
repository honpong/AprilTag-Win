//
//  TMMeasurement.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/6/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>


@interface TMMeasurement : NSManagedObject

@property (nonatomic, retain) NSNumber * horzDist;
@property (nonatomic, retain) NSString * name;
@property (nonatomic, retain) NSNumber * pointToPoint;
@property (nonatomic, retain) NSDate * timestamp;
@property (nonatomic, retain) NSNumber * totalPath;
@property (nonatomic, retain) NSNumber * vertDist;

+(NSString*)formattedDistance:(NSNumber*)dist withUnits:(int)units withFractional:(int)fractional;

@end
