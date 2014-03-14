//
//  RCVector.m
//  RCCore
//
//  Created by Eagle Jones on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

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
    //create a dictionary and add the two memebers of this class as floats
    NSMutableDictionary *tmpDic = [NSMutableDictionary dictionaryWithCapacity:8];
    tmpDic[@"v0"] = @(self.x);
    tmpDic[@"v1"] = @(self.y);
    tmpDic[@"v2"] = @(self.z);
    tmpDic[@"v3"] = @(self.v3);
    tmpDic[@"std0"] = @(self.stdx);
    tmpDic[@"std1"] = @(self.stdy);
    tmpDic[@"std2"] = @(self.stdz);
    tmpDic[@"std3"] = @(self.std3);

    //we return an immutable version
    return [NSDictionary dictionaryWithDictionary:tmpDic];
}

@end