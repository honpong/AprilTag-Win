//
//  RCLocationPopUp.m
//  RCCore
//
//  Created by Ben Hirashima on 9/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCLocationPopUp.h"

@implementation RCLocationPopUp

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.label.text = @"Allow the app to use your location to improve accuracy?";
        [self.button1 setTitle:@"Allow location (recommended)" forState:UIControlStateNormal];
        [self.button2 setTitle:@"Later" forState:UIControlStateNormal];
        [self.button3 setTitle:@"Never!" forState:UIControlStateNormal];
    }
    return self;
}

- (void) button1Tapped
{
    if ([self.delegate respondsToSelector:@selector(handleAllowLocationButton)]) [self.delegate handleAllowLocationButton];
}

- (void) button2Tapped
{
    if ([self.delegate respondsToSelector:@selector(handleLaterLocationButton)]) [self.delegate handleLaterLocationButton];
}

- (void) button3Tapped
{
    if ([self.delegate respondsToSelector:@selector(handleNeverLocationButton)]) [self.delegate handleNeverLocationButton];
}

@end
