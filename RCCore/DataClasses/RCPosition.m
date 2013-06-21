//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCPosition.h"

@implementation RCPosition
{

}

- (id) initWithX:(float)x withStdX:(float)stdx withY:(float)y withStdY:(float)stdy withZ:(float)z withStdZ:(float)stdz withPath:(float)path withStdPath:(float)stdPath
{
    if(self = [super init])
    {
        _x = x;
        _stdx = stdx;
        _y = y;
        _stdy = stdy;
        _z = z;
        _stdz = stdz;
        _path = path;
        _stdPath = stdPath;
    }
    return self;
}

@end