//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#include "RCScalar.h"
#include "RCPoint.h"
#import <CoreGraphics/CoreGraphics.h>

/** Represents a visual feature detected and tracked by computer vision.
 
 Features are typically located at points in an image with strong visual contrast, such as corners. Such points typically correspond to fixed geometric points in the physical world. Most data in this class is defined relative to the most recently processed video frame, which is available in [RCSensorFusionData sampleBuffer].
 */
@interface RCFeaturePoint : NSObject

/** A unique identifier for the feature.
 
 This identifier should be the same for a given feature when it is tracked or matched across frames, and globally unique for a given session of RCSensorFusion. Note that if a particular feature is not found in a frame, but is re-detected in a subsequent frame, a new id will be assigned. */
@property (nonatomic, readonly) uint64_t id;

/** The horizontal position, in units of pixels, of the feature in the most recently processed video frame. */
@property (nonatomic, readonly) float x;

/** The vertical position, in units of pixels, of the feature in the most recently processed video frame. */
@property (nonatomic, readonly) float y;

/** The current estimate of the depth of the feature, in units of meters, relative to the camera position at the time the feature was first detected.
 
 Note: This is not the same as the distance of the feature from the camera. Instead it represents the translation of the feature along the camera's Z axis. */
@property (nonatomic, readonly) RCScalar *originalDepth;

/** An RCPoint object representing the current estimate of the 3D position of the feature relative to the global reference frame, as defined in [RCSensorFusionData transformation]. */
@property (nonatomic, readonly) RCPoint *worldPoint;

/** Flag reflecting whether the feature is initialized.
 
 When a feature is first detected, we have no information about its depth and therefore do not know its 3D position.
 
 TRUE if the feature has been tracked for sufficiently long that depth and worldPoint may be considered valid.
 */
@property (nonatomic, readonly) bool initialized;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithId:(uint64_t)id withX:(float)x withY:(float)y withOriginalDepth:(RCScalar *)originalDepth withWorldPoint:(RCPoint *)worldPoint withInitialized:(bool)initialized;
- (NSDictionary*) dictionaryRepresentation;

/** @returns The distance in pixels between two points. */
- (float) pixelDistanceToPoint:(CGPoint)cgPoint;

/** A convenient way to get a CGPoint.

 @returns A CGPoint that represents the coordinates contained in this object. */
- (CGPoint) makeCGPoint;

@end
