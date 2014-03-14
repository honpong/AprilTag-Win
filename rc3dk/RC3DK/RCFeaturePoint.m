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
    //instead of making this flat, we're going to call a function which recursively calls to_dictionary on other classes.
    NSMutableDictionary *tmpDic = [NSMutableDictionary dictionaryWithCapacity:6];
    tmpDic[@"id"] = [NSNumber numberWithUnsignedInt:self.id];
    tmpDic[@"x"] = @(self.x);
    tmpDic[@"y"] = @(self.y);
    tmpDic[@"originalDepth"] = [self.originalDepth dictionaryRepresentation];
    tmpDic[@"worldPoint"] = [self.worldPoint dictionaryRepresentation];
    tmpDic[@"initialized"] = @(self.initialized);
    
    //we return an immutable version
    return [NSDictionary dictionaryWithDictionary:tmpDic];
}

- (float) pixelDistanceToPoint:(CGPoint)cgPoint
{
    return sqrtf(powf(cgPoint.x - self.x, 2) + powf(cgPoint.y - self.y, 2));
}

- (CGPoint) makeCGPoint
{
    return CGPointMake(self.x, self.y);
}

@end
