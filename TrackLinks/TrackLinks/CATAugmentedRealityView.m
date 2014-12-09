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
    NSMutableArray* pointsPool;
    float videoScale;
    int videoFrameOffset;
    RCFeaturePoint* lastPointTapped;
    BOOL isInitialized;
    //ARDelegate *AROverlay;
}
@synthesize videoView, featuresView, featuresLayer, initializingFeaturesLayer, AROverlay;

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
    
    AROverlay = [[MPARDelegate alloc] init];
    [videoView setDelegate:AROverlay];
    
    featuresView = [[UIView alloc] initWithFrame:CGRectZero];
    [self addSubview:featuresView];
    [featuresView addMatchSuperviewConstraints];
    
    [self setupFeatureLayers];
    
    isInitialized = YES;
}

- (void) layoutSubviews
{
    videoView.frame = self.frame;
    featuresLayer.frame = self.frame;
    initializingFeaturesLayer.frame = self.frame;
    
    [super layoutSubviews];
}

- (void) setupFeatureLayers
{
    featuresLayer = [[CATFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:[UIColor colorWithRed:0 green:200 blue:255 alpha:1]]; // cyan color
    featuresLayer.hidden = YES;
    [featuresView.layer addSublayer:featuresLayer];

    initializingFeaturesLayer = [[CATFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:[UIColor colorWithRed:200 green:0 blue:0 alpha:.5]];
    initializingFeaturesLayer.hidden = YES;
    [featuresView.layer addSublayer:initializingFeaturesLayer];
}

- (void) animateOpen
{
    [UIView animateWithDuration: .3
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         videoView.frame = self.frame;
                     }
                     completion:^(BOOL finished){
                         
                     }];
}

- (void) showFeatures
{
    featuresLayer.hidden = NO;
    initializingFeaturesLayer.hidden = NO;
}

- (void) hideFeatures
{
    featuresLayer.hidden = YES;
    initializingFeaturesLayer.hidden = YES;
}

#define MESH

@end
