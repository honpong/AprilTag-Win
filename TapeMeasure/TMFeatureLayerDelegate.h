//
//  TMFeaturesLayerDelegate.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#define FEATURE_RADIUS 2
#define FRAME_SIZE 8
#define FRAME_RADIUS FRAME_SIZE/2

@interface TMFeatureLayerDelegate : NSObject

@property (nonatomic) UIColor* color;

@end
