//
//  TMAboutView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMAboutView.h"

@implementation TMAboutView

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        UILabel* topLabel = [UILabel new];
        topLabel.textAlignment = NSTextAlignmentCenter;
        topLabel.text = @"Endless Tape Measure, by";
        
        UIImageView* logo = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 200, 50)];
        logo.image = [UIImage imageNamed:@"HorzLogo"];
        
        UIControl* urlButton = [[UIControl alloc] initWithFrame:CGRectMake(0, 0, 200, 21)];
        [urlButton addTarget:self action:@selector(linkTapped:) forControlEvents:UIControlEventTouchUpInside];
        
        UILabel* bottomLabel = [UILabel new];
        bottomLabel.textAlignment = NSTextAlignmentCenter;
        bottomLabel.textColor = [UIColor blueColor];
        NSDictionary *underlineAttribute = @{NSUnderlineStyleAttributeName: @(NSUnderlineStyleSingle)};
        bottomLabel.attributedText = [[NSAttributedString alloc] initWithString:@"http://realitycap.com" attributes:underlineAttribute];
        
        [urlButton addSubview:bottomLabel];
        [self addSubview:logo];
        [self addSubview:topLabel];
        [self addSubview:urlButton];
        
        [logo addCenterInSuperviewConstraints];
        [logo addWidthConstraint:200 andHeightConstraint:50];
        [topLabel addCenterXInSuperviewConstraints];
        [topLabel addBottomSpaceToViewConstraint:logo withDist:10];
        [urlButton addCenterXInSuperviewConstraints];
        [urlButton addTopSpaceToViewConstraint:logo withDist:10];
        [urlButton addWidthConstraint:200 andHeightConstraint:30];
        [bottomLabel addCenterInSuperviewConstraints];
    }
    return self;
}

@end
