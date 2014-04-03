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

@property float meters;
@property UnitsScale scale;
@property NSString* unitSymbol;

@property int wholeMiles;
@property int wholeYards;
@property int wholeFeet;
@property int wholeInches;
@property RCFraction* fraction;

+ (id) distWithMeters:(float)meters withScale:(UnitsScale)unitsScale;
+ (id) distWithMiles:(int)miles withYards:(int)yards withFeet:(int)feet withInches:(int)inches withNominator:(int)nom withDenominator:(int)denom;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)scale;
- (NSString*) getString;
- (NSMutableString*) getStringWithoutFractionOrUnitsSymbol;

@end
