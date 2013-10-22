//
//  TMFeaturePoint.m
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeaturePoint.h"

static NSString* kTMKeyX = @"kTMKeyX";
static NSString* kTMKeyY = @"kTMKeyY";
static NSString* kTMKeyOriginalDepth = @"kTMKeyOriginalPoint";
static NSString* kTMKeyWorldPoint = @"kTMKeyWorldPoint";

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

- (void) encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeFloat:self.x forKey:kTMKeyX];
    [encoder encodeFloat:self.y forKey:kTMKeyY];
    [encoder encodeObject:self.originalDepth forKey:kTMKeyOriginalDepth];
    [encoder encodeObject:self.worldPoint forKey:kTMKeyWorldPoint];
}

- (id) initWithCoder:(NSCoder *)decoder
{
    float x = [decoder decodeFloatForKey:kTMKeyX];
    float y = [decoder decodeFloatForKey:kTMKeyY];
    TMScalar* originalDepth = [decoder decodeObjectForKey:kTMKeyOriginalDepth];
    TMPoint* worldPoint = [decoder decodeObjectForKey:kTMKeyWorldPoint];
    
    TMFeaturePoint* featurePoint = [[TMFeaturePoint alloc] initWithX:x withY:y withOriginalDepth:originalDepth withWorldPoint:worldPoint];
    
    return featurePoint;
}

#pragma mark - Data Helpers

+ (TMFeaturePoint*) unarchivePackageData:(NSData *)data
{
    TMFeaturePoint *featurePoint = [NSKeyedUnarchiver unarchiveObjectWithData:data];
    return featurePoint;
}

@end
