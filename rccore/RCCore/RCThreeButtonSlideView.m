//
//  RCThreeButtonSlideView.m
//  RCCore
//
//  Created by Ben Hirashima on 9/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCThreeButtonSlideView.h"
#import "UIView+RCConstraints.h"

@implementation RCThreeButtonSlideView
{
    UIColor* lightBlue;
}
@synthesize label, button1, button2, button3;

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
    
    self.layer.cornerRadius = 10;
    self.clipsToBounds = YES;
    
    label = [UILabel new];
    label.textColor = [UIColor whiteColor];
    label.numberOfLines = 2;
    label.textAlignment = NSTextAlignmentCenter;
    
    button1 = [RCInvertButton new];
    button1.titleLabel.textColor = [UIColor whiteColor];
    
    button2 = [RCInvertButton new];
    button2.titleLabel.textColor = [UIColor whiteColor];
    
    button3 = [RCInvertButton new];
    button3.titleLabel.textColor = [UIColor whiteColor];
    
    [self addSubview:label];
    [label addLeftSpaceToSuperviewConstraint:10];
    [label addRightSpaceToSuperviewConstraint:10];
    [label addTopSpaceToSuperviewConstraint:14];
    
    [self addSubview:button3];
    [button3 addBottomSpaceToSuperviewConstraint:0];
    [button3 addRightSpaceToSuperviewConstraint:0];
    [button3 addWidthConstraint:width/2 andHeightConstraint:buttonHeight];
    [button3 addTarget:self action:@selector(button3Tapped) forControlEvents:UIControlEventTouchUpInside];
    
    [self addSubview:button2];
    [button2 addBottomSpaceToSuperviewConstraint:0];
    [button2 addLeftSpaceToSuperviewConstraint:0];
    [button2 addWidthConstraint:width/2 andHeightConstraint:buttonHeight];
    [button2 addTarget:self action:@selector(button2Tapped) forControlEvents:UIControlEventTouchUpInside];
    
    [self addSubview:button1];
    [button1 addBottomSpaceToViewConstraint:button3 withDist:0]; // must go after button3 has been added to view hierarchy
    [button1 addMatchWidthToSuperviewConstraints];
    [button1 addHeightConstraint:buttonHeight];
    [button1 addTarget:self action:@selector(button1Tapped) forControlEvents:UIControlEventTouchUpInside];
    
    [self setPrimaryColor:lightBlue];
}

- (void) didMoveToSuperview
{
    [self addWidthConstraint:width];
    [self setHeightConstraint:height];
}

- (void) setPrimaryColor:(UIColor*)color
{
    self.backgroundColor = color;
    button1.invertedColor = color;
    button2.invertedColor = color;
    button3.invertedColor = color;
}

- (void) button1Tapped
{
    
}

- (void) button2Tapped
{
    
}

- (void) button3Tapped
{
    
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
