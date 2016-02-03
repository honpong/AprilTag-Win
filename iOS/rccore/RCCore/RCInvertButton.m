//
//  RCInvertButton.m
//  RCCore
//
//  Created by Ben Hirashima on 8/26/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCInvertButton.h"

@implementation RCInvertButton

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self initialize];
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [self initialize];
    }
    return self;
}

- (void) initialize
{
    originalTextColor = self.titleLabel.textColor;
    originalBgColor = self.backgroundColor;
    self.invertedColor = self.backgroundColor;
}

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    originalTextColor = self.titleLabel.textColor;
    
    [super touchesBegan:touches withEvent:event];
    
    self.backgroundColor = originalTextColor;
    self.titleLabel.textColor = self.invertedColor;
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
    [self revertButtonState];
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
    [self revertButtonState];
}

- (void) revertButtonState
{
    self.backgroundColor = originalBgColor;
    self.titleLabel.textColor = originalTextColor;
}

@end
