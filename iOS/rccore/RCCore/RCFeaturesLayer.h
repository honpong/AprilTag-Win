//
//  RCFeaturesLayer.h
//  RCCore
//
//  Created by Ben Hirashima on 7/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import "RCFeatureLayerDelegate.h"
#import "RC3DK.h"

#define VIDEO_WIDTH 480.
#define VIDEO_HEIGHT 640.

@interface RCFeaturesLayer : CALayer

@property (nonatomic, readonly) NSArray* features;

- (id) initWithFeatureCount:(int)count andColor:(UIColor*)featureColor;

/** @param features An array of RCFeaturePoint objects */
- (void) updateFeatures:(NSArray*)features;

- (RCFeaturePoint*)getClosestFeatureTo:(CGPoint)tappedPoint;
- (CGPoint) screenPointFromFeature:(RCFeaturePoint*)feature;
- (CGPoint) cameraPointFromScreenPoint:(CGPoint)screenPoint;

@end