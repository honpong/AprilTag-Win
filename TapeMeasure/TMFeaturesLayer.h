//
//  TMFeatureLayerManager.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMPoint.h"
#import "TMPoint+TMPointExt.h"
#import <RC3DK/RC3DK.h>
#import "TMDataManagerFactory.h"
#import <RCCore/RCFeatureLayerDelegate.h>

#define VIDEO_WIDTH 480
#define VIDEO_HEIGHT 640

@interface TMFeaturesLayer : CALayer

- (id) initWithFeatureCount:(int)count andColor:(UIColor*)featureColor;
- (void) updateFeatures:(NSArray*)features;
/** @param features An array of RCFeaturePoint objects */
- (TMPoint*) getClosestPointTo:(CGPoint)searchPoint;
/** @param points An array of TMPoint objects */
- (void) setFeaturePositions:(NSArray*)points;

@end
