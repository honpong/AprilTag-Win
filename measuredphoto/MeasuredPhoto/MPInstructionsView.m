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
        self.translatesAutoresizingMaskIntoConstraints = NO;
        
        circleLayer = [CALayer new];
        [self.layer addSublayer:circleLayer];
        
        dotLayer = [CALayer new];
        [self.layer addSublayer:dotLayer];
    }
    return self;
}

- (void) layoutSubviews
{
    [super layoutSubviews];
    
    circleLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    circleLayerDel = [MPCircleLayerDelegate new];
    circleLayer.delegate = circleLayerDel;
    [circleLayer setNeedsDisplay];
    
    dotLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    dotLayerDel = [MPDotLayerDelegate new];
    dotLayer.delegate = dotLayerDel;
    [dotLayer setNeedsDisplay];
}

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    
}

- (void) moveDotTo:(CGPoint)point
{
    dotLayer.frame = CGRectMake(point.x, point.y, dotLayer.frame.size.width, dotLayer.frame.size.height);
    [dotLayer setNeedsLayout];
}

@end
