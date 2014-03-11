//
//  MPMeasurementsLayer.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/8/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import "MPLineLayer.h"
#import "MPLineLayerDelegate.h"
#import <RCCore/RCCore.h>
#import "MPMeasurementView.h"

@interface MPMeasurementsView : UIView

- (id) initWithFeaturesLayer:(RCFeaturesLayer*)layer;
- (void) addMeasurementBetweenPointA:(RCFeaturePoint*)pointA andPointB:(RCFeaturePoint*)pointB;
- (void) clearMeasurements;

@end
