//
//  MPMeasurementView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPLineLayer.h"
#import "MPLineLayerDelegate.h"
#import <RCCore/RCCore.h>

@interface MPMeasurementView : UIView

@property (nonatomic, readonly) UILabel* label;
@property (nonatomic, readonly) MPLineLayer* lineLayer;
@property (nonatomic, readonly) RCFeaturePoint* pointA;
@property (nonatomic, readonly) RCFeaturePoint* pointB;

- (id) initWithFeaturesLayer:(RCFeaturesLayer*)layer andPointA:(RCFeaturePoint*)pointA andPointB:(RCFeaturePoint*)pointB;

@end
