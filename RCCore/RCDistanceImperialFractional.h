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
{
    NSString* stringRep;
    float convertedDist;
}

@property float meters;
@property UnitsScale scale;
@property NSString* unitSymbol;

@property int wholeMiles;
@property int wholeYards;
@property int wholeFeet;
@property int wholeInches;
@property RCFraction* fraction;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)scale;
- (NSString*) getString;
- (NSMutableString*) getStringWithoutFractionOrUnitsSymbol;

@end
