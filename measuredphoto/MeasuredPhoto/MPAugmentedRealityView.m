//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPAugmentedRealityView.h"
#import "MPLoupe.h"

@implementation MPAugmentedRealityView
{    
    NSMutableArray* pointsPool;
    float videoScale;
    int videoFrameOffset;
    
    BOOL isInitialized;
}
@synthesize videoView, featuresView, featuresLayer, selectedFeaturesLayer, initializingFeaturesLayer, measurementsView, photoView;

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
    [self constrainToSelf:featuresView];
    
    [self setupFeatureLayers];
    
    measurementsView = [[MPMeasurementsView alloc] initWithFeaturesLayer:featuresLayer];
    [self insertSubview:measurementsView aboveSubview:featuresView];
    [self constrainToSelf:measurementsView];
        
    photoView = [[MPImageView alloc] initWithFrame:self.frame];
    photoView.hidden = YES;
    [self insertSubview:photoView aboveSubview:videoView];
    [self constrainToSelf:photoView];

	MPLoupe *loupe = [[MPLoupe alloc] init];
	self.magnifyingGlass = loupe;
	loupe.scaleAtTouchPoint = NO;
    self.magnifyingGlass.viewToMagnify = photoView;
    
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

//- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
//{
//    LOGME
//    
//    if (touches && touches.count == 1)
//    {
//        magView.hidden = NO;
//        UITouch* touch = touches.allObjects[0];
//        CGPoint touchPoint = [touch locationInView:self];
//        [self moveMagTo:touchPoint];
//    }
//}
//
//- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
//{
//    LOGME
//    magView.hidden = YES;
//}
//
//- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
//{
//    LOGME
//    magView.hidden = YES;
//}
//
//- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
//{
//    if (touches && touches.count == 1)
//    {
//        UITouch* touch = touches.allObjects[0];
//        CGPoint touchPoint = [touch locationInView:self];
//        [self moveMagTo:touchPoint];
//    }
//}
//
//- (void) moveMagTo:(CGPoint)point
//{
//    DLog(@"%f, %f", point.x, point.y);
//    magView.center = CGPointMake(point.x, point.y - 100);
//    magView.photo.center = CGPointMake(point.x - magView.center.x + 50, point.y - magView.center.y - 50);
//}

@end
