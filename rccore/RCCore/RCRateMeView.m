//
//  RCRateMeView.m
//  RCCore
//
//  Created by Ben Hirashima on 8/26/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCRateMeView.h"
#import "UIView+RCConstraints.h"

@implementation RCRateMeView

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
    self.backgroundColor = [UIColor colorWithRed:0 green:.6 blue:1. alpha:1.];
    self.layer.cornerRadius = 10;
    self.clipsToBounds = YES;
    
    UILabel* label = [UILabel new];
    label.text = @"Thanks for trying our app. Please rate it on the AppStore!";
    label.textColor = [UIColor whiteColor];
    label.numberOfLines = 2;
    label.textAlignment = NSTextAlignmentCenter;
    
    UIButton* yesButton = [UIButton new];
    [yesButton setTitle:@"Rate now" forState:UIControlStateNormal];
    yesButton.titleLabel.textColor = [UIColor whiteColor];
    
    UIButton* laterButton = [UIButton new];
    [laterButton setTitle:@"Later" forState:UIControlStateNormal];
    laterButton.titleLabel.textColor = [UIColor whiteColor];
    
    UIButton* noButton = [UIButton new];
    [noButton setTitle:@"Never!" forState:UIControlStateNormal];
    noButton.titleLabel.textColor = [UIColor whiteColor];
    
    [self addSubview:label];
    [label addLeadingSpaceToSuperviewConstraint:10];
    [label addTrailingSpaceToSuperviewConstraint:10];
    [label addTopSpaceToSuperviewConstraint:14];
    
    [self addSubview:laterButton];
    [laterButton addBottomSpaceToSuperviewConstraint:1];
    [laterButton addLeadingSpaceToSuperviewConstraint:0];
    [laterButton addWidthConstraint:width/2 andHeightConstraint:buttonHeight];
    
    [self addSubview:noButton];
    [noButton addBottomSpaceToSuperviewConstraint:1];
    [noButton addTrailingSpaceToSuperviewConstraint:0];
    [noButton addWidthConstraint:width/2 andHeightConstraint:buttonHeight];
    
    [self addSubview:yesButton];
    [yesButton addBottomSpaceToViewConstraint:noButton withDist:0];
    [yesButton addMatchWidthToSuperviewConstraints];
    [yesButton addHeightConstraint:buttonHeight];
}

- (void) didMoveToSuperview
{
    [self addWidthConstraint:width];
    [self setHeightConstraint:height];
}

- (void)drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSetStrokeColorWithColor(context, [[UIColor colorWithWhite:1 alpha:.6] CGColor]);
    CGContextSetLineWidth(context, 1.);
    
    CGFloat topLineHeight = rect.size.height - 1 - buttonHeight*2;
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
