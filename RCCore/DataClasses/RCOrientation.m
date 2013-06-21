//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCOrientation.h"

@implementation RCOrientation
{

}

- (id) initWithRx:(float)rx withStdRx:(float)stdrx withRy:(float)ry withStdRy:(float)stdry withRz:(float)rz withStdRz:(float)stdrz
{
    if(self = [super init])
    {
        _rx = rx;
        _stdrx = stdrx;
        _ry = ry;
        _stdry = stdry;
        _rz = rz;
        _stdrz = stdrz;
    }
    return self;
}

@end