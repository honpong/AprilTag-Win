//
//  MPMeasurementView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPMeasurementView.h"

@implementation MPMeasurementView
{
    RCFeaturesLayer* featuresLayer;
    float lineAngle;
}
@synthesize lineLayer, label, pointA, pointB;

- (id) initWithFeaturesLayer:(RCFeaturesLayer*)layer andPointA:(RCFeaturePoint*)pointA_ andPointB:(RCFeaturePoint*)pointB_;
{
    if (self = [super initWithFrame:layer.frame])
    {
        featuresLayer = layer;
        pointA = pointA_;
        pointB = pointB_;
        
        CGPoint screenPointA = [featuresLayer screenPointFromFeature:pointA];
        CGPoint screenPointB = [featuresLayer screenPointFromFeature:pointB];
        
        // create a new line layer
        lineLayer = [[MPLineLayer alloc] initWithPointA:screenPointA andPointB:screenPointB];
        lineLayer.delegate = [MPLineLayerDelegate sharedInstance];
        lineLayer.frame = self.frame;
        [self.layer addSublayer:lineLayer];
        [lineLayer setNeedsDisplay];
        
        lineAngle = [self calculateLineAngleInRadians:screenPointA withPointB:screenPointB];
        
        // make a new distance label
        RCScalar *distMeters = [[RCTranslation translationFromPoint:pointA.worldPoint toPoint:pointB.worldPoint] getDistance];
        RCDistanceImperial* distObj = [[RCDistanceImperial alloc] initWithMeters:distMeters.scalar withScale:UnitsScaleIN];
        label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 100, 30)];
        label.text = [distObj getString];
        label.font = [UIFont systemFontOfSize:20];
        label.textColor = [UIColor redColor];
        label.textAlignment = NSTextAlignmentCenter;
        label.backgroundColor = [UIColor clearColor];
        
        // put the label on the center of the line, then shift it off the line a bit
        CGPoint midPoint = [self getMidPointBetweenPointA:screenPointA andPointB:screenPointB];
        midPoint.x += cos(lineAngle - M_PI_2) * label.frame.size.height / 2;
        midPoint.y += sin(lineAngle - M_PI_2) * label.frame.size.height / 2;
        label.center = midPoint;
        
        // rotate the label to an angle that matches the line, and is closest to right side up
        float labelAngle = lineAngle;
        if (lineAngle > M_PI_2 || lineAngle < -M_PI_2) labelAngle = lineAngle + M_PI;
        label.transform = CGAffineTransformMakeRotation(labelAngle);
        
        [self addSubview:label];
    }
    return self;
}

- (CGPoint) getMidPointBetweenPointA:(CGPoint)screenPointA andPointB:(CGPoint)screenPointB
{
    float midX = (screenPointA.x + screenPointB.x) / 2;
    float midY = (screenPointA.y + screenPointB.y) / 2;
    return CGPointMake(midX, midY);
}

- (float) calculateLineAngleInRadians:(CGPoint)screenPointA withPointB:(CGPoint)screenPointB
{
    int deltaX = screenPointA.x - screenPointB.x;
    int deltaY = screenPointA.y - screenPointB.y;
    float angleInDegrees = atan2(deltaY, deltaX) * 180 / M_PI;
    return angleInDegrees * 0.0174532925;
}

- (void) rotateLabelToOrientation:(UIDeviceOrientation)orienation
{
    
}

@end
