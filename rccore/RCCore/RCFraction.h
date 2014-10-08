//
//  RCFraction.h
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RCFraction : NSObject

@property (readonly) float floatValue;
@property int nominator;
@property int denominator;

- (BOOL) isEqualToOne;

+ (RCFraction*) fractionWithInches:(float)inches;
+ (RCFraction*) fractionWithNominator:(int)nom withDenominator:(int)denom;

@end
