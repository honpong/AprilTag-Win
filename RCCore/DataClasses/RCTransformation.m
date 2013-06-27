//
//  RCTransformation.m
//  RCCore
//
//  Created by Ben Hirashima on 6/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCTransformation.h"

@implementation RCTransformation

- (id) initWithTranslation:(RCTranslation*)translation withRotation:(RCRotation*)rotation
{
    if (self = [super init])
    {
        _translation = translation;
        _rotation = rotation;
    }
    return self;
}

@end
