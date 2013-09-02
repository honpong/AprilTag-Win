//
//  RCOpenGLView.h
//  RC3DKSampleVis
//
//  Created by Brian on 8/26/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface RCOpenGLView : NSOpenGLView

/**
 Represents the position of the camera relative to the scene
 */
typedef NS_ENUM(int, RCViewpoint) {
    RCViewpointTopDown = 0,
    RCViewpointDeviceView,
    RCViewpointSide
};


/**
 Represents what types of features we should display.
 */
typedef NS_ENUM(int, RCFeatureFilter) {
    RCFeatureFilterShowAll = 0,
    RCFeatureFilterShowGood
};


- (void) drawRect: (NSRect) bounds;
- (void) drawForTime: (float) time;
- (void) observeFeatureWithId:(uint64_t)id x:(float)x y:(float)y z:(float)z lastSeen:(float)lastSeen good:(bool)good;
- (void) observePathWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time;
- (void) setViewpoint:(RCViewpoint)viewpoint;
- (void) setFeatureFilter:(RCFeatureFilter)featureType;

- (void) reset;

@end
