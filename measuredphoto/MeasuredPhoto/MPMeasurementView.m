//
//  MPMeasurementView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPMeasurementView.h"
#import <RCCore/RCCore.h>
#import "MPCapturePhoto.h"

@implementation MPMeasurementView
{
    RCFeaturesLayer* featuresLayer;
    float lineAnglePortrait;
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
        
        lineAnglePortrait = [self calculateLineAngleInRadians:screenPointA withPointB:screenPointB];
        
        RCScalar *distMeters = [[RCTranslation translationFromPoint:pointA.worldPoint toPoint:pointB.worldPoint] getDistance];
        
        // make a new distance label
        id<RCDistance> distObj;
        Units units = (Units)[[NSUserDefaults standardUserDefaults] integerForKey:PREF_UNITS];
        UnitsScale scale = [self autoSelectUnitsScale:distMeters.scalar withUnits:units];
        if (units == UnitsImperial)
        {
            distObj = [[RCDistanceImperial alloc] initWithMeters:distMeters.scalar withScale:scale];
        }
        else
        {
            distObj = [[RCDistanceMetric alloc] initWithMeters:distMeters.scalar withScale:scale];
        }
        
        label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 100, 30)];
        label.text = [distObj description];
        label.font = [UIFont systemFontOfSize:20];
        label.textColor = [UIColor redColor];
        label.textAlignment = NSTextAlignmentCenter;
        label.backgroundColor = [UIColor clearColor];
        
        // put the label on the center of the line, then shift it off the line a bit
        CGPoint midPoint = [self getMidPointBetweenPointA:screenPointA andPointB:screenPointB];
        midPoint.x += cos(lineAnglePortrait - M_PI_2) * label.frame.size.height / 2;
        midPoint.y += sin(lineAnglePortrait - M_PI_2) * label.frame.size.height / 2;
        label.center = midPoint;
        
        // rotate the label to an angle that matches the line, and is closest to right side up
        [self rotateMeasurementLabel];
        [self addSubview:label];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleOrientationChange:)
                                                     name:MPUIOrientationDidChangeNotification
                                                   object:nil];
    }
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
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

- (void) handleOrientationChange:(NSNotification*)notification
{
    if (notification.object)
    {
        MPOrientationChangeData* data = (MPOrientationChangeData*)notification.object;
        
        [UIView animateWithDuration: .5
                              delay: 0
                            options: UIViewAnimationOptionCurveEaseIn
                         animations:^{
                             [self rotateMeasurementLabel:data.orientation];
                         }
                         completion:nil];
    }
}

- (void) rotateMeasurementLabel
{
    UIDeviceOrientation orientation = [MPCapturePhoto getCurrentUIOrientation];
    [self rotateMeasurementLabel:orientation];
}

- (void) rotateMeasurementLabel:(UIDeviceOrientation)orientation
{
    NSNumber* radians = [self getRotationInRadians:orientation];
    if (radians)
    {
        float labelAngle = [self getBestLabelAngle:lineAnglePortrait withOffset:[radians floatValue]];
        label.transform = CGAffineTransformRotate(CGAffineTransformIdentity, labelAngle);
    }
}

- (float) getBestLabelAngle:(float)lineAngle withOffset:(float)offset
{
    if (lineAngle < 0) lineAngle = lineAngle + M_PI;
    return lineAngle > M_PI_2 + offset || lineAngle < -M_PI_2 + offset ? lineAngle + M_PI : lineAngle;
}

- (UnitsScale) autoSelectUnitsScale:(float)meters withUnits:(Units)units
{
    if (units == UnitsMetric)
    {
        if (meters < 1) return UnitsScaleCM;
        else if (meters >= 1000) return UnitsScaleKM;
        else return UnitsScaleM;
    }
    else
    {
        float inches = meters * INCHES_PER_METER;
        
        if (inches <= (INCHES_PER_FOOT * 3)) return UnitsScaleIN;
        else if (inches >= INCHES_PER_MILE) return UnitsScaleMI;
        else return UnitsScaleFT; //default
    }
}

@end
