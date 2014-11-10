//
//  RCVideoPreviewCRT.m
//  RCCore
//
//  Created by Eagle Jones on 9/25/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCVideoPreviewCRT.h"

static const NSTimeInterval animationDuration = .2;

@interface RCVideoPreview ()

- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer;
- (void)initialize;

@end

@implementation RCVideoPreviewCRT
{
    //For fading the screen to white
    bool fadeToWhite;
    bool fadeFromWhite;
    float fadeTime;
    NSDate *fadeStart;
    UIDeviceOrientation animationOrientation;
    void(^animationComplete)(BOOL finished);
    BOOL didReceiveFirstFrame;
}

- (void) initialize
{
    [super initialize];
    fadeToWhite = false;
    fadeFromWhite = false;
    animationComplete = nil;
    crtClosedFrame = CGRectZero;
    didReceiveFirstFrame = NO;
}

- (void)displayPixelBuffer:(CVImageBufferRef)pixelBuffer
{
    bool callCompletion = false;
    if(fadeToWhite || fadeFromWhite)
    {
        if (!didReceiveFirstFrame)
        {
            fadeStart = [NSDate date];
            didReceiveFirstFrame = YES;
        }
        float horzScale = 1., vertScale = 1.;
        float whiteness = 0.;
        float elapsed = -[fadeStart timeIntervalSinceNow] / fadeTime;
        if(elapsed > 1) elapsed = 1;
        if(fadeToWhite && fadeFromWhite)
            whiteness = elapsed < .5 ? elapsed * 2. : 2. - elapsed * 2.;
        else if(fadeToWhite)
        {
            if(elapsed < .5)
            {
                whiteness = elapsed * 2.;
                horzScale = 1.;
            }
            else
            {
                whiteness = 1.;
                horzScale = 2. - elapsed * 2.;
            }
        }
        else //fadeFromWhite
            whiteness = 1. - elapsed;
        vertScale = 1. - whiteness;
        //if(vertScale == 0) vertScale = 1. / self.bounds.size.width;
        [self setWhiteness:whiteness];
        if(elapsed >= 1.)
        {
            fadeToWhite = fadeFromWhite = false;
            callCompletion = true;
        }
        
        [self setHorizontalScale:horzScale withVerticalScale:vertScale];
    }
    
    [super displayPixelBuffer:pixelBuffer];

    if(callCompletion && animationComplete)
    {
        animationComplete(true);
        animationComplete = nil;
    }
}

- (void) animateOpen:(UIDeviceOrientation) orientation
{
    animationOrientation = orientation;
    [self fadeToWhite:NO fromWhite:YES inSeconds:animationDuration];
}

- (void) animateClosed:(UIDeviceOrientation)orientation withCompletionBlock:(void(^)(BOOL finished))completion
{
    animationOrientation = orientation;
    animationComplete = completion;
    [self fadeToWhite:YES fromWhite:NO inSeconds:animationDuration * 1.5];
}

- (void) fadeToWhite:(bool)to fromWhite:(bool)from inSeconds:(float)seconds;
{
    fadeStart = [NSDate date];
    fadeTime = seconds;
    fadeToWhite = to;
    fadeFromWhite = from;
}

@end
