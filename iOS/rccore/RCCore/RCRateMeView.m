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

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.label.text = @"Thanks for trying our app. Help us out by rating it on the App Store!";
        [self.button1 setTitle:@"Rate now" forState:UIControlStateNormal];
        [self.button2 setTitle:@"Later" forState:UIControlStateNormal];
        [self.button3 setTitle:@"Never!" forState:UIControlStateNormal];
    }
    return self;
}

- (void) button1Tapped
{
    if ([self.delegate respondsToSelector:@selector(handleRateNowButton)]) [self.delegate handleRateNowButton];
}

- (void) button2Tapped
{
    if ([self.delegate respondsToSelector:@selector(handleRateLaterButton)]) [self.delegate handleRateLaterButton];
}

- (void) button3Tapped
{
    if ([self.delegate respondsToSelector:@selector(handleRateNeverButton)]) [self.delegate handleRateNeverButton];
}

@end
