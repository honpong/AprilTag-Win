//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPVideoPreview.h"
#import <QuartzCore/CAEAGLLayer.h>

@implementation MPVideoPreview

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
//        [self setTransformFromCurrentVideoOrientationToOrientation:AVCaptureVideoOrientationPortrait];
    }
    return self;
}

- (void) layoutSubviews
{
//    [self setTransformFromCurrentVideoOrientationToOrientation:AVCaptureVideoOrientationPortrait];
}

@end

