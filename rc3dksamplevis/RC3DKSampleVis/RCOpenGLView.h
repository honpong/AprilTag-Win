//
//  RCOpenGLView.h
//  RC3DKSampleVis
//
//  Created by Brian on 8/26/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "ConnectionManager.h"

#import <Cocoa/Cocoa.h>

@interface RCOpenGLView : NSOpenGLView <RCConnectionManagerDelegate>
/**
 Represents the position of the camera relative to the scene
 */
typedef NS_ENUM(int, RCViewpoint) {
    RCViewpointTopDown = 0,
    RCViewpointSide,
    RCViewpointAnimating
};


/**
 Represents what types of features we should display.
 */
typedef NS_ENUM(int, RCFeatureFilter) {
    RCFeatureFilterShowAll = 0,
    RCFeatureFilterShowGood
};

- (void) setViewpoint:(RCViewpoint)viewpoint;
- (void) setFeatureFilter:(RCFeatureFilter)featureType;

- (void) reset;

@end
