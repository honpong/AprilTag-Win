//
//  RCTipView.m
//  RCCore
//
//  Created by Ben Hirashima on 9/8/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCTipView.h"

@implementation RCTipView

- (id) initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        self.backgroundColor = [UIColor colorWithWhite:.5 alpha:.5];
        self.layer.cornerRadius = 10.;
        self.clipsToBounds = YES;
        self.numberOfLines = 4;
        self.textColor = [UIColor whiteColor];
        self.userInteractionEnabled = YES;
        
        self.currentTipIndex = -1;
    }
    return self;
}

- (void) drawTextInRect:(CGRect)rect
{
    UIEdgeInsets insets = {5, 12, 5, 12};
    [super drawTextInRect:UIEdgeInsetsInsetRect(rect, insets)];
    
    // draw white triangle in bottom right corner
    CGPoint startPoint = CGPointMake(rect.size.width - 15, rect.size.height - 20);
    
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextMoveToPoint(context, startPoint.x, startPoint.y);
    CGContextAddLineToPoint(context, startPoint.x, startPoint.y + 10);
    CGContextAddLineToPoint(context, startPoint.x + 5, startPoint.y + 5);
    CGContextAddLineToPoint(context, startPoint.x, startPoint.y);
    
    CGContextSetAlpha(context, 1.0);
    CGContextSetFillColorWithColor(context, [[UIColor whiteColor] CGColor]);
    CGContextFillPath(context);
}

- (void) updateTipText
{
    if (self.currentTipIndex >= self.tips.count) return;
    self.text = [NSString stringWithFormat:@"Tip: %@", self.tips[self.currentTipIndex]];
    if ([self.delegate respondsToSelector:@selector(tipIndexUpdated:)]) [self.delegate tipIndexUpdated:self.currentTipIndex];
}

- (void) showNextTip
{
    [self showTip:self.currentTipIndex + 1];
}

- (void) showTip:(int)index
{
    self.currentTipIndex = index < self.tips.count ? index : 0;
    [self updateTipText];
}

- (void) setTips:(NSArray *)tips
{
    _tips = tips;
    self.currentTipIndex = -1;
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (touches.count != 1) return;
    
    UITouch* touch = touches.anyObject;
    if ([self pointInside:[touch locationInView:self] withEvent:event]) [self showNextTip];
}

@end
