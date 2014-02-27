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
#import <RC3DK/RC3DK.h>
#import "UIView+MPCascadingRotation.h"

@implementation MPInstructionsView
{
    CALayer* circleLayer;
    CALayer* dotLayer;
    MPCircleLayerDelegate* circleLayerDel;
    MPDotLayerDelegate* dotLayerDel;
}
@synthesize delegate;

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

- (void) moveDotTo:(CGPoint)point
{
    dotLayer.frame = CGRectMake(point.x, point.y, dotLayer.frame.size.width, dotLayer.frame.size.height);
    [dotLayer setNeedsLayout];
}

- (void) moveDotToCenter
{
    [self moveDotTo:CGPointMake(0, 0)];
}

- (void) updateDotPosition:(RCTransformation*)transformation
{
    // TODO replace this fake calculation with one that calculates the distance moved in the plane of the photo
    float distFromStartPoint = sqrt(transformation.translation.x * transformation.translation.x + transformation.translation.y * transformation.translation.y + transformation.translation.z * transformation.translation.z);
    float targetDist = .2;
    float progress = distFromStartPoint / targetDist;
    
    if (progress >= 1)
    {
        if ([delegate respondsToSelector:@selector(moveComplete)]) [delegate moveComplete];
    }
    else
    {
        float xPos = 200 * progress;
        [self moveDotTo:CGPointMake(xPos, 0)];
    }
}

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    [self applyRotationTransformation:orientation];
}

@end
