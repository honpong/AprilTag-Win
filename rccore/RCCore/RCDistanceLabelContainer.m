//
//  RCDistanceLabelContainer.m
//  RCCore
//
//  Created by Ben Hirashima on 10/2/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCDistanceLabelContainer.h"
#import "UIView+RCConstraints.h"

@interface RCDistanceLabelContainer ()

@property (readwrite) UILabel* distanceLabel;
@property (readwrite) RCFractionLabel* fractionLabel;
@property (readwrite) UILabel* symbolLabel;

@end

@implementation RCDistanceLabelContainer
{
    NSLayoutConstraint* heightConstraint;
    NSLayoutConstraint* containerHeightConstraint;
    NSLayoutConstraint* distHeightConstraint;
    NSLayoutConstraint* symbolHeightConstraint;
    NSLayoutConstraint* symbolTrailingSpaceConstraint;
    NSLayoutConstraint* alignmentConstraint;
    NSLayoutConstraint* distLeadingSpaceConstraint;
    NSLayoutConstraint* distTrailingSpaceConstraint;
}
@synthesize distanceLabel, fractionLabel, symbolLabel, centerAlignmentExcludesFraction, textAlignment;

- (id) initWithCoder:(NSCoder *)decoder
{
    if (self = [super initWithCoder:decoder])
    {
        [self setupViews];
    }
    return self;
}

- (id) initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self setupViews];
    }
    return self;
}

- (void) setupViews
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    self.backgroundColor = [UIColor clearColor];
    
    distanceLabel = [UILabel new];
    distanceLabel.translatesAutoresizingMaskIntoConstraints = NO;
    distanceLabel.textAlignment = NSTextAlignmentRight;
    distanceLabel.backgroundColor = [UIColor clearColor];
    [self addSubview:distanceLabel];
    
    fractionLabel = [RCFractionLabel new];
    fractionLabel.backgroundColor = [UIColor clearColor];
    [self addSubview:fractionLabel];
    
    symbolLabel = [UILabel new];
    symbolLabel.translatesAutoresizingMaskIntoConstraints = NO;
    symbolLabel.backgroundColor = [UIColor clearColor];
    symbolLabel.text = @"\"";
    [self addSubview:symbolLabel];
}

- (void) updateConstraints
{
    [distanceLabel removeConstraintsFromSuperview];
    [fractionLabel removeConstraintsFromSuperview];
    [symbolLabel removeConstraintsFromSuperview];
    
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[distanceLabel]-1-[fractionLabel][symbolLabel]"
                                                                          options:0
                                                                          metrics:nil
                                                                            views:NSDictionaryOfVariableBindings(distanceLabel, fractionLabel, symbolLabel)]];
    
    // symbol label
    [symbolLabel addBottomSpaceToSuperviewConstraint:0];
    
    // fraction label
    [fractionLabel addBottomSpaceToSuperviewConstraint:0];
    
    // distance label
    [distanceLabel addBottomSpaceToSuperviewConstraint:0];
    distLeadingSpaceConstraint = [distanceLabel addLeftSpaceToSuperviewConstraint:0];
    
    [super updateConstraints];
}

- (CGSize) intrinsicContentSize
{
    return [self sizeThatFits:CGSizeZero];
}

- (CGSize) sizeThatFits:(CGSize)size
{
    [distanceLabel sizeToFit];
    [fractionLabel sizeToFit];
    [symbolLabel sizeToFit];
    
    CGFloat width;
    
    if (textAlignment == NSTextAlignmentCenter)
    {
        if (centerAlignmentExcludesFraction && distanceLabel.bounds.size.width)
            width = distanceLabel.bounds.size.width;
        else
            width = distanceLabel.bounds.size.width + fractionLabel.bounds.size.width;
    }
    else
    {
        width = distanceLabel.bounds.size.width + fractionLabel.bounds.size.width + symbolLabel.bounds.size.width;
    }
    
    CGFloat height;
    if (distanceLabel.bounds.size.height > 0)
        height = distanceLabel.bounds.size.height;
    else
        height = fractionLabel.bounds.size.height;
    
    return CGSizeMake(width, height);
}

- (void)invalidateIntrinsicContentSize
{
    [distanceLabel invalidateIntrinsicContentSize];
    [fractionLabel invalidateIntrinsicContentSize];
    [symbolLabel invalidateIntrinsicContentSize];
    [super invalidateIntrinsicContentSize];
}

@end
