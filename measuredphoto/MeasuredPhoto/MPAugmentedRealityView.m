//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPAugmentedRealityView.h"
#import "MPLoupe.h"
#import <RCCore/RCCore.h>
#import "MPCapturePhoto.h"

@implementation MPAugmentedRealityView
{    
    NSMutableArray* pointsPool;
    float videoScale;
    int videoFrameOffset;
    RCFeaturePoint* lastPointTapped;
    BOOL isInitialized;
}
@synthesize videoView, featuresView, featuresLayer, selectedFeaturesLayer, initializingFeaturesLayer, measurementsView, photoView, delegate;

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
    
    videoView = [[MPVideoPreview alloc] initWithFrame:self.frame];
    videoView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:videoView];
    [self sendSubviewToBack:videoView];
    
    photoView = [[MPImageView alloc] initWithFrame:self.frame];
    photoView.hidden = YES;
    photoView.contentMode = UIViewContentModeScaleAspectFill;
    [self insertSubview:photoView aboveSubview:videoView];
    [photoView addMatchSuperviewConstraints];
    
    featuresView = [[UIView alloc] initWithFrame:CGRectZero];
    [self addSubview:featuresView];
    [featuresView addMatchSuperviewConstraints];
    
    [self setupFeatureLayers];
    
    measurementsView = [[MPMeasurementsView alloc] initWithFeaturesLayer:featuresLayer];
    [self insertSubview:measurementsView aboveSubview:featuresView];
    [measurementsView addMatchSuperviewConstraints];

	self.magnifyingGlass= [[MPLoupe alloc] init];
	self.magnifyingGlass.scaleAtTouchPoint = YES;
    self.magnifyingGlass.viewToMagnify = photoView;
    self.magnifyingGlassShowDelay = .5;
    
    self.magGlassEnabled = NO;
    isInitialized = YES;
}

- (void) layoutSubviews
{
//    videoView.frame = [videoView getCrtClosedFrame:[MPCapturePhoto getCurrentUIOrientation]];
    videoView.frame = self.frame;
    selectedFeaturesLayer.frame = self.frame;
    featuresLayer.frame = self.frame;
    initializingFeaturesLayer.frame = self.frame;
    
    [super layoutSubviews];
}

- (void) setupFeatureLayers
{
    selectedFeaturesLayer = [[RCFeaturesLayer alloc] initWithFeatureCount:2 andColor:[UIColor greenColor]];
    [featuresView.layer addSublayer:selectedFeaturesLayer];
    
    featuresLayer = [[RCFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:[UIColor colorWithRed:0 green:200 blue:255 alpha:1]]; // cyan color
    featuresLayer.hidden = YES;
    [featuresView.layer insertSublayer:featuresLayer below:selectedFeaturesLayer];

    initializingFeaturesLayer = [[RCFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:[UIColor colorWithRed:200 green:0 blue:0 alpha:.5]];
    initializingFeaturesLayer.hidden = YES;
    [featuresView.layer insertSublayer:initializingFeaturesLayer below:selectedFeaturesLayer];
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

- (void) selectFeature:(RCFeaturePoint *)point
{
    if (point)
    {
        selectedFeaturesLayer.hidden = NO;
        [selectedFeaturesLayer updateFeatures:@[point]];
        for (CALayer* layer in selectedFeaturesLayer.sublayers)
        {
            layer.opacity = 1.;
        }
    }
}

- (RCFeaturePoint*) selectFeatureNearest:(CGPoint)coordinateTapped
{
    RCFeaturePoint* point = [featuresLayer getClosestFeatureTo:coordinateTapped];
    if(point)
    {
        [self selectFeature:point];
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

#define MESH

#pragma mark - touch events

- (void) handleFeatureTapped:(CGPoint)coordinateTapped
{
    CGPoint cameraPoint = [featuresLayer cameraPointFromScreenPoint:coordinateTapped];
#ifdef MESH
    RCFeaturePoint* pointTapped = [[RCStereo sharedInstance] triangulatePointWithMesh:cameraPoint];
#else
    RCFeaturePoint* pointTapped = [[RCStereo sharedInstance] triangulatePoint:cameraPoint];
#endif
    
    if(pointTapped)
    {
        [self selectFeature:pointTapped];
        
        if ([delegate respondsToSelector:@selector(featureTapped)]) [delegate featureTapped];
        
        if (lastPointTapped)
        {
            [measurementsView addMeasurementBetweenPointA:pointTapped andPointB:lastPointTapped];
            [self resetSelectedFeatures];
            
            if ([delegate respondsToSelector:@selector(measurementCompleted)]) [delegate measurementCompleted];
        }
        else
        {
            lastPointTapped = pointTapped;
        }
    }
}

- (void) resetSelectedFeatures
{
    lastPointTapped = nil;
    [self clearSelectedFeatures];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (self.isMagGlassEnabled) [super touchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (self.isMagGlassEnabled) [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (!self.isMagGlassEnabled) return;
        
    [super touchesEnded:touches withEvent:event];
    
    if (touches && touches.count == 1)
    {
        UITouch* touch = touches.allObjects[0];
        CGPoint touchPoint = [touch locationInView:self];
        [self handleFeatureTapped:touchPoint];
    }
}

@end
