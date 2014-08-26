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
    NSLayoutConstraint* bottomToSuperviewConstraint;
    NSLayoutConstraint* heightConstraint;
}
@synthesize state;

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:(NSCoder *)aDecoder])
    {
        [self initialize];
    }
    return self;
}

- (id) initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self initialize];
    }
    return self;
}

- (void) initialize
{
    state = RCSlideBannerStateShowing; // assume view is showing by default
    self.translatesAutoresizingMaskIntoConstraints = NO;
}

- (void) didMoveToSuperview
{
    [self addLeadingSpaceToSuperviewConstraint:0];
    [self addTrailingSpaceToSuperviewConstraint:0];
    bottomToSuperviewConstraint = [self addBottomSpaceToSuperviewConstraint:0];
}

- (void) setHeightConstraint:(CGFloat)height
{
    heightConstraint = [self getHeightConstraint:height];
    [self addConstraint:heightConstraint];
}

- (void) showInstantly
{
    self.hidden = NO;
    if (state == RCSlideBannerStateShowing) return;
    state = RCSlideBannerStateShowing;
    [self moveUp];
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
    [self moveDown];
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
    if (bottomToSuperviewConstraint) bottomToSuperviewConstraint.constant = heightConstraint.constant;
}

- (void) moveUp
{
    if (bottomToSuperviewConstraint) bottomToSuperviewConstraint.constant = 0;
}

@end
