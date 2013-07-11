//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMAugmentedRealityView.h"

@implementation TMAugmentedRealityView
{
    TMCrosshairsLayerDelegate *crosshairsDelegate;
    CALayer *crosshairsLayer;
    
    NSMutableArray* pointsPool;
    float videoScale;
    int videoFrameOffset;
    
    BOOL isInitialized;
}
@synthesize videoView, featuresLayer, selectedFeaturesLayer;

- (void) initialize
{
    if (isInitialized) return;
    
    LOGME
    [TMOpenGLManagerFactory getInstance];
    videoView = [[TMVideoPreview alloc] initWithFrame:CGRectZero];
    [videoView setTransformFromCurrentVideoOrientationToOrientation:UIInterfaceOrientationPortrait];
    [self addSubview:videoView];
    [self sendSubviewToBack:videoView];
 	CGRect bounds = CGRectZero;
 	bounds.size = [self convertRect:self.bounds toView:videoView].size;
 	videoView.bounds = bounds;
    videoView.center = CGPointMake(self.bounds.size.width/2.0, self.bounds.size.height/2.0);
    
    crosshairsDelegate = [TMCrosshairsLayerDelegate new];
    crosshairsLayer = [CALayer new];
    [crosshairsLayer setDelegate:crosshairsDelegate];
    crosshairsLayer.hidden = YES;
    crosshairsLayer.frame = self.frame;
    [crosshairsLayer setNeedsDisplay];
    [self.layer addSublayer:crosshairsLayer];
    
    [self setupFeatureLayers];
        
    isInitialized = YES;
}

- (void) setupFeatureLayers
{
    selectedFeaturesLayer = [[TMFeaturesLayer alloc] initWithFeatureCount:2 andColor:[UIColor redColor]];
    selectedFeaturesLayer.frame = self.frame;
    [selectedFeaturesLayer setNeedsDisplay];
    [self.layer insertSublayer:selectedFeaturesLayer below:crosshairsLayer];
    
    featuresLayer = [[TMFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:nil];
    featuresLayer.hidden = YES;
    featuresLayer.frame = self.frame;
    [featuresLayer setNeedsDisplay];
    [self.layer insertSublayer:featuresLayer below:selectedFeaturesLayer];
}

- (void) selectFeatureNearest:(CGPoint)coordinateTapped
{
    TMPoint* point = [featuresLayer getClosestPointTo:coordinateTapped];
    [selectedFeaturesLayer setFeaturePositions:[NSArray arrayWithObject:point]];
}

- (void) showCrosshairs
{
    crosshairsLayer.hidden = NO;
    [crosshairsLayer needsLayout];
}

- (void) hideCrosshairs
{
    crosshairsLayer.hidden = YES;
    [crosshairsLayer needsLayout];
}

- (void) showFeatures
{
    featuresLayer.hidden = NO;
}

- (void) hideFeatures
{
    featuresLayer.hidden = YES;
}

@end
