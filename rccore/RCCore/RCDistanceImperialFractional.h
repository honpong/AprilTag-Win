//
//  RCDistanceImperialFractional.h
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceImperial.h"
#import "RCFraction.h"

@interface RCDistanceImperialFractional : NSObject <RCDistance>

@property (readonly) float meters;
@property (readonly) UnitsScale scale;
@property (readonly) NSString* unitSymbol;

@property (readonly) int wholeMiles;
@property (readonly) int wholeYards;
@property (readonly) int wholeFeet;
@property (readonly) int wholeInches;
@property (readonly) RCFraction* fraction;

+ (RCDistanceImperialFractional*) distWithMeters:(float)meters withScale:(UnitsScale)unitsScale;
+ (RCDistanceImperialFractional*) distWithMiles:(int)miles withYards:(int)yards withFeet:(int)feet withInches:(int)inches withNominator:(int)nom withDenominator:(int)denom withScale:(UnitsScale)unitsScale;

- (NSString*) getStringWithoutFractionOrUnitsSymbol;

@end
