//
//  ViewController.h
//  RC3DKMobileVis
//
//  Created by Brian on 2/27/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@interface ViewController : GLKViewController

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

/*
- (void) observeTime:(float) time;
- (void) observeFeatureWithId:(uint64_t)id x:(float)x y:(float)y z:(float)z lastSeen:(float)lastSeen good:(bool)good;
- (void) observePathWithTranslationX:(float)x y:(float)y z:(float)z time:(float)time;
- (void) setViewpoint:(RCViewpoint)viewpoint;
- (void) setFeatureFilter:(RCFeatureFilter)featureType;

- (void) reset;
*/

@end
