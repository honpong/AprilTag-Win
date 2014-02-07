//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPAugmentedRealityView.h"
#import "MPLoupe.h"
#import "UIView+MPCascadingRotation.h"
#import "UIView+MPConstraints.h"

@implementation MPAugmentedRealityView
{    
    NSMutableArray* pointsPool;
    float videoScale;
    int videoFrameOffset;
    
    BOOL isInitialized;
}
@synthesize videoView, featuresView, featuresLayer, selectedFeaturesLayer, initializingFeaturesLayer, measurementsView, photoView, instructionsView;

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
    
    OPENGL_MANAGER;
    
    videoView = [[MPVideoPreview alloc] initWithFrame:self.frame];
    videoView.translatesAutoresizingMaskIntoConstraints = NO; // necessary?
    [self addSubview:videoView];
    [self sendSubviewToBack:videoView];
    
    featuresView = [[UIView alloc] initWithFrame:CGRectZero];
    [self insertSubview:featuresView aboveSubview:videoView];
    [featuresView addMatchSuperviewConstraints];
    
    [self setupFeatureLayers];
    
    measurementsView = [[MPMeasurementsView alloc] initWithFeaturesLayer:featuresLayer];
    [self insertSubview:measurementsView aboveSubview:featuresView];
    [measurementsView addMatchSuperviewConstraints];
        
    photoView = [[MPImageView alloc] initWithFrame:self.frame];
    photoView.hidden = YES;
    [self insertSubview:photoView aboveSubview:videoView];
    [photoView addMatchSuperviewConstraints];

	self.magnifyingGlass= [[MPLoupe alloc] init];
	self.magnifyingGlass.scaleAtTouchPoint = NO;
    self.magnifyingGlass.viewToMagnify = photoView;
    
    instructionsView = [[MPInstructionsView alloc] initWithFrame:self.frame];
    [self addSubview:instructionsView];
    [self bringSubviewToFront:instructionsView];
    
    self.magGlassEnabled = NO;
    isInitialized = YES;
}

- (void) layoutSubviews
{
    [super layoutSubviews];
    [videoView setTransformFromCurrentVideoOrientationToOrientation:AVCaptureVideoOrientationPortrait];
    videoView.frame = self.frame;
    selectedFeaturesLayer.frame = self.frame;
    featuresLayer.frame = self.frame;
    initializingFeaturesLayer.frame = self.frame;
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

- (void) selectFeature:(RCFeaturePoint *)point
{
    if (point)
    {
        selectedFeaturesLayer.hidden = NO;
        [selectedFeaturesLayer updateFeatures:[NSArray arrayWithObject:point]];
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

#pragma mark - touch events

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
        CGPoint offsetPoint = CGPointMake(touchPoint.x, touchPoint.y + self.magnifyingGlass.defaultOffset);
        CGPoint cameraPoint = [self.featuresLayer cameraPointFromScreenPoint:offsetPoint];
        RCFeaturePoint* pointTapped = [SENSOR_FUSION triangulatePointWithX:cameraPoint.x withY:cameraPoint.y];

        if(pointTapped)
            [self selectFeature:pointTapped];
    }
}

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    [self rotateChildViews:orientation];
}

@end
