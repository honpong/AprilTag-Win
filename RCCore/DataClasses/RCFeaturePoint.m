//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCFeaturePoint.h"

@implementation RCFeaturePoint
{

}

- (id) initWithId:(uint64_t)id withX:(float)x withStdX:(float)stdx withY:(float)y withStdY:(float)stdy withDepth:(float)depth withStdDepth:(float)stdDepth withWx:(float)wx withStdWx:(float)stdwx withWy:(float)wy withStdWy:(float)stdwy withWz:(float)wz withStdWz:(float)stdwz
{
    if(self = [super init])
    {
        _x = x;
        _stdx = stdx;
        _y = y;
        _stdy = stdy;
        _depth = depth;
        _stdDepth = stdDepth;
        _wx = wx;
        _stdwx = stdwx;
        _wy = wy;
        _stdwy = stdwy;
        _wz = wz;
        _stdwz = stdwz;
    }
    return self;
}

@end