//
//  MPSlideBanner.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPSlideBanner.h"
#import "UIView+MPCascadingRotation.h"

@interface MPSlideBanner ()
@property (readwrite, nonatomic) MPSlideBannerState state;
@property (readwrite, nonatomic) UIInterfaceOrientation orientation;
@end

@implementation MPSlideBanner
@synthesize position, state, orientation;

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:(NSCoder *)aDecoder])
    {
        state = MPSlideBannerStateShowing; // assume view is showing in storyboard
        position = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) ? MPSlideBannerPositionBottom : MPSlideBannerPositionTop;
        orientation = UIInterfaceOrientationPortrait;
    }
    return self;
}

- (void) showInstantly
{
    self.hidden = NO;
    if (state == MPSlideBannerStateShowing) return;
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
    if (state == MPSlideBannerStateHidden) return;
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
    switch (orientation)
    {
        case UIInterfaceOrientationLandscapeLeft:
            self.center = CGPointMake(self.center.x - self.bounds.size.height, self.center.y);
            break;
        case UIInterfaceOrientationLandscapeRight:
            self.center = CGPointMake(self.center.x + self.bounds.size.height, self.center.y);
            break;
        case UIInterfaceOrientationPortrait:
            self.center = CGPointMake(self.center.x, self.center.y + self.bounds.size.height);
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            self.center = CGPointMake(self.center.x, self.center.y - self.bounds.size.height);
            break;
        default:
            break;
    }
}

- (void) moveUp
{
    switch (orientation)
    {
        case UIInterfaceOrientationLandscapeLeft:
            self.center = CGPointMake(self.center.x + self.bounds.size.height, self.center.y);
            break;
        case UIInterfaceOrientationLandscapeRight:
            self.center = CGPointMake(self.center.x - self.bounds.size.height, self.center.y);
            break;
        case UIInterfaceOrientationPortrait:
            self.center = CGPointMake(self.center.x, self.center.y - self.bounds.size.height);
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            self.center = CGPointMake(self.center.x, self.center.y + self.bounds.size.height);
            break;
        default:
            break;
    }
}

- (void) handleOrientationChange:(UIDeviceOrientation)deviceOrientation
{
    [self applyRotationTransformation:deviceOrientation];
    
    for (NSLayoutConstraint *con in self.superview.constraints)
    {
        if (con.firstItem == self || con.secondItem == self)
        {
            [self.superview removeConstraint:con];
        }
    }
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [self handleOrientationChangeIPad:deviceOrientation];
    }
    else
    {
        [self handleOrientationChangeIPhone:deviceOrientation];
    }
}

- (void) handleOrientationChangeIPad:(UIDeviceOrientation)deviceOrientation
{
    NSArray *questionH, *questionV;
    NSLayoutConstraint* questionCenterH;
    
    float distFromBottom = state == MPSlideBannerStateShowing ? 0 : -80;
    
    switch (deviceOrientation)
    {
        case UIDeviceOrientationPortrait:
        {
            orientation = UIInterfaceOrientationPortrait;
            questionH = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"[self(384)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionV = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"V:[self(80)]-(%0.0f)-|", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionCenterH = [NSLayoutConstraint
                               constraintWithItem:self.superview
                               attribute:NSLayoutAttributeCenterX
                               relatedBy:NSLayoutRelationEqual
                               toItem:self
                               attribute:NSLayoutAttributeCenterX
                               multiplier:1
                               constant:0];
            break;
        }
        case UIDeviceOrientationPortraitUpsideDown:
        {
            orientation = UIInterfaceOrientationPortraitUpsideDown;
            questionH = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"[self(384)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionV = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"V:|-(%0.0f)-[self(80)]", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionCenterH = [NSLayoutConstraint
                               constraintWithItem:self.superview
                               attribute:NSLayoutAttributeCenterX
                               relatedBy:NSLayoutRelationEqual
                               toItem:self
                               attribute:NSLayoutAttributeCenterX
                               multiplier:1
                               constant:0];
            break;
        }
        case UIDeviceOrientationLandscapeLeft:
        {
            orientation = UIInterfaceOrientationLandscapeLeft;
            questionH = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"|-(%0.0f)-[self(80)]", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionV = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"V:[self(384)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionCenterH = [NSLayoutConstraint
                               constraintWithItem:self.superview
                               attribute:NSLayoutAttributeCenterY
                               relatedBy:NSLayoutRelationEqual
                               toItem:self
                               attribute:NSLayoutAttributeCenterY
                               multiplier:1
                               constant:0];
            break;
        }
        case UIDeviceOrientationLandscapeRight:
        {
            orientation = UIInterfaceOrientationLandscapeRight;
            questionH = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"[self(80)]-(%0.0f)-|", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionV = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"V:[self(384)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionCenterH = [NSLayoutConstraint
                               constraintWithItem:self.superview
                               attribute:NSLayoutAttributeCenterY
                               relatedBy:NSLayoutRelationEqual
                               toItem:self
                               attribute:NSLayoutAttributeCenterY
                               multiplier:1
                               constant:0];
            break;
        }
        default:
            break;
    }
    
    if (questionH && questionV && questionCenterH)
    {
        [self.superview addConstraint:questionCenterH];
        [self.superview addConstraints:questionH];
        [self.superview addConstraints:questionV];
    }
}

- (void) handleOrientationChangeIPhone:(UIDeviceOrientation)deviceOrientation
{
    NSArray *questionH, *questionV;
    NSLayoutConstraint* questionCenterH;
    
    float distFromBottom = state == MPSlideBannerStateShowing ? 0 : -80;
    
    switch (deviceOrientation)
    {
        case UIDeviceOrientationPortrait:
        {
            orientation = UIInterfaceOrientationPortrait;
            position = MPSlideBannerPositionTop;
            questionH = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"[self(320)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionV = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"V:|-(%0.0f)-[self(80)]", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionCenterH = [NSLayoutConstraint
                               constraintWithItem:self.superview
                               attribute:NSLayoutAttributeCenterX
                               relatedBy:NSLayoutRelationEqual
                               toItem:self
                               attribute:NSLayoutAttributeCenterX
                               multiplier:1
                               constant:0];
            break;
        }
        case UIDeviceOrientationPortraitUpsideDown:
        {
            orientation = UIInterfaceOrientationPortraitUpsideDown;
            position = MPSlideBannerPositionBottom;
            questionH = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"[self(320)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionV = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"V:|-(%0.0f)-[self(80)]", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionCenterH = [NSLayoutConstraint
                               constraintWithItem:self.superview
                               attribute:NSLayoutAttributeCenterX
                               relatedBy:NSLayoutRelationEqual
                               toItem:self
                               attribute:NSLayoutAttributeCenterX
                               multiplier:1
                               constant:0];
            break;
        }
        case UIDeviceOrientationLandscapeLeft:
        {
            orientation = UIInterfaceOrientationLandscapeLeft;
            position = MPSlideBannerPositionBottom;
            questionH = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"|-(%0.0f)-[self(80)]", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionV = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"V:[self(320)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionCenterH = [NSLayoutConstraint
                               constraintWithItem:self.superview
                               attribute:NSLayoutAttributeCenterY
                               relatedBy:NSLayoutRelationEqual
                               toItem:self
                               attribute:NSLayoutAttributeCenterY
                               multiplier:1
                               constant:0];
            break;
        }
        case UIDeviceOrientationLandscapeRight:
        {
            orientation = UIInterfaceOrientationLandscapeRight;
            position = MPSlideBannerPositionBottom;
            questionH = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"[self(80)]-(%0.0f)-|", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionV = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"V:[self(320)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            questionCenterH = [NSLayoutConstraint
                               constraintWithItem:self.superview
                               attribute:NSLayoutAttributeCenterY
                               relatedBy:NSLayoutRelationEqual
                               toItem:self
                               attribute:NSLayoutAttributeCenterY
                               multiplier:1
                               constant:0];
            break;
        }
        default:
            break;
    }
    
    if (questionH && questionV && questionCenterH)
    {
        [self.superview addConstraint:questionCenterH];
        [self.superview addConstraints:questionH];
        [self.superview addConstraints:questionV];
    }
}

@end
