//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPAugmentedRealityView.h"

@implementation MPAugmentedRealityView
{    
    NSMutableArray* pointsPool;
    float videoScale;
    int videoFrameOffset;
    
    BOOL isInitialized;
}
@synthesize videoView, featuresView, featuresLayer, selectedFeaturesLayer, initializingFeaturesLayer, measurementsView;

- (void) initialize
{
    if (isInitialized) return;
    
    LOGME
    
    OPENGL_MANAGER;
    
    videoView = [[MPVideoPreview alloc] initWithFrame:CGRectZero];
    [videoView setTransformFromCurrentVideoOrientationToOrientation:AVCaptureVideoOrientationPortrait];
    videoView.frame = CGRectMake(0, 0, self.frame.size.width, self.frame.size.height); // must be set AFTER setTransformFromCurrentVideoOrientationToOrientation
    [self addSubview:videoView];
    [self sendSubviewToBack:videoView];
    
    featuresView = [[UIView alloc] initWithFrame:self.frame];
    [self insertSubview:featuresView aboveSubview:videoView];
    
    [self setupFeatureLayers];
        
    isInitialized = YES;
}

- (void) setupFeatureLayers
{
    selectedFeaturesLayer = [[RCFeaturesLayer alloc] initWithFeatureCount:2 andColor:[UIColor greenColor]];
    selectedFeaturesLayer.bounds = self.bounds;
    selectedFeaturesLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    [selectedFeaturesLayer setNeedsDisplay];
    [featuresView.layer addSublayer:selectedFeaturesLayer];
    
    featuresLayer = [[RCFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:[UIColor colorWithRed:0 green:200 blue:255 alpha:1]]; // cyan color
    featuresLayer.hidden = YES;
    featuresLayer.bounds = self.bounds;
    featuresLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    [featuresLayer setNeedsDisplay];
//    featuresLayer.backgroundColor = [[UIColor yellowColor] CGColor];
    [featuresView.layer insertSublayer:featuresLayer below:selectedFeaturesLayer];

    initializingFeaturesLayer = [[RCFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:[UIColor colorWithRed:200 green:0 blue:0 alpha:.5]];
    initializingFeaturesLayer.hidden = YES;
    initializingFeaturesLayer.bounds = self.bounds;
    initializingFeaturesLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    [initializingFeaturesLayer setNeedsDisplay];
    [featuresView.layer insertSublayer:initializingFeaturesLayer below:selectedFeaturesLayer];
    
    measurementsView = [[MPMeasurementsView alloc] initWithFeaturesLayer:featuresLayer];
    [self insertSubview:measurementsView aboveSubview:featuresView];
}

- (RCFeaturePoint*) selectFeatureNearest:(CGPoint)coordinateTapped
{
    RCFeaturePoint* point = [featuresLayer getClosestFeatureTo:coordinateTapped];
    if (point)
    {
        selectedFeaturesLayer.hidden = NO;
        [selectedFeaturesLayer updateFeatures:[NSArray arrayWithObject:point]];
        for (CALayer* layer in selectedFeaturesLayer.sublayers)
        {
            layer.opacity = 1.;
        }
    }
    return point;
}

- (void) clearSelectedFeatures
{
    selectedFeaturesLayer.hidden = YES;
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

@end
