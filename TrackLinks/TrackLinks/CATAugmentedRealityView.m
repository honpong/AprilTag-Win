//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "CATAugmentedRealityView.h"
#import "CATCapturePhoto.h"
#import "UIView+RCConstraints.h"

@implementation CATAugmentedRealityView
{
    float videoScale;
    int videoFrameOffset;
    BOOL isInitialized;
}
@synthesize videoView, AROverlay;

- (id) initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
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
    if (isInitialized) return;
    
    videoView = [[RCVideoPreview alloc] initWithFrame:self.frame];
    videoView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:videoView];
    [self sendSubviewToBack:videoView];
    
    AROverlay = [[CATARDelegate alloc] init];
    [videoView setDelegate:AROverlay];
    
    isInitialized = YES;
}

- (void) layoutSubviews
{
    videoView.frame = self.frame;
    
    [super layoutSubviews];
}

@end
