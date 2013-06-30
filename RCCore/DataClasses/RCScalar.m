//
//  RCScalar.m
//  RCCore
//
//  Created by Eagle Jones on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCScalar.h"

@implementation RCScalar

- (id) initWithScalar:(float)scalar withStdDev:(float)stdDev
{
    if(self = [super init])
    {
        _scalar = scalar;
        _standardDeviation = stdDev;
    }
    return self;
}

@end
