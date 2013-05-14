//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCConstants.h"

#define METERS_PER_INCH 0.0254
#define METERS_PER_FOOT 0.3048
#define METERS_PER_YARD 0.9144
#define METERS_PER_MILE 1609.344
#define INCHES_PER_METER 39.3700787
#define INCHES_PER_FOOT 12
#define INCHES_PER_YARD 36
#define INCHES_PER_MILE 63360
#define SIXTEENTH_INCH 0.03125

@interface RCDistanceFormatter : NSObject

+ (NSString*)getFormattedDistance:(float)meters withUnits:(Units)units withScale:(UnitsScale)scale withFractional:(BOOL)fractional;
+ (UnitsScale)autoSelectUnitsScale:(float)meters withUnits:(Units)units;

@end


