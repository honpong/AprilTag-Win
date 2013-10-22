//
//  TMPoint.m
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMPoint.h"

static NSString* kTMKeyX = @"kTMKeyX";
static NSString* kTMKeyY = @"kTMKeyY";
static NSString* kTMKeyZ = @"kTMKeyZ";
static NSString* kTMKeyStdX = @"kTMKeyStdX";
static NSString* kTMKeyStdY = @"kTMKeyStdY";
static NSString* kTMKeyStdZ = @"kTMKeyStdZ";

@implementation TMPoint

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


#pragma mark - NSCoding

- (void) encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeFloat:self.x forKey:kTMKeyX];
    [encoder encodeFloat:self.y forKey:kTMKeyY];
    [encoder encodeFloat:self.z forKey:kTMKeyZ];
    [encoder encodeFloat:self.stdx forKey:kTMKeyStdX];
    [encoder encodeFloat:self.stdy forKey:kTMKeyStdY];
    [encoder encodeFloat:self.stdz forKey:kTMKeyStdZ];
}

- (id) initWithCoder:(NSCoder *)decoder
{
    float x = [decoder decodeFloatForKey:kTMKeyX];
    float y = [decoder decodeFloatForKey:kTMKeyY];
    float z = [decoder decodeFloatForKey:kTMKeyZ];
    float stdx = [decoder decodeFloatForKey:kTMKeyStdX];
    float stdy = [decoder decodeFloatForKey:kTMKeyStdY];
    float stdz = [decoder decodeFloatForKey:kTMKeyStdZ];
    
    TMPoint* point = [[TMPoint alloc] initWithX:x withStdX:y withY:z withStdY:stdx withZ:stdy withStdZ:stdz];
    
    return point;
}

#pragma mark - Data Helpers

+ (TMPoint*) unarchivePackageData:(NSData *)data
{
    TMPoint *point = [NSKeyedUnarchiver unarchiveObjectWithData:data];
    return point;
}


@end
