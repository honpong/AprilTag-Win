//
//  MPInstructionsView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPInstructionsView.h"
#import "MPCircleLayerDelegate.h"
#import "MPDotLayerDelegate.h"
#import "UIView+MPConstraints.h"

@implementation MPInstructionsView
{
    CALayer* circleLayer;
    CALayer* dotLayer;
    MPCircleLayerDelegate* circleLayerDel;
    MPDotLayerDelegate* dotLayerDel;
}

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self addCenterInSuperviewConstraints];
        [self addWidthConstraint:200. andHeightConstraint:200.];
        
        circleLayer = [CALayer new];
        circleLayerDel = [MPCircleLayerDelegate new];
        circleLayer.delegate = circleLayerDel;
        [self.layer addSublayer:circleLayer];
        
        dotLayer = [CALayer new];
        dotLayerDel = [MPDotLayerDelegate new];
        dotLayer.delegate = dotLayerDel;
        [self.layer addSublayer:dotLayer];
    }
    return self;
}

- (void) layoutSubviews
{
    [super layoutSubviews];
    circleLayer.frame = self.frame;
    [circleLayer setNeedsDisplay];
    dotLayer.frame = self.frame;
    [dotLayer setNeedsDisplay];
}

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    
}

@end
