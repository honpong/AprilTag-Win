//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMAugmentedRealityView.h"
#import "TMLineLayerDelegate.h"

@implementation TMAugmentedRealityView
{
    TMCrosshairsLayerDelegate *crosshairsDelegate;
    CALayer *crosshairsLayer;
    
    TMLineLayerDelegate* lineLayerDelegate;
    CALayer* lineLayer;
    
    NSMutableArray* pointsPool;
    float videoScale;
    int videoFrameOffset;
    
    BOOL isInitialized;
}
@synthesize videoView, featuresLayer, selectedFeaturesLayer, lineLayer;

- (void) initialize
{
    if (isInitialized) return;
    
    LOGME
    
    [TMOpenGLManagerFactory getInstance];
    
    videoView = [[TMVideoPreview alloc] initWithFrame:CGRectZero];
    [videoView setTransformFromCurrentVideoOrientationToOrientation:UIInterfaceOrientationPortrait];
    videoView.frame = CGRectMake(0, 0, self.frame.size.width, self.frame.size.height); // must be set AFTER setTransformFromCurrentVideoOrientationToOrientation
    [self addSubview:videoView];
    [self sendSubviewToBack:videoView];
    
    crosshairsDelegate = [TMCrosshairsLayerDelegate new];
    crosshairsLayer = [CALayer new];
    [crosshairsLayer setDelegate:crosshairsDelegate];
    crosshairsLayer.hidden = YES;
    crosshairsLayer.bounds = self.bounds;
    crosshairsLayer.position = self.center;
    [crosshairsLayer setNeedsDisplay];
    [self.layer addSublayer:crosshairsLayer];
    
    lineLayerDelegate = [TMLineLayerDelegate new];
    lineLayer = [CALayer new];
    lineLayer.delegate = lineLayerDelegate;
    lineLayer.bounds = self.bounds;
    lineLayer.position = self.center;
    [self.layer insertSublayer:lineLayer below:crosshairsLayer];
    
    [self setupFeatureLayers];
        
    isInitialized = YES;
}

- (void) setupFeatureLayers
{
    selectedFeaturesLayer = [[TMFeaturesLayer alloc] initWithFeatureCount:2 andColor:[UIColor greenColor]];
    selectedFeaturesLayer.bounds = self.bounds;
    selectedFeaturesLayer.position = self.center;
    [selectedFeaturesLayer setNeedsDisplay];
    [self.layer insertSublayer:selectedFeaturesLayer above:lineLayer];
    
    featuresLayer = [[TMFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:nil];
    featuresLayer.hidden = YES;
    featuresLayer.bounds = self.bounds;
    featuresLayer.position = self.center;
    [featuresLayer setNeedsDisplay];
    [self.layer insertSublayer:featuresLayer below:selectedFeaturesLayer];
}

- (TMPoint*) selectFeatureNearest:(CGPoint)coordinateTapped
{
    TMPoint* point = [featuresLayer getClosestPointTo:coordinateTapped];
    [selectedFeaturesLayer setFeaturePositions:[NSArray arrayWithObject:point]];
    return point;
}

- (void) drawLineBetweenPointA:(TMPoint*)pointA andPointB:(TMPoint*)pointB withDistance:(float)distance
{
    [lineLayerDelegate setPointA:[pointA makeCGPoint] andPointB:[pointB makeCGPoint]];
    [lineLayer setNeedsDisplay];
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
