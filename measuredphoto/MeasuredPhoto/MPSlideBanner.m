//
//  MPSlideBanner.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPSlideBanner.h"

@interface MPSlideBanner ()
@property (readwrite, nonatomic) MPSlideBannerState state;
@end

@implementation MPSlideBanner
@synthesize position, state, orientation;

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:(NSCoder *)aDecoder])
    {
        state = MPSlideBannerStateShowing; // assume view is showing in storyboard
        position = MPSlideBannerPositionBottom; // default position is bottom of screen
    }
    return self;
}

- (void) showInstantly
{
    state = MPSlideBannerStateShowing;
    position == MPSlideBannerPositionBottom ? [self moveUp] : [self moveDown];
}

- (void) showAnimated
{
    if (state != MPSlideBannerStateHidden) return;
    state = MPSlideBannerStateAnimating;
    [UIView animateWithDuration: .5
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [self showInstantly];
                     }
                     completion:nil];
}

- (void) hideInstantly
{
    state = MPSlideBannerStateHidden;
    position == MPSlideBannerPositionBottom ? [self moveDown] : [self moveUp];
}

- (void) hideWithDelay:(float)secs onCompletion:(void (^)(BOOL finished))completionBlock
{
    if (state != MPSlideBannerStateShowing) return;
    state = MPSlideBannerStateAnimating;
    [UIView animateWithDuration: .5
                          delay: secs
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [self hideInstantly];
                     }
                     completion:completionBlock];
}

- (void) moveDown
{
    if (orientation == UIInterfaceOrientationLandscapeLeft)
    {
        self.center = CGPointMake(self.center.x - self.bounds.size.height, self.center.y);
    }
    else
    {
        self.center = CGPointMake(self.center.x, self.center.y + self.bounds.size.height);
    }
}

- (void) moveUp
{
    if (orientation == UIInterfaceOrientationLandscapeLeft)
    {
        self.center = CGPointMake(self.center.x + self.bounds.size.height, self.center.y);
    }
    else
    {
        self.center = CGPointMake(self.center.x, self.center.y - self.bounds.size.height);
    }
}

@end
