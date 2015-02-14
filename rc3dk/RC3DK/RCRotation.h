//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#include "RCPoint.h"
#include "RCTranslation.h"

/** Represents a 3D rotation. */
@interface RCRotation: NSObject

/** Instantiate an RCRotation with the given quaternion parameters. */
- (id) initWithQuaternionW:(float)w withX:(float)x withY:(float)y withZ:(float)z;
- (id) initWithAxisX:(float)ax withAxisY:(float)ay withAxisZ:(float)az withAngle:(float)theta;

/** The w element of the quaternion. */
@property (nonatomic, readonly) float quaternionW;
/** The x element of the quaternion. */
@property (nonatomic, readonly) float quaternionX;
/** The y element of the quaternion. */
@property (nonatomic, readonly) float quaternionY;
/** The z element of the quaternion. */
@property (nonatomic, readonly) float quaternionZ;

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

/** Flip one axis of the coordinate system to move between left and right handed coordinates.
 @param axis The zero-based index of the axis (x = 0, y = 1, z = 2). Other values produced undefined behavior.
 @returns The rotation in the mirrored coordinate system.
 */
- (RCRotation *)flipAxis:(int)axis;

/** Compute the composition of two rotations.
 
 The composition is performed in such a way that, if R1 and R2 are RCRotation objects, and pt is an RCPoint object, the following two lines would produce the same results (up to numerical precision):
 
    [[R1 composeWithRotation:R2] transformPoint:pt];
    [R1 transformPoint:[R2 transformPoint:pt]];

 @param other An RCRotation object
 @returns An RCRotation object representing the combined rotation. */
- (RCRotation *) composeWithRotation:(RCRotation *)other;

/** Return a dictionary representation of the quaternion. */
- (NSDictionary*) dictionaryRepresentation;

@end