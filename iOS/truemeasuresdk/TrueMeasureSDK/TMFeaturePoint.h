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

/** Represents a visual feature in an image. Contains both it's 2D location in the image, in units of pixels, and it's 3D location
 in space, in units of meters.*/
@interface TMFeaturePoint : NSObject <NSSecureCoding>

/** The horizontal position, in units of pixels, of the feature in the image. */
@property (nonatomic, readonly) float x;

/** The vertical position, in units of pixels, of the feature in the image. */
@property (nonatomic, readonly) float y;

/** The current estimate of the depth of the feature, in units of meters, relative to the camera position at the time the feature was detected.
 
 Note: This is not the same as the distance of the feature from the camera. Instead it represents the translation of the feature along the camera's Z axis. */
@property (nonatomic, readonly) TMScalar *originalDepth;

/** A TMPoint object representing the 3D position of the feature relative to the global reference frame. */
@property (nonatomic, readonly) TMPoint *worldPoint;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithX:(float)x withY:(float)y withOriginalDepth:(TMScalar *)originalDepth withWorldPoint:(TMPoint *)worldPoint;

/** @returns The distance in pixels between two points. */
- (float) pixelDistanceToPoint:(CGPoint)cgPoint;

/** A convenient way to get a CGPoint.
 
 @returns A CGPoint that represents the 2D pixel coordinates contained in this object. */
- (CGPoint) makeCGPoint;

@end
