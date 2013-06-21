//
//  RCTransformation.m
//  RCCore
//
//  Created by Ben Hirashima on 6/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCTransformation.h"

@implementation RCTransformation

- (id) initWithPosition:(RCPosition*)position withOrientation:(RCOrientation*)orientation
{
    if (self = [super init])
    {
        _position = position;
        _orientation = orientation;
    }
    return self;
}

@end
