//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCVector.h"
#import "RCPoint.h"
#import "RCScalar.h"

/** Represents a 3D translation in units of meters. */
@interface RCTranslation : RCVector

/** Apply the translation to a point.
 @param point An RCPoint object
 @returns The translated version of the point.
 */
- (RCPoint *)transformPoint:(RCPoint *)point;

/** Compute the straight line distance spanned by the translation.
 @returns An RCScalar object representing the distance in meters.
 */
- (RCScalar *)getDistance;

/** Compute the inverse translation.
 @returns The inverse of the translation.
 */
- (RCTranslation *)getInverse;

/** Flip one axis of the coordinate system to move between left and right handed coordinates.
 @param axis The zero-based index of the axis (x = 0, y = 1, z = 2). Other values produced undefined behavior.
 @returns The translation in the mirrored coordinate system.
 */
- (RCTranslation *)flipAxis:(int)axis;

/** Compute the composition of two translations.
 
    Composition of transformations is commutative.
 
 @param other An RCTranslation object
 @returns An RCTranslation object representing the combined translation. */
- (RCTranslation *)composeWithTranslation:(RCTranslation *)other;

/** Compute the translation between two points.
 @param fromPoint The first RCPoint.
 @param toPoint The second RCPoint.
 @return A new RCTranslation object that such that calling [RCTranslation transformPoint:fromPoint] would return a point with the same 3D position as toPoint.
 */
+ (RCTranslation *)translationFromPoint:(RCPoint *)fromPoint toPoint:(RCPoint *)toPoint;

@end