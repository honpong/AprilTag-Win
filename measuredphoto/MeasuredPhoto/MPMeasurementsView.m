//
//  MPMeasurementsLayer.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/8/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPMeasurementsView.h"

@implementation MPMeasurementsView
{
    MPLineLayerDelegate* lineLayerDelegate;
    RCFeaturesLayer* featuresLayer;
}

- (id) initWithFeaturesLayer:(RCFeaturesLayer*)layer
{
    if (self = [super initWithFrame:layer.frame])
    {
        lineLayerDelegate = [MPLineLayerDelegate new];
        featuresLayer = layer;
    }
    return self;
}

- (void) drawMeasurementBetweenPointA:(RCFeaturePoint*)pointA andPointB:(RCFeaturePoint*)pointB
{
    CGPoint screenPointA = [featuresLayer screenPointFromFeature:pointA];
    CGPoint screenPointB = [featuresLayer screenPointFromFeature:pointB];
    
    // create a new line layer
    MPLineLayer* lineLayer = [[MPLineLayer alloc] initWithPointA:screenPointA andPointB:screenPointB];
    lineLayer.delegate = lineLayerDelegate;
    lineLayer.frame = self.frame;
    [self.layer addSublayer:lineLayer];
    [lineLayer setNeedsDisplay];
    
    // calculate the angle of the line
    int deltaX = screenPointA.x - screenPointB.x;
    int deltaY = screenPointA.y - screenPointB.y;
    float angleInDegrees = atan2(deltaY, deltaX) * 180 / M_PI;
    float angleInRadians = angleInDegrees * 0.0174532925;
    
    // make a new distance label
    RCScalar *distMeters = [[RCTranslation translationFromPoint:pointA.worldPoint toPoint:pointB.worldPoint] getDistance];
    RCDistanceImperial* distObj = [[RCDistanceImperial alloc] initWithMeters:distMeters.scalar withScale:UnitsScaleIN];
    UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 100, 30)];
    label.text = [distObj getString];
    label.font = [UIFont systemFontOfSize:20];
    label.textColor = [UIColor redColor];
    label.textAlignment = NSTextAlignmentCenter;
    label.backgroundColor = [UIColor clearColor];
    
    // put the label on the center of the line, then shift it off the line a bit
    CGPoint midPoint = [self getMidPointBetweenPointA:screenPointA andPointB:screenPointB];
    midPoint.x += cos(angleInRadians - M_PI_2) * label.frame.size.height / 2;
    midPoint.y += sin(angleInRadians - M_PI_2) * label.frame.size.height / 2;
    label.center = midPoint;
    
    // rotate the label to an angle that matches the line, and is closest to right side up
    float labelAngle = angleInRadians;
    if (angleInRadians > M_PI_2 || angleInRadians < -M_PI_2) labelAngle = angleInRadians + M_PI;
    label.transform = CGAffineTransformMakeRotation(labelAngle);
    
    [self addSubview:label];
}

- (CGPoint) getMidPointBetweenPointA:(CGPoint)pointA andPointB:(CGPoint)pointB
{
    float midX = (pointA.x + pointB.x) / 2;
    float midY = (pointA.y + pointB.y) / 2;
    return CGPointMake(midX, midY);
}

- (void) clearMeasurements
{
    NSArray* subViews = [NSArray arrayWithArray:self.subviews];
    for (UIView* view in subViews)
    {
        [view removeFromSuperview];
    }
    NSArray* subLayers = [NSArray arrayWithArray:self.layer.sublayers];
    for (CALayer* layer in subLayers)
    {
        [layer removeFromSuperlayer];
    }
}

@end
