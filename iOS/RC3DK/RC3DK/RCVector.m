//
//  RCVector.m
//  RCCore
//
//  Created by Eagle Jones on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//
#import <Foundation/Foundation.h>
#import "RCVector.h"

@implementation RCVector
{
    
}

- (float) x
{
    return _vector[0];
}

- (float) y
{
    return _vector[1];
}

- (float) z
{
    return _vector[2];
}

- (float) v0
{
    return _vector[0];
}

- (float) v1
{
    return _vector[1];
}

- (float) v2
{
    return _vector[2];
}

- (float) v3
{
    return _vector[3];
}

- (float) stdx
{
    return _standardDeviation[0];
}

- (float) stdy
{
    return _standardDeviation[1];
}

- (float) stdz
{
    return _standardDeviation[2];
}


- (float) std0
{
    return _vector[0];
}

- (float) std1
{
    return _vector[1];
}

- (float) std2
{
    return _vector[2];
}

- (float) std3
{
    return _vector[3];
}

- (id) initWithVector:(vFloat)vector withStandardDeviation:(vFloat)standardDeviation
{
    if(self = [super init])
    {
        _vector = vector;
        _standardDeviation = standardDeviation;
    }
    return self;
}

- (id) initWithX:(float)x withStdX:(float)stdx withY:(float)y withStdY:(float)stdy withZ:(float)z withStdZ:(float)stdz
{
    if(self = [super init])
    {
        _vector = (vFloat){x, y, z, 0.};
        _standardDeviation = (vFloat){stdx, stdy, stdz, 0.};
    }
    return self;
}

- (id) initWithX:(float)x withY:(float)y withZ:(float)z
{
    if(self = [super init])
    {
        _vector = (vFloat){x, y, z, 0.};
    }
    return self;
}

- (NSDictionary*) dictionaryRepresentation
{
    NSDictionary* dict = @{
                           @"v0": @(self.x),
                           @"v1": @(self.y),
                           @"v2": @(self.z),
                           @"v3": @(self.v3),
                           @"std0": @(self.stdx),
                           @"std1": @(self.stdy),
                           @"std2": @(self.stdz),
                           @"std3": @(self.std3)
                           };
    return dict;
}

@end