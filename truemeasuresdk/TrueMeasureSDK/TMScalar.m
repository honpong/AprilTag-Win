//
//  TMScalar.m
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMScalar.h"

static NSString* const kTMKeyScalar = @"kTMKeyScalar";
static NSString* const kTMKeyStdDev = @"kTMKeyStdDev";

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

#pragma mark - NSCoding

+ (BOOL) supportsSecureCoding
{
    return YES;
}

- (void) encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeFloat:self.scalar forKey:kTMKeyScalar];
    [encoder encodeFloat:self.standardDeviation forKey:kTMKeyStdDev];
}

- (id) initWithCoder:(NSCoder *)decoder
{
    if (self = [super init])
    {
        _scalar = [decoder decodeFloatForKey:kTMKeyScalar];
        _standardDeviation = [decoder decodeFloatForKey:kTMKeyStdDev];
    }
    return self;
}

@end
