//
//  TMScalar.m
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMScalar.h"

static NSString* kTMKeyScalar = @"kTMKeyScalar";
static NSString* kTMKeyStdDev = @"kTMKeyStdDev";

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

- (void) encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeFloat:self.scalar forKey:kTMKeyScalar];
    [encoder encodeFloat:self.standardDeviation forKey:kTMKeyStdDev];
}

- (id) initWithCoder:(NSCoder *)decoder
{
    float scalar = [decoder decodeFloatForKey:kTMKeyScalar];
    float stdDev = [decoder decodeFloatForKey:kTMKeyStdDev];
    
    TMScalar* result = [[TMScalar alloc] initWithScalar:scalar withStdDev:stdDev];
    return result;
}

#pragma mark - Data Helpers

+ (TMScalar*) unarchivePackageData:(NSData *)data
{
    TMScalar *scalar = [NSKeyedUnarchiver unarchiveObjectWithData:data];
    return scalar;
}

@end
