//
//  TMDistanceFormatter.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@class TMMeasurement;

typedef struct {
    int nominator;
    int denominator;
} Fraction;

typedef enum {
    UnitsMetric = 0, UnitsImperial = 1
} Units;

//items are numbered by their order in the segmented button list. that's why there's two 0s, etc. not ideal, but convenient and workable.
typedef enum {
    UnitsScaleKM = 0, UnitsScaleM = 1, UnitsScaleCM = 2, UnitsScaleMI = 0, UnitsScaleYD = 1, UnitsScaleFT = 2, UnitsScaleIN = 3
} UnitsScale;

@interface TMDistanceFormatter : NSObject
+ (NSString*)formattedDistance:(NSNumber*)meters withMeasurement:(TMMeasurement*)measurement;
+ (NSString*)formattedDistance:(float)meters withUnits:(Units)units withScale:(UnitsScale)scale withFractional:(BOOL)fractional;
@end


