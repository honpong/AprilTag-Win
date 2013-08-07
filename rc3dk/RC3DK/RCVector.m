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

- (NSDictionary*) dictionaryRepresenation
{
    //create a dictionary and add the two memebers of this class as floats
    NSMutableDictionary *tmpDic = [NSMutableDictionary dictionaryWithCapacity:6];
    [tmpDic setObject:[NSNumber numberWithFloat:self.x] forKey:@"x"];
    [tmpDic setObject:[NSNumber numberWithFloat:self.y] forKey:@"y"];
    [tmpDic setObject:[NSNumber numberWithFloat:self.z] forKey:@"z"];
    [tmpDic setObject:[NSNumber numberWithFloat:self.stdx] forKey:@"stdx"];
    [tmpDic setObject:[NSNumber numberWithFloat:self.stdy] forKey:@"stdy"];
    [tmpDic setObject:[NSNumber numberWithFloat:self.stdz] forKey:@"stdz"];
    
    //we return an immutable version
    return [NSDictionary dictionaryWithDictionary:tmpDic];
}

@end