//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#define METERS_PER_INCH 0.0254
#define METERS_PER_FOOT 0.3048
#define METERS_PER_YARD 0.9144
#define INCHES_PER_METER 39.3700787
#define INCHES_PER_FOOT 12
#define INCHES_PER_YARD 36
#define INCHES_PER_MILE 63360

typedef struct {
    int nominator;
    int denominator;
} Fraction;

typedef enum {
    UnitsMetric = 0, UnitsImperial = 1
} Units;

//items are numbered by their order in the segmented button list. that's why there's two 0s, etc. not ideal, but convenient and workable.
typedef enum {
    UnitsScaleKM = 0, UnitsScaleM = 1, UnitsScaleCM = 2,
    UnitsScaleMI = 0, UnitsScaleYD = 1, UnitsScaleFT = 2, UnitsScaleIN = 3
} UnitsScale;

@interface RCDistanceFormatter : NSObject

+ (NSString*)getFormattedDistance:(float)meters withUnits:(Units)units withScale:(UnitsScale)scale withFractional:(BOOL)fractional;

@end


