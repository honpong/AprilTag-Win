//
//  TMFeatureLayerManager.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeatureLayerDelegate.h"
#import "TMPoint.h"
#import "TMPoint+TMPointExt.h"

@interface TMFeaturesLayer : CALayer

@property (nonatomic) UIColor* color;

- (id) initWithFeatureCount:(int)count;
- (void) setFeaturePositions:(NSArray*)points;
- (void) drawFeature:(TMPoint*)point;

@end
