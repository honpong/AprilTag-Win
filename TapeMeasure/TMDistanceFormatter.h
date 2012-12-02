//
//  TMDistanceFormatter.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@class TMMeasurement;

@interface TMDistanceFormatter : NSObject
+ (NSString*)formattedDistance:(NSNumber*)dist withMeasurement:(TMMeasurement*)measurement;
+ (NSString*)formattedDistance:(NSNumber*)dist withUnits:(NSNumber*)units withScale:(NSNumber*)scale withFractional:(NSNumber*)fractional;
@end
