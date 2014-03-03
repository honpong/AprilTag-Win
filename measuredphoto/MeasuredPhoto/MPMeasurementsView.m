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

- (void) addMeasurementBetweenPointA:(RCFeaturePoint*)pointA andPointB:(RCFeaturePoint*)pointB
{
    MPMeasurementView* newView = [[MPMeasurementView alloc] initWithFeaturesLayer:featuresLayer andPointA:pointA andPointB:pointB];
    [self addSubview:newView];
}

- (void) clearMeasurements
{
    NSArray* subViews = [NSArray arrayWithArray:self.subviews];
    for (UIView* view in subViews)
    {
        [view removeFromSuperview];
    }
}

@end
