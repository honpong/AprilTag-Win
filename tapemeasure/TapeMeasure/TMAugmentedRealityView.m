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
    
    NSMutableArray* pointsPool;
    float videoScale;
    int videoFrameOffset;
    
    BOOL isInitialized;
    
    NSArray* lineLabels;
}
@synthesize videoView, featuresLayer, selectedFeaturesLayer;

- (void) initialize
{
    if (isInitialized) return;
    
    LOGME
    
    OPENGL_MANAGER;
    
    videoView = [[TMVideoPreview alloc] initWithFrame:CGRectZero];
    [videoView setTransformFromCurrentVideoOrientationToOrientation:UIInterfaceOrientationPortrait];
    videoView.frame = CGRectMake(0, 0, self.frame.size.width, self.frame.size.height); // must be set AFTER setTransformFromCurrentVideoOrientationToOrientation
    [self addSubview:videoView];
    [self sendSubviewToBack:videoView];
    
    crosshairsDelegate = [TMCrosshairsLayerDelegate new];
    crosshairsLayer = [CALayer new];
    [crosshairsLayer setDelegate:crosshairsDelegate];
    crosshairsLayer.hidden = YES;
    crosshairsLayer.frame = CGRectMake(0, 0, self.frame.size.width, self.frame.size.height);
    [crosshairsLayer setNeedsDisplay];
    [self.layer addSublayer:crosshairsLayer];
    
    lineLayerDelegate = [TMLineLayerDelegate new];
    
    [self setupFeatureLayers];
    
    lineLabels = [NSMutableArray new];
        
    isInitialized = YES;
}

- (void) setupFeatureLayers
{
    selectedFeaturesLayer = [[RCFeaturesLayer alloc] initWithFeatureCount:2 andColor:[UIColor greenColor]];
    selectedFeaturesLayer.bounds = self.bounds;
    selectedFeaturesLayer.position = self.center;
    [selectedFeaturesLayer setNeedsDisplay];
    [self.layer insertSublayer:selectedFeaturesLayer above:crosshairsLayer];
    
    featuresLayer = [[RCFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT andColor:nil];
    featuresLayer.hidden = YES;
    featuresLayer.bounds = self.bounds;
    featuresLayer.position = self.center;
    [featuresLayer setNeedsDisplay];
    [self.layer insertSublayer:featuresLayer below:selectedFeaturesLayer];
}

- (RCFeaturePoint*) selectFeatureNearest:(CGPoint)coordinateTapped
{
    RCFeaturePoint* point = [featuresLayer getClosestFeatureTo:coordinateTapped];
    selectedFeaturesLayer.hidden = NO;
    [selectedFeaturesLayer updateFeatures:@[point]];
    return point;
}

- (void) clearSelectedFeatures
{
    selectedFeaturesLayer.hidden = YES;
}

- (void) drawMeasurementBetweenPointA:(RCFeaturePoint*)pointA andPointB:(RCFeaturePoint*)pointB
{
    // create a new line layer
    TMLineLayer* lineLayer = [[TMLineLayer alloc] initWithPointA:[pointA makeCGPoint] andPointB:[pointB makeCGPoint]];
    lineLayer.delegate = lineLayerDelegate;
    lineLayer.bounds = self.bounds;
    lineLayer.position = self.center;
    [self.layer insertSublayer:lineLayer below:crosshairsLayer];
    [lineLayer setNeedsDisplay];
        
    // calculate the angle of the line
    int deltaX = pointA.x - pointB.x;
    int deltaY = pointA.y - pointB.y;
    float angleInDegrees = atan2(deltaY, deltaX) * 180 / M_PI;
    float angleInRadians = angleInDegrees * 0.0174532925;
    
    // make a new distance label
    RCScalar *distMeters = [[RCTranslation translationFromPoint:pointA.worldPoint toPoint:pointB.worldPoint] getDistance];
    RCDistanceImperial* distObj = [[RCDistanceImperial alloc] initWithMeters:distMeters.scalar withScale:UnitsScaleIN];
    UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 100, 21)];
    label.text = [distObj getString];
    label.center = self.center;
    label.textColor = [UIColor redColor];
    label.textAlignment = NSTextAlignmentCenter;
    label.backgroundColor = [UIColor clearColor];
    
    // put the label on the center of the line, then shift it off the line a bit
    CGPoint midPoint = [self getMidPointBetweenPointA:[pointA makeCGPoint] andPointB:[pointB makeCGPoint]];
    midPoint.x += cos(angleInRadians - M_PI_2) * 10;
    midPoint.y += sin(angleInRadians - M_PI_2) * 10;
    label.center = midPoint;
    
    // rotate the label to an angle that matches the line, and is closest to right side up
    float labelAngle = angleInRadians;
    if (angleInRadians > M_PI_2 || angleInRadians < -M_PI_2) labelAngle = angleInRadians + M_PI;
    label.transform = CGAffineTransformMakeRotation(labelAngle);
        
    [self insertSubview:label aboveSubview:videoView];
}

- (CGPoint) getMidPointBetweenPointA:(CGPoint)pointA andPointB:(CGPoint)pointB
{
    float midX = (pointA.x + pointB.x) / 2;
    float midY = (pointA.y + pointB.y) / 2;
    return CGPointMake(midX, midY);
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
