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

- (id) initWithId:(uint64_t)id withX:(float)x withY:(float)y withDepth:(RCScalar *)depth withWorldPoint:(RCPoint *)worldPoint withInitialized:(bool)initialized
{
    if(self = [super init])
    {
        _id = id;
        _x = x;
        _y = y;
        _depth = depth;
        _worldPoint = worldPoint;
        _initialized = initialized;
    }
    return self;
}

- (NSDictionary*) dictionaryRepresentation
{
    //instead of making this flat, we're going to call a function which recursively calls to_dictionary on other classes.
    NSMutableDictionary *tmpDic = [NSMutableDictionary dictionaryWithCapacity:6];
    [tmpDic setObject:[NSNumber numberWithUnsignedInt:self.id] forKey:@"id"];
    [tmpDic setObject:[NSNumber numberWithFloat:self.x] forKey:@"x"];
    [tmpDic setObject:[NSNumber numberWithFloat:self.y] forKey:@"y"];
    [tmpDic setObject:[self.depth dictionaryRepresentation] forKey:@"depth"];
    [tmpDic setObject:[self.worldPoint dictionaryRepresentation] forKey:@"worldPoint"];
    [tmpDic setObject:[NSNumber numberWithBool:self.initialized] forKey:@"initialized"];
    
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