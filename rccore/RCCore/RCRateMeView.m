//
//  RCRateMeView.m
//  RCCore
//
//  Created by Ben Hirashima on 8/26/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCRateMeView.h"
#import "UIView+RCConstraints.h"
#import "RCInvertButton.h"

@implementation RCRateMeView
{
    UILabel* label;
    RCInvertButton* yesButton;
    RCInvertButton* laterButton;
    RCInvertButton* noButton;
    UIColor* lightBlue;
}

static const CGFloat width = 300;
static const CGFloat height = 160;
static const CGFloat buttonHeight = 45;

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self initialize];
    }
    return self;
}

- (void) initialize
{
    lightBlue = [UIColor colorWithRed:0 green:.6 blue:1. alpha:1.];
    
    self.backgroundColor = lightBlue;
    self.layer.cornerRadius = 10;
    self.clipsToBounds = YES;
    
    label = [UILabel new];
    label.text = @"Thanks for trying our app. Help us out by rating it on the App Store!";
    label.textColor = [UIColor whiteColor];
    label.numberOfLines = 2;
    label.textAlignment = NSTextAlignmentCenter;
    
    yesButton = [RCInvertButton new];
    [yesButton setTitle:@"Rate now" forState:UIControlStateNormal];
    yesButton.titleLabel.textColor = [UIColor whiteColor];
    yesButton.invertedColor = lightBlue;
    
    laterButton = [RCInvertButton new];
    [laterButton setTitle:@"Later" forState:UIControlStateNormal];
    laterButton.titleLabel.textColor = [UIColor whiteColor];
    laterButton.invertedColor = lightBlue;
    
    noButton = [RCInvertButton new];
    [noButton setTitle:@"Never!" forState:UIControlStateNormal];
    noButton.titleLabel.textColor = [UIColor whiteColor];
    noButton.invertedColor = lightBlue;
    
    [self addSubview:label];
    [label addLeadingSpaceToSuperviewConstraint:10];
    [label addTrailingSpaceToSuperviewConstraint:10];
    [label addTopSpaceToSuperviewConstraint:14];
    
    [self addSubview:laterButton];
    [laterButton addBottomSpaceToSuperviewConstraint:0];
    [laterButton addLeadingSpaceToSuperviewConstraint:0];
    [laterButton addWidthConstraint:width/2 andHeightConstraint:buttonHeight];
    [laterButton addTarget:self action:@selector(laterButtonTapped) forControlEvents:UIControlEventTouchUpInside];
    
    [self addSubview:noButton];
    [noButton addBottomSpaceToSuperviewConstraint:0];
    [noButton addTrailingSpaceToSuperviewConstraint:0];
    [noButton addWidthConstraint:width/2 andHeightConstraint:buttonHeight];
    [noButton addTarget:self action:@selector(neverButtonTapped) forControlEvents:UIControlEventTouchUpInside];
    
    [self addSubview:yesButton];
    [yesButton addBottomSpaceToViewConstraint:noButton withDist:0];
    [yesButton addMatchWidthToSuperviewConstraints];
    [yesButton addHeightConstraint:buttonHeight];
    [yesButton addTarget:self action:@selector(rateButtonTapped) forControlEvents:UIControlEventTouchUpInside];
}

- (void) didMoveToSuperview
{
    [self addWidthConstraint:width];
    [self setHeightConstraint:height];
}

- (void) rateButtonTapped
{
    if ([self.delegate respondsToSelector:@selector(handleRateNowButton)]) [self.delegate handleRateNowButton];
}

- (void) laterButtonTapped
{
    if ([self.delegate respondsToSelector:@selector(handleRateLaterButton)]) [self.delegate handleRateLaterButton];
}

- (void) neverButtonTapped
{
    if ([self.delegate respondsToSelector:@selector(handleRateNeverButton)]) [self.delegate handleRateNeverButton];
}

- (void)drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSetStrokeColorWithColor(context, [[UIColor colorWithWhite:1 alpha:.6] CGColor]);
    CGContextSetLineWidth(context, 1.);
    
    CGFloat topLineHeight = rect.size.height - buttonHeight*2;
    CGFloat midLineHeight = rect.size.height - buttonHeight;
    
	CGContextMoveToPoint(context, 0, topLineHeight);
    CGContextAddLineToPoint(context, rect.size.width, topLineHeight);
    
    CGContextMoveToPoint(context, 0, midLineHeight);
    CGContextAddLineToPoint(context, rect.size.width, midLineHeight);
    
    CGContextMoveToPoint(context, rect.size.width / 2, midLineHeight);
    CGContextAddLineToPoint(context, rect.size.width / 2, rect.size.height);
    
    CGContextStrokePath(context);
}


@end
