//
//  MPOrientationChangeData.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/1/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPOrientationChangeData.h"

@implementation MPOrientationChangeData

+ (MPOrientationChangeData*) dataWithOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated
{
    return (MPOrientationChangeData*)[[MPOrientationChangeData alloc] initWithOrientation:orientation animated:animated];
}

- (id) initWithOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated
{
    if (self = [super init])
    {
        self.orientation = orientation;
        self.animated = animated;
    }
    return self;
}

@end
