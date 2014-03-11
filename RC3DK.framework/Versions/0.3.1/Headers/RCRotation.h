//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#include "RCVector.h"
#include "RCPoint.h"
#include "RCTranslation.h"

/** Represents a 3D rotation. */
@interface RCRotation : RCVector

/** Fills in a matrix representation of the rotation that is compatible with OpenGL.
 
 This matrix can be loaded in an OpenGL ES 2.0 shader by calling glUniformMatrix4fv, with transpose set to false.
 @param matrix A C-style array of 16 floats that will be overwritten with the rotation matrix.
 */
- (void) getOpenGLMatrix:(float[16])matrix;

/** Apply the rotation to a point.
 @param point An RCPoint object
 @returns The rotated version of the point.
 */
- (RCPoint *) transformPoint:(RCPoint *)point;

/** Apply the rotation to a translation vector.
 @param translation An RCTranslation object
 @returns The rotated version of the translation.
 */
- (RCTranslation *) transformTranslation:(RCTranslation *)translation;

/** Compute the inverse rotation.
 @returns The inverse of the rotation.
 */
- (RCRotation *) getInverse;

/** Compute the composition of two rotations.
 
 The composition is performed in such a way that, if R1 and R2 are RCRotation objects, and pt is an RCPoint object, the following two lines would produce the same results (up to numerical precision):
 
    [[R1 composeWithRotation:R2] transformPoint:pt];
    [R1 transformPoint:[R2 transformPoint:pt]];

 @param other An RCRotation object
 @returns An RCRotation object representing the combined rotation. */
- (RCRotation *) composeWithRotation:(RCRotation *)other;

@end