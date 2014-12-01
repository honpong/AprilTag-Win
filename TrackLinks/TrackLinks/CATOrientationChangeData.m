//
//  MPOrientationChangeData.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/1/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "CATOrientationChangeData.h"

@implementation CATOrientationChangeData

+ (CATOrientationChangeData*) dataWithOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated
{
    return (CATOrientationChangeData*)[[CATOrientationChangeData alloc] initWithOrientation:orientation animated:animated];
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
