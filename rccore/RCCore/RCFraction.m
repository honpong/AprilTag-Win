//
//  RCFraction.m
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFraction.h"

@implementation RCFraction
@synthesize nominator, denominator, floatValue;

+ (RCFraction*) fractionWithInches:(float)inches
{
    return [[RCFraction alloc] initWithInches:inches];
}

+ (RCFraction*) fractionWithNominator:(int)nom withDenominator:(int)denom
{
    return [[RCFraction alloc] initWithNominator:nom withDenominator:denom];
}

- (id) initWithInches:(float)inches
{
    if(self = [super init])
    {
        int wholeInches = floor(inches);
        float remainder = inches - wholeInches;
        floatValue = remainder;
        
        int eighths = roundf(remainder * 8);
        
        int gcd = [RCFraction gcdForNumber1:eighths andNumber2:8];
        
        nominator = eighths / gcd;
        denominator = 8 / gcd;
    }
    
    return self;
}

- (id) initWithNominator:(int)nom withDenominator:(int)denom
{
    if(self = [super init])
    {
        nominator = nom;
        denominator = denom;
    }
    
    return self;
}

- (BOOL) isEqualToOne
{
    return nominator / denominator == 1 ? YES : NO;
}

- (NSString*) description
{
    return [NSString stringWithFormat:@"%u/%u", nominator, denominator];
}

+ (int) gcdForNumber1:(int) m andNumber2:(int) n
{
    if(!(m && n)) return 1;
    
    while( m!= n) // execute loop until m == n
    {
        if( m > n)
            m= m - n; // large - small , store the results in large variable
        else
            n= n - m;
    }
    return m;
}

- (float)floatValue
{
    return (float)nominator/denominator;
}

@end
