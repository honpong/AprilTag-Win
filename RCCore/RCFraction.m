//
//  RCFraction.m
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFraction.h"

@implementation RCFraction
@synthesize nominator, denominator;

+ (RCFraction*) fractionWithInches:(float)inches
{
    return [[RCFraction alloc] initWithInches:inches];
}

- (id) initWithInches:(float)inches
{
    if(self = [super init])
    {
        int wholeInches = floor(inches);
        float remainder = inches - wholeInches;
        
        int sixteenths = roundf(remainder * 16);
        
        int gcd = [RCFraction gcdForNumber1:sixteenths andNumber2:16];
        
        nominator = sixteenths / gcd;
        denominator = 16 / gcd;
    }
    
    return self;
}

- (BOOL) isEqualToOne
{
    return nominator / denominator == 1 ? YES : NO;
}

- (NSString*) getString
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

@end
