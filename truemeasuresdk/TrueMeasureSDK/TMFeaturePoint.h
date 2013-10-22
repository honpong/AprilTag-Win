//
//  TMFeaturePoint.h
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import "TMScalar.h"
#import "TMPoint.h"

@interface TMFeaturePoint : NSObject

/** The horizontal position, in units of pixels, of the feature in the most recently processed video frame. */
@property (nonatomic, readonly) float x;

/** The vertical position, in units of pixels, of the feature in the most recently processed video frame. */
@property (nonatomic, readonly) float y;

/** The current estimate of the depth of the feature, in units of meters, relative to the camera position at the time the feature was first detected.
 
 Note: This is not the same as the distance of the feature from the camera. Instead it represents the translation of the feature along the camera's Z axis. */
@property (nonatomic, readonly) TMScalar *originalDepth;

/** An TMPoint object representing the current estimate of the 3D position of the feature relative to the global reference frame, as defined in [RCSensorFusionData transformation]. */
@property (nonatomic, readonly) TMPoint *worldPoint;

/** Flag reflecting whether the feature is initialized.
 
 When a feature is first detected, we have no information about its depth and therefore do not know its 3D position.
 
 TRUE if the feature has been tracked for sufficiently long that depth and worldPoint may be considered valid.
 */
@property (nonatomic, readonly) bool initialized;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithX:(float)x withY:(float)y withOriginalDepth:(TMScalar *)originalDepth withWorldPoint:(TMPoint *)worldPoint withInitialized:(bool)initialized;

/** @returns The distance in pixels between two points. */
- (float) pixelDistanceToPoint:(CGPoint)cgPoint;

/** A convenient way to get a CGPoint.
 
 @returns A CGPoint that represents the coordinates contained in this object. */
- (CGPoint) makeCGPoint;

@end
