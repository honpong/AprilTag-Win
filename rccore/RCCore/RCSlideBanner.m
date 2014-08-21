//
//  RCSlideBanner.m
//  RCCore
//
//  Created by Ben Hirashima on 8/21/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCSlideBanner.h"
#import "UIView+RCConstraints.h"

@interface RCSlideBanner ()
@property (readwrite, nonatomic) RCSlideBannerState state;
@end

@implementation RCSlideBanner
{
    NSLayoutConstraint* topToSuperviewConstraint;
}
@synthesize state;

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:(NSCoder *)aDecoder])
    {
        state = RCSlideBannerStateShowing; // assume view is showing in storyboard
    }
    return self;
}

- (void) layoutSubviews
{
    [super layoutSubviews];
    topToSuperviewConstraint = [self findTopToSuperviewConstraint];
}

- (void) showInstantly
{
    self.hidden = NO;
    if (state == RCSlideBannerStateShowing) return;
    state = RCSlideBannerStateShowing;
    [self moveDown];
}

- (void) showAnimated
{
    if (state != RCSlideBannerStateHidden) return;
    state = RCSlideBannerStateAnimating;
    
    [self.superview layoutIfNeeded];
    [UIView animateWithDuration: .5
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [self showInstantly];
                         [self.superview layoutIfNeeded];
                     }
                     completion:nil];
}

- (void) hide
{
    if (state == RCSlideBannerStateHidden) return;
    state = RCSlideBannerStateHidden;
    [self moveUp];
}

- (void) hideInstantly
{
    [self hide];
    self.hidden = YES;
}

- (void) hideWithDelay:(float)secs onCompletion:(void (^)(BOOL finished))completionBlock
{
    if (state != RCSlideBannerStateShowing) return;
    state = RCSlideBannerStateAnimating;
    
    [self.superview layoutIfNeeded];
    [UIView animateWithDuration: .5
                          delay: secs
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [self hide];
                         [self.superview layoutIfNeeded];
                     }
                     completion:^(BOOL finished){
                         self.hidden = YES;
                         if (completionBlock) completionBlock(finished);
                     }];
}

- (void) moveDown
{
    if (topToSuperviewConstraint) topToSuperviewConstraint.constant = 0;
}

- (void) moveUp
{
    if (topToSuperviewConstraint) topToSuperviewConstraint.constant = -self.bounds.size.height;
}

@end
