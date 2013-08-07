//
//  TMMeasurementTypeCell.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 4/2/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurementTypeCell.h"

@implementation TMMeasurementTypeCell

- (void) setImage: (UIImage*)image
{
    [self.button setImage:image forState:UIControlStateNormal];
    [self.button setImage:image forState:UIControlStateHighlighted];
}

- (void) setText: (NSString*)text
{
    self.label.text = text;
}

@end
