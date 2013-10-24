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

@interface TMPoint ()

@property (nonatomic, readwrite) vFloat vector;
@property (nonatomic, readwrite) vFloat standardDeviation;

@property (nonatomic, readwrite) float x;
@property (nonatomic, readwrite) float y;
@property (nonatomic, readwrite) float z;

@property (nonatomic, readwrite) float stdx;
@property (nonatomic, readwrite) float stdy;
@property (nonatomic, readwrite) float stdz;

@end

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
    if (self = [super init])
    {
        self.x = [decoder decodeFloatForKey:kTMKeyX];
        self.y = [decoder decodeFloatForKey:kTMKeyY];
        self.z = [decoder decodeFloatForKey:kTMKeyZ];
        self.stdx = [decoder decodeFloatForKey:kTMKeyStdX];
        self.stdy = [decoder decodeFloatForKey:kTMKeyStdY];
        self.stdz = [decoder decodeFloatForKey:kTMKeyStdZ];
    }
    return self;
}

@end
