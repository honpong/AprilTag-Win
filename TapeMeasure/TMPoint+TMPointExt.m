//
//  TMPoint+TMPointExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMPoint+TMPointExt.h"

@implementation TMPoint (TMPointExt)

- (float) distanceToPoint:(CGPoint)cgPoint
{
    return sqrtf(powf(cgPoint.x - self.imageX, 2) + powf(cgPoint.y - self.imageY, 2));
}

@end
