//
//  TMScalar.m
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMScalar.h"

@implementation TMScalar

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
