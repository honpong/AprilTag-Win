//
//  TMFeaturePoint.m
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeaturePoint.h"

static NSString* const kTMKeyX = @"kTMKeyX";
static NSString* const kTMKeyY = @"kTMKeyY";
static NSString* const kTMKeyOriginalDepth = @"kTMKeyOriginalPoint";
static NSString* const kTMKeyWorldPoint = @"kTMKeyWorldPoint";

@interface TMFeaturePoint ()

@property (nonatomic, readwrite) float x;
@property (nonatomic, readwrite) float y;
@property (nonatomic, readwrite) TMScalar *originalDepth;
@property (nonatomic, readwrite) TMPoint *worldPoint;

@end

@implementation TMFeaturePoint

- (id) initWithX:(float)x withY:(float)y withOriginalDepth:(TMScalar *)originalDepth withWorldPoint:(TMPoint *)worldPoint
{
    if(self = [super init])
    {
        _x = x;
        _y = y;
        _originalDepth = originalDepth;
        _worldPoint = worldPoint;
    }
    return self;
}

- (float) pixelDistanceToPoint:(CGPoint)cgPoint
{
    return sqrtf(powf(cgPoint.x - self.x, 2) + powf(cgPoint.y - self.y, 2));
}

- (CGPoint) makeCGPoint
{
    return CGPointMake(self.x, self.y);
}

#pragma mark - NSCoding

+ (BOOL) supportsSecureCoding
{
    return YES;
}

- (void) encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeFloat:self.x forKey:kTMKeyX];
    [encoder encodeFloat:self.y forKey:kTMKeyY];
    [encoder encodeObject:self.originalDepth forKey:kTMKeyOriginalDepth];
    [encoder encodeObject:self.worldPoint forKey:kTMKeyWorldPoint];
}

- (id) initWithCoder:(NSCoder *)decoder
{
    if (self = [super init])
    {
        _x = [decoder decodeFloatForKey:kTMKeyX];
        _y = [decoder decodeFloatForKey:kTMKeyY];
        _originalDepth = [decoder decodeObjectOfClass:[TMScalar class] forKey:kTMKeyOriginalDepth];
        _worldPoint = [decoder decodeObjectOfClass:[TMPoint class] forKey:kTMKeyWorldPoint];
    }
    return self;
}

@end
