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
    CGFloat originalBottomSpace;
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

- (void) setHeightConstraint:(CGFloat)height
{
    heightConstraint = [self addHeightConstraint:height];
}

- (void) setBottomSpaceToSuperviewConstraint:(CGFloat)distance
{
    bottomToSuperviewConstraint = [self addBottomSpaceToSuperviewConstraint:distance];
    originalBottomSpace = distance;
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
                        options: UIViewAnimationOptionCurveEaseOut
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

- (void) hideAnimated
{
    [self hideAnimated:nil];
}

- (void) hideAnimated:(void (^)(BOOL finished))completionBlock
{
    if (state != RCSlideBannerStateShowing) return;
    state = RCSlideBannerStateAnimating;
    
    [self.superview layoutIfNeeded];
    [UIView animateWithDuration: .3
                          delay: 0
                        options: UIViewAnimationOptionCurveLinear
                     animations:^{
                         [self hide];
                         [self.superview layoutIfNeeded];
                     }
                     completion:^(BOOL finished){
                         self.hidden = YES;
                         if (completionBlock) completionBlock(finished);
                     }];
}

- (void) hideWithDelay:(float)secs onCompletion:(void (^)(BOOL finished))completionBlock
{
    [self hideAnimated:completionBlock];
}

- (void) moveDown
{
    if (bottomToSuperviewConstraint) bottomToSuperviewConstraint.constant = heightConstraint.constant + originalBottomSpace;
}

- (void) moveUp
{
    if (bottomToSuperviewConstraint) bottomToSuperviewConstraint.constant = -originalBottomSpace;
}

@end
