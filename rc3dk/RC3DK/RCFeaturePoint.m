//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCFeaturePoint.h"
#import "RCPoint.h"

@implementation RCFeaturePoint
{

}

- (id) initWithId:(uint64_t)id withX:(float)x withY:(float)y withOriginalDepth:(RCScalar *)originalDepth withWorldPoint:(RCPoint *)worldPoint withInitialized:(bool)initialized
{
    if(self = [super init])
    {
        _id = id;
        _x = x;
        _y = y;
        _originalDepth = originalDepth;
        _worldPoint = worldPoint;
        _initialized = initialized;
    }
    return self;
}

- (NSDictionary*) dictionaryRepresentation
{
    NSDictionary* dict = @{
                        @"id": @(self.id),
                        @"x": @(self.x),
                        @"y": @(self.y),
                        @"originalDepth": [self.originalDepth dictionaryRepresentation],
                        @"worldPoint": [self.worldPoint dictionaryRepresentation],
                        @"initialized": @(self.initialized)
                        };
    
    return dict;
}

- (float) pixelDistanceToPoint:(CGPoint)cgPoint
{
    return sqrtf(powf(cgPoint.x - self.x, 2) + powf(cgPoint.y - self.y, 2));
}

- (CGPoint) makeCGPoint
{
    return CGPointMake(self.x, self.y);
}

- (NSDictionary *)proxyForJson
{
    NSDictionary* dict = @{
                           @"x": @(self.x),
                           @"y": @(self.y),
                           @"originalDepth": [self.originalDepth dictionaryRepresentation],
                           @"worldPoint": [self.worldPoint dictionaryRepresentation],
                           @"initialized": @(self.initialized)
                           };
    return dict;
}

@end
