//
//  MPAugRealityWithMagView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPAugRealityWithMagView.h"

@implementation MPAugRealityWithMagView
@synthesize videoView, featuresView, featuresLayer, selectedFeaturesLayer, initializingFeaturesLayer, measurementsView, magView;

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        magView = [[MPMagView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
        magView.backgroundColor = [UIColor redColor];
        magView.hidden = YES;
        [self insertSubview:magView aboveSubview:featuresView];
    }
    return self;
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
    // Drawing code
}
*/

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    LOGME
    
    if (touches && touches.count == 1)
    {
        UITouch* touch = touches.allObjects[0];
        CGPoint touchPoint = [touch locationInView:self];
        [self moveMagTo:touchPoint];
    }
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    LOGME
    magView.hidden = YES;
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    LOGME
    magView.hidden = YES;
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    LOGME
    
    if (touches && touches.count == 1)
    {
        UITouch* touch = touches.allObjects[0];
        CGPoint touchPoint = [touch locationInView:self];
        [self moveMagTo:touchPoint];
    }
}

- (void) moveMagTo:(CGPoint)point
{
    magView.hidden = NO;
    magView.frame = CGRectMake(point.x - 50, point.y - 150, magView.frame.size.width, magView.frame.size.height);
    magView.arView.frame = CGRectMake(-point.x + 480, -point.y + 640, 480 * 2, 640 * 2);
}

@end
