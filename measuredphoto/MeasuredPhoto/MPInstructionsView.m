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
@synthesize messageBox;

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        self.translatesAutoresizingMaskIntoConstraints = NO;
        
        circleLayer = [CALayer new];
        [self.layer addSublayer:circleLayer];
        
        dotLayer = [CALayer new];
        [self.layer addSublayer:dotLayer];
        
        messageBox = [MPPaddedLabel new];
        messageBox.text = @"Move up, down, or sideways until the dot reaches the edge of the circle";
        messageBox.numberOfLines = 4;
        messageBox.textAlignment = NSTextAlignmentCenter;
        messageBox.backgroundColor = [UIColor blackColor];
        messageBox.textColor = [UIColor whiteColor];
        messageBox.alpha = .3;
        messageBox.translatesAutoresizingMaskIntoConstraints = NO;
        
        [self addSubview:messageBox];
        [messageBox addWidthConstraint:320 andHeightConstraint:90];
        [messageBox addCenterXInSuperviewConstraints];
        [self addConstraint:[NSLayoutConstraint constraintWithItem:messageBox
                                                         attribute:NSLayoutAttributeTop
                                                         relatedBy:NSLayoutRelationEqual
                                                            toItem:self
                                                         attribute:NSLayoutAttributeBottom
                                                        multiplier:1.
                                                          constant:20.]];
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

- (void) moveDotToX:(CGFloat)x andY:(CGFloat)y
{
    dotLayer.frame = CGRectMake(x, y, dotLayer.frame.size.width, dotLayer.frame.size.height);
    [dotLayer setNeedsLayout];
}

@end
